/*
 * bl0937.h
 *
 * Created on: Jan 14, 2021
 * Author: Mauricio Barroso Benavides
 */

#ifndef _BL0937_H_
#define _BL0937_H_

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

#define V_REF				1.218			/*!< Internal reference voltage */
#define F_OSC				2000000			/*!< Frequency of internal oscillator */
#define READING_INTERVAL	3000			/*!< Minimum delay between selecting a mode and reading a sample */
#define PULSE_TIMEOUT		200000			/*!< Maximum pulse with in microseconds */

/* typedef -------------------------------------------------------------------*/

typedef enum
{
	MODE_CURRENT = 0,	/*!<  */
	MODE_VOLTAGE		/*!<  */
} bl0937_mode_e;

typedef struct
{
	gpio_num_t sel_pin;
	gpio_num_t cf_pin;
	gpio_num_t cf1_pin;
	float current_resistor;
	float voltage_resistor;
	float vref;
	float current_multiplier;
	float voltage_multiplier;
	float power_multiplier;
	uint32_t pulse_timeout;
	volatile uint32_t voltage_pulse_width;
	volatile uint32_t current_pulse_width;
	volatile uint32_t power_pulse_width;
	volatile uint32_t pulse_count;
	float current;
	uint16_t voltage;
	uint16_t power;
	bl0937_mode_e current_mode;
	volatile bl0937_mode_e mode;
	volatile uint32_t last_cf_interrupt;
	volatile uint32_t last_cf1_interrupt;
	volatile uint32_t first_cf1_interrupt;
} bl0937_t;

/* external data declaration -------------------------------------------------*/

/* external functions declaration --------------------------------------------*/
esp_err_t bl0937_init(bl0937_t * const me);
void bl0937_set_mode(bl0937_t * const me, bl0937_mode_e mode);
bl0937_mode_e bl0937_get_mode(bl0937_t * const me);
bl0937_mode_e bl0937_toggle_mode(bl0937_t * const me);
uint16_t bl0937_get_current(bl0937_t * const me);
uint16_t bl0937_get_voltage(bl0937_t * const me);
uint32_t bl0937_get_energy(bl0937_t * const me);
uint16_t bl0937_get_active_power(bl0937_t * const me);
uint16_t bl0937_get_apparent_power(bl0937_t * const me);
float bl0937_get_power_factor(bl0937_t * const me);
void bl0937_reset_energy(bl0937_t * const me);
void bl0937_expected_current(bl0937_t * const me, float value);
void bl0937_expected_voltage(bl0937_t * const me, uint16_t value);
void bl0937_expected_active_power(bl0937_t * const me, uint16_t value);
void bl0937_reset_multipliers(bl0937_t * const me);
void bl0937_set_resistors(bl0937_t * const me, float current, float voltage_upstream, float voltage_downstream);
float bl0937_get_current_multiplier(bl0937_t * const me);
float bl0937_get_voltage_multiplier(bl0937_t * const me);
float bl0937_get_power_multiplier(bl0937_t * const me);
void bl0937_set_current_multiplier(bl0937_t * const me, float current_multiplier);
void bl0937_set_voltage_multiplier(bl0937_t * const me, float voltage_multiplier);
void bl0937_set_power_multiplier(bl0937_t * const me, float power_multiplier);

/* cplusplus -----------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

/** @} doxygen end group definition */

/* end of file ---------------------------------------------------------------*/

#endif /* #ifndef _BL0937_H_ */
