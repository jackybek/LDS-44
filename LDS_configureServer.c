#include <open62541/plugin/log_stdout.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/create_certificate.h>
#include <open62541/plugin/securitypolicy.h>
#include <open62541/plugin/accesscontrol_default.h>
#include <open62541/plugin/nodestore_default.h>
#include <stdio.h>

// the following is required for Unified Automation UAExpert
static const size_t usernamePasswordsSize = 2;
static UA_UsernamePasswordLogin logins[2] = {
        {UA_STRING_STATIC("jackybek"), UA_STRING_STATIC("thisisatestpassword24")},
        {UA_STRING_STATIC("admin"),UA_STRING_STATIC("defaultadminpassword24")}

int configureServer(UA_Server *);

int configureServer(UA_Server *uaLDSServer)
{
	UA_StatusCode status;
	
	UA_ServerConfig *config = UA_Server_getConfig(uaLDSServer);

	if (config->nodestore.context == NULL)
		UA_Nodestore_HashMap(&config->nodestore);

    config->accessControl.clear(&config1->accessControl);
    UA_AccessControl_default(config, UA_FALSE, 
				&config->securityPolicies[config->securityPoliciesSize-1].policyUri, 
				usernamePasswordsSize, logins>
				
    config->shutdownDelay = 0; //5000.0; // millisecond
                config1->securityPolicyNoneDiscoveryOnly = UA_FALSE;
    

    // Server Description
    UA_BuildInfo_clear(&config1->buildInfo);
    const char* env_product_uri = getenv("PRODUCT_URI");
    const char* env_manufacturer_name = getenv("MANUFACTURER_NAME");
    const char* env_product_name = getenv("PRODUCT_NAME");
    const char* env_application_uri_server = getenv("APPLICATION_URI_SERVER");
    const char* env_application_name = getenv("APPLICATION_NAME");

    config->buildInfo.productUri = UA_STRING_ALLOC(env_product_uri);  //(PRODUCT_URI);
    config->buildInfo.manufacturerName = UA_STRING_ALLOC(env_manufacturer_name); //(MANUFACTURER_NAME);
    config->buildInfo.productName = UA_STRING_ALLOC(env_product_name); //(PRODUCT_NAME);
    config->buildInfo.softwareVersion = UA_STRING_ALLOC(VERSION(UA_OPEN62541_VER_MAJOR, UA_OPEN62541_VER_MINOR,
                                                UA_OPEN62541_VER_PATCH, UA_OPEN62541_VER_LABEL));
												
	config->buildInfo.buildDate = UA_DateTime_now();
    config->buildInfo.buildNumber = UA_STRING_ALLOC(__DATE__ " " __TIME__);

    UA_ApplicationDescription_clear(&config->applicationDescription);
    UA_String_clear(&config->applicationDescription.applicationUri);
    config->applicationDescription.applicationUri.length = strlen(env_application_uri_server); //(APPLICATION_URI_SERVER);
    config->applicationDescription.applicationUri = UA_String_fromChars(env_application_uri_server); //(APPLICATION_URI_SERVER); //UA_STRING_ALLOC(APPLICATION_URI>
    config->applicationDescription.productUri = UA_STRING_ALLOC(env_product_uri); //PRODUCT_URI);
    config->applicationDescription.applicationName = UA_LOCALIZEDTEXT_ALLOC("en", env_application_name); //, APPLICATION_NAME);
											
    // LDS ++ refer to https://opcfoundation.org/UA/schemas/1.04/ServerCapabilities.csv
    // NA, DA, HD, AC, HE, GDS, LDS, DI, ADI, FDI, FDIC, PLC, S95, RCP, PUB, AUTOID, MDIS, CNC, PLK, FDT, TMC, CSPP, 61850, PACKML, MTC
    // AUTOML, SERCOS, MIMOSA, WITSML, DEXPI, IOLINK, VROBOT, PNO
    config->applicationDescription.applicationType = UA_APPLICATIONTYPE_DISCOVERYSERVER;   // acts as DISCOVERY SERVER ONLY OR UA_APPLICATIONTYPE_SERVER

    // Multicast DNS related settings - LDS - refer to github/open62541/open62541/examples/discovery/server_lds.c
    config->mdnsEnabled = true;
    config->mdnsConfig.mdnsServerName = UA_String_fromChars("Local Discovery Server");
    config->mdnsInterfaceIP = UA_String_fromChars("0.0.0.0");  // 42.42.42.42
    // set the capabilities
    config->mdnsConfig.serverCapabilitiesSize = 1;
    UA_String *caps = (UA_String *)UA_Array_new(1, &UA_TYPES[UA_TYPES_STRING]);
    caps[0] = UA_String_fromChars("LDS");   // local discovery service
    //caps[1] = UA_String_fromChars("HD");  // provide historical data
    //caps[2] = UA_String_fromChars("DA");  // provide current data
    //caps[3] = UA_String_fromChars("GDS"); // global discovery Server information model
    config->mdnsConfig.serverCapabilities = caps;
    config->discoveryCleanupTimeout = 60*60;
    config->verifyRequestTimestamp = UA_RULEHANDLING_ACCEPT; //UA_RULEHANDLING_WARN;

    // config1->applicationDescription.gatewayServerUri = UA_STRING_NULL;
    // config1->applicationDescription.discoveryProfileUri = UA_STRING_NULL;
    // config1->applicationDescription.discoveryUrlsSize = 0;
    // config1->applicationDescription.discoveryUrls = NULL;

    // Certificate Verification that accepts every certificate. Can be overwritten when the policy is specialized.
    // required for LDS
	UA_CertificateVerification_AcceptAll(&config.certificateVerification);
    config->secureChannelPKI.clear(&config->secureChannelPKI);
    //UA_ByteString_clear(&certificate);
    //UA_ByteString_clear(&privateKey);
    //for (size_t i = 0; i < trustListSize; i++)
    //    UA_ByteString_clear(&trustList[i]);
												
    // Limits for SecureChannels - required for LDS
    config->maxSecureChannels = 40;
    config->maxSecurityTokenLifetime = 10 * 60 * 1000; // 10 minutes */

    // Limits for Sessions - required for LDS
    config->maxSessions = 100;
    config->maxSessionTimeout = 60 * 60 * 1000;    // 1 hour

    // Limits for Discovery - required for LDS
    config->discoveryCleanupTimeout = 60 * 60;
	
}