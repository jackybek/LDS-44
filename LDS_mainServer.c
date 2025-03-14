#ifdef compile
.. Local Directory Server ..
sudo gcc -g -std=c99 -lrt -v -I/usr/local/include -I/home/pi/open62541/ -I/home/pi/open62541/plugins/ -lm -lrt \
-L/usr/local/lib -lpthread  -lcrypto -lssl -lmbedcrypto -lmbedtls -lmbedx509 \
open62541.c LDS_StartServer.c LDS_mainServer.c -o myNewLDSServer >& error-msg

./valgrind --leak-check=yes --show-leak-kinds=all ./myNewServer 192.168.1.109 192.168.1.88 192.168.1.119

gcc with -g to debug
gcc -mandroid -g -std=c99 -lrt -v -I/home/pi/open62541/ -I/home/pi/open62541/plugins/ -ljson-c -lmariadbclient -lpthread -lxml2 -lcrypto -lssl open62541.c \
SV_ExtractXMLElementNames.c SV_Event.c SV_Monitor.c SV_Method.c SV_Historizing.c SV_PopulateOPCUANodes.c SV_CreateOPCUANodes.c SV_StartOPCUAServer.c \
json_checker.c SV_PubSub.c SV_mainOPCUAServer.c -o myAndroidServer >& error-msg

#endif

//#include "open62541.h"
#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <stdio.h>
//#include <mariadb/mysql.h>

#ifdef useless
#define MAX_STRING_SIZE 64
typedef struct {
char Tag[MAX_STRING_SIZE];
char Name[MAX_STRING_SIZE];
float Probability;
char CASnumber[MAX_STRING_SIZE];
int Concentration;
} AlarmStruct;

typedef struct {
char Tag[MAX_STRING_SIZE];
char Name[MAX_STRING_SIZE];
float Probability;
char CASnumber[MAX_STRING_SIZE];
int Concentration;
} NonAlarmStruct;
#endif

int main(int argc, char** argv);
void* StartOPCUALDSServer(void* x_void_ptr, char*);
//UA_NodeId CreateOPCUANodes(void* x_void_ptr);
//void PopulateOPCUANodes(char* g_argv_ip);
int g_argc;

#ifdef useless
int sockfd;
int command_sockfd;

char g_argv_ip[255];
int g_argv_port;
UA_Boolean UA_Nodes_Setup;

char SoftwareVersion[MAX_STRING_SIZE];
char DataBlockVersion[MAX_STRING_SIZE];
char InstrumentTime[MAX_STRING_SIZE];
char MeasurementTime[MAX_STRING_SIZE];

char BootStatus[MAX_STRING_SIZE];
char SnapshotStatus[MAX_STRING_SIZE];
char SCPStatus[MAX_STRING_SIZE];
char SFTPStatus[MAX_STRING_SIZE];
char RunScriptStatus[MAX_STRING_SIZE];
char ArchiveStatus[MAX_STRING_SIZE];
char AncillarySensorStatus[MAX_STRING_SIZE];

char Sensor[MAX_STRING_SIZE];
UA_Int16 OperatingTime;
char WarningMessage[MAX_STRING_SIZE];

UA_Float IgramPP;
UA_Float IgramDC;
UA_Float LaserPP;
UA_Float LaserDC;
UA_Float SingleBeamAt900;
UA_Float SingleBeamAt2500;
UA_Int16 SignalToNoiseAt2500;
UA_Float CenterBurstLocation;
UA_Float DetectorTemp;
UA_Float LaserFrequency;
UA_Int16 HardDriveSpace;
UA_Int16 Flow;
UA_Int16 Temperature;
UA_Float Pressure;
UA_Int16 TempOptics;
UA_Int16 BadScanCounter;
UA_Int16 FreeMemorySpace;

char LABFilename[MAX_STRING_SIZE];
char LOGFilename[MAX_STRING_SIZE];
char LgFilename[MAX_STRING_SIZE];
char SecondLgFilename[MAX_STRING_SIZE];

UA_Float SystemCounter;
UA_Float DetectorCounter;
UA_Float LaserCounter;
UA_Float FlowPumpCounter;
UA_Float DesiccantCounter;

UA_Int16 NoOfAlarms;
UA_Int16 NoOfNonAlarms;

