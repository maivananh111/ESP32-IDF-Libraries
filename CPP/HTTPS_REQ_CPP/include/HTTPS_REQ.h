/*
 * HTTPS_REQ.h
 *
 *  Created on: 9 thg 4, 2022
 *      Author: A315-56
 */

#ifndef COMPONENTS_HTTPS_REQ_INCLUDE_HTTPS_REQ_H_
#define COMPONENTS_HTTPS_REQ_INCLUDE_HTTPS_REQ_H_


#include "stdio.h"
#include <iostream>
#include <string>
#include "esp_tls.h"

using namespace std;


#ifdef __cplusplus
extern "C" {
#endif

class HTTPS{
	public:
		HTTPS(string URL);
		void Set_Method (string Http_Req_Method);
		void Set_Path	(string Web_Path);
		void Set_Version(string Http_Version);
		void Set_Content_Type(string Http_Content_Type);
		void Set_Connection  (string Http_Connection);
		void Set_Data	(string Req_Data);

		/* HTTPS */
		void Open_Connection   (esp_tls_cfg_t cfg);
		void Send_Request	   (string Request);
		void Read_Response	   (void);
		void Close_Connection  (void);
		void Https_Send_Request(esp_tls_cfg_t cfg, string Request);
		void Https_Send_Request(esp_tls_cfg_t cfg);
		void Https_Send_Request_Crt_Bundle   (string Request);
		void Https_Send_Request_Crt_Bundle   (void);
		void Https_Send_Request_Saved_Session(string Request);
		void Https_Send_Request_Saved_Session(void);
		char *Https_Get_Response(void);

		struct esp_tls *tls;

	private:
		void Set(char **Src, string Str);
		void Request_Format(char **SRC);

		const char *TAGS = "HTTPS";
		char *REQUEST;
		char *EX;
		char *Request_Data;
		char *Method = (char *)"GET";
		char *Host;
		char *Web_Url;
		char *Path = (char *)"/";
		char *Version = (char *)"HTTP/1.1";
		char *Content_Type = (char *)"application/json";
		char *Connection = (char *)"close";

		esp_tls_client_session_t *tls_client_session = NULL;
};

#ifdef __cplusplus
}
#endif


#endif /* COMPONENTS_HTTPS_REQ_INCLUDE_HTTPS_REQ_H_ */
