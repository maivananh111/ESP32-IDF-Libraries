/*
 * HTTPS_REQ.h
 *
 *  Created on: 18 thg 4, 2022
 *      Author: A315-56
 */

#ifndef _HTTPS_REQ_H_
#define _HTTPS_REQ_H_


#include "stdio.h"
#include "esp_tls.h"


void https_Set_URL    (char *URL);
void https_Set_Method (char *Http_Req_Method);
void https_Set_Path	  (char *Web_Path);
void https_Set_Version(char *Http_Version);
void https_Set_Content_Type(char *Http_Content_Type);
void https_Set_Connection  (char *Http_Connection);
void https_Set_Data   (char *Req_Data);


void https_Open_Conn   (esp_tls_cfg_t cfg);
void https_Send_Req    (char *Request);
void https_Send_Internal_Req(void);
void https_Read_Resp   (void);
void https_Close_Conn  (void);


void https_Send_Request(esp_tls_cfg_t cfg, char *Request);
void https_Send(esp_tls_cfg_t cfg);

void https_Send_Request_CACert_Buf(char *Request);
void https_Send_CACert_Buf(void);

void https_Send_Request_GolbalCA_Store(char *Request);
void https_Send_GolbalCA_Store(void);

void https_Send_Request_Crt_Bundle(char *Request);
void https_Send_Crt_Bundle(void);

void https_Send_Request_Saved_Session(char *Request);
void https_Send_Saved_Session(void);

char *https_Get_Response	 (void);


#endif /* _HTTPS_REQ_H_ */
