//#include "open62541.h"
//#include "SV_NewMonitor.h"
//#include <open62541/plugin/network.h>
#include <open62541/client_highlevel.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/client_config_default.h>
#include <open62541/plugin/create_certificate.h>
#include <open62541/plugin/securitypolicy.h>
#include <open62541/plugin/accesscontrol_default.h>
#include <open62541/plugin/nodestore_default.h>
//#include <open62541/plugin/pki_default.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>

//=============================================================================
// Program is based on github/open62541/op62541/examples/discovery/server_lds.c
//=============================================================================

//#define UA_ENABLE_HISTORIZING
#define UA_ENABLE_DISCOVERY
#define UA_ENABLE_DISCOVERY_MULTICAST

#define MANUFACTURER_NAME "Virtual Skies"
#define PRODUCT_URI "http://lds.virtualskies.com.sg"
#define APPLICATION_NAME "OPCUA LDS Server based on open62541"
#define APPLICATION_URI "urn:lds.virtualskies.com.sg" //"urn:virtualskies.com.sg"		// maps to CN during cert generatioin

#define APPLICATION_URI_SERVER "urn:lds.virtualskies.com.sg"	//"urn:virtualskies.server.local_discovery_server"

#define PRODUCT_NAME "Virtual Skies OPC UA LDS Server"
#define STRINGIFY(arg) #arg
#define VERSION(MAJOR, MINOR, PATCH, LABEL) \
    STRINGIFY(MAJOR) "." STRINGIFY(MINOR) "." STRINGIFY(PATCH) LABEL

#define PRIVATEKEYLOC "/usr/local/ssl/private/ldsprivate-key.pem"
#define SSLCERTIFICATELOC "/etc/ssl/certs/ldscert44.pem"
#define TRUSTLISTLOC "/usr/local/ssl/trustlist/trustlist.crl"

/* Struct initialization works across ANSI C/C99/C++ if it is done when the
 * variable is first declared. Assigning values to existing structs is
 * heterogeneous across the three. */
static UA_INLINE UA_UInt32Range
UA_UINT32RANGE(UA_UInt32 min, UA_UInt32 max) {
    UA_UInt32Range range = {min, max};
    return range;
}

static UA_INLINE UA_DurationRange
UA_DURATIONRANGE(UA_Duration min, UA_Duration max) {
    UA_DurationRange range = {min, max};
    return range;
}

const UA_ByteString UA_SECURITY_POLICY_BASIC128_URI =
    {56, (UA_Byte *)"http://opcfoundation.org/UA/SecurityPolicy#Basic128Rsa15"};

//void* StartOPCUAServer(void* x_void_ptr, char *OPCipaddress, char *brokeripadress);
void* StartOPCUALDSServer(void* x_void_ptr, char* argv);

//UA_NodeId CreateOPCUANodes(UA_Server *);
//void CreateServerMethodItems(UA_Server *, UA_NodeId);
//void CreateServerHistorizingItems(UA_Server *);
//void CreateServerPubSub(UA_Server *, char* , UA_Int16);
//void CreateServerPubSub(UA_Server *, char *, UA_Int16, char *);

//void CreateServerEvents(UA_Server *);

// forward declaration
//void GetHistoryDBConnection(void);
//void CloseHistoryDBConnection(void);

UA_ByteString loadFile(const char *const path);
/*
static UA_Boolean allowAddNode(UA_Server *server, UA_AccessControl *ac,
                const UA_NodeId *sessionId, void *sessionContext,
                const UA_AddNodesItem *item);
static UA_Boolean allowDeleteNode(UA_Server *server, UA_AccessControl *ac,
                const UA_NodeId *sessionId, void *sessionContext,
                const UA_DeleteNodesItem *item);
static UA_Boolean allowBrowseNode(UA_Server *server, UA_AccessControl *ac,
                const UA_NodeId *sessionId, void *sessionContext,
                const UA_NodeId *nodeId, void *nodeContext);
*/
#ifdef UA_ENABLE_HISTORIZING
static UA_Boolean allowHistoryUpdateUpdateData(UA_Server *server, UA_AccessControl *ac,
		const UA_NodeId *sessionId, void *sessionContext,
		const UA_NodeId *nodeId,
		UA_PerformUpdateType performInsertReplace,
		const UA_DataValue *value);

