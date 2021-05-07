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

static const char * TAG = "bl0937";

/* external data declaration -------------------------------------------------*/

/* internal functions declaration --------------------------------------------*/

static void calculate_default_multipliers(bl0937_t * const me);
static void check_cf_signal(bl0937_t * const me);
static void check_cf1_signal(bl0937_t * const me);
static void IRAM_ATTR cf_isr(void * arg);
static void IRAM_ATTR cf1_isr(void * arg);

/* external functions definition ---------------------------------------------*/

esp_err_t bl0937_init(bl0937_t * const me)
{
	ESP_LOGI(TAG, "Initializing bl0937 instance...");

	esp_err_t ret;

	/* Configure SEL pin */
	gpio_config_t gpio_conf;
	gpio_conf.intr_type = GPIO_INTR_DISABLE;
	gpio_conf.mode = GPIO_MODE_OUTPUT;
	gpio_conf.pin_bit_mask = (1ULL << me->sel_pin);
	gpio_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
	gpio_conf.pull_up_en = GPIO_PULLUP_DISABLE;
	ret = gpio_config(&gpio_conf);

	if(ret != ESP_OK)
		return ret;

	/* Configure CF1 and CF pins */
	gpio_conf.intr_type = GPIO_INTR_ANYEDGE;
	gpio_conf.mode = GPIO_MODE_INPUT;
	gpio_conf.pin_bit_mask = (1ULL << me->cf_pin) | (1ULL << me->cf1_pin);
	gpio_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
	gpio_conf.pull_up_en = GPIO_PULLUP_ENABLE;
	ret = gpio_config(&gpio_conf);

	if(ret != ESP_OK)
		return ret;

	/* Initialize local variables */
	me->vref = V_REF;
	me->pulse_timeout = PULSE_TIMEOUT;
	me->voltage_pulse_width = 0;
	me->current_pulse_width = 0;
	me->power_pulse_width = 0;
	me->pulse_count = 0;
	me->current = 0;
	me->voltage = 0;
	me->power = 0;
	me->current_mode = MODE_CURRENT;	/* MODE_CURRENT for BL0937 */
	me->last_cf_interrupt = 0;
	me->last_cf1_interrupt = 0;
	me->first_cf1_interrupt = 0;
	me->mode = me->current_mode;

	if(ret != ESP_OK)
		return ret;

	/* Calculate default multipliers */
	calculate_default_multipliers(me);

	/* Set pin level according the mode */
	ret = gpio_set_level(me->sel_pin, me->mode);

	/* Install ISR service and configure interrupts handlers */
	ret = gpio_install_isr_service(0);

	if(ret != ESP_OK)
			return ret;

	ret = gpio_isr_handler_add(me->cf_pin, cf_isr, (void *)me);

	if(ret != ESP_OK)
			return ret;

	ret = gpio_isr_handler_add(me->cf1_pin, cf1_isr, (void *)me);

	if(ret != ESP_OK)
			return ret;

	return ret;
}

void bl0937_set_mode(bl0937_t * const me, bl0937_mode_e mode)
{
    me->mode = (mode == MODE_CURRENT) ? me->current_mode : 1 - me->current_mode;
    gpio_set_level(me->sel_pin, me->mode);

    me->last_cf1_interrupt = me->first_cf1_interrupt = esp_timer_get_time();
}

bl0937_mode_e bl0937_get_mode(bl0937_t * const me)
{
    return (me->mode == me->current_mode) ? MODE_CURRENT : MODE_VOLTAGE;
}

bl0937_mode_e bl0937_toggle_mode(bl0937_t * const me)
{
    bl0937_mode_e new_mode = bl0937_get_mode(me) == MODE_CURRENT ? MODE_VOLTAGE : MODE_CURRENT;
    bl0937_set_mode(me, new_mode);
    return new_mode;
}

uint16_t bl0937_get_current(bl0937_t * const me)
{
    bl0937_get_active_power(me);

    if (me->power == 0)
    	me->current_pulse_width = 0;
    else
    	check_cf1_signal(me);

    me->current = (me->current_pulse_width > 0) ? me->current_multiplier / me->current_pulse_width  : 0;

    return (uint16_t)(me->current * 100);
}

uint16_t bl0937_get_voltage(bl0937_t * const me)
{
	check_cf1_signal(me);

    me->voltage = (me->voltage_pulse_width > 0) ? me->voltage_multiplier / me->voltage_pulse_width : 0;
    return me->voltage;
}

uint32_t bl0937_get_energy(bl0937_t * const me)
{
    return me->pulse_count * me->power_multiplier / 1000000l;
}

uint16_t bl0937_get_active_power(bl0937_t * const me)
{
	check_cf_signal(me);

    me->power = (me->power_pulse_width > 0) ? me->power_multiplier / me->power_pulse_width : 0;
    return me->power;
}

