/*
 * bitec_wifi.h
 *
 * Created on: Mar 23, 2021
 * Author: Mauricio Barroso Benavides
 */

#ifndef _bitec_WIFI_H_
#define _bitec_WIFI_H_

/* inclusions ----------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_err.h"

#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"

#include "wifi_provisioning/manager.h"
#include "wifi_provisioning/scheme_softap.h"

/* cplusplus -----------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

/* macros --------------------------------------------------------------------*/

/* Wi-Fi provisioning event bits */
#define WIFI_PROV_START_BIT				BIT0
#define WIFI_PROV_CRED_RECV_BIT			BIT1
#define WIFI_PROV_CRED_FAIL_BIT			BIT2
#define WIFI_PROV_CRED_SUCCESS_BIT		BIT3
#define WIFI_PROV_END_BIT				BIT4

/* IP event bits */
#define IP_EVENT_STA_GOT_IP_BIT			BIT5

/* Wi-Fi event bits */
#define WIFI_EVENT_STA_CONNECTED_BIT	BIT6
#define WIFI_EVENT_STA_DISCONNECTED_BIT	BIT7

/* typedef -------------------------------------------------------------------*/

typedef void (* wifi_event_handler_t)(void *, esp_event_base_t, int32_t, void *);
typedef void (* ip_event_handler_t)(void *, esp_event_base_t, int32_t, void *);
typedef void (* prov_event_handler_t)(void *, esp_event_base_t, int32_t, void *);

typedef struct
{
	EventGroupHandle_t event_group;
	wifi_event_handler_t wifi_event_handler;
	ip_event_handler_t ip_event_handler;
	prov_event_handler_t prov_event_handler;
} bitec_wifi_t;

/* external data declaration -------------------------------------------------*/


/* external functions declaration --------------------------------------------*/

esp_err_t bitec_wifi_init(bitec_wifi_t * const me);

/* cplusplus -----------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

/** @} doxygen end group definition */

/* end of file ---------------------------------------------------------------*/

#endif /* #ifndef _BITEC_WIFI_H_ */
