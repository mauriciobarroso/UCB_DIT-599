/*
 * bitec_mqtt.c
 *
 * Created on: Mar 23, 2021
 * Author: Mauricio Barroso Benavides
 */

/* inclusions ----------------------------------------------------------------*/

#include "include/bitec_mqtt.h"
#include "mqtt_client.h"
#include "esp_log.h"

/* macros --------------------------------------------------------------------*/

#ifdef CONFIG_BITEC_MQTT_LWT_ENABLE
#define MQTT_LWT_TOPIC	CONFIG_BITEC_MQTT_LWT_TOPIC CONFIG_APPLICATION_DEVICE_ID
#endif

/* typedef -------------------------------------------------------------------*/

/* internal data declaration -------------------------------------------------*/

static const char * TAG = "digihome_mqtt";

/* external data declaration -------------------------------------------------*/

/* internal functions declaration --------------------------------------------*/

static void mqtt_event_handler(void * handler_args, esp_event_base_t base, int32_t event_id, void * event_data);

/* external functions definition ---------------------------------------------*/

esp_err_t bitec_mqtt_init(bitec_mqtt_t * const me)
{
	esp_err_t ret;

	/* Create Wi-Fi event group */
	me->event_group = xEventGroupCreate();

	if(me->config.uri == NULL)
	{
		me->config.uri = CONFIG_BITEC_MQTT_BROKER_URL;
		me->config.client_cert_pem = (const char *)client_cert_pem_start;
		me->config.client_key_pem = (const char *)client_key_pem_start;
		me->config.cert_pem = (const char *)server_cert_pem_start;
#ifdef CONFIG_BITEC_MQTT_LWT_ENABLE
		me->config.lwt_topic = MQTT_LWT_TOPIC;
		me->config.lwt_msg = CONFIG_BITEC_MQTT_LWT_MESSAGE;
		me->config.lwt_msg_len = CONFIG_BITEC_MQTT_LWT_LENGHT;
		me->config.lwt_qos = CONFIG_BITEC_MQTT_LWT_QOS;
#endif
		me->config.user_context = (void *)me;
	}

	me->client = esp_mqtt_client_init(&me->config);

	if(me->event_handler == NULL)
		ret = esp_mqtt_client_register_event(me->client,MQTT_EVENT_ANY, mqtt_event_handler, me->client);
	else
		ret = esp_mqtt_client_register_event(me->client,MQTT_EVENT_ANY, me->event_handler, me->client);

	return ret;
}

/* internal functions definition ---------------------------------------------*/

static void mqtt_event_handler(void * handler_args, esp_event_base_t base, int32_t event_id, void * event_data)
{
	esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
	bitec_mqtt_t * mqtt = (bitec_mqtt_t *)event->user_context;
	mqtt->event_data = event;

	// your_context_t *context = event->context;
	switch (mqtt->event_data->event_id)
	{
		case MQTT_EVENT_CONNECTED:
			ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");

			xEventGroupSetBits(mqtt->event_group, MQTT_EVENT_CONNECTED_BIT);

			break;

		case MQTT_EVENT_DISCONNECTED:
			ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");

			xEventGroupSetBits(mqtt->event_group, MQTT_EVENT_DISCONNECTED_BIT);

			break;


		case MQTT_EVENT_SUBSCRIBED:
			ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);

			xEventGroupSetBits(mqtt->event_group, MQTT_EVENT_SUBSCRIBED_BIT);

			break;

		case MQTT_EVENT_UNSUBSCRIBED:
			ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);

			xEventGroupSetBits(mqtt->event_group, MQTT_EVENT_UNSUBSCRIBED_BIT);

			break;

		case MQTT_EVENT_PUBLISHED:
			ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);

			xEventGroupSetBits(mqtt->event_group, MQTT_EVENT_PUBLISHED_BIT);

			break;

		case MQTT_EVENT_DATA:
			ESP_LOGI(TAG, "MQTT_EVENT_DATA");

			/* Print MQTT incoming messages */
			ESP_LOGI(TAG, "topic=%.*s\r", event->topic_len, event->topic);
			ESP_LOGI(TAG, "data=%.*s\r\n", event->data_len, event->data);

			xEventGroupSetBits(mqtt->event_group, MQTT_EVENT_DATA_BIT);

			break;

		case MQTT_EVENT_ERROR:
			ESP_LOGI(TAG, "MQTT_EVENT_ERROR");

			xEventGroupSetBits(mqtt->event_group, MQTT_EVENT_ERROR_BIT);

			break;

		default:
			ESP_LOGI(TAG, "Other event id:%d", event->event_id);

			break;
	}
}

/* end of file ---------------------------------------------------------------*/