int NoOfAlarmsNode;
int NoOfNonAlarmsNode;
AlarmStruct arrayOfAlarm[255];  //101
AlarmStruct arrayOfNonAlarm[255];
char AlarmTag[MAX_STRING_SIZE];
char AlarmName[MAX_STRING_SIZE];
UA_Float AlarmProbability;
char AlarmCASnumber[MAX_STRING_SIZE];
UA_Int16 AlarmConcentration;

UA_Boolean UA_Nodes_Setup = false;

// for Historizing & PubSub
UA_NodeId outSoftwareVersion_Id;
UA_NodeId outIgramPP_Id;
UA_NodeId outIgramDC_Id;
UA_NodeId outLaserPP_Id;
UA_NodeId outLaserDC_Id;
UA_NodeId outSingleBeamAt900_Id;
UA_NodeId outSingleBeamAt2500_Id;
UA_NodeId outSignalToNoiseAt2500_Id;
UA_NodeId outCenterBurstLocation_Id;
UA_NodeId outDetectorTemp_Id;
UA_NodeId outLaserFrequency_Id;
UA_NodeId outHardDriveSpace_Id;
UA_NodeId outFlow_Id;
UA_NodeId outTemperature_Id;
UA_NodeId outPressure_Id;
UA_NodeId outTempOptics_Id;
UA_NodeId outBadScanCounter_Id;
UA_NodeId outFreeMemorySpace_Id;
UA_NodeId outLABFilename_Id;
UA_NodeId outLOGFilename_Id;
UA_NodeId outLgFilename_Id;
UA_NodeId outSecondLgFilename_Id;
UA_NodeId outSystemCounter_Id;
UA_NodeId outDetectorCounter_Id;
UA_NodeId outLaserCounter_Id;
UA_NodeId outFlowPumpCounter_Id;
UA_NodeId outDesiccantCounter_Id;

//MYSQL *conn;
#endif

int main(int argc, char *argv[])
{
        //pthread_t OPCUAServerthread;
        //pthread_t Airgardthread;
	//int results;

	//UA_Nodes_Setup = false;
	//UA_NodeId r2_airgard_data_Id;

	if (argc != 2)
	{
		//printf("Usage : ./myNewLDSServer <local ip> <sensor ip> [<mqtt broker ip> <{pubsub}|pub|sub]> \n");
		printf("Usage : ./myNewLDSServer <local ip> \n");
		exit (0);
	}

        UA_Server *server = NULL; // UA_Server_new();	// UA_Server_new(config)
	//UA_ServerConfig *config = UA_Server_getConfig(server); do this in StartOPCUAServer.c
	//UA_ServerConfig_setDefault(config);	// do this in StartOPCUAServer.c

        g_argc = argc;
//	strcpy(g_argv_ip, argv[1]);	// 192.168.1.44
//	g_argv_port = atoi(argv[2]);

//	printf("In main(): g_argc = %d, argv = %s %s %s \n", g_argc, argv[0], argv[1], argv[2]);
//	printf("In main() after processing argv: g_argc = %d, g_argv = %s %s %d\n", g_argc, argv[0], g_argv_ip, g_argv_port);

    //if (results = pthread_create(&OPCUAServerthread, NULL, StartOPCUAServer, server))
		//StartOPCUAServer(server, argv[1], argv); //(server, 192.168.1.109, 192.168.1.11);
		StartOPCUALDSServer(server, argv[1]); //(server, 192.168.1.44);
	/*
		if (UA_NodeId_isNull(&r2_airgard_data_Id))
                {
                        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "AG_mainOPCUAlient : fail to get handle to r2_airgard_data_Id");
                        goto cleanup;
                }

		while (1)
		{
			r2_airgard_data_Id = CreateOPCUANodes(server);
			PopulateOPCUANodes(argv[2]);
		}
	*/

		//printf("Error creating thread : StartOPCUAServer\n") ;
	//else
		//printf("%d Success : pthread_create StartOPCUAServer\n", results);


//        if (results = pthread_create(&Airgardthread, NULL, ConnectToAirgard, server))
//		printf("Error creating thread : ConnectToAirgard\n") ;
//	else
//		printf("%d Success : pthread_create ConnectToAirgard\n", results);

	//pthread_join(OPCthread, NULL);
	//pthread_join(Airgardthread, NULL);

	//pthread_exit(NULL);
//cleanup:
//	return 0;
}
