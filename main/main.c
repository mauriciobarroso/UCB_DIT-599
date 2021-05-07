/*
 * main.c
 *
 * Created on: Mar 22, 2021
 * Author: Mauricio Barroso Benavides
 */

/* inclusions ----------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"


#include "esp_log.h"
#include "esp_event.h"
#include "driver/adc.h"
#include "nvs_flash.h"
#include "cJSON.h"

#include "bitec_wifi.h"
#include "bitec_mqtt.h"
#include "bitec_button.h"
#include "ws2812_led.h"
#include "bl0937.h"

/* macros --------------------------------------------------------------------*/

#ifdef CONFIG_APPLICATION_USER_DEFINED_SUBSCRIPTION_1_ENABLE
#define MQTT_SUBSCRIBE_1	CONFIG_APPLICATION_USER_DEFINED_SUBSCRIPTION_1_TOPIC CONFIG_APPLICATION_DEVICE_ID
#endif

#ifdef CONFIG_APPLICATION_USER_DEFINED_SUBSCRIPTION_2_ENABLE
#define MQTT_SUBSCRIBE_2	CONFIG_APPLICATION_USER_DEFINED_SUBSCRIPTION_2_TOPIC CONFIG_APPLICATION_DEVICE_ID
#endif

#ifdef CONFIG_APPLICATION_CONNECT_PUBLISHING_ENABLE
#define MQTT_CONNECT	CONFIG_APPLICATION_CONNECT_PUBLISHING_TOPIC CONFIG_APPLICATION_DEVICE_ID
#endif

#define WIFI_RECONNECT_TIME	30000		/*!<  */
#define SEND_DATA_TIME		5000		/*!<  */
#define NO_OF_TIMES			12			/*!<  */

#define UUID_SIZE			36 			/*!< UUID size in bytes */

#define MQTT_DEVICE_STATUS	"status"	/*!<  */

/* ADC macros */
#define NO_OF_SAMPLES   	64      	/*!< Multisampling */

/* typedef -------------------------------------------------------------------*/

typedef struct
{
	bool light;
	int illumination;
	bool presence;
	float voltage;
	float current;
	float power;
} payload_t;

typedef struct
{
	char * device;		/*!< Device identifier in UUID form */
	payload_t payload;	/*!< Data to send to MQTT broker */
} json_message_t;

/* data declaration ----------------------------------------------------------*/

static const char * TAG = "app";

static TaskHandle_t reconnect_handle = NULL;
static TaskHandle_t send_data_handle = NULL;
static bitec_wifi_t wifi;
static bitec_mqtt_t mqtt;
static bitec_button_t button;
static bl0937_t bl0937;

/* Application variables */
static json_message_t message;

/* function declaration ------------------------------------------------------*/

static void wifi_events_task(void * arg);
static void mqtt_events_task(void * arg);
static void button_events_task(void * arg);

static void reconnect_task(void * arg);
static void send_data_task(void * arg);
static void get_sensors_task(void * arg);


/**/
static esp_err_t nvs_init(void);

/* main ----------------------------------------------------------------------*/

