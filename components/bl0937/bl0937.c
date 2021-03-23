/*
 * mcp401x.c
 *
 * Created on: Jan 13, 2021
 * Author: Mauricio Barroso Benavides
 */

/* inclusions ----------------------------------------------------------------*/

#include "bl0937.h"

/* macros --------------------------------------------------------------------*/

/* typedef -------------------------------------------------------------------*/

/* internal data declaration -------------------------------------------------*/

//static const char * TAG = "bl0937";

static double current_resistor = R_CURRENT;
static double voltage_resistor = R_VOLTAGE;

static float current_multiplier;
static float voltage_multiplier;
static float power_multiplier;

volatile unsigned long voltage_pulse_width = 0;
volatile unsigned long current_pulse_width = 0;
volatile unsigned long power_pulse_width = 0;
volatile unsigned long pulse_count = 0;

double current = 0;
unsigned int voltage = 0;
unsigned int power = 0;

volatile int64_t last_cf_interrupt = 0;
volatile int64_t last_cf1_interrupt = 0;
volatile int64_t first_cf1_interrupt = 0;

/* external data declaration -------------------------------------------------*/

/* internal functions declaration --------------------------------------------*/

static void calculate_default_multipliers(void);
static void IRAM_ATTR cf_isr(void * arg);
static void IRAM_ATTR cf1_isr(void * arg);

/* external functions definition ---------------------------------------------*/

void check_cf_signal(bl0937_t * const me)
{
	if((esp_timer_get_time() - last_cf_interrupt) > PULSE_TIMEOUT)
		power_pulse_width = 0;
}

void check_cf1_signal(bl0937_t * const me)
{
	if((esp_timer_get_time() - last_cf1_interrupt) > PULSE_TIMEOUT)
	{
		if(me->mode == MODE_CURRENT)
			current_pulse_width = 0;
		else
			voltage_pulse_width = 0;

		bl0937_toggle_mode(me);
	}
}

esp_err_t bl0937_init(bl0937_t * const me)
{
	esp_err_t ret;

	/* Configure SEL pin */
	gpio_config_t gpio_conf;
	gpio_conf.intr_type = GPIO_INTR_DISABLE;
	gpio_conf.mode = GPIO_MODE_OUTPUT;
	gpio_conf.pin_bit_mask = (1ULL << me->pins.sel);
	gpio_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
	gpio_conf.pull_up_en = GPIO_PULLUP_DISABLE;
	ret = gpio_config(&gpio_conf);

	if(ret != ESP_OK)
		return ret;

	/* Configure CF1 and CF pins */
	gpio_conf.intr_type = GPIO_INTR_NEGEDGE;
	gpio_conf.mode = GPIO_MODE_INPUT;
	gpio_conf.pin_bit_mask = (1ULL << me->pins.cf1) | (1ULL << me->pins.cf);
	gpio_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
	gpio_conf.pull_up_en = GPIO_PULLUP_ENABLE;
	ret = gpio_config(&gpio_conf);

	if(ret != ESP_OK)
		return ret;


	/* Calculate default multipliers */
	calculate_default_multipliers();

	/* Set pin level according the mode */
	ret = gpio_set_level(me->pins.sel, me->mode);

	if(ret != ESP_OK)
			return ret;

	/* Install ISR service and configure interrupts handlers */
	ret = gpio_install_isr_service(0);

	if(ret != ESP_OK)
			return ret;

	ret = gpio_isr_handler_add(me->pins.cf, cf_isr, (void *)me);

	if(ret != ESP_OK)
			return ret;

	ret = gpio_isr_handler_add(me->pins.cf1, cf1_isr, (void *)me);

	if(ret != ESP_OK)
			return ret;

	return ret;
}

esp_err_t bl0937_set_mode(bl0937_t * const me, bl0937_mode_e mode)
{
	esp_err_t ret;

	me->mode = mode;

	ret = gpio_set_level(me->pins.sel, me->mode);

	if(ret != ESP_OK)
		return ret;

	first_cf1_interrupt = esp_timer_get_time();
	last_cf1_interrupt = first_cf1_interrupt;

	return ret;
}

bl0937_mode_e bl0937_get_mode(bl0937_t * const me)
{
	return me->mode;
}

void bl0937_toggle_mode(bl0937_t * const me)
{
	bl0937_set_mode(me, !me->mode);
}

