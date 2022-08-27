/*
 * HTTPS_REQ.c
 *
 *  Created on: 18 thg 4, 2022
 *      Author: A315-56
 */
#include "HTTPS_REQ.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_netif.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "esp_crt_bundle.h"
#include "esp_tls.h"


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

extern const uint8_t server_root_cert_pem_start[] asm("_binary_server_root_cert_pem_start");
extern const uint8_t server_root_cert_pem_end[]   asm("_binary_server_root_cert_pem_end");

static esp_tls_client_session_t *tls_client_session = NULL;
struct esp_tls *tls;

static void Set(char **Src, char *Str);
static void Request_Format(char **SRC);

static void Set(char **Src, char *Str){
	asprintf(Src, "%s", Str);
}

void https_Set_URL(char *URL){
	Set(&Web_Url, URL);
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
}

void https_Set_Method(char *Http_Req_Method){
	Set(&Method, Http_Req_Method);
}

void https_Set_Path(char *Web_Path){
	Set(&Path, Web_Path);
}

void https_Set_Version(char *Http_Version){
	Set(&Version, Http_Version);
}

void https_Set_Content_Type(char *Http_Content_Type){
	Set(&Content_Type, Http_Content_Type);
}

void https_Set_Connection(char *Http_Connection){
	Set(&Connection, Http_Connection);
}

void https_Set_Data(char *Req_Data){
	Set(&Request_Data, Req_Data);
}

static void Request_Format(char **SRC){
	/* "Method Path Version\r\nHost:Web_host\r\nContent-Type: Content type\r\nContent-Length: length\r\n\r\nDataBody\r\n" */
	asprintf(SRC, "%s %s %s\r\nHost: %s\r\nConnection: %s\r\nContent-Type: %s\r\nContent-Length:%d\r\n\r\n%s\r\n",
			Method, Path, Version, Host, Connection, Content_Type, strlen(Request_Data), Request_Data);
}

/* ------------------------HTTPS-------------------------- */
void https_Close_Conn(void){
    esp_tls_conn_delete(tls);
	ESP_LOGI(TAGS, "Connection closed!");
	vTaskDelay(50 / portTICK_PERIOD_MS);
//	tls_client_session = NULL;
	free(REQUEST);
	free(EX);
	free(Request_Data);
}

void https_Open_Conn(esp_tls_cfg_t cfg){
    tls = esp_tls_conn_http_new(Web_Url, &cfg);
    if (tls != NULL) ESP_LOGI(TAGS, "Connection opened...");
    else {
        ESP_LOGE(TAGS, "Open Connection failed...");
        https_Close_Conn();
        return;
    }
    if(tls_client_session == NULL) tls_client_session = esp_tls_get_client_session(tls);
}

void https_Send_Req(char *Request){
	int ret;
	asprintf(&REQUEST, "%s", Request);
    size_t written_bytes = 0;
    do {
        ret = esp_tls_conn_write(tls, REQUEST + written_bytes, strlen(REQUEST) + 1 - written_bytes);
        if (ret >= 0) {
//            ESP_LOGI(TAGS, "%d bytes written", ret);
            written_bytes += ret;
        } else if (ret != ESP_TLS_ERR_SSL_WANT_READ  && ret != ESP_TLS_ERR_SSL_WANT_WRITE) {
            ESP_LOGE(TAGS, "esp_tls_conn_write  returned: [0x%02X](%s)", ret, esp_err_to_name(ret));
            ESP_LOGE(TAGS, "Connection write failed...");
            https_Close_Conn();
    		return;
        }
    } while (written_bytes < (strlen(REQUEST) + 1));
}

void https_Send_Internal_Req(void){
	int ret;
	Request_Format(&REQUEST);
//	asprintf(&REQUEST, "%s", Request);
    size_t written_bytes = 0;
    do {
        ret = esp_tls_conn_write(tls, REQUEST + written_bytes, strlen(REQUEST) + 1 - written_bytes);
        if (ret >= 0) {
//            ESP_LOGI(TAGS, "%d bytes written", ret);
            written_bytes += ret;
        } else if (ret != ESP_TLS_ERR_SSL_WANT_READ  && ret != ESP_TLS_ERR_SSL_WANT_WRITE) {
            ESP_LOGE(TAGS, "esp_tls_conn_write  returned: [0x%02X](%s)", ret, esp_err_to_name(ret));
            ESP_LOGE(TAGS, "Connection write failed...");
            https_Close_Conn();
    		return;
        }
    } while (written_bytes < (strlen(REQUEST) + 1));
}