uint16_t bl0937_get_apparent_power(bl0937_t * const me)
{
    float current = bl0937_get_current(me);
    uint16_t voltage = bl0937_get_voltage(me);

    return voltage * current;
}

float bl0937_get_power_factor(bl0937_t * const me)
{
    uint16_t active = bl0937_get_active_power(me);
    uint16_t apparent = bl0937_get_apparent_power(me);
    if(active > apparent)
    	return 1;

    if(apparent == 0)
    	return 0;

    return (float) active / apparent;
}

void bl0937_reset_energy(bl0937_t * const me)
{
    me->pulse_count = 0;
}

void bl0937_expected_current(bl0937_t * const me, float value)
{
    if(me->current == 0)
    	bl0937_get_current(me);

    if(me->current > 0)
    	me->current_multiplier *= (value / me->current);
}

void bl0937_expected_voltage(bl0937_t * const me, uint16_t value)
{
    if(me->voltage == 0)
    	bl0937_get_voltage(me);

    if(me->voltage > 0)
    	me->voltage_multiplier *= ((float) value / me->voltage);
}

void bl0937_expected_active_power(bl0937_t * const me, uint16_t value)
{
    if (me->power == 0)
    	bl0937_get_active_power(me);

    if(me->power > 0)
    	me->power_multiplier *= ((float) value / me->power);
}

void bl0937_reset_multipliers(bl0937_t * const me)
{
	calculate_default_multipliers(me);
}

void bl0937_set_resistors(bl0937_t * const me, float current, float voltage_upstream, float voltage_downstream)
{
    if (voltage_downstream > 0)
    {
        me->current_resistor = current;
        me->voltage_resistor = (voltage_upstream + voltage_downstream) / voltage_downstream;
        calculate_default_multipliers(me);
    }
}

float bl0937_get_current_multiplier(bl0937_t * const me)
{
	return me->current_multiplier;
}

float bl0937_get_voltage_multiplier(bl0937_t * const me)
{
	return me->voltage_multiplier;
}

float bl0937_get_power_multiplier(bl0937_t * const me)
{
	return me->power_multiplier;
}

void bl0937_set_current_multiplier(bl0937_t * const me, float current_multiplier)
{
	me->current_multiplier = current_multiplier;
}

void bl0937_set_voltage_multiplier(bl0937_t * const me, float voltage_multiplier)
{
	me->voltage_multiplier = voltage_multiplier;
}

void bl0937_set_power_multiplier(bl0937_t * const me, float power_multiplier)
{
	me->power_multiplier = power_multiplier;
}

/* internal functions definition ---------------------------------------------*/

static void calculate_default_multipliers(bl0937_t * const me)
{
	me->power_multiplier = (50850000.0 * me->vref * me->vref * me->voltage_resistor / me->current_resistor / 48.0 / F_OSC) / 1.1371681416f;  //15102450
    me->voltage_multiplier = (221380000.0 * me->vref * me->voltage_resistor /  2.0 / F_OSC) / 1.0474137931f; //221384120,171674
    me->current_multiplier = (531500000.0 * me->vref / me->current_resistor / 24.0 / F_OSC) / 1.166666f; //
}

static void check_cf_signal(bl0937_t * const me)
{
	if ((esp_timer_get_time() - me->last_cf_interrupt) > me->pulse_timeout)
		me->power_pulse_width = 0;
}

static void check_cf1_signal(bl0937_t * const me)
{
    if ((esp_timer_get_time() - me->last_cf1_interrupt) > me->pulse_timeout)
    {
        if(me->mode == me->current_mode)
        	me->current_pulse_width = 0;
        else
        	me->voltage_pulse_width = 0;

        bl0937_toggle_mode(me);
    }
}

static void IRAM_ATTR cf_isr(void * arg)
{
	bl0937_t * me = (bl0937_t *)arg;
	uint32_t now = esp_timer_get_time();

	me->power_pulse_width = now - me->last_cf_interrupt;
	me->last_cf_interrupt = now;
	me->pulse_count++;

	portYIELD_FROM_ISR();
}

static void IRAM_ATTR cf1_isr(void * arg)
{
	bl0937_t * me = (bl0937_t *)arg;
    uint32_t now = esp_timer_get_time();

    if((now - me->first_cf1_interrupt) > me->pulse_timeout)
    {
    	uint32_t pulse_width;

        if (me->last_cf1_interrupt == me->first_cf1_interrupt)
            pulse_width = 0;
        else
            pulse_width = now - me->last_cf1_interrupt;

        if (me->mode == me->current_mode)
            me->current_pulse_width = pulse_width;
        else
            me->voltage_pulse_width = pulse_width;

        me->mode = 1 - me->mode;

        gpio_set_level(me->sel_pin, me->mode);
        me->first_cf1_interrupt = now;
    }

    me->last_cf1_interrupt = now;

	portYIELD_FROM_ISR();
}

/* end of file ---------------------------------------------------------------*/
