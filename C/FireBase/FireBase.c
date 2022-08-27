/*
 * FireBase.c
 *
 *  Created on: 21 thg 4, 2022
 *      Author: A315-56
 */
#include "FireBase.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_event.h"


extern const char server_root_cert_pem_start[] asm("_binary_server_root_cert_pem_start");
//extern const char server_root_cert_pem_end[]   asm("_binary_server_root_cert_pem_end");

static char *PRJ_URL;
static char *DATA_RESP;
static char *URL, *DATA;
static char Get_String[128] = {'\0'};
static char Path_t[50] = {'\0'};
static char Name_t[50] = {'\0'};
esp_http_client_config_t config_post = {};
esp_http_client_handle_t client = NULL;
static const char *TAG = "FIREBASE HTTP CLIENT";

esp_err_t client_event_handler(esp_http_client_event_handle_t evt);
static void esp_httpclient_send(char *URL, char *Data, esp_http_client_method_t METHOD);
static int find_lastof_chr(char *Source, char chr);
static void Get_Path(char *Source);
static void Get_Name(char *Source);
static void free_mem(void);


esp_err_t client_event_handler(esp_http_client_event_handle_t evt){
    switch(evt->event_id){
		case HTTP_EVENT_ERROR:
			ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
		break;
		case HTTP_EVENT_ON_CONNECTED:
			ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
		break;
		case HTTP_EVENT_HEADER_SENT:
			ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
		break;
		case HTTP_EVENT_ON_HEADER:
			ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
		break;
		case HTTP_EVENT_ON_DATA:
//			asprintf(&DATA_RESP, "%s", (char *)evt->data);
			DATA_RESP = (char *)malloc(evt->data_len * sizeof(char));
			memcpy(DATA_RESP, (char *)evt->data, evt->data_len);
			ESP_LOGI(TAG, "GET_DATA: length = %d, Data = %s\n", evt->data_len, DATA_RESP);
//			free(DATA_RESP);
		break;
		default:
		break;
    }
    return ESP_OK;
}

void FireBase_Init(char *URL){
	asprintf(&PRJ_URL,  "%s", URL);
	config_post.cert_pem = (const char *)server_root_cert_pem_start;
	config_post.event_handler = client_event_handler;
	config_post.transport_type = HTTP_TRANSPORT_OVER_TCP;
}

static void esp_httpclient_send(char *URL, char *Data, esp_http_client_method_t METHOD){
    config_post.url = URL;
	config_post.method = METHOD;

    client = esp_http_client_init(&config_post);

    esp_http_client_set_post_field(client, Data, strlen(Data));
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "Connection", "close");

    esp_http_client_perform(client);
    esp_http_client_cleanup(client);
}

static int find_lastof_chr(char *Source, char chr){
    int index = -1;
    for (int i = 0; i < strlen(Source); i++)
        if (Source[i] == chr)
            index = i;
    return index;
}

static void Get_Path(char *Source){
	memset(Path_t, '\0', 50);
	int index = find_lastof_chr(Source, '/');
	for(int i=0; i<index; i++){
		Path_t[i] = Source[i];
	}
}

static void Get_Name(char *Source){
	memset(Name_t, '\0', 50);
	int index = find_lastof_chr(Source, '/');
	for(int i=index+1; i<strlen(Source); i++){
		Name_t[i - (index+1)] = Source[i];
	}
}

static void free_mem(void){
	free(DATA_RESP);
	free(URL);
	free(DATA);
}

void FireBase_SetInt(char *Path, int DataInt){
	Get_Path(Path);
	Get_Name(Path);
	asprintf(&URL, "%s/%s/.json", PRJ_URL, Path_t);
	asprintf(&DATA, "{\"%s\": %d}", Name_t, DataInt);
	esp_httpclient_send(URL, DATA,HTTP_METHOD_PATCH);
	free_mem();
}

void FireBase_SetDouble(char *Path, double DataFloat){
	Get_Path(Path);
	Get_Name(Path);
	asprintf(&URL, "%s/%s/.json", PRJ_URL, Path_t);
	asprintf(&DATA, "{\"%s\": %f}", Name_t, DataFloat);
	esp_httpclient_send(URL, DATA, HTTP_METHOD_PATCH);
	free_mem();
}

void FireBase_SetBool(char *Path, bool DataBool){
	Get_Path(Path);
	Get_Name(Path);
	asprintf(&URL, "%s/%s/.json", PRJ_URL, Path_t);
	if(DataBool) asprintf(&DATA, "{\"%s\": %s}", Name_t, "true");
	else asprintf(&DATA, "{\"%s\": %s}", Name_t, "false");
	esp_httpclient_send(URL, DATA, HTTP_METHOD_PATCH);
	free_mem();
}

void FireBase_SetString(char *Path, char *DataString){
	Get_Path(Path);
	Get_Name(Path);
	asprintf(&URL, "%s/%s/.json", PRJ_URL, Path_t);
	asprintf(&DATA, "{\"%s\": \"%s\"}", Name_t, DataString);
	esp_httpclient_send(URL, DATA, HTTP_METHOD_PATCH);
	free_mem();
}

int FireBase_GetInt(char *Path){
	asprintf(&URL, "%s/%s/.json?key=AIzaSyDBiFiOo5n1Pc_RORqlnr93xOb9XH5e06g", PRJ_URL, Path);
	esp_httpclient_send(URL, " ",HTTP_METHOD_GET);
	int temp = atoi(DATA_RESP);
	free_mem();
	return temp;
}

double FireBase_GetDouble(char *Path){
	asprintf(&URL, "%s/%s/.json?key=AIzaSyDBiFiOo5n1Pc_RORqlnr93xOb9XH5e06g", PRJ_URL, Path);
	esp_httpclient_send(URL, " ",HTTP_METHOD_GET);
	double temp = atof(DATA_RESP);
	free_mem();
	return temp;
}

bool FireBase_GetBool(char *Path){
	asprintf(&URL, "%s/%s/.json?key=AIzaSyDBiFiOo5n1Pc_RORqlnr93xOb9XH5e06g", PRJ_URL, Path);
	esp_httpclient_send(URL, " ",HTTP_METHOD_GET);
	bool temp = false;
	if(strcmp(DATA_RESP, "true") == 0) temp = true;
	else temp = false;
	free_mem();
	return temp;
}

char *FireBase_GetString(char *Path){
	asprintf(&URL, "%s/%s/.json?key=AIzaSyDBiFiOo5n1Pc_RORqlnr93xOb9XH5e06g", PRJ_URL, Path);
	esp_httpclient_send(URL, " ",HTTP_METHOD_GET);
	memset(Get_String, '\0', 128);
	memcpy(Get_String, DATA_RESP, strlen(DATA_RESP));
	free_mem();
	return Get_String;
}
