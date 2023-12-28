#ifdef compile
sudo gcc -g -std=c99 -lrt -v -I/usr/local/include -I/home/pi/open62541/ -I/home/pi/open62541/plugins/ -lm -lrt \
-L/usr/local/lib -L/home/pi/open62541/build/bin -lpthread -lcrypto -lssl -lmbedcrypto -lmbedtls -lmbedx509 \
-lopen62541 GDS_StartServer.c -o myNewGDSServer >& error-msg

./valgrind --leak-check=yes --show-leak-kinds=all ./myNewServer 192.168.1.109 192.168.1.88 192.168.1.119

gcc with -g to debug
gcc -mandroid -g -std=c99 -lrt -v -I/home/pi/open62541/ -I/home/pi/open62541/plugins/ -ljson-c -lmariadbclient -lpthrea$
SV_ExtractXMLElementNames.c SV_Event.c SV_Monitor.c SV_Method.c SV_Historizing.c SV_PopulateOPCUANodes.c SV_CreateOPCUA$
json_checker.c SV_PubSub.c SV_mainOPCUAServer.c -o myAndroidServer >& error-msg

#endif

//=============================================================================
// Program is based on https://github.com/opcua-tsn-team-kalycito/open62541/blob/GDS-PushManagement/examples/gds/gds_server.c
//=============================================================================

/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */
#include "open62541.h"
//#include "common.h"

#define UA_ENABLE_DISCOVERY
#define UA_ENABLE_DISCOVERY_MULTICAST

#define MANUFACTURER_NAME "Virtual Skies"
#define PRODUCT_URI "http://gds.virtualskies.com.sg"
#define APPLICATION_NAME "OPCUA GDS Server based on open62541"
#define APPLICATION_URI "urn:gds.virtualskies.com.sg"

#define APPLICATION_URI_SERVER "urn:virtualskies.server.global_discovery_server"
#define PRODUCT_NAME "Virtual Skies OPC UA GDS Server"
#define STRINGIFY(arg) #arg
#define VERSION(MAJOR, MINOR, PATCH, LABEL) \
    STRINGIFY(MAJOR) "." STRINGIFY(MINOR) "." STRINGIFY(PATCH) LABEL

#define PRIVATEKEYLOC "/usr/local/ssl/private/gdsprivate-key.pem"
#define SSLCERTIFICATELOC "/etc/ssl/certs/gdscert88.pem"
#define TRUSTLISTLOC "/usr/local/ssl/trustlist/trustlist.crl"


// global variable //
static const size_t usernamePasswordsSize = 2;
static UA_UsernamePasswordLogin logins[2] = {
        {UA_STRING_STATIC("jackybek"), UA_STRING_STATIC("thisisatestpassword24")},
        {UA_STRING_STATIC("admin"),UA_STRING_STATIC("defaultadminpassword24")}
};

UA_ByteString loadFile(const char *const path);

UA_Boolean running = true;

static void stopHandler(int sig) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "received ctrl-c");
    running = false;
}


UA_ByteString loadFile(const char *const path)
{
    UA_ByteString fileContents = UA_STRING_NULL;

    // Open the file
    FILE *fp = fopen(path, "rb");
    if(!fp) {
        errno = 0; /* We read errno also from the tcp layer... */
        return fileContents;
    }

    /* Get the file length, allocate the data and read */
    fseek(fp, 0, SEEK_END);
    fileContents.length = (size_t)ftell(fp);
    fileContents.data = (UA_Byte *)UA_malloc(fileContents.length * sizeof(UA_Byte));
    	if(fileContents.data)
	{
        	fseek(fp, 0, SEEK_SET);
        	size_t read = fread(fileContents.data, sizeof(UA_Byte), fileContents.length, fp);
        	if(read != fileContents.length)
            		UA_ByteString_clear(&fileContents);
    	}
	else
        	fileContents.length = 0;
    fclose(fp);

    return fileContents;
}


