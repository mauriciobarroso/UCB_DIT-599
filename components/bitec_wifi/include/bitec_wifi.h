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

/* typedef -------------------------------------------------------------------*/

typedef void (* wifi_event_handler_t)(void *, esp_event_base_t, int32_t, void *);
typedef void (* ip_event_handler_t)(void *, esp_event_base_t, int32_t, void *);
typedef void (* prov_event_handler_t)(void *, esp_event_base_t, int32_t, void *);

typedef struct
{
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