static UA_Boolean allowHistoryUpdateDeleteRawModified(UA_Server *server, UA_AccessControl *ac,
		const UA_NodeId *sessionId, void *sessionContext,
		const UA_NodeId *nodeId,
		UA_DateTime startTimestamp,
		UA_DateTime endTimestamp,
		bool isDeleteModified);
#endif

static volatile UA_Boolean running = true;
extern int g_argc;

// sample found in /open62541/examples/common.h
// parses the certificate file - used in StartOPCUAServer.c

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
    if(fileContents.data) {
        fseek(fp, 0, SEEK_SET);
        size_t read = fread(fileContents.data, sizeof(UA_Byte), fileContents.length, fp);
        if(read != fileContents.length)
            UA_ByteString_clear(&fileContents);
    } else {
        fileContents.length = 0;
    }
    fclose(fp);

    return fileContents;
}


static void stopHandler(int sig)
{
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "received ctrl-c");
	running = false;
}

char *discovery_url = NULL;

static void
serverOnNetworkCallback(const UA_ServerOnNetwork *serverOnNetwork, UA_Boolean isServerAnnounce,
			UA_Boolean isTxtReceived, void *data)
{
     if(discovery_url != NULL || !isServerAnnounce) {
        UA_LOG_DEBUG(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "serverOnNetworkCallback called, but discovery URL "
                     "already initialized or is not announcing. Ignoring.");
        return; // we already have everything we need or we only want server announces
    }

    if(!isTxtReceived)
        return; // we wait until the corresponding TXT record is announced.
                // Problem: how to handle if a Server does not announce the
                // optional TXT?

    // here you can filter for a specific LDS server, e.g. call FindServers on
    // the serverOnNetwork to make sure you are registering with the correct
    // LDS. We will ignore this for now
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Another server announced itself on %.*s",
                (int)serverOnNetwork->discoveryUrl.length, serverOnNetwork->discoveryUrl.data);

    if(discovery_url != NULL)
        UA_free(discovery_url);
    discovery_url = (char*)UA_malloc(serverOnNetwork->discoveryUrl.length + 1);
    memcpy(discovery_url, serverOnNetwork->discoveryUrl.data, serverOnNetwork->discoveryUrl.length);
    discovery_url[serverOnNetwork->discoveryUrl.length] = 0;
}

static UA_Boolean
allowAddNode(UA_Server *server, UA_AccessControl *ac,
		const UA_NodeId *sessionId, void *sessionContext,
		const UA_AddNodesItem *item)
{
	//printf("Called allowAddNode \n");
	return UA_TRUE;
}

static UA_Boolean
allowDeleteNode(UA_Server *server, UA_AccessControl *ac,
		const UA_NodeId *sessionId, void *sessionContext,
		const UA_DeleteNodesItem *item)
{
	//printf("Called allowDeleteNode \n");
	return UA_TRUE;
}

static UA_Boolean
allowBrowseNode(UA_Server *server, UA_AccessControl *ac,
                const UA_NodeId *sessionId, void *sessionContext,
                const UA_NodeId *nodeId, void *nodeContext)
{
        //printf("Called allowBrowseNode \n");
        return UA_TRUE;
}

static UA_Boolean allowHistoryUpdateUpdateData(UA_Server *server, UA_AccessControl *ac,
                const UA_NodeId *sessionId, void *sessionContext,
                const UA_NodeId *nodeId,
                UA_PerformUpdateType performInsertReplace,
                const UA_DataValue *value)
{
	//printf("Called allowHistoryUpdateUpdateData \n");
	return UA_TRUE;
}

static UA_Boolean allowHistoryUpdateDeleteRawModified(UA_Server *server, UA_AccessControl *ac,
                const UA_NodeId *sessionId, void *sessionContext,
                const UA_NodeId *nodeId,
                UA_DateTime startTimestamp,
                UA_DateTime endTimestamp,
                bool isDeleteModified)
{
        //printf("Called allowHistoryUpdateDeleteRawModified \n");
	return UA_TRUE;
}

