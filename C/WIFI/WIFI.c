#include "WIFI.h"

#include <stdio.h>
#include "string.h"
#include "esp_system.h"
#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "driver/gpio.h"


#define WIFI_MAXIMUM_RETRY  		  5
#define WIFI_CONNECTED_BIT 			  BIT0
#define WIFI_FAIL_BIT      			  BIT1


static const char *TAG = "WIFI";

static uint8_t connect_retry_num = 0;
static EventGroupHandle_t s_wifi_event_group;
static wifi_state_t WiFi_State = WIFI_CONNECT_FAILED;
static esp_netif_t *netif = NULL;
static uint16_t max_scan_num = 20;
static wifi_ap_record_t scan_ap_info[20];


static void WiFi_Event_Handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

static void WiFi_Event_Handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data){
    if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
        ESP_LOGI(TAG, "WIFI_EVENT_STA_START");
    }

    else if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (connect_retry_num < WIFI_MAXIMUM_RETRY) {
            esp_wifi_connect();
            connect_retry_num++;
            ESP_LOGW(TAG, "Retry to connect to the access point.");
        }
        else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            esp_wifi_disconnect();
            ESP_LOGW(TAG,"WiFi disconnecteded.");
        }
        ESP_LOGE(TAG,"WIFI_EVENT_STA_DISCONNECTED");
    }

    if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_STOP) {
        esp_wifi_disconnect();
        ESP_LOGE(TAG, "WIFI_EVENT_STA_STOP");
    }

    else if(event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Station IP:" IPSTR, IP2STR(&event -> ip_info.ip));
        connect_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

esp_netif_t *WiFi_STA_Connect(char *SSID, char *PASS, wifi_auth_mode_t auth){
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    netif = esp_netif_create_default_wifi_sta();
    esp_wifi_set_default_wifi_sta_handlers();

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,  &WiFi_Event_Handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &WiFi_Event_Handler, NULL));

	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
	esp_wifi_set_ps(WIFI_PS_NONE);

    wifi_config_t wifi_config = {0};
    memcpy(wifi_config.sta.ssid, SSID, strlen(SSID));
    memcpy(wifi_config.sta.password, PASS, strlen(PASS));
	wifi_config.sta.threshold.authmode = auth;
	wifi_config.sta.channel = 1;
	wifi_config.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_stationtion finished.");

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Connected to ap SSID: %s password: %s", SSID, PASS);
        WiFi_State = WIFI_CONNECTED;
    }
    else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(TAG, "Failed to connect to SSID: %s, password: %s", SSID, PASS);
        WiFi_State = WIFI_CONNECT_FAILED;
    }
    else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
    return netif;
}

void WiFi_STA_Disconnect(void){
	esp_err_t err = esp_wifi_stop();
	if (err == ESP_ERR_WIFI_NOT_INIT) return;

	ESP_ERROR_CHECK(err);
	ESP_ERROR_CHECK(esp_wifi_deinit());
	ESP_ERROR_CHECK(esp_wifi_clear_default_wifi_driver_and_handlers(netif));
	esp_netif_destroy(netif);

	ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &WiFi_Event_Handler));
	ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &WiFi_Event_Handler));
	xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);

	ESP_ERROR_CHECK(esp_wifi_clear_default_wifi_driver_and_handlers(netif));

	netif = NULL;
	WiFi_State = WIFI_CONNECT_FAILED;
	connect_retry_num = 0;
}

wifi_state_t WiFi_GetState(void){
	return WiFi_State;
}

esp_netif_t *WiFi_STA_get_netif(void){
    return netif;
}

esp_err_t WiFi_STA_Set_IPV4(char *LocalIP, char *NetMask, char *DefaultGateWay){
	esp_netif_ip_info_t ip_info = {0};
	if(netif){
		memset(&ip_info, 0, sizeof(esp_netif_ip_info_t));
		esp_netif_dhcpc_stop(netif);
        ip_info.ip.addr = esp_ip4addr_aton((const char *)LocalIP);
        ip_info.netmask.addr = esp_ip4addr_aton((const char *)NetMask);
        ip_info.gw.addr = esp_ip4addr_aton((const char *)DefaultGateWay);
        esp_err_t err = esp_netif_set_ip_info(netif, &ip_info);
	    if (err != ESP_OK) {
	        ESP_LOGE(TAG, "Failed to set station IP");
	        return err;
	    }
	    return ESP_OK;
	}
	return ESP_FAIL;
}

esp_err_t WiFi_STA_Set_IPDHCP(void){
	if(netif){
		esp_err_t err = esp_netif_dhcpc_start(netif);
	    if (err != ESP_OK) {
	        ESP_LOGE(TAG, "Failed to start dhcp client.");
	        return err;
	    }
	    return ESP_OK;
	}
	return ESP_FAIL;
}

char *LocalIP(esp_netif_t *WiFi_Netif){
	esp_netif_ip_info_t IP_info_t = {0};
	char *buf;
	buf = malloc(16*sizeof(char));
	esp_netif_get_ip_info(WiFi_Netif, &IP_info_t);
	esp_ip4addr_ntoa(&IP_info_t.ip, (char *)buf, 16);
	return (char *)buf;
}

uint8_t WiFi_STA_Scan(void){
	uint16_t ap_count = 0;
	memset(scan_ap_info, 0, sizeof(scan_ap_info));
	ESP_ERROR_CHECK(esp_wifi_scan_start(NULL, true));
	ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
	ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&max_scan_num, scan_ap_info));
    return (uint8_t)ap_count;
}

char *WiFi_STA_Scan_Get_SSID(uint8_t Number){
	char *buffer;
	uint8_t len = sizeof(scan_ap_info[Number].ssid);
	buffer = malloc(len * sizeof(uint8_t));
	memcpy(buffer, scan_ap_info[Number].ssid, sizeof(scan_ap_info[Number].ssid));
	return buffer;
}