void https_Read_Resp(void){
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
//        ESP_LOGW(TAGS, "%d bytes read", len);
 /*       for (int i = 0; i < len; i++) {
            putchar(RESPONSE[i]);
        }
        putchar('\n');*/
        asprintf(&EX, "%s", RESPONSE);
    } while (1);
}

char *https_Get_Resp(void){
	return EX;
}

void https_Send_Request(esp_tls_cfg_t cfg, char *Request){
	https_Open_Conn(cfg);
	https_Send_Req(Request);
	https_Read_Resp();
	https_Close_Conn();
}

void https_Send(esp_tls_cfg_t cfg){
	Request_Format(&REQUEST);
	https_Send_Request(cfg, REQUEST);
}

void https_Send_Request_Crt_Bundle(char *Request){
    ESP_LOGI(TAGS, "https_request using crt bundle");
    esp_tls_cfg_t cfg = {
	    .crt_bundle_attach = esp_crt_bundle_attach,
    };
    https_Send_Request(cfg, Request);
}

void https_Send_Crt_Bundle(void){
    ESP_LOGI(TAGS, "https_request using crt bundle");
    esp_tls_cfg_t cfg = {
		.crt_bundle_attach = esp_crt_bundle_attach,
    };
    https_Send(cfg);
}

void https_Send_Request_CACert_Buf(char *Request){
    ESP_LOGI(TAGS, "https_request using cacert_buf");
    esp_tls_cfg_t cfg = {
        .cacert_buf = (const unsigned char *) server_root_cert_pem_start,
        .cacert_bytes = server_root_cert_pem_end - server_root_cert_pem_start,
    };
    https_Send_Request(cfg, Request);
}

void https_Send_CACert_Buf(void){
    ESP_LOGI(TAGS, "https_request using cacert_buf");
    esp_tls_cfg_t cfg = {
        .cacert_buf = (const unsigned char *) server_root_cert_pem_start,
        .cacert_bytes = server_root_cert_pem_end - server_root_cert_pem_start,
    };
    https_Send(cfg);
}

void https_Send_Request_GolbalCA_Store(char *Request){
    esp_err_t esp_ret = ESP_FAIL;
    ESP_LOGI(TAGS, "https_request using global ca_store");
    esp_ret = esp_tls_set_global_ca_store(server_root_cert_pem_start, server_root_cert_pem_end - server_root_cert_pem_start);
    if (esp_ret != ESP_OK) {
        ESP_LOGE(TAGS, "Error in setting the global ca store: [%02X] (%s),could not complete the https_request using global_ca_store", esp_ret, esp_err_to_name(esp_ret));
        return;
    }
    esp_tls_cfg_t cfg = {
        .use_global_ca_store = true,
    };
    https_Send_Request(cfg, Request);
    esp_tls_free_global_ca_store();
}

void https_Send_GolbalCA_Store(void){
    esp_err_t esp_ret = ESP_FAIL;
    ESP_LOGI(TAGS, "https_request using global ca_store");
    esp_ret = esp_tls_set_global_ca_store(server_root_cert_pem_start, server_root_cert_pem_end - server_root_cert_pem_start);
    if (esp_ret != ESP_OK) {
        ESP_LOGE(TAGS, "Error in setting the global ca store: [%02X] (%s),could not complete the https_request using global_ca_store", esp_ret, esp_err_to_name(esp_ret));
        return;
    }
    esp_tls_cfg_t cfg = {
        .use_global_ca_store = true,
    };
    https_Send(cfg);
    esp_tls_free_global_ca_store();
}

void https_Send_Request_Saved_Session(char *Request){
    ESP_LOGI(TAGS, "https_request using saved client session");
    esp_tls_cfg_t cfg = {
		.client_session = tls_client_session,
    };
    https_Send_Request(cfg, Request);
}

void https_Send_Saved_Session(void){
    ESP_LOGI(TAGS, "https_request using saved client session");
    esp_tls_cfg_t cfg = {
        .client_session = tls_client_session,
    };
    https_Send(cfg);
    free(tls_client_session);
    tls_client_session = NULL;
}