// global variable //
static const size_t usernamePasswordsSize = 2;
static UA_UsernamePasswordLogin logins[2] = {
        {UA_STRING_STATIC("jackybek"), UA_STRING_STATIC("thisisatestpassword24")},
	{UA_STRING_STATIC("admin"),UA_STRING_STATIC("defaultadminpassword24")}
};

/*
 * Get the endpoint from the server, where we can call RegisterServer2 (or RegisterServer).
 * This is normally the endpoint with highest supported encryption mode.
 *
 * @param discoveryServerUrl The discovery url from the remote server
 * @return The endpoint description (which needs to be freed) or NULL
 * https://github.com/open62541/open62541/blob/master/examples/discovery/server_multicast.c
 */
static
UA_EndpointDescription *getRegisterEndpointFromServer(const char *discoveryServerUrl) {
    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));
    UA_EndpointDescription *endpointArray = NULL;
    size_t endpointArraySize = 0;
    UA_StatusCode retval = UA_Client_getEndpoints(client, discoveryServerUrl,
                                                  &endpointArraySize, &endpointArray);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Array_delete(endpointArray, endpointArraySize,
                        &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "GetEndpoints failed with %s", UA_StatusCode_name(retval));
        UA_Client_delete(client);
        return NULL;
    }

    UA_LOG_DEBUG(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Server has %lu endpoints", (unsigned long)endpointArraySize);
    UA_EndpointDescription *foundEndpoint = NULL;
    for(size_t i = 0; i < endpointArraySize; i++) {
        UA_LOG_DEBUG(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "\tURL = %.*s, SecurityMode = %s",
                     (int) endpointArray[i].endpointUrl.length,
                     endpointArray[i].endpointUrl.data,
                     endpointArray[i].securityMode == UA_MESSAGESECURITYMODE_NONE ? "None" :
                     endpointArray[i].securityMode == UA_MESSAGESECURITYMODE_SIGN ? "Sign" :
                     endpointArray[i].securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT ? "SignAndEncrypt" :
                     "Invalid"
        );
        // find the endpoint with highest supported security mode
        if((UA_String_equal(&endpointArray[i].securityPolicyUri, &UA_SECURITY_POLICY_NONE_URI) ||
            UA_String_equal(&endpointArray[i].securityPolicyUri, &UA_SECURITY_POLICY_BASIC128_URI)) && (
            foundEndpoint == NULL || foundEndpoint->securityMode < endpointArray[i].securityMode))
            foundEndpoint = &endpointArray[i];
    }
    UA_EndpointDescription *returnEndpoint = NULL;
    if(foundEndpoint != NULL) {
        returnEndpoint = UA_EndpointDescription_new();
        UA_EndpointDescription_copy(foundEndpoint, returnEndpoint);
    }
    UA_Array_delete(endpointArray, endpointArraySize,
                    &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
    UA_Client_delete(client);
    return returnEndpoint;
}

//================================================================
static UA_StatusCode
createEndpoint(UA_ServerConfig *conf, UA_EndpointDescription *endpoint,
               const UA_SecurityPolicy *securityPolicy,
               UA_MessageSecurityMode securityMode)
{
    UA_EndpointDescription_init(endpoint);

    endpoint->securityMode = securityMode;
    UA_String_copy(&securityPolicy->policyUri, &endpoint->securityPolicyUri);
    endpoint->transportProfileUri =
        UA_STRING_ALLOC("http://opcfoundation.org/UA-Profile/Transport/uatcp-uasc-uabinary");

   // endpoint->securityLevel = (UA_Byte) securityMode;
   // UA_String_copy(&securityPolicy->policyUri, &endpoint->securityPolicyUri);
   // endpoint->transportProfileUri =
   //     UA_STRING_ALLOC("http://opcfoundation.org/UA-Profile/Transport/uatcp-uasc-uabinary");

    /* Add security level value for the corresponding message security mode */
    endpoint->securityLevel = (UA_Byte) securityMode;

    /* Enable all login mechanisms from the access control plugin  */
    UA_StatusCode retval = UA_Array_copy(conf->accessControl.userTokenPolicies,
                                         conf->accessControl.userTokenPoliciesSize,
                                         (void **)&endpoint->userIdentityTokens,
                                         &UA_TYPES[UA_TYPES_USERTOKENPOLICY]);

    	if(retval != UA_STATUSCODE_GOOD)
	{
        	UA_String_clear(&endpoint->securityPolicyUri);
        	UA_String_clear(&endpoint->transportProfileUri);
        	return retval;
    	}
    	endpoint->userIdentityTokensSize = conf->accessControl.userTokenPoliciesSize;

    	UA_String_copy(&securityPolicy->localCertificate, &endpoint->serverCertificate);
    	UA_ApplicationDescription_copy(&conf->applicationDescription, &endpoint->server);

    	return UA_STATUSCODE_GOOD;
}



//================================================================
//void* StartOPCUAServer(void* x_void_ptr, char* OPCipaddress, char* brokeripaddress)
//void* StartOPCUAServer(void* x_void_ptr, char* argv[])
void* StartOPCUALDSServer(void* x_void_ptr, char* argv)
{
	int sockfd;
	char* OPCLDSipaddress = argv;
	//char* brokeripaddress = argv[3];

	signal(SIGINT, stopHandler);
	signal(SIGTERM, stopHandler);

	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"In StartOPCLDSServer thread: argc=%d, argv=%s\n", g_argc, argv);

	//UA_Server* uaServer = (UA_Server*)x_void_ptr; -- caused it to crash when UA_newWithConfig(config);
	//UA_ServerConfig *config = UA_Server_getConfig(uaServer);

	UA_Server *uaLDSServer1 = NULL;

	UA_ServerConfig config1;
	memset(&config1, 0, sizeof(UA_ServerConfig));

	if (g_argc==2) 	// myNewLDSServer 192.168.1.44 //192.168.1.88 [192.168.1.11]
	{
		/*
			  #ifdef UA_ENABLE_PUBSUB
				// Details about the connection configuration and handling are located in
					//  the pubsub connection tutorial
				printf("\t----------------------Before setupUadpRange----------------- \n");
				//   setupUadpRange(server);
			  #endif
		*/
		//hostname or ip address available
			//copy the hostname from char * to an open62541 variable
		//printf("in g_argc segment %s %d\n", g_argv_ip, g_argv_port);

		//char* OPCUAServerIP = ipaddress;
		//UA_String hostname;
		//UA_String_init(&hostname);
		//hostname.length = strlen(g_argv[1]);
		//hostname.data = (UA_Byte *) g_argv[1];
		//hostname.length = strlen(g_argv_ip);
		//hostname.data = (UA_Byte *) g_argv_ip; // this should be the ipaddress of the OPCUA Server : 192.168.2.33

		//hostname.data = (UA_Byte*)OPCUAServerIP;

		//Change the configuration - deprecated in v1.1.3
		//UA_ServerConfig_setCustomHostname(config, hostname);
		//printf("hostname.data (ip) = %s\n", hostname.data);

		//#endif

                // start OPCUA LDS Server
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "starting LDS server...");

		UA_StatusCode retval;
                //----------------------------------------
                // configure port 4840 as the port for external communication - support encryption and userid/ password
                // ---------------------------------------
		// encryption routine
    		/* Load certificate and private key */
		const char* env_sslcertificateloc = getenv("SSLCERTIFICATELOC");

		UA_ByteString certificate = loadFile(env_sslcertificateloc); //(SSLCERTIFICATELOC);
		//UA_ByteString certificate = loadFile("/etc/ssl/certs/ldscert44.pem");	// => symbolic link
   		//UA_ByteString certificate = loadFile("/usr/local/ssl/certs/ldscert44.pem");  // actual location
		if (certificate.length == 0)
		{
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"LDS_StartServer.c : cannot find /usr/local/ssl/certs/ldscert44.pem");
			goto cleanup;
		}
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"LDS_StartServer.c : Successfully loaded LDS Certficate : %s", env_sslcertificateloc);
 
		const char* env_privatekeyloc = getenv("PRIVATEKEYLOC");
    		UA_ByteString privateKey = loadFile(env_privatekeyloc); //loadFile(PRIVATEKEYLOC);
    		//UA_ByteString privateKey = loadFile("/usr/local/ssl/private/ldsprivate-key.pem");
		if (privateKey.length == 0)
		{
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"LDS_StartServer.c : cannot find /usr/local/ssl/private/ldsprivate-key.pem");
			goto cleanup;
		}
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"LDS_StartOPCUAServer.c : Successfully loaded LDS privateKey : %s", env_privatekeyloc);

    		/* Load the trustlist */
    		size_t trustListSize = 1;
    		//UA_STACKARRAY(UA_ByteString, trustList, trustListSize);
    		//for(size_t i = 0; i < trustListSize; i++)
        	//	trustList[i] = loadFile(TRUSTLISTLOC); //"/usr/local/ssl/trustlist/trustlist.crl");
		UA_ByteString *trustList = (UA_ByteString *)UA_Array_new(1, &UA_TYPES[UA_TYPES_BYTESTRING]);
		UA_ByteString_copy(&certificate, &trustList[0]);
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"LDS_StartOPCUAServer.c : Successfully loaded LDS trustlist");

    		/* Loading of a issuer list, not used in this application */
    		size_t issuerListSize = 0;
    		UA_ByteString *issuerList = NULL;

    		/* Loading of a revocation list currently unsupported */
    		UA_ByteString *revocationList = NULL;
    		size_t revocationListSize = 0;
	/*
		#ifdef __linux__
			const char *trustlistFolder = NULL;
			const char *issuerlistFolder = NULL;
			const char *revocationlistFolder = NULL;

			retval = UA_ServerConfig_setDefaultWithSecurityPolicies(&config1, 4840,
							&certificate, &privateKey,
							NULL, 0,
							NULL, 0,
							NULL, 0);
			if (retval != UA_STATUSCODE_GOOD)
			{
				UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"LDS_StartServer.c : error loading Server Configuration");
				goto cleanup;
			}
			else
				UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"LDS_StartOPCUAServer.c : Successfully loaded Server Configuration");

			config1.certificateVerification.clear(&config1.certificateVerification);
				#ifdef UA_ENABLE_CERT_REJECTED_DIR
					retval = UA_CertificateVerification_CertFolders(&config1.certificateVerification,
                                                                        trustlistFolder, issuerlistFolder,
                                                                        revocationlistFolder, NULL);
				#else
					retval = UA_CertificateVerification_CertFolders(&config1.certificateVerification,
                                                                        trustlistFolder, issuerlistFolder,
                                                                        revocationlistFolder);
				#endif
			if (retval != UA_STATUSCODE_GOOD)
                	{
                        	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"LDS_StartServer.c : error verifying Certificate folders");
                        	goto cleanup;
                	}
			else
				UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"LDS_StartOPCUAServer.c : Successfully verified Certificate folders");

		#else //not __linux__
	*/
                        retval = UA_ServerConfig_setDefaultWithSecurityPolicies(&config1, 4840,			// swap config with config1
                                                       &certificate, &privateKey,
                                                       trustList, trustListSize,
                                                       issuerList, issuerListSize,
                                                       revocationList, revocationListSize);

			if (retval != UA_STATUSCODE_GOOD)
                	{
                        	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"LDS_StartServer.c : error loading Server Configuration");
                        	goto cleanup;
                	}
	//	#endif

                // refer to open62541.org->Server->Server Configuration & plugins/ua_config_default for the list of members in the UA_ServerConfig structure
		if (!&config1)
		{
			//return UA_STATUSCODE_BADINVALIDARGUMENT;
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"LDS_StartServer.c : error loading Server Configuration");
			goto cleanup;
		}
		if (config1.nodestore.context == NULL)
			UA_Nodestore_HashMap(&config1.nodestore);

		//if (!config1.logger.log)
		//	config1.logger = UA_Log_Stdout;

                // Change the configuration
                char* OPCUALDSServerIP = OPCLDSipaddress;   // 192.168.2.44
                //UA_String hostname;
                //UA_String_init(&hostname);
                //hostname.length = strlen(g_argv[1]);
                //hostname.data = (UA_Byte *) g_argv[1];
                //hostname.length = strlen(OPCUALDSServerIP);
                //hostname.data = (UA_Byte*)OPCUALDSServerIP;
                //UA_ServerConfig_setCustomHostname(&config1, hostname);
                //printf("hostname.data (ip) = %s\n", hostname.data);

                config1.shutdownDelay = 0; //5000.0; // millisecond
                config1.securityPolicyNoneDiscoveryOnly = UA_TRUE;
 //		config1.serverCertificate = certificate;

		// Server Description
		UA_BuildInfo_clear(&config1.buildInfo);
		const char* env_product_uri = getenv("PRODUCT_URI");
		const char* env_manufacturer_name = getenv("MANUFACTURER_NAME");
		const char* env_product_name = getenv("PRODUCT_NAME");
		const char* env_application_uri_server = getenv("APPLICATION_URI_SERVER");
		const char* env_application_name = getenv("APPLICATION_NAME");

		config1.buildInfo.productUri = UA_STRING_ALLOC(env_product_uri);  //(PRODUCT_URI);
		config1.buildInfo.manufacturerName = UA_STRING_ALLOC(env_manufacturer_name); //(MANUFACTURER_NAME);
		config1.buildInfo.productName = UA_STRING_ALLOC(env_product_name); //(PRODUCT_NAME);
		config1.buildInfo.softwareVersion =
			UA_STRING_ALLOC(VERSION(UA_OPEN62541_VER_MAJOR, UA_OPEN62541_VER_MINOR,
						UA_OPEN62541_VER_PATCH, UA_OPEN62541_VER_LABEL));

		config1.buildInfo.buildDate = UA_DateTime_now();
		config1.buildInfo.buildNumber = UA_STRING_ALLOC(__DATE__ " " __TIME__);

		UA_ApplicationDescription_clear(&config1.applicationDescription);
		UA_String_clear(&config1.applicationDescription.applicationUri);
		config1.applicationDescription.applicationUri.length = strlen(env_application_uri_server); //(APPLICATION_URI_SERVER);
                config1.applicationDescription.applicationUri = UA_String_fromChars(env_application_uri_server); //(APPLICATION_URI_SERVER); //UA_STRING_ALLOC(APPLICATION_URI_SERVER);
		config1.applicationDescription.productUri = UA_STRING_ALLOC(env_product_uri); //PRODUCT_URI);
		config1.applicationDescription.applicationName = UA_LOCALIZEDTEXT_ALLOC("en", env_application_name); //, APPLICATION_NAME);

		// LDS ++ refer to https://opcfoundation.org/UA/schemas/1.04/ServerCapabilities.csv
		// NA, DA, HD, AC, HE, GDS, LDS, DI, ADI, FDI, FDIC, PLC, S95, RCP, PUB, AUTOID, MDIS, CNC, PLK, FDT, TMC, CSPP, 61850, PACKML, MTC
		// AUTOML, SERCOS, MIMOSA, WITSML, DEXPI, IOLINK, VROBOT, PNO
		config1.applicationDescription.applicationType = UA_APPLICATIONTYPE_DISCOVERYSERVER;	// acts as DISCOVERY SERVER ONLY OR UA_APPLICATIONTYPE_SERVER

		// Multicast DNS related settings - LDS - refer to github/open62541/open62541/examples/discovery/server_lds.c
		config1.mdnsEnabled = true;
		config1.mdnsConfig.mdnsServerName = UA_String_fromChars("Local Discovery Server");
		config1.mdnsInterfaceIP = UA_String_fromChars("0.0.0.0");	// 42.42.42.42
		// set the capabilities
		config1.mdnsConfig.serverCapabilitiesSize = 1;
		UA_String *caps = (UA_String *)UA_Array_new(1, &UA_TYPES[UA_TYPES_STRING]);
		caps[0] = UA_String_fromChars("LDS");	// local discovery service
		//caps[1] = UA_String_fromChars("HD");	// provide historical data
		//caps[2] = UA_String_fromChars("DA");	// provide current data
		//caps[3] = UA_String_fromChars("GDS");	// global discovery Server information model
		config1.mdnsConfig.serverCapabilities = caps;
		config1.discoveryCleanupTimeout = 60*60;

		config1.verifyRequestTimestamp = UA_RULEHANDLING_ACCEPT; //UA_RULEHANDLING_WARN;

	    	/* config1->applicationDescription.gatewayServerUri = UA_STRING_NULL; */
    		/* config1->applicationDescription.discoveryProfileUri = UA_STRING_NULL; */
    		/* config1->applicationDescription.discoveryUrlsSize = 0; */
    		/* config1->applicationDescription.discoveryUrls = NULL; */

		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "LDS_StartOPCUAServer.c : after making changes to config1");

		// Certificate Verification that accepts every certificate. Can be overwritten when the policy is specialized.
		// required for LDS
