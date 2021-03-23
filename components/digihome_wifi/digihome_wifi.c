/*
 * digihome_wifi.c
 *
 * Created on: Jan 19, 2021
 * Author: Mauricio Barroso Benavides
 */

/* inclusions ----------------------------------------------------------------*/

#include "include/digihome_wifi.h"

/* macros --------------------------------------------------------------------*/

#define POP_PIN					"abcd1234"		/*!< Proff of possession PIN */
#define DEFAULT_SERVICE_NAME	"default_ssid"	/*!< Default service name */

/* typedef -------------------------------------------------------------------*/

/* internal data declaration -------------------------------------------------*/

static const char * TAG = "digihome_wifi";

/* external data declaration -------------------------------------------------*/

/* internal functions declaration --------------------------------------------*/

//static void wifi_event_handler(void * arg, esp_event_base_t event_base, int event_id, void * event_data);
//static void ip_event_handler(void * arg, esp_event_base_t event_base, int event_id, void * event_data);
//static void prov_event_handler(void * arg, esp_event_base_t event_base, int event_id, void * event_data);
static esp_err_t custom_prov_data_handler(uint32_t session_id, const uint8_t * inbuf, ssize_t inlen, uint8_t ** outbuf, ssize_t * outlen, void * priv_data);
static void get_device_service_name(char * service_name, size_t max);

/* external functions definition ---------------------------------------------*/

esp_err_t digihome_wifi_init(digihome_wifi_t * const me)
{
    /* Initialize stack TCP/IP */
    ESP_ERROR_CHECK(esp_netif_init());

    /* Create event loop */
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* Create netif instances */
    esp_netif_create_default_wifi_sta();
    esp_netif_create_default_wifi_ap();

    /* Register event handlers */
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, me->wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, me->ip_event_handler, NULL, NULL));

    /* Initialize Wi-Fi */
    wifi_init_config_t wifi_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_config));

    /* Start Wi-Fi in station mode */
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_start());

    /* Initialize provisioning manager */

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, me->prov_event_handler, NULL, NULL));

    wifi_prov_mgr_config_t prov_config =
    {
    		.scheme = wifi_prov_scheme_softap,
    		.scheme_event_handler = WIFI_PROV_EVENT_HANDLER_NONE
    };
    ESP_ERROR_CHECK(wifi_prov_mgr_init(prov_config));

    /* Check if are Wi-Fi credentials provisioned */
    bool provisioned = false;
	ESP_ERROR_CHECK(wifi_prov_mgr_is_provisioned(&provisioned));

	if(provisioned)
	{
		ESP_LOGI(TAG, "Already provisioned, connecting to AP");

		/* We don't need the manager as device is already provisioned,
		* so let's release it's resources */
		wifi_prov_mgr_deinit();

		/* Try connecting to Wi-Fi router using stored credentials */
		esp_wifi_connect();
	}
	else
	{
		ESP_LOGI(TAG, "Starting provisioning");

		/* Reserve memory for the service name */
		char * service_name = NULL;
		service_name = malloc(12 * (sizeof(char)));

		/* Create endpoint */
		wifi_prov_mgr_endpoint_create("custom-data");

		/* Get SoftAP SSID name */
		if(service_name != NULL)
		{
			get_device_service_name(service_name, 12);
			ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(WIFI_PROV_SECURITY_1, (const char *)POP_PIN, service_name, NULL));
			free(service_name);
		}
		else
			ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(WIFI_PROV_SECURITY_1, (const char *)POP_PIN, DEFAULT_SERVICE_NAME, NULL));

		/* Register previous created endpoint */
		wifi_prov_mgr_endpoint_register("custom-data", custom_prov_data_handler, NULL);
	}

	return ESP_OK;
}

/* internal functions definition ---------------------------------------------*/

static void get_device_service_name(char *service_name, size_t max)
{
    uint8_t eth_mac[6];
    const char *ssid_prefix = "PROV_";
    esp_wifi_get_mac(WIFI_IF_STA, eth_mac);
    snprintf(service_name, max, "%s%02X%02X%02X", ssid_prefix, eth_mac[3], eth_mac[4], eth_mac[5]);
}

static esp_err_t custom_prov_data_handler(uint32_t session_id, const uint8_t * inbuf, ssize_t inlen, uint8_t ** outbuf, ssize_t * outlen, void * priv_data)
{
    if (inbuf)
    	ESP_LOGI(TAG, "Received data: %.*s", inlen, (char *)inbuf);
    char response[] = "SUCCESS";
    * outbuf = (uint8_t *)strdup(response);
    if (*outbuf == NULL)
    {
        ESP_LOGE(TAG, "System out of memory");
        return ESP_ERR_NO_MEM;
    }

    * outlen = strlen(response) + 1; /* +1 for NULL terminating byte */

    return ESP_OK;
}

/* end of file ---------------------------------------------------------------*/