int main(int argc, char* argv[])
{
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    /*
    if(argc < 3) {
        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Missing arguments. Arguments are "
                     "<server-certificate.der> <private-key.der> "
                     "[<trustlist1.crl>, ...]");
        return 1;
    }
    */
                // start OPCUA GDS Server
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "starting GDS server...");

                UA_StatusCode retval;

    		/* Load certificate and private key */
	    	UA_ByteString certificate = loadFile(SSLCERTIFICATELOC);	//loadFile(argv[1]);
                if (certificate.length == 0)
                {
                        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"GDS_StartServer.c : cannot find /usr/local/ssl/certs/gdscert88.pem");
                        //goto cleanup;
			exit(-1);
                }

    		UA_ByteString privateKey = loadFile(PRIVATEKEYLOC);		//loadFile(argv[2]);
                if (privateKey.length == 0)
                {
                        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"GDS_StartServer.c : cannot find /usr/local/ssl/private/gdsprivate-key.pem");
                        //goto cleanup;
			exit(-1);
                }

		/* Load the trustlist */
    		size_t trustListSize = 0;
    		if(argc > 3)
        		trustListSize = (size_t)argc-3;
    		UA_STACKARRAY(UA_ByteString, trustList, trustListSize);
    		for(size_t i = 0; i < trustListSize; i++)
        		trustList[i] = loadFile(argv[i+3]);

                /* Loading of a issuer list, not used in this application */
                size_t issuerListSize = 0;
                UA_ByteString *issuerList = NULL;

    		/* Loading of a revocation list currently unsupported */
    		UA_ByteString *revocationList = NULL;
    		size_t revocationListSize = 0;

		/*
    		UA_ServerConfig *config = UA_ServerConfig_new_basic128rsa15(4841, &certificate, &privateKey,
                                          trustList, trustListSize,
                                          revocationList, revocationListSize);
		*/
        	UA_ServerConfig config1;
        	memset(&config1, 0, sizeof(UA_ServerConfig));

		UA_ServerConfig_setDefaultWithSecurityPolicies(&config1, 4841,
                                                       &certificate, &privateKey,
                                                       trustList, trustListSize,
                                                       issuerList, issuerListSize,
                                                       revocationList, revocationListSize);

    		if(!&config1)
		{
        		UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "GDS_StartServer.c : Could not load the server config");
        		return 1;
    		}
                if (config1.nodestore.context == NULL)
                        UA_Nodestore_HashMap(&config1.nodestore);


    //UA_String_deleteMembers(&config->applicationDescription.applicationUri);
    /*
    config->applicationDescription.applicationUri =
            UA_String_fromChars("urn:open62541.example.global_discovery_server");
    */
        config1.shutdownDelay = 0; //5000.0; // millisecond
        config1.securityPolicyNoneDiscoveryOnly = UA_TRUE;
        config1.serverCertificate = certificate;

        // Server Description
        UA_BuildInfo_clear(&config1.buildInfo);
        config1.buildInfo.productUri = UA_STRING_ALLOC(PRODUCT_URI);
        config1.buildInfo.manufacturerName = UA_STRING_ALLOC(MANUFACTURER_NAME);
        config1.buildInfo.productName = UA_STRING_ALLOC(PRODUCT_NAME);
        config1.buildInfo.softwareVersion =
                        UA_STRING_ALLOC(VERSION(UA_OPEN62541_VER_MAJOR, UA_OPEN62541_VER_MINOR,
                                                UA_OPEN62541_VER_PATCH, UA_OPEN62541_VER_LABEL));

	UA_ApplicationDescription_clear(&config1.applicationDescription);
	config1.applicationDescription.applicationUri = UA_STRING_ALLOC(APPLICATION_URI);
	config1.applicationDescription.productUri = UA_STRING_ALLOC(PRODUCT_URI);
	config1.applicationDescription.applicationName = UA_LOCALIZEDTEXT_ALLOC("en", APPLICATION_NAME);

        config1.applicationDescription.applicationType = UA_APPLICATIONTYPE_DISCOVERYSERVER; 

    // See http://www.opcfoundation.org/UA/schemas/1.03/ServerCapabilities.csv
    	config1.mdnsEnabled = true;
    	config1.mdnsConfig.mdnsServerName = UA_String_fromChars("Global Discovery Server");
    	// set the capabilibities
    	config1.mdnsConfig.serverCapabilitiesSize = 1;
    	UA_String *caps = UA_String_new();
    	*caps = UA_String_fromChars("GDS");
    	config1.mdnsConfig.serverCapabilities = caps;
        config1.discoveryCleanupTimeout = 60*60;

                config1.verifyRequestTimestamp = UA_RULEHANDLING_ACCEPT; //UA_RULEHANDLING_WARN;
                // Certificate Verification that accepts every certificate. Can be overwritten when the policy is specialised
                // required for GDS
                UA_CertificateVerification_AcceptAll(&config1.certificateVerification);

                // Limits for SecureChannels - required for GDS
                config1.maxSecureChannels = 40;
                config1.maxSecurityTokenLifetime = 10 * 60 * 1000; // 10 minutes */

                // Limits for SecureChannels - required for GDS
                config1.maxSecureChannels = 40;
                config1.maxSecurityTokenLifetime = 10 * 60 * 1000; // 10 minutes */

                // Limits for Sessions - required for GDS
                config1.maxSessions = 100;
                config1.maxSessionTimeout = 60 * 60 * 1000;     // 1 hour

                // Limits for MonitoredItems - not required for GDS
                //config1.samplingIntervalLimits = UA_DURATIONRANGE(50.0, 24.0 * 3600.0 * 1000.0);
                //config1.queueSizeLimits = UA_UINT32RANGE(1, 100);

                // Limits for Discovery - required for GDS
                config1.discoveryCleanupTimeout = 60 * 60;

                // Limits for SecureChannels - required for GDS
                config1.maxSecureChannels = 40;
                config1.maxSecurityTokenLifetime = 10 * 60 * 1000; // 10 minutes */

                // Limits for Sessions - required for GDS
                config1.maxSessions = 100;
                config1.maxSessionTimeout = 60 * 60 * 1000;     // 1 hour

                // Limits for MonitoredItems - not required for GDS
                //config1.samplingIntervalLimits = UA_DURATIONRANGE(50.0, 24.0 * 3600.0 * 1000.0);
                //config1.queueSizeLimits = UA_UINT32RANGE(1, 100);

                // Limits for Discovery - required for GDS
                config1.discoveryCleanupTimeout = 60 * 60;

    		UA_ByteString_clear(&certificate);
    		UA_ByteString_clear(&privateKey);
    		for(size_t i = 0; i < trustListSize; i++)
        		UA_ByteString_clear(&trustList[i]);

                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "GDS_StartServer.c : after encryption routine");
        // end encryption routine

	// add userid and password routine
                // disable anonymous logins (2nd parameter set to false), enable 2 user/password logins
                config1.accessControl.clear(&config1.accessControl);
                retval = UA_AccessControl_default(&config1, false, &config1.securityPolicies[config1.securityPoliciesSize-1].policyUri, 2, logins);
                if (retval != UA_STATUSCODE_GOOD)
                        goto cleanup;

		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "adding 2 user credentials to OPCUA GDS server ...\n");

		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "starting OPCUA GDS server ...\n");
    		UA_Server *uaGDSServer1 = UA_Server_newWithConfig(&config1);

        	//Add a new namespace to the server => Returns the index of the new namespace i.e. namespaceIndex
        	UA_Int16 nsIdx_GDS = UA_Server_addNamespace(uaGDSServer1, "GDS");
        	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "New Namespace added with Nr. %d", nsIdx_GDS);

    		UA_Server_run(uaGDSServer1, &running);
        	if (retval != UA_STATUSCODE_GOOD)
                	goto cleanup;
        	else
                	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "OPCUA GDS server started successfully ...\n");


cleanup:
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "OPCUA GDS Server was unexpectedly shut down");
	if (uaGDSServer1)
    		UA_Server_delete(uaGDSServer1);
	else
    		UA_ServerConfig_clean(&config1);
}
