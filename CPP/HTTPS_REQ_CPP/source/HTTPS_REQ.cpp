#include <string.h>
#include "esp_log.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include "esp_crt_bundle.h"

#include "HTTPS_REQ.h"



void HTTPS::Set(char **Src, string Str){
	asprintf(Src, "%s", Str.c_str());
}

HTTPS::HTTPS(string URL){
	Set(&Web_Url, URL);
}

void HTTPS::Set_Method(string Http_Req_Method){
	Set(&Method, Http_Req_Method);
}

void HTTPS::Set_Path(string Web_Path){
	Set(&Path, Web_Path);
}

void HTTPS::Set_Version(string Http_Version){
	Set(&Version, Http_Version);
}

void HTTPS::Set_Content_Type(string Http_Content_Type){
	Set(&Content_Type, Http_Content_Type);
}

void HTTPS::Set_Connection(string Http_Connection){
	Set(&Connection, Http_Connection);
}

void HTTPS::Set_Data(string Req_Data){
	Set(&Request_Data, Req_Data);
}

void HTTPS::Request_Format(char **SRC){
	char Web_Host[128] = {0};
	int first_find = 0;
	for(int i=0; i<strlen(Web_Url); i++){
		if(Web_Url[i] == '/' && Web_Url[i-1] == '/'){
			first_find++;
			break;
		} first_find++;
	}
	for(int i=first_find; i<strlen(Web_Url); i++) Web_Host[i-first_find] = Web_Url[i];
	asprintf(&Host, "%s", (char *)Web_Host);
	/* "Method Path Version\r\nHost:Web_host\r\nContent-Type: Content type\r\nContent-Length: length\r\n\r\nDataBody\r\n" */
	asprintf(SRC, "%s %s %s\r\nHost: %s\r\nConnection: %s\r\nContent-Type: %s\r\nContent-Length:%d\r\n\r\n%s\r\n",
			Method, Path, Version, Host, Connection, Content_Type, strlen(Request_Data), Request_Data);
}

/* ------------------------HTTPS-------------------------- */
void HTTPS::Open_Connection(esp_tls_cfg_t cfg){
    tls = esp_tls_conn_http_new(Web_Url, &cfg);
    if (tls != NULL) ESP_LOGI(TAGS, "Connection established...");
    else {
        ESP_LOGE(TAGS, "Open Connection failed...");
        Close_Connection();
        return;
    }
    if (tls_client_session == NULL) tls_client_session = esp_tls_get_client_session(tls);
}

void HTTPS::Send_Request(string Request){
	int ret;
	asprintf(&REQUEST, "%s", Request.c_str());
    size_t written_bytes = 0;
    do {
        ret = esp_tls_conn_write(tls, REQUEST + written_bytes, strlen(REQUEST) + 1 - written_bytes);
        if (ret >= 0) {
            ESP_LOGI(TAGS, "%d bytes written", ret);
            written_bytes += ret;
        } else if (ret != ESP_TLS_ERR_SSL_WANT_READ  && ret != ESP_TLS_ERR_SSL_WANT_WRITE) {
            ESP_LOGE(TAGS, "esp_tls_conn_write  returned: [0x%02X](%s)", ret, esp_err_to_name(ret));
            ESP_LOGE(TAGS, "Connection write failed...");
            Close_Connection();
    		return;
        }
    } while (written_bytes < (strlen(REQUEST) + 1));
}

void HTTPS::Read_Response(void){
	char RESPONSE[512];
	int ret, len;
    do {
        len = sizeof(RESPONSE) - 1;
        bzero(RESPONSE, sizeof(RESPONSE));
        ret = esp_tls_conn_read(tls, (char *)RESPONSE, len);
        if (ret == ESP_TLS_ERR_SSL_WANT_WRITE  || ret == ESP_TLS_ERR_SSL_WANT_READ) continue;
        if (ret < 0) {
            ESP_LOGE(TAGS, "esp_tls_conn_read  returned [-0x%02X](%s)", -ret, esp_err_to_name(ret));
            break;
        }
        if (ret == 0) {
            ESP_LOGI(TAGS, "connection closed");
            break;
        }
        len = ret;
        ESP_LOGW(TAGS, "%d bytes read", len);
 /*       for (int i = 0; i < len; i++) {
            putchar(RESPONSE[i]);
        }
        putchar('\n');*/
        asprintf(&EX, "%s", RESPONSE);
    } while (1);
}
void HTTPS::Close_Connection(void){
    esp_tls_conn_delete(tls);
	ESP_LOGI(TAGS, "Connection closed!");
	vTaskDelay(50 / portTICK_PERIOD_MS);
//	free(REQUEST);
//	free(EX);
//	free(Request_Data);
}

char *HTTPS::Https_Get_Response(void){
	return EX;
}

void HTTPS::Https_Send_Request(esp_tls_cfg_t cfg, string Request){
	Open_Connection(cfg);
	Send_Request(Request);
	Read_Response();
	Close_Connection();
}

void HTTPS::Https_Send_Request(esp_tls_cfg_t cfg){
	Request_Format(&REQUEST);
	Https_Send_Request(cfg, REQUEST);
}


void HTTPS::Https_Send_Request_Crt_Bundle(string Request){
    ESP_LOGI(TAGS, "https_request using crt bundle");
    esp_tls_cfg_t cfg = {
	    .crt_bundle_attach = esp_crt_bundle_attach,
    };
    Https_Send_Request(cfg, Request);
}

void HTTPS::Https_Send_Request_Crt_Bundle(void){
    ESP_LOGI(TAGS, "https_request using crt bundle");
    esp_tls_cfg_t cfg = {
		.crt_bundle_attach = esp_crt_bundle_attach,
    };
    Https_Send_Request(cfg);
}

void HTTPS::Https_Send_Request_Saved_Session(string Request){
    ESP_LOGI(TAGS, "https_request using saved client session");
    esp_tls_cfg_t cfg = {
		.client_session = tls_client_session,
    };
    Https_Send_Request(cfg, Request);
}

void HTTPS::Https_Send_Request_Saved_Session(void){
    ESP_LOGI(TAGS, "https_request using saved client session");
    esp_tls_cfg_t cfg = {
        .client_session = tls_client_session,
    };
    Https_Send_Request(cfg);
    free(tls_client_session);
    tls_client_session = NULL;
}