void app_main(void)
{
	message.device = CONFIG_APPLICATION_DEVICE_ID;

	ESP_LOGI(TAG, "Initializing device...");

	/* Initialize Relay GPIO */
	gpio_config_t gpio_conf;
	gpio_conf.intr_type = GPIO_INTR_DISABLE;
	gpio_conf.mode = GPIO_MODE_OUTPUT;
	gpio_conf.pin_bit_mask = (1ULL << GPIO_NUM_6);
	gpio_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
	gpio_conf.pull_up_en = GPIO_PULLUP_DISABLE;
	ESP_ERROR_CHECK(gpio_config(&gpio_conf));

	/* Configure PIR pin */
	gpio_conf.intr_type = GPIO_INTR_DISABLE;
	gpio_conf.mode = GPIO_MODE_INPUT;
	gpio_conf.pin_bit_mask = (1ULL << GPIO_NUM_8);
	gpio_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
	gpio_conf.pull_up_en = GPIO_PULLUP_DISABLE;
	gpio_config(&gpio_conf);

	/* Configure ADC */
	adc1_config_width(ADC_WIDTH_BIT_13);
	adc1_config_channel_atten(ADC_CHANNEL_6, ADC_ATTEN_DB_11);


	/* Initialize BL0937 instance */
	bl0937.cf_pin = CONFIG_BL0937_CF_PIN;
	bl0937.cf1_pin = CONFIG_BL0937_CF1_PIN;
	bl0937.sel_pin = CONFIG_BL0937_SEL_PIN;
	bl0937.current_resistor = (float)CONFIG_BL0937_R_CURRENT / 1000;
	bl0937.voltage_resistor = CONFIG_BL0937_R_VOLTAGE;

	ESP_ERROR_CHECK(bl0937_init(&bl0937));

	/* Initialize WS2812B LED */
	ESP_ERROR_CHECK(ws2812_led_init());

	/* Initialize button instance */
	ESP_ERROR_CHECK(bitec_button_init(&button));

	/* Initizalize NVS storage */
	ESP_ERROR_CHECK(nvs_init());

    /* Initialize Wi-Fi component */
    ESP_ERROR_CHECK(bitec_wifi_init(&wifi));

    /* Initialize MQTT component */
	ESP_ERROR_CHECK(bitec_mqtt_init(&mqtt));

	/* Create RTOS tasks */
	/* Create FreeRTOS tasks */
	xTaskCreate(wifi_events_task, "Wi-Fi Events Task", configMINIMAL_STACK_SIZE * 4, NULL, configMAX_PRIORITIES - 2, NULL);
	xTaskCreate(button_events_task, "Buton Events Task", configMINIMAL_STACK_SIZE * 4, NULL, configMAX_PRIORITIES - 3, NULL);
	xTaskCreate(mqtt_events_task, "MQTT Events Task", configMINIMAL_STACK_SIZE * 4, NULL, configMAX_PRIORITIES - 1, NULL);
	xTaskCreate(get_sensors_task, "Get Sensors Task", configMINIMAL_STACK_SIZE * 2, NULL, tskIDLE_PRIORITY + 3, NULL);
}

/* function definition -------------------------------------------------------*/

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
		vTaskDelayUntil(&last_time_wake, pdMS_TO_TICKS(WIFI_RECONNECT_TIME));
	}
}

static void get_sensors_task(void * arg)
{
	TickType_t last_time_wake = 0;
	uint8_t counter = 0;

	for(;;)
	{
		/* Get ADC samples */
		for(uint8_t i = 0; i < NO_OF_SAMPLES; i++)
			message.payload.illumination += adc1_get_raw((adc1_channel_t)ADC_CHANNEL_6);

		/* Get ADC and PIR values, and print them */
		message.payload.illumination /= NO_OF_SAMPLES;
		message.payload.presence = gpio_get_level(GPIO_NUM_8);

		/* Set Relay value */
		if(message.payload.illumination < 2000 && message.payload.presence)
		{
			message.payload.light = true;
			gpio_set_level(GPIO_NUM_6, message.payload.light);
		}
		else if(message.payload.illumination >= 4000 && !message.payload.light)
		{
			message.payload.light = false;
			gpio_set_level(GPIO_NUM_6, message.payload.light);
		}
		else if(!message.payload.presence)
		{
			message.payload.light = false;
			gpio_set_level(GPIO_NUM_6, message.payload.light);
		}

		/* Add 1 to counter and notify send data task when counter is NO_OF_TIMES */
		counter++;

		if(counter >= NO_OF_TIMES)
		{
			if(send_data_handle != NULL)
				xTaskNotifyGive(send_data_handle);

			counter = 0;
		}

		/* Wait SEND_DATA_TIME to get sensors values again */
		vTaskDelayUntil(&last_time_wake, pdMS_TO_TICKS(SEND_DATA_TIME));
	}
}

