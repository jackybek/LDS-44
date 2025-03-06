#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <stdio.h>

int main(int argc, char** argv);
int encryptServer(UA_Server *);
int configureServer(UA_Server *);
char * discovery_url = NULL;
static void stopHandler(int);

static volatile UA_Boolean running = true;
static void stopHandler(int sig)
{
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "received ctrl-c");
        running = false;
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
     UA_ServerConfig_setMinimal(config, 4840, NULL);

     int retval = encryptServer(uaLDSServer);
	 if retval != UA_STATUSCODE_GOOD
	 {
		UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,"Could not encrypt the LDS server : %s", UA_StatusCode_name(retval));
		return EXIT_FAILURE;
	 }

	 int retval = configureServer(uaLDSServer);
	 if retval != UA_STATUSCODE_GOOD
	 {
		UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,"Could not configure the LDS server : %s", UA_StatusCode_name(retval));
		return EXIT_FAILURE;
	 }
	 
	//Add a new namespace to the server => Returns the index of the new namespace i.e. namespaceIndex
    UA_Int16 nsIdx_LDS = UA_Server_addNamespace(uaLDSServer1, "LDS");
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "New Namespace added with Nr. %d", nsIdx_LDS);

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
         * currently uses encrpytion optionally */
        UA_EndpointDescription *endpointRegister = getRegisterEndpointFromServer(discovery_url);
        UA_free(discovery_url);
        if(endpointRegister == NULL || endpointRegister->securityMode == UA_MESSAGESECURITYMODE_INVALID)
        {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                    "Could not find any suitable endpoints on discovery server");
            goto cleanup;
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
		UA_Server_delete(uaServer);
        return EXIT_SUCCESS;

	 
}
