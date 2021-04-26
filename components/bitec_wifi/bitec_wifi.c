/*
 * bitec_wifi.c
 *
 * Created on: Mar 23, 2021
 * Author: Mauricio Barroso Benavides
 */

/* inclusions ----------------------------------------------------------------*/

#include "include/bitec_wifi.h"

/* macros --------------------------------------------------------------------*/

/* typedef -------------------------------------------------------------------*/

/* internal data declaration -------------------------------------------------*/

static const char * TAG = "bitec_wifi";

/* external data declaration -------------------------------------------------*/

/* internal functions declaration --------------------------------------------*/

/* Provisioning utilities */
static esp_err_t custom_prov_data_handler(uint32_t session_id, const uint8_t * inbuf, ssize_t inlen, uint8_t * * outbuf, ssize_t * outlen, void * priv_data);
static void get_device_service_name(char * service_name, size_t max);

/* Event handlers */
static void wifi_event_handler(void * arg, esp_event_base_t event_base, int event_id, void * event_data);
static void ip_event_handler(void * arg, esp_event_base_t event_base, int event_id, void * event_data);
static void prov_event_handler(void * arg, esp_event_base_t event_base, int event_id, void * event_data);

/* external functions definition ---------------------------------------------*/

esp_err_t bitec_wifi_init(bitec_wifi_t * const me)
{
	/* Create Wi-Fi event group */
	me->event_group = xEventGroupCreate();

    /* Initialize stack TCP/IP */
    ESP_ERROR_CHECK(esp_netif_init());

    /* Create event loop */
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* Create netif instances */
    esp_netif_create_default_wifi_sta();
    esp_netif_create_default_wifi_ap();

    /* Register event handlers */
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, (void *)me, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, ip_event_handler, (void *)me, NULL));

    /* Initialize Wi-Fi */
    wifi_init_config_t wifi_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_config));

    /* Start Wi-Fi in station mode */
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_start());

    /* Initialize provisioning manager */

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, prov_event_handler, (void *)me, NULL));

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
		ESP_LOGI(TAG, "Already provisioned. Connecting to AP...");

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
			ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(WIFI_PROV_SECURITY_1, strlen(CONFIG_BITEC_WIFI_POP_PIN) > 1 ? CONFIG_BITEC_WIFI_POP_PIN : NULL, service_name, NULL));
			free(service_name);
		}
		else
			ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(WIFI_PROV_SECURITY_1, strlen(CONFIG_BITEC_WIFI_POP_PIN) > 1 ? CONFIG_BITEC_WIFI_POP_PIN : NULL, "PROV_DEFAULT", NULL));

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

static esp_err_t custom_prov_data_handler(uint32_t session_id, const uint8_t * inbuf, ssize_t inlen, uint8_t * * outbuf, ssize_t * outlen, void * priv_data)
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

static void ip_event_handler(void * arg, esp_event_base_t event_base, int event_id, void * event_data)
{
	bitec_wifi_t * wifi = (bitec_wifi_t *)arg;

	switch(event_id)
	{
		case IP_EVENT_STA_GOT_IP:
		{
			ESP_LOGI(TAG, "IP_EVENT_STA_GOT_IP");

			ip_event_got_ip_t * event = (ip_event_got_ip_t*) event_data;
			ESP_LOGI(TAG, "Connected with IP Address:" IPSTR, IP2STR(&event->ip_info.ip));

			xEventGroupSetBits(wifi->event_group, IP_EVENT_STA_GOT_IP_BIT);

			break;
		}

		default:
			break;
	}
}

static void prov_event_handler(void * arg, esp_event_base_t event_base, int event_id, void * event_data)
{
	bitec_wifi_t * wifi = (bitec_wifi_t *)arg;

	switch(event_id)
	{
		case WIFI_PROV_START:
			ESP_LOGI(TAG, "WIFI_PROV_START");

			ESP_LOGI(TAG, "Provisioning started");

			xEventGroupSetBits(wifi->event_group, WIFI_PROV_START_BIT);


			break;

		case WIFI_PROV_CRED_RECV:
		{
			ESP_LOGI(TAG, "WIFI_PROV_CRED_RECV");

			/* Print received Wi-Fi credentials and set RGB LED in blue color */
			wifi_sta_config_t * wifi_sta_cfg = (wifi_sta_config_t *)event_data;
			ESP_LOGI(TAG, "Credentials received, SSID: %s & Password: %s", (const char *) wifi_sta_cfg->ssid, (const char *) wifi_sta_cfg->password);

			xEventGroupSetBits(wifi->event_group, WIFI_PROV_CRED_RECV_BIT);

			break;
		}

		case WIFI_PROV_CRED_FAIL:
		{
			ESP_LOGI(TAG, "WIFI_PROV_CRED_FAIL");

			wifi_prov_sta_fail_reason_t * reason = (wifi_prov_sta_fail_reason_t *)event_data;
			ESP_LOGE(TAG, "Provisioning failed!\n\tReason : %s"
			"\n\tPlease reset to factory and retry provisioning",
			( * reason == WIFI_PROV_STA_AUTH_ERROR) ?
			"Wi-Fi station authentication failed" : "Wi-Fi access-point not found");

			/* Set WIFI_FAIL_BIT to indicate provisioning error */
			xEventGroupSetBits(wifi->event_group, WIFI_PROV_CRED_FAIL_BIT);

			break;
		}

		case WIFI_PROV_CRED_SUCCESS:
			ESP_LOGI(TAG, "WIFI_PROV_CRED_SUCCESS");

			ESP_LOGI(TAG, "Provisioning successful");

			xEventGroupSetBits(wifi->event_group, WIFI_PROV_CRED_SUCCESS_BIT);

			break;

		case WIFI_PROV_END:
			ESP_LOGI(TAG, "WIFI_PROV_END");

			/* De-initialize manager once provisioning is finished */
			wifi_prov_mgr_deinit();

			xEventGroupSetBits(wifi->event_group, WIFI_PROV_END_BIT);


			break;

		default:
			break;
	}
}

static void wifi_event_handler(void * arg, esp_event_base_t event_base, int event_id, void * event_data)
{
	bitec_wifi_t * wifi = (bitec_wifi_t *)arg;

	switch(event_id)
	{
		case WIFI_EVENT_STA_DISCONNECTED:
			ESP_LOGI(TAG, "WIFI_EVENT_STA_DISCONNECTED");

			xEventGroupSetBits(wifi->event_group, WIFI_EVENT_STA_DISCONNECTED_BIT);

			break;

		case WIFI_EVENT_STA_CONNECTED:
			ESP_LOGI(TAG, "WIFI_EVENT_STA_CONNECTED");

			xEventGroupSetBits(wifi->event_group, WIFI_EVENT_STA_CONNECTED_BIT);

			break;

		default:
			break;
	}
}
/* end of file ---------------------------------------------------------------*/
