/*
 * bl0931.h
 *
 * Created on: Jan 14, 2021
 * Author: Mauricio Barroso Benavides
 */

#ifndef _BL0931_H_
#define _BL0931_H_

/* inclusions ----------------------------------------------------------------*/

#include <stdio.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"

#include "driver/gpio.h"

/* cplusplus -----------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

/* macros --------------------------------------------------------------------*/

#define V_REF				1.218			/*  */
#define R_CURRENT			0.001			/*  */
#define R_VOLTAGE			((6 * 330) + 1)	/*  */
#define F_OSC				2000000			/*  */
#define READING_INTERVAL	3000			/*  */
#define PULSE_TIMEOUT		200000			/*  */

/* typedef -------------------------------------------------------------------*/

typedef enum
{
	MODE_CURRENT = 0,	/*  */
	MODE_VOLTAGE		/*  */
} bl0937_mode_e;

typedef struct
{
	gpio_num_t sel;
	gpio_num_t cf;
	gpio_num_t cf1;
} bl0937_pins_t;

typedef struct
{
	float current;
	float voltage;
} bl0937_resistor_t;

typedef struct
{
	float current;
	float voltage;
	float power;
} bl0937_multipliers_t;

typedef struct
{
	bl0937_mode_e mode;
	bl0937_pins_t pins;
	bl0937_multipliers_t multipliers;
} bl0937_t;

/* external data declaration -------------------------------------------------*/

/* external functions declaration --------------------------------------------*/
void check_cf_signal(bl0937_t * const me);
void check_cf1_signal(bl0937_t * const me);
esp_err_t bl0937_init(bl0937_t * const me);
esp_err_t bl0937_set_mode(bl0937_t * const me, bl0937_mode_e mode);
bl0937_mode_e bl0937_get_mode(bl0937_t * const me);
void bl0937_toggle_mode(bl0937_t * const me);
float bl0937_get_current(bl0937_t * const me);
float bl0937_get_voltage(bl0937_t * const me);
float bl0937_get_energy(bl0937_t * const me);
float bl0937_get_active_power(bl0937_t * const me);
float bl0937_get_apparent_power(bl0937_t * const me);
//uint16_t bl0937_get_reactive_power(bl0937_t * const me);
float bl0937_get_power_factor(bl0937_t * const me);
void bl0937_reset_energy(bl0937_t * const me);
void bl0937_expected_current(bl0937_t * const me, float val);
void bl0937_expected_voltage(bl0937_t * const me, float val);
void bl0937_expected_active_power(bl0937_t * const me, float val);
void bl0937s_reset_multipliers(bl0937_t * const me);
void bl0937_set_resistors(bl0937_t * const me, float current, float voltage_upstream, float voltage_downstream);

float bl0937_get_current_multiplier(bl0937_t * const me);
float bl0937_get_voltage_multiplier(bl0937_t * const me);
float bl0937_get_power_multiplier(bl0937_t * const me);

void bl0937_set_current_multiplier(bl0937_t * const me, float val);
void bl0937_set_voltage_multiplier(bl0937_t * const me, float val);
void bl0937_set_power_multiplier(bl0937_t * const me, float val);

/* cplusplus -----------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

/** @} doxygen end group definition */

/* end of file ---------------------------------------------------------------*/

#endif /* #ifndef _BL0931_H_ */
