ifdef compile
sudo gcc -g -std=c99 -lrt -v -I/usr/local/include -I/home/pi/open62541/ -I/home/pi/open62541/plugins/ -lm -lrt \
-L/usr/local/lib -lpthread  -lcrypto -lssl -lmbedcrypto -lmbedtls -lmbedx509 \
open62541.c GDS_StartServer.c GDS_mainServer.c -o myNewGDSServer >& error-msg

./valgrind --leak-check=yes --show-leak-kinds=all ./myNewServer 192.168.1.109 192.168.1.88 192.168.1.119

gcc with -g to debug
gcc -mandroid -g -std=c99 -lrt -v -I/home/pi/open62541/ -I/home/pi/open62541/plugins/ -ljson-c -lmariadbclient -lpthrea$
SV_ExtractXMLElementNames.c SV_Event.c SV_Monitor.c SV_Method.c SV_Historizing.c SV_PopulateOPCUANodes.c SV_CreateOPCUA$
json_checker.c SV_PubSub.c SV_mainOPCUAServer.c -o myAndroidServer >& error-msg

#endif

#include "open62541.h"
#define MANUFACTURER_NAME "Virtual Skies"
#define PRODUCT_URI "http://gds.virtualskies.com.sg"
#define APPLICATION_NAME "OPCUA GDS Server based on open62541"
#define APPLICATION_URI "urn:gds.virtualskies.com.sg"
#define APPLICATION_URI_SERVER "urn:virtualskies.server.global_discovery_server"

#define VERSION(MAJOR, MINOR, PATCH, LABEL) \
    STRINGIFY(MAJOR) "." STRINGIFY(MINOR) "." STRINGIFY(PATCH) LABEL

#define PRIVATEKEYLOC "/usr/local/ssl/private/62541GDSprivate-key.pem"
#define SSLCERTIFICATELOC "/etc/ssl/certs/62541GDSServerCert.pem"
#define TRUSTLISTLOC "/usr/local/ssl/trustlist/trustlist.crl"



int main(void);

int main()
{

    /* Load certificate and private key */
    UA_ByteString certificate = loadFile(SSLCERTIFICATELOC);
    UA_ByteString privateKey = loadFile(PRIVATEKEYLOC);

    /* Load the trustlist */
    size_t trustListSize = 0;
    UA_STACKARRAY(UA_ByteString, trustList, trustListSize);
    for(size_t i = 0; i < trustListSize; i++)
        trustList[i] = loadFile(TRUSTLISTLOC);

    /* Loading of a revocation list currently unsupported */
    UA_ByteString *revocationList = NULL;
    size_t revocationListSize = 0;

    UA_ServerConfig *config1 =
        UA_ServerConfig_new_basic128rsa15(4841, &certificate, &privateKey,
                                          trustList, trustListSize,
                                          revocationList, revocationListSize);
    UA_ByteString_clear(&certificate);
    UA_ByteString_clear(&privateKey);
    for(size_t i = 0; i < trustListSize; i++)
        UA_ByteString_clear(&trustList[i]);

    if(!config1) {
        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Could not create the server config");
        return 1;
    }

    // Server Description
    UA_BuildInfo_clear(&config1.buildInfo);
    config1.buildInfo.productUri = UA_STRING_ALLOC(PRODUCT_URI);
    config1.buildInfo.manufacturerName = UA_STRING_ALLOC(MANUFACTURER_NAME);
    config1.buildInfo.productName = UA_STRING_ALLOC(PRODUCT_NAME);
    config1.buildInfo.softwareVersion =
           UA_STRING_ALLOC(VERSION(UA_OPEN62541_VER_MAJOR, UA_OPEN62541_VER_MINOR,
                                   UA_OPEN62541_VER_PATCH, UA_OPEN62541_VER_LABEL));

    config1.buildInfo.buildDate = UA_DateTime_now();
    config1.buildInfo.buildNumber = UA_STRING_ALLOC(__DATE__ " " __TIME__);


    UA_String_deleteMembers(&config->applicationDescription.applicationUri);
    config1->applicationDescription.applicationUri = UA_STRING_ALLOC(APPLICATION_URI); //UA_String_fromChars("urn:open62541.example.global_discovery_server");
    config1.applicationDescription.productUri = UA_STRING_ALLOC(PRODUCT_URI);
    config1.applicationDescription.applicationName = UA_LOCALIZEDTEXT_ALLOC("en", APPLICATION_NAME);


                // LDS ++ refer to https://opcfoundation.org/UA/schemas/1.04/ServerCapabilities.csv
                // NA, DA, HD, AC, HE, GDS, LDS, DI, ADI, FDI, FDIC, PLC, S95, RCP, PUB, AUTOID, MDIS, CNC, PLK, FDT, T$
                // AUTOML, SERCOS, MIMOSA, WITSML, DEXPI, IOLINK, VROBOT, PNO

    config1->applicationDescription.applicationType = UA_APPLICATIONTYPE_DISCOVERYSERVER;


    // See http://www.opcfoundation.org/UA/schemas/1.03/ServerCapabilities.csv
    config->serverCapabilitiesSize = 1;
    UA_String *caps = UA_String_new();
    *caps = UA_String_fromChars("GDS");

    config1->serverCapabilities = caps;
    config1->discoveryCleanupTimeout = 60*60;

    UA_Server *server = UA_Server_new(config1);
    UA_Server_run(server, &running);

    UA_Server_delete(server);
    UA_ServerConfig_delete(config1);
    return 0;

}
