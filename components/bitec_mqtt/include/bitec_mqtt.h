/*
 * bitec_mqtt.h
 *
 * Created on: Mar 23, 2021
 * Author: Mauricio Barroso Benavides
 */

#ifndef _BITEC_MQTT_H_
#define _BITEC_MQTT_H_

/* inclusions ----------------------------------------------------------------*/

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "mqtt_client.h"

/* cplusplus -----------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

/* macros --------------------------------------------------------------------*/

/* MQTT event bits */
#define MQTT_EVENT_CONNECTED_BIT	BIT0
#define MQTT_EVENT_DISCONNECTED_BIT	BIT1
#define MQTT_EVENT_SUBSCRIBED_BIT	BIT2
#define MQTT_EVENT_UNSUBSCRIBED_BIT	BIT3
#define MQTT_EVENT_PUBLISHED_BIT	BIT4
#define MQTT_EVENT_DATA_BIT			BIT5
#define MQTT_EVENT_ERROR_BIT		BIT6

/* typedef -------------------------------------------------------------------*/

typedef void (* mqtt_event_handler_t)(void *, esp_event_base_t, int32_t, void *);

typedef struct
{
	esp_mqtt_client_handle_t client;	/*!< MQTT client handle */
	esp_mqtt_client_config_t config;	/*!< MQTT configuration */
	mqtt_event_handler_t event_handler;	/*!< MQTT pointer to event handler function */
	esp_mqtt_event_handle_t event_data;	/*!< todo: set description */
	EventGroupHandle_t event_group;		/*!< todo: set description */
} bitec_mqtt_t;

/* external data declaration -------------------------------------------------*/

/* Server certificate*/
extern const uint8_t server_cert_pem_start[] asm("_binary_ca_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_ca_pem_end");

/* Thing certificate */
extern const uint8_t client_cert_pem_start[] asm("_binary_certificate_pem_crt_start");
extern const uint8_t client_cert_pem_end[] asm("_binary_certificate_pem_crt_end");

/* Private key */
extern const uint8_t client_key_pem_start[] asm("_binary_private_pem_key_start");
extern const uint8_t client_key_pem_end[] asm("_binary_private_pem_key_end");

/* external functions declaration --------------------------------------------*/

esp_err_t bitec_mqtt_init(bitec_mqtt_t * const me);

/* cplusplus -----------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

/** @} doxygen end group definition */

/* end of file ---------------------------------------------------------------*/

#endif /* #ifndef _BITEC_MQTT_H_ */