//		UA_CertificateVerification_AcceptAll(&config1.certificateVerification);
		config->secureChannelPKI.clear(&config->secureChannelPKI);
		UA_ByteString_clear(&certificate);
		UA_ByteString_clear(&privateKey);
		for (size_t i = 0; i < trustListSize; i++)
			UA_ByteString_clear_&trustList[i]);
		
		// Limits for SecureChannels - required for LDS
		config1.maxSecureChannels = 40;
		config1.maxSecurityTokenLifetime = 10 * 60 * 1000; // 10 minutes */

		// Limits for Sessions - required for LDS
		config1.maxSessions = 100;
		config1.maxSessionTimeout = 60 * 60 * 1000;	// 1 hour

		// Limits for MonitoredItems - not required for LDS
		//config1.samplingIntervalLimits = UA_DURATIONRANGE(50.0, 24.0 * 3600.0 * 1000.0);
		//config1.queueSizeLimits = UA_UINT32RANGE(1, 100);

		// Limits for Discovery - required for LDS
		config1.discoveryCleanupTimeout = 60 * 60;

		UA_ByteString_clear(&certificate);
    		UA_ByteString_clear(&privateKey);
    		for(size_t i = 0; i < trustListSize; i++)
        		UA_ByteString_clear(&trustList[i]);
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "LDS_StartServer.c : after encryption routine");
	// end encryption routine

	// add userid and password routine
  		// disable anonymous logins (2nd parameter set to false), enable 2 user/password logins
		config1.accessControl.clear(&config1.accessControl);
		retval = UA_AccessControl_default(&config1, UA_FALSE, NULL, &config1.securityPolicies[config1.securityPoliciesSize-1].policyUri, 2, logins);
		if (retval != UA_STATUSCODE_GOOD)
			goto cleanup;

		// set accessControl functions for nodeManagement - not required for a LDS server
		/*
		config1.accessControl.allowAddNode = allowAddNode;
		config1.accessControl.allowDeleteNode = allowDeleteNode;
		config1.accessControl.allowBrowseNode = allowBrowseNode;
		config1.accessControl.allowHistoryUpdateUpdateData = allowHistoryUpdateUpdateData;
		config1.accessControl.allowHistoryUpdateDeleteRawModified = allowHistoryUpdateDeleteRawModified;
		*/
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "adding 2 user credentials to OPCUA LDS server ...\n");
	// end userid and password routine

	// Historizing
	/* not required for a LDS server
                config1.accessHistoryDataCapability = true;
                config1.maxReturnDataValues = 0 ;  // 0 means unlimited size
                config1.accessHistoryEventsCapability = true;
                config1.maxReturnEventValues = 0;  // 0 means unlimted size
                config1.insertDataCapability = true;
                config1.insertEventCapability = true;
                config1.insertAnnotationsCapability = true;
                config1.replaceDataCapability = true;
                config1.replaceEventCapability = true;
                config1.updateDataCapability = true;
                config1.updateEventCapability = true;
                config1.deleteRawCapability = true;
                config1.deleteEventCapability = true;
                config1.deleteAtTimeDataCapability = true;*/
	/*
		#ifdef UA_ENABLE_PUBSUB
					printf("\tJust about to start the OPCUA Server UADP thread\n");

					// start OPCUA Server
					UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "starting server...");

					retval = UA_Server_run(server, &running); // blocking call - to test United Automation (ok)
					if(retval != UA_STATUSCODE_GOOD)
							goto cleanup;
		#else
	*/
	// start OPCUA LDS Server
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "starting OPCUA LDS server ...\n");

	uaLDSServer1 = UA_Server_newWithConfig(&config1);

        //Add a new namespace to the server => Returns the index of the new namespace i.e. namespaceIndex
        UA_Int16 nsIdx_LDS = UA_Server_addNamespace(uaLDSServer1, "LDS");
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "New Namespace added with Nr. %d", nsIdx_LDS);

	// create nodes - not required as LDS
	//UA_NodeId r2_airgard_method_Id = CreateOPCUANodes(uaServer1);
	// create method items - not required as LDS
	//CreateServerMethodItems(uaServer1, r2_airgard_method_Id);
	// connect to History database engine - mySQL - not required as LDS
	//GetHistoryDBConnection();

	// create events for remote control : methodcall -> event
