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

int main(int argc, char** argv);
int encryptServer(UA_Server *, UA_ServerConfig *);
int configureServer(UA_Server *);
static void stopHandler(int);
static void serverOnNetworkCallback(const UA_ServerOnNetwork *, UA_Boolean, UA_Boolean, void *);

char * discovery_url = NULL;
static volatile UA_Boolean running = true;

static void stopHandler(int sig)
{
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "received ctrl-c");
        running = false;
}

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
    } else
	    return endpointArray;
}

int main(int argc, char *argv[])
{
     if (argc != 2)
        {
                printf("Usage : ./myNewLDSServer <local ip> \n");
                return EXIT_FAILURE;
        }

     signal(SIGINT, stopHandler);
     signal(SIGTERM, stopHandler);
 
	UA_Server *uaLDSServer = UA_Server_new();
     	UA_ServerConfig *config = UA_Server_getConfig(uaLDSServer);
	UA_ServerConfig_clean(config);
     	UA_ServerConfig_setMinimal(config, 4840, NULL);

     int status = encryptServer(uaLDSServer, config);
	 if (status != UA_STATUSCODE_GOOD)
	 {
		UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,"Could not encrypt the LDS server : %s", UA_StatusCode_name(status));
		return EXIT_FAILURE;
	 }

	 status = configureServer(uaLDSServer);
	 if (status != UA_STATUSCODE_GOOD)
	 {
		UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,"Could not configure the LDS server : %s", UA_StatusCode_name(status));
		return EXIT_FAILURE;
	 }
 
	//Add a new namespace to the server => Returns the index of the new namespace i.e. namespaceIndex
    	UA_Int16 nsIdx_LDS = UA_Server_addNamespace(uaLDSServer, "LDS");
    	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "New Namespace added with Nr. %d", nsIdx_LDS);

    	status = UA_Server_run_startup(uaLDSServer);
    	UA_Server_setServerOnNetworkCallback(uaLDSServer, serverOnNetworkCallback, NULL);
    	if (status != UA_STATUSCODE_GOOD)
    	{
        	UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,"Could not start the LDS server. StatusCode %s", UA_StatusCode_name(status));
        	UA_Server_delete(uaLDSServer);
        	goto cleanup;
    	}
    	else
    	{
        	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "OPCUA LDS server started successfully. Waiting for announcement of LDS Server ...\n");
        	while (running && discovery_url == NULL)
            		UA_Server_run_iterate(uaLDSServer, true);
        	if (!running)
        	{
            		UA_Server_delete(uaLDSServer);
            		UA_free(discovery_url);
            		goto cleanup;
        	}
        	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "LDS-ME server found on %s", discovery_url);

        	/* Check if the server supports sign and encrypt. OPC Foundation LDS
         	 * requires an encrypted session for RegisterServer call, our server
	         * currently uses encrpytion optionally 
	  	*/
        	UA_EndpointDescription *endpointRegister = getRegisterEndpointFromServer(discovery_url);
        	UA_free(discovery_url);
        	if(endpointRegister == NULL || endpointRegister->securityMode == UA_MESSAGESECURITYMODE_INVALID)
        	{
            		UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                    	"Could not find any suitable endpoints on discovery server");
            		goto cleanup;
        	}
	}	
cleanup:
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "OPCUA LDS Server shutting down");
        if (uaLDSServer)
        {
                UA_Server_run_shutdown(uaLDSServer);
                UA_Server_delete(uaLDSServer); 
                return (EXIT_SUCCESS);
        }

        //close(sockfd);

        while(running)
                UA_Server_run_iterate(uaLDSServer, true);

        UA_Server_run_shutdown(uaLDSServer);
		UA_Server_delete(uaLDSServer);
        return EXIT_SUCCESS;
}