float bl0937_get_current(bl0937_t * const me)
{
	bl0937_get_active_power(me);	/* TODO: verify */

	if(!power)
		current_pulse_width = 0;
	else
		check_cf1_signal(me);

	current = (current_pulse_width > 0) ? (current_multiplier / current_pulse_width) : 0;

	return (current * 100);
}

float bl0937_get_voltage(bl0937_t * const me)
{
	check_cf1_signal(me);

	voltage = (voltage_pulse_width > 0) ? (voltage_multiplier / voltage_pulse_width) : 0;

	return voltage;
}

float bl0937_get_energy(bl0937_t * const me)
{
	return ((pulse_count * power_multiplier) / 1000000L);
}

float bl0937_get_active_power(bl0937_t * const me)
{
	check_cf_signal(me);

	power = (power_pulse_width > 0) ? (power_multiplier / power_pulse_width) : 0;

	return power;
}

float bl0937_get_apparent_power(bl0937_t * const me)
{
	return (bl0937_get_current(me) * bl0937_get_voltage(me));
}

float bl0937_get_power_factor(bl0937_t * const me)
{
	float active = bl0937_get_active_power(me);
	float apparent = bl0937_get_apparent_power(me);

	if(active > apparent)
		return 1;
	if(!apparent)
		return 0;

	return (active/apparent);
}

void bl0937_reset_energy(bl0937_t * const me)
{
	pulse_count = 0;
}

void bl0937_expected_current(bl0937_t * const me, float value)
{
	if(current == 0)
		bl0937_get_current(me);
	if(current > 0)
		current_multiplier *= (value / current);
}

void bl0937_expected_voltage(bl0937_t * const me, float value)
{
	if(voltage == 0)
		bl0937_get_voltage(me);
	if(voltage > 0)
		voltage_multiplier *= (value / voltage);
}

void bl0937_expected_active_power(bl0937_t * const me, float val)
{
	if(power == 0)
		bl0937_get_active_power(me);
	if(power > 0)
		power_multiplier *= (val / power);
}

void bl0937_reset_multipliers(bl0937_t * const me)
{
	calculate_default_multipliers();
}

void bl0937_set_resistors(bl0937_t * const me, float current_, float voltage_upstream, float voltage_downstream)
{
	if(voltage_downstream > 0)
	{
		current_resistor = current_;
		voltage_resistor = (voltage_upstream + voltage_downstream) / voltage_downstream;
		calculate_default_multipliers();
	}
}

float bl0937_get_current_multiplier(bl0937_t * const me)
{
	return current_multiplier;
}

float bl0937_get_voltage_multiplier(bl0937_t * const me)
{
	return voltage_multiplier;
}

float bl0937_get_power_multiplier(bl0937_t * const me)
{
	return power_multiplier;
}

void bl0937_set_current_multiplier(bl0937_t * const me, float val)
{
	current_multiplier = val;
}

void bl0937_set_voltage_multiplier(bl0937_t * const me, float val)
{
	voltage_multiplier = val;
}

void bl0937_set_power_multiplier(bl0937_t * const me, float val)
{
	power_multiplier = val;
}

/* internal functions definition ---------------------------------------------*/

static void calculate_default_multipliers(void)
{
	current_multiplier = (531500000.0 * V_REF / current_resistor / 24.0 / F_OSC) / 1.166666F;
	voltage_multiplier = (221380000.0 * V_REF * voltage_resistor / 2.0 / F_OSC) / 1.0474137931F;
	power_multiplier = (50850000.0 * V_REF * V_REF * voltage_resistor / current_resistor / 48.0 / F_OSC) / 1.1371681416F;
}

static void IRAM_ATTR cf_isr(void * arg)
{
	uint32_t now = esp_timer_get_time();

	power_pulse_width = now - last_cf_interrupt;
	last_cf_interrupt = now;
	pulse_count++;

	portYIELD_FROM_ISR();
}

static void IRAM_ATTR cf1_isr(void * arg)
{
	bl0937_t * data = (bl0937_t *)arg;
	uint32_t now = esp_timer_get_time();

	if((now - first_cf1_interrupt) > PULSE_TIMEOUT)
	{
		uint32_t pulse_width;

		if(last_cf1_interrupt == first_cf1_interrupt)
			pulse_width = 0;
		else
			pulse_width = now - last_cf1_interrupt;

		if(data->mode == MODE_CURRENT)
			current_pulse_width = pulse_width;
		else
			voltage_pulse_width = pulse_width;

		bl0937_toggle_mode(data);
	}

	portYIELD_FROM_ISR();
}

/* end of file ---------------------------------------------------------------*/
