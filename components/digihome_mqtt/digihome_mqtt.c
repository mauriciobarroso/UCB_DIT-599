/*
 * digihome_mqtt.c
 *
 * Created on: Jan 6, 2021
 * Author: Mauricio Barroso Benavides
 */

/* inclusions ----------------------------------------------------------------*/

#include "include/digihome_mqtt.h"
#include "mqtt_client.h"
#include "esp_log.h"

/* macros --------------------------------------------------------------------*/

/* typedef -------------------------------------------------------------------*/

/* internal data declaration -------------------------------------------------*/

static const char * TAG = "digihome_mqtt";

/* external data declaration -------------------------------------------------*/

/* internal functions declaration --------------------------------------------*/

static void mqtt_event_handler(void * handler_args, esp_event_base_t base, int32_t event_id, void * event_data);

/* external functions definition ---------------------------------------------*/

esp_err_t digihome_mqtt_init(digihome_mqtt_t * const me)
{
	esp_err_t ret;

	if(me->config.uri == NULL)
	{
		me->config.uri = MQTT_TEST_BROKER;
		me->config.client_cert_pem = (const char *)client_cert_pem_start;
		me->config.client_key_pem = (const char *)client_key_pem_start;
		me->config.cert_pem = (const char *)server_cert_pem_start;
		me->config.lwt_topic = MQTT_DISCONNECTED_TOPIC;
		me->config.lwt_msg = "disconnected";
		me->config.lwt_msg_len = 12;
	}

	me->client = esp_mqtt_client_init(&me->config);

	if(me->event_handler == NULL)
		ret = esp_mqtt_client_register_event(me->client,ESP_EVENT_ANY_ID, mqtt_event_handler, me->client);
	else
		ret = esp_mqtt_client_register_event(me->client,ESP_EVENT_ANY_ID, me->event_handler, me->client);

	return ret;
}

/* internal functions definition ---------------------------------------------*/

static void mqtt_event_handler(void * handler_args, esp_event_base_t base, int32_t event_id, void * event_data)
{
	esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

	// your_context_t *context = event->context;
	switch (event->event_id)
	{
		case MQTT_EVENT_CONNECTED:
			ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");

			break;

		case MQTT_EVENT_DISCONNECTED:
			ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");

			break;


		case MQTT_EVENT_SUBSCRIBED:
			ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);

			break;

		case MQTT_EVENT_UNSUBSCRIBED:
			ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);

			break;

		case MQTT_EVENT_PUBLISHED:
			ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);

			break;

		case MQTT_EVENT_DATA:
			ESP_LOGI(TAG, "MQTT_EVENT_DATA");

			/* Print MQTT incoming messages */
			ESP_LOGI(TAG, "topic=%.*s\r\n", event->topic_len, event->topic);
			ESP_LOGI(TAG, "data=%.*s\r\n", event->data_len, event->data);

			break;

		case MQTT_EVENT_ERROR:
			ESP_LOGI(TAG, "MQTT_EVENT_ERROR");

			break;

		default:
			ESP_LOGI(TAG, "Other event id:%d", event->event_id);

			break;
	}
}

/* end of file ---------------------------------------------------------------*/
