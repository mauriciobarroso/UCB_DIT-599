/*
 * main.c
 *
 * Created on: Dec 22, 2021
 * Author: Mauricio Barroso Benavides
 */

/* inclusions ----------------------------------------------------------------*/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_event.h"

#include "nvs_flash.h"

#include "digihome_wifi.h"
#include "ws2812_led.h"

/* macros --------------------------------------------------------------------*/

/* typedef -------------------------------------------------------------------*/

/* data declaration ----------------------------------------------------------*/

static const char * TAG = "app";

static TaskHandle_t reconnect_handle;
static digihome_wifi_t wifi;

/* function declaration ------------------------------------------------------*/

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int event_id, void* event_data);
static void ip_event_handler(void* arg, esp_event_base_t event_base, int event_id, void* event_data);
static void prov_event_handler(void* arg, esp_event_base_t event_base, int event_id, void* event_data);

static void reconnect_task(void * arg);

/**/
static esp_err_t nvs_init(void);

/* main ----------------------------------------------------------------------*/

void app_main(void)
{
	ESP_LOGI(TAG, "Initializing device...");

	/* Initialize WS2812B LED */
	ESP_ERROR_CHECK(ws2812_led_init());

	/* Initizalize NVS storage */
	ESP_ERROR_CHECK(nvs_init());

    /* Initialize Wi-Fi component */
    wifi.wifi_event_handler = wifi_event_handler;
    wifi.ip_event_handler = ip_event_handler;
    wifi.prov_event_handler = prov_event_handler;
    ESP_ERROR_CHECK(digihome_wifi_init(&wifi));
}

/* function definition -------------------------------------------------------*/

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int event_id, void* event_data)
{
	switch(event_id)
	{
		case WIFI_EVENT_STA_START:
			ws2812_led_set_hsv(240, 100, 25);	/* Set LED blue */

			break;

		case WIFI_EVENT_STA_DISCONNECTED:
			/* Create task to reconnect to AP and set RGB led in blue color */
//			xTaskCreate(reconnect_task, "Reconnect Task", configMINIMAL_STACK_SIZE * 2, NULL, tskIDLE_PRIORITY + 1, &reconnect_handle);
	        ws2812_led_set_hsv(240, 100, 25);

			break;

		case WIFI_EVENT_STA_CONNECTED:
			/* Delete task to reconnect to AP */
//			vTaskDelete(reconnect_handle);

			break;

		default:
			break;
	}
}

static void ip_event_handler(void* arg, esp_event_base_t event_base, int event_id, void* event_data)
{
	switch(event_id)
	{
		case IP_EVENT_STA_GOT_IP:
		{
			ip_event_got_ip_t * event = (ip_event_got_ip_t*) event_data;
			ESP_LOGI(TAG, "Connected with IP Address:" IPSTR, IP2STR(&event->ip_info.ip));

			ws2812_led_set_hsv(120, 100, 25);	/* Set LED green */

			break;
		}

		default:
			break;
	}
}

static void prov_event_handler(void* arg, esp_event_base_t event_base, int event_id, void* event_data)
{
	switch(event_id)
	{
		case WIFI_PROV_START:
			ESP_LOGI(TAG, "Provisioning started");

			ws2812_led_set_hsv(0, 100, 25);	/* Set LED red */

			break;

		case WIFI_PROV_CRED_RECV:
		{
			wifi_sta_config_t * wifi_sta_cfg = (wifi_sta_config_t *)event_data;
			ESP_LOGI(TAG, "Credentials received, SSID: %s & Password: %s",
					(const char *) wifi_sta_cfg->ssid, (const char *) wifi_sta_cfg->password);

			ws2812_led_set_hsv(240, 100, 25);	/* Set LED blue */

			break;
		}

		case WIFI_PROV_CRED_FAIL:
		{
			wifi_prov_sta_fail_reason_t *reason = (wifi_prov_sta_fail_reason_t *)event_data;
			ESP_LOGE(TAG, "Provisioning failed!\n\tReason : %s"
			"\n\tPlease reset to factory and retry provisioning",
			( * reason == WIFI_PROV_STA_AUTH_ERROR) ?
			"Wi-Fi station authentication failed" : "Wi-Fi access-point not found");
			break;
		}

		case WIFI_PROV_CRED_SUCCESS:
			ESP_LOGI(TAG, "Provisioning successful");
			break;

		case WIFI_PROV_END:
			/* De-initialize manager once provisioning is finished */
			wifi_prov_mgr_deinit();

			break;

		default:
			break;
	}
}

/* RTOS tasks funtions */
static void reconnect_task(void * arg)
{
	TickType_t last_time_wake = xTaskGetTickCount();

	for(;;)
	{
		/* Try connecting to Wi-Fi router using stored credentials. If connection is successful
		 * then the task delete itself, in other cases this function is executed again*/
		ESP_LOGI(TAG, "Unable to connect. Retrying...");
		esp_wifi_connect();

		/* Wait 30 sec to try reconnecting */
		vTaskDelayUntil(&last_time_wake, pdMS_TO_TICKS(30000));	/* todo: put the time in macros */
	}
}

static esp_err_t nvs_init(void)
{
	esp_err_t ret;

	const esp_partition_t * partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS_KEYS, NULL);

	if(partition != NULL)
	{
		nvs_sec_cfg_t nvs_sec_cfg;

		if(nvs_flash_read_security_cfg(partition, &nvs_sec_cfg) != ESP_OK)
			ESP_ERROR_CHECK(nvs_flash_generate_keys(partition, &nvs_sec_cfg));

		/* Initialize secure NVS */
		ESP_ERROR_CHECK(nvs_flash_erase());	/* Erase any stored Wi-Fi credential  */
		ret = nvs_flash_secure_init(&nvs_sec_cfg);
	}
	else
		ret = ESP_FAIL;

	return ret;
}


/* end of file ---------------------------------------------------------------*/