static void send_data_task(void * arg)
{
	uint32_t event_to_process;

	for(;;)
	{
		event_to_process = ulTaskNotifyTake( pdTRUE, portMAX_DELAY );

		if(event_to_process != 0)
		{
			/* Get electrical parameter values */
			message.payload.voltage = (float)bl0937_get_voltage(&bl0937);
			message.payload.current = (float)bl0937_get_current(&bl0937);
			message.payload.power = (float)bl0937_get_apparent_power(&bl0937);

			/* Create JSON message */
			char * string = NULL;
			cJSON * device = NULL;
			cJSON * payload = NULL;
			cJSON * light = NULL;
			cJSON * illumination = NULL;
			cJSON * presence = NULL;
			cJSON * voltage = NULL;
			cJSON * current = NULL;
			cJSON * power = NULL;

			cJSON * data = cJSON_CreateObject();
			if(data == NULL)
				break;

			device = cJSON_CreateString(message.device);
			if(device == NULL)
				break;

			payload = cJSON_CreateObject();
			if(payload == NULL)
				break;

			light = cJSON_CreateNumber(message.payload.light);
			if(payload == NULL)
				break;

			illumination = cJSON_CreateNumber(message.payload.illumination);
			if(illumination == NULL)
				break;

			voltage = cJSON_CreateNumber(message.payload.voltage);
			if(voltage == NULL)
				break;

			current = cJSON_CreateNumber(message.payload.current);
			if(current == NULL)
				break;

			power = cJSON_CreateNumber(message.payload.power);
			if(power == NULL)
				break;

			presence = cJSON_CreateNumber(message.payload.presence);
			if(presence == NULL)
				break;

			cJSON_AddItemToObject(data, "device", device);
			cJSON_AddItemToObject(data, "payload", payload);
			cJSON_AddItemToObject(payload, "light", light);
			cJSON_AddItemToObject(payload, "illumination", illumination);
			cJSON_AddItemToObject(payload, "presence", presence);
			cJSON_AddItemToObject(payload, "voltage", voltage);
			cJSON_AddItemToObject(payload, "current", current);
			cJSON_AddItemToObject(payload, "power", power);

			string = cJSON_Print(data);

			/* Send json message to broker*/
			ESP_LOGI(TAG, "Publising %s to %s", string, MQTT_DEVICE_STATUS);
			esp_mqtt_client_publish(mqtt.client, MQTT_DEVICE_STATUS, string, 0, 0, 0);

			/* Delete JSON object */
			cJSON_Delete(data);
		}
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
//		ESP_ERROR_CHECK(nvs_flash_erase());	/* Erase any stored Wi-Fi credential  */
		ret = nvs_flash_secure_init(&nvs_sec_cfg);
	}
	else
		return ESP_FAIL;

	return ret;
}

static void wifi_events_task(void * arg)
{
	EventBits_t bits;

	for(;;)
	{
		/* Wait until some bit is set */
		bits = xEventGroupWaitBits(wifi.event_group, 0xFF, pdTRUE, pdFALSE, portMAX_DELAY);

		if(bits & WIFI_PROV_CRED_FAIL_BIT)
			esp_restart();	/* Restart the device */
		else if(bits & WIFI_PROV_CRED_RECV_BIT)
			ws2812_led_set_hsv(240, 100, 25); /* Set RGB LED in blue color*/
		else if(bits & IP_EVENT_STA_GOT_IP_BIT)
		{
			esp_mqtt_client_start(mqtt.client);	/* Start MQTT client */
			ws2812_led_set_hsv(120, 100, 25);	/* Set RGB LED in green color */
		}
		else if(bits & WIFI_EVENT_STA_CONNECTED_BIT)
		{
			/* Delete task to reconnect to AP */
			if(reconnect_handle != NULL)
			{
				vTaskDelete(reconnect_handle);
				reconnect_handle = NULL;
			}
		}
		else if(bits & WIFI_EVENT_STA_DISCONNECTED_BIT)
		{
			/* Create task to reconnect to AP and set RGB led in blue color */
			if(reconnect_handle == NULL)
				xTaskCreate(reconnect_task, "Reconnect Task", configMINIMAL_STACK_SIZE * 3, NULL, tskIDLE_PRIORITY + 1, &reconnect_handle);

	        ws2812_led_set_hsv(240, 100, 25);
		}
		else
			ESP_LOGI(TAG, "Wi-Fi unexpected Event");
	}
}