//	CreateServerEvents(uaServer1);

        // create monitored items - not required as LDS
        //CreateServerMonitoredItems(uaServer1);
	// create historizing - needs monitored items to be setup first - not required as LDS
	//CreateServerHistorizingItems(uaServer1);
	// enable pub sub uadp / mqtt / AMQP
	printf("LDS_StartOPCUAServer.c : g_argc = %d \n", g_argc);
	/*
	if (g_argc == 3)
	{
		// myNewServer 192.168.1.33 192.168.1.88	== using PubSub UADP
		CreateServerPubSub(uaLDSServer1, NULL, nsIdx_LDS, NULL);	// namespaceIndex is needed for connectionProperties 
	}
	else if (g_argc == 4)
		CreateServerPubSub(uaLDSServer1, brokeripaddress, nsIdx_LDS, "pubsub");
	else if (g_argc == 5)	// argv[4] = pubsub, pub, sub
		CreateServerPubSub(uaLDSServer1, brokeripaddress, nsIdx_LDS, argv[4]);
	*/
	/*
	#ifdef UA_ENABLE_WEBSOCKET_SERVER
    		UA_ServerConfig_addNetworkLayerWS(UA_Server_getConfig(uaLDSServer1), 7681, 0, 0, NULL, NULL);
	#endif
	*/
	retval = UA_Server_run_startup(uaLDSServer1);
	UA_Server_setServerOnNetworkCallback(uaLDSServer1, serverOnNetworkCallback, NULL);
	if (retval != UA_STATUSCODE_GOOD)
	{
		UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,"Could not start the LDS server. StatusCode %s", UA_StatusCode_name(retval));
		UA_Server_delete(uaLDSServer1);
		goto cleanup;
	}
	else
	{
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "OPCUA LDS server started successfully. Waiting for announcement of LDS Server ...\n");
		while (running && discovery_url == NULL)
			UA_Server_run_iterate(uaLDSServer1, true);
		if (!running)
		{
			UA_Server_delete(uaLDSServer1);
			UA_free(discovery_url);
			goto cleanup;
		}
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "LDS-ME server found on %s", discovery_url);

    		/* Check if the server supports sign and encrypt. OPC Foundation LDS
     		* requires an encrypted session for RegisterServer call, our server
     		* currently uses encrpytion optionally */
    		UA_EndpointDescription *endpointRegister = getRegisterEndpointFromServer(discovery_url);
    		UA_free(discovery_url);
    		if(endpointRegister == NULL || endpointRegister->securityMode == UA_MESSAGESECURITYMODE_INVALID) 
		{
        		UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     	"Could not find any suitable endpoints on discovery server");
        		UA_Server_delete(uaLDSServer1);
        		goto cleanup;
		}
	//	#endif
	}
	} // if (argc ==2)

cleanup:
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "OPCUA LDS Server was unexpectedly shut down");
	if (uaLDSServer1)	// (uaServer)
		UA_Server_delete(uaLDSServer1); // UA_Server_delete(uaServer);
	//else
		//UA_ServerConfig_clean(&config1); // UA_ServerConfig_clean(config);

	close(sockfd);
	//CloseHistoryDBConnection();
	//return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
//#endif
}


