/*
 * digihome_mqtt.h
 *
 * Created on: Jan 6, 2021
 * Author: Mauricio Barroso Benavides
 */

#ifndef _DIGIHOME_MQTT_H_
#define _DIGIHOME_MQTT_H_

/* inclusions ----------------------------------------------------------------*/

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mqtt_client.h"

/* cplusplus -----------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

/* macros --------------------------------------------------------------------*/

/* todo: move these macros to main Kconfig */
#define MQTT_DEVICE_ID		"mauricio"
#define MQTT_TEST_BROKER	"mqtts://api.digihome.technology:1883"			/*!< MQTT broker */

/* MQTT */
#define CONNECTED_TOPIC				"connected/"									/*!< Topic root */
#define MQTT_CONNECTED_TOPIC		CONNECTED_TOPIC MQTT_DEVICE_ID		/*!< Topic to subscribe */
#define DISCONNECTED_TOPIC			"disconnected/"									/*!< Topic root */
#define MQTT_DISCONNECTED_TOPIC		DISCONNECTED_TOPIC MQTT_DEVICE_ID		/*!< Topic to subscribe */
#define UPDATES_TOPIC				"updates/"										/*!< Topic root */
#define MQTT_UPDATES_TOPIC			UPDATES_TOPIC MQTT_DEVICE_ID			/*!< Topic to subscribe */
#define FIRMWARE_UPDATE_TOPIC		"firmwareUpdate/"								/*!< Topic root */
#define MQTT_FIRMWARE_UPDATE_TOPIC	FIRMWARE_UPDATE_TOPIC MQTT_DEVICE_ID	/*!< Topic to subscribe */
#define STATUS_TOPIC				"status/"										/*!< Topic root */
#define MQTT_STATUS_TOPIC			STATUS_TOPIC MQTT_DEVICE_ID			/*!< Topic to subscribe */
#define SENSORUPDATES_TOPIC			"sensorUpdates/"								/*!< Topic root */
#define MQTT_SENSORUPDATES_TOPIC	SENSORUPDATES_TOPIC MQTT_DEVICE_ID	/*!< Topic to subscribe */

/* typedef -------------------------------------------------------------------*/

typedef void (* mqtt_event_handler_t)(void *, esp_event_base_t, int32_t, void *);

typedef struct
{
	esp_mqtt_client_handle_t client;	/*!< MQTT client handle */
	esp_mqtt_client_config_t config;	/*!< MQTT configuration */
	mqtt_event_handler_t event_handler;	/*!< MQTT pointer to event handler function */
} digihome_mqtt_t;

/* external data declaration -------------------------------------------------*/


/* MQTT certificates */
extern const uint8_t client_cert_pem_start[] asm("_binary_client_crt_start");
extern const uint8_t client_cert_pem_end[] asm("_binary_client_crt_end");
extern const uint8_t client_key_pem_start[] asm("_binary_client_key_start");
extern const uint8_t client_key_pem_end[] asm("_binary_client_key_end");
extern const uint8_t server_cert_pem_start[] asm("_binary_ca_crt_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_ca_crt_end");

/* external functions declaration --------------------------------------------*/

esp_err_t digihome_mqtt_init(digihome_mqtt_t * const me);

/* cplusplus -----------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

/** @} doxygen end group definition */

/* end of file ---------------------------------------------------------------*/

#endif /* #ifndef _DIGIHOME_MQTT_H_ */