static void mqtt_events_task(void * arg)
{
	EventBits_t bits;

	for(;;)
	{
		/* Wait until some bit is set */
		bits = xEventGroupWaitBits(mqtt.event_group, MQTT_EVENT_CONNECTED_BIT | MQTT_EVENT_DATA_BIT, pdTRUE, pdFALSE, portMAX_DELAY);

		if(bits & MQTT_EVENT_CONNECTED_BIT)
		{
			ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED_BIT set!");

			/* Send connected message */
			ESP_LOGI(TAG, "Publising to %s", MQTT_CONNECT);
			esp_mqtt_client_publish(mqtt.client, MQTT_CONNECT, CONFIG_APPLICATION_CONNECT_PUBLISHING_MESSAGE, 0, 0, 0);

			/* Subscribe to user defined topics */
#ifdef CONFIG_APPLICATION_USER_DEFINED_SUBSCRIPTION_1_ENABLE
			ESP_LOGI(TAG, "Subscribing to %s", MQTT_SUBSCRIBE_1);
			esp_mqtt_client_subscribe(mqtt.client, MQTT_SUBSCRIBE_1, 0);
#endif

#ifdef CONFIG_APPLICATION_USER_DEFINED_SUBSCRIPTION_2_ENABLE
			ESP_LOGI(TAG, "Subscribing to %s", MQTT_SUBSCRIBE_2);
			esp_mqtt_client_subscribe(event->client, MQTT_SUBSCRIBE_2, 0);
#endif

			/* Create task to publish the electrical parameters of the devices */
			if(send_data_handle == NULL)
				xTaskCreate(send_data_task, "Electric Parameters Task", configMINIMAL_STACK_SIZE * 3, NULL, tskIDLE_PRIORITY + 2, &send_data_handle);

		}

		else if(bits & MQTT_EVENT_DISCONNECTED_BIT)
		{
			ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED_BIT set!");

			/* Delete task to publish the electrical parameters of the device */
			if(send_data_handle != NULL)
			{
				vTaskDelete(send_data_handle);
				send_data_handle = NULL;
			}
		}

		else if(bits & MQTT_EVENT_DATA_BIT)
		{
			ESP_LOGI(TAG, "MQTT_EVENT_DATA_BIT set!");

			/* Print MQTT incoming messages */
			ESP_LOGI(TAG, "topic=%.*s\r", mqtt.event_data->topic_len, mqtt.event_data->topic);
			ESP_LOGI(TAG, "data=%.*s\r\n", mqtt.event_data->data_len, mqtt.event_data->data);

			char * string = NULL;
			string = malloc(sizeof(char) * UUID_SIZE * 2);

			if(string != NULL)
			{
				sprintf(string, "%.*s", mqtt.event_data->topic_len, mqtt.event_data->topic);

#ifdef CONFIG_APPLICATION_USER_DEFINED_SUBSCRIPTION_1_ENABLE
				if(!strcmp(string, MQTT_SUBSCRIBE_1))
				{
					/* Parse incoming json message */
					sprintf(string, "%.*s", mqtt.event_data->data_len, mqtt.event_data->data);
//					cJSON * root = cJSON_Parse(string);

					/* Value obtained of MQTT message */
//					int json_val = cJSON_GetObjectItem(root, "state")->valueint;

					/* Reply incoming json message to broker*/
					ESP_LOGI(TAG, "Publising to %s", MQTT_DEVICE_STATUS);
					esp_mqtt_client_publish(mqtt.client, MQTT_DEVICE_STATUS, string, 0, 0, 0);
				}
#endif

				free(string);
			}

		}
		else
			ESP_LOGI(TAG, "MQTT unexpected Event");
	}
}

static void button_events_task(void * arg)
{
	EventBits_t bits;

	for(;;)
	{
		/* Wait until some bit is set */
		bits = xEventGroupWaitBits(button.event_group, BUTTON_SHORT_PRESS_BIT| BUTTON_MEDIUM_PRESS_BIT | BUTTON_LONG_PRESS_BIT, pdTRUE, pdFALSE, portMAX_DELAY);

		if(bits & BUTTON_SHORT_PRESS_BIT)
			ESP_LOGI(TAG, "BUTTON_SHORT_PRESS_BIT set!");

		else if(bits & BUTTON_MEDIUM_PRESS_BIT)
		{
			ESP_LOGI(TAG, "BUTTON_MEDIUM_PRESS_BIT set!");

			/* Erase any stored Wi-Fi credential  */
			ESP_LOGI(TAG, "Erasing Wi-Fi credentials");

			esp_err_t ret;

			nvs_handle_t nvs_handle;
			ret = nvs_open("nvs.net80211", NVS_READWRITE, &nvs_handle);

			if(ret == ESP_OK)
				nvs_erase_all(nvs_handle);

			/* Close NVS */
			ret = nvs_commit(nvs_handle);
			nvs_close(nvs_handle);

			if(ret == ESP_OK)
				/* Restart device */
				esp_restart();
		}
		else if(bits & BUTTON_LONG_PRESS_BIT)
			ESP_LOGI(TAG, "BUTTON_LONG_PRESS_BIT set!");
		else
			ESP_LOGI(TAG, "Button unexpected Event");
	}
}

/* end of file ---------------------------------------------------------------*/
