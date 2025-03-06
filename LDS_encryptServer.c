#include <open62541/plugin/log_stdout.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/create_certificate.h>
#include <open62541/plugin/securitypolicy.h>
#include <open62541/plugin/accesscontrol_default.h>
#include <open62541/plugin/nodestore_default.h>
#include <stdio.h>

UA_ByteString loadFile(const char *const);
int encryptServer(UA_Server *);
int generateSSCert(UA_Server *, UA_ByteString *, size_t, UA_ByteString *, size_t, UA_ByteString *, size_t);

int encryptServer(UA_Server *uaLDSServer)
{
	UA_StatusCode status;
	
	UA_ServerConfig *config = UA_Server_getConfig(uaLDSServer);
	
	const char *env_sslcertificateloc = getenv("SSLCERTIFICATELOC");
    	UA_ByteString certificate = loadFile(env_sslcertificateloc);
	const char *env_privatekeyloc = getenv("PRIVATEKEYLOC");
   	UA_ByteString privateKey = loadFile(env_privatekeyloc);
	
	if (certificate.length == 0)
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"LDS_encryptServer.c : cannot find %s", env_sslcertificateloc);
	if (privateKey.length == 0)
		 UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"LDS_encrypterver.c : cannot find %s", env_privatekeyloc);

	// Load trustlist
	UA_ByteString *trustList = (UA_ByteString *)UA_Array_new(1, &UA_TYPES[UA_TYPES_BYTESTRING]);
	UA_ByteString_copy(&certificate, &trustList[0]);
	size_t trustListSize = 1;
    	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"LDS_encryptServer.c : Successfully loaded LDS trustlist");
	// Loading of a issuer list, not used in this application
    	UA_ByteString *issuerList = NULL;
	size_t issuerListSize = 0;
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"LDS_encryptServer.c : issueList is not supported");
	// Loading of a revocation list currently unsupported
	UA_ByteString *revocationList = NULL;
    	size_t revocationListSize = 0;
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"LDS_encryptServer.c : revocationList is not supported");	
	
	
	if ( (certificate.length != 0) && (privateKey.length != 0) ) 
	{
		status = UA_ServerConfig_setDefaultWithSecurityPolicies(config, 4840,                  
                                                       &certificate, &privateKey,
                                                       trustList, trustListSize,
                                                       issuerList, issuerListSize,
                                                       revocationList, revocationListSize);
		UA_assert(status == UA_STATUSCODE_GOOD);
		if (status != UA_STATUSCODE_GOOD)
			UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
        		"LDS_encrypterver.c : Could not initiaise server with default security policues with error code %s",
        		UA_StatusCode_name(status));
		else
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
        		"LDS_encrypterver.c : Server initialised with default security policues with error code %s",
        		UA_StatusCode_name(status));
	}
	else
	{
		status = generateSSCert(uaLDSServer,
				trustList, trustListSize,
                                issuerList, issuerListSize,
                                revocationList, revocationListSize);
		
		UA_assert(status == UA_STATUSCODE_GOOD);
	}
	
	// add the security policies
	status = UA_ServerConfig_addSecurityPolicyBasic256Sha256(config, &certificate, &privateKey);
    	if(status != UA_STATUSCODE_GOOD) {
		UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
        	"LDS_encrypterver.c : Could not add SecurityPolicy#Basic256Sha256 with error code %s",
        	UA_StatusCode_name(status));
    }

    status = UA_ServerConfig_addSecurityPolicyAes256Sha256RsaPss(config, &certificate, &privateKey);
    if(status != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
        "LDS_encrypterver.c : Could not add SecurityPolicy#AES256Sha256RsaPss with error code %s",
        UA_StatusCode_name(status));
    }

    status = UA_ServerConfig_addSecurityPolicyAes128Sha256RsaOaep(config, &certificate, &privateKey);
    if(status != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
        "LDS_encrypterver.c : Could not add SecurityPolicy#Aes128Sha256RsaOaep with error code %s",
        UA_StatusCode_name(status));
    }
	
	UA_ByteString_clear(trustList);
	return status;
}
	
int generateSSCert(UA_Server *uaLDSServer,
		   UA_ByteString *trustList, size_t trustListSize,
                   UA_ByteString *issuerList, size_t issuerListSize,
                   UA_ByteString *revocationList, size_t revocationListSize)
{	
	
	UA_ByteString derPrivKey;
	UA_ByteString derCert;

	UA_String subject[3] = {UA_STRING_STATIC("C=SG",
				UA_STRING_STATIC("S=Singapore",
				UA_STRING_STATUC("CN=lds.virtualskies.com.sg") };
	
	/*
	UA_String subject[7] = {UA_STRING_STATIC("C=SG"),
				UA_STRING_STATIC("S=Singapore"),
				UA_STRING_STATIC("LO=Singapore"),
				UA_STRING_STATIC("O=Virtual Skies"),
				UA_STRING_STATIC("U=IT"),
				UA_STRING_STATIC("CN=lds.virtualskies.com.sg"),
				UA_STRING_STATIC("EM=jacky81100@yahoo.com") };
	*/
	
	UA_UInt32 lenSubject = 3;
	UA_String subjectAltName[2] = {UA_STRING_STATIC("DNS.1:localhost"),
				       UA_STRING_STATIC("DNS.2:lds.virtualskies.com.sg") };
	UA_UInt32 lenSubjectAltName = 2;
	
	UA_KeyValueMap *kvm = UA_KeyValueMap_new();
	UA_UInt16 expiresIn = 3650;
	UA_KeyValueMap_setScalar(kvm, UA_QUALIFIEDNAME(0, "expires-in-days"),
							(void *)&expiresIn , &UA_TYPES[UA_TYPES_UINT16]);
	UA_UInt16 keyLength = 2048;
	UA_KeyValueMap_setScalar(kvm, UA_QUALIFIEDNAME(0, "key-size-bits"),
							(void *)&keyLength, &UA_TYPES[UA_TYPES_UINT16]);
							
	
	// creates the certificate and keys
	UA_StatusCode status = UA_CreateCertificate(
						UA_Log_Stdout, subject, lenSubject, subjectAltName, lenSubjectAltName,
						UA_CERTIFICATEFORMAT_DER, kvm, &derPrivKey, &derCert);
	UA_KeyValueMap_delete(kvm);
	
	UA_assert(status == UA_STATUSCODE_GOOD);
	UA_assert(derPrivKey.length > 0);
	UA_assert(derCert.length > 0);
	
	UA_ServerConfig *config = UA_Server_getConfig(uaLDSServer);

	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"LDS_encryptServer.c : Generating self signed certificate and key");
	status = UA_ServerConfig_setDefaultWithSecurityPolicies(config, 4840, 
							&derCert, &derPrivKey,
							trustList, trustListSize, 
							issuerList, issuerListSize, 
							revocationList, revocationListSize);
	config->tcpReuseAddr = true;
	UA_assert(status == UA_STATUSCODE_GOOD);
	
	UA_ByteString_clear(&derCert);
	UA_ByteString_clear(&derPrivKey);
	
	return status;
}
