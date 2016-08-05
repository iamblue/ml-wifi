#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "os.h"
#include "semphr.h"
#include "wifi_api.h"
#include "lwip/ip4_addr.h"
#include "lwip/inet.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "lwip/dhcp.h"
#include "ethernetif.h"
#include "portmacro.h"
#include "app_common.h"

#include "jerry.h"
#include "microlattice.h"

static SemaphoreHandle_t ip_ready;

/**
  * @brief  dhcp got ip will callback this function.
  * @param[in] struct netif *netif: which network interface got ip.
  * @retval None
  */
static void _ip_ready_callback(struct netif *netif)
{
    if (!ip4_addr_isany_val(netif->ip_addr)) {
        char ip_addr[17] = {0};
        if (NULL != inet_ntoa(netif->ip_addr)) {
            strcpy(ip_addr, inet_ntoa(netif->ip_addr));
            LOG_I(common, "************************");
            LOG_I(common, "DHCP got IP:%s", ip_addr);
            LOG_I(common, "************************");
            wifi_callback();
        } else {
            LOG_E(common, "DHCP got Failed");
        }
    }
    xSemaphoreGive(ip_ready);
    LOG_I(common, "ip ready");
}

static int32_t _wifi_event_handler(wifi_event_t event,
        uint8_t *payload,
        uint32_t length)
{
    struct netif *sta_if;
    LOG_I(common, "wifi event: %d", event);

    switch(event)
    {
    case WIFI_EVENT_IOT_PORT_SECURE:
        sta_if = netif_find_by_type(NETIF_TYPE_STA);
        netif_set_link_up(sta_if);
        LOG_I(common, "wifi connected");
        break;
    case WIFI_EVENT_IOT_DISCONNECTED:
        sta_if = netif_find_by_type(NETIF_TYPE_STA);
        netif_set_link_down(sta_if);
        LOG_I(common, "wifi disconnected");
        break;
    }

    return 1;
}



void wifi_connect(void)
{
  char script [] = "global.eventStatus.emit('wifiConnect', true);";
  jerry_api_value_t eval_ret;
  jerry_api_eval (script, strlen (script), false, false, &eval_ret);
  jerry_api_release_value (&eval_ret);
}

wifi_config_t wifi_config = {0};
lwip_tcpip_config_t tcpip_config = {{0}, {0}, {0}, {0}, {0}, {0}};

void wifi_initial_task() {
  struct netif *sta_if;
  wifi_init(&wifi_config, NULL);
  lwip_tcpip_init(&tcpip_config, WIFI_MODE_STA_ONLY);

  ip_ready = xSemaphoreCreateBinary();

  sta_if = netif_find_by_type(NETIF_TYPE_STA);
  netif_set_status_callback(sta_if, _ip_ready_callback);
  dhcp_start(sta_if);

  // wait for result
  xSemaphoreTake(ip_ready, portMAX_DELAY);
  vTaskDelete(NULL);
}

void wifi_connect_task(void *parameter) {
  wifi_connect();
  for (;;) {
      ;
  }
}

void wifi_callback(const struct netif *netif) {
  xTaskCreate(wifi_connect_task, "WifiTask", 8048, NULL, 1, NULL);
}

DELCARE_HANDLER(__wifiActive) {
  wifi_config_set_radio(args_p[0].v_uint32);

  ret_val_p->type = JERRY_API_DATA_TYPE_BOOLEAN;
  ret_val_p->v_bool = true;
  return true;
}

DELCARE_HANDLER(__wifi) {
  if (args_p[0].type == JERRY_API_DATA_TYPE_OBJECT) {
    jerry_api_value_t mode;
    jerry_api_value_t ssid;
    jerry_api_value_t password;

    // mode
    bool is_ok = jerry_api_get_object_field_value (args_p[0].v_object, (jerry_api_char_t *) "mode", &mode);
    // ssid
    jerry_api_get_object_field_value (args_p[0].v_object, (jerry_api_char_t *) "ssid", &ssid);
    // password
    jerry_api_get_object_field_value (args_p[0].v_object, (jerry_api_char_t *) "password", &password);

    if (is_ok && mode.type == JERRY_API_DATA_TYPE_STRING) {
      /* ssid */
      int ssid_req_sz = -jerry_api_string_to_char_buffer(ssid.v_string, NULL, 0);
      char * ssid_buffer = (char*) malloc (ssid_req_sz);
      ssid_req_sz = jerry_api_string_to_char_buffer (ssid.v_string, (jerry_api_char_t *) ssid_buffer, ssid_req_sz);
      ssid_buffer[ssid_req_sz] = '\0';


      /* mode */
      int mode_req_sz = -jerry_api_string_to_char_buffer(mode.v_string, NULL, 0);
      char * mode_buffer = (char*) malloc (mode_req_sz);
      mode_req_sz = jerry_api_string_to_char_buffer (mode.v_string, (jerry_api_char_t *) mode_buffer, mode_req_sz);
      mode_buffer[mode_req_sz] = '\0';

      /* password */
      int password_req_sz = -jerry_api_string_to_char_buffer(password.v_string, NULL, 0);
      char * password_buffer = (char*) malloc (password_req_sz);
      password_req_sz = jerry_api_string_to_char_buffer (password.v_string, (jerry_api_char_t *) password_buffer, password_req_sz);
      password_buffer[password_req_sz] = '\0';

      wifi_connection_register_event_handler(WIFI_EVENT_IOT_INIT_COMPLETE , _wifi_event_handler);
      wifi_connection_register_event_handler(WIFI_EVENT_IOT_CONNECTED, _wifi_event_handler);
      wifi_connection_register_event_handler(WIFI_EVENT_IOT_PORT_SECURE, _wifi_event_handler);
      wifi_connection_register_event_handler(WIFI_EVENT_IOT_DISCONNECTED, _wifi_event_handler);

      if (strncmp (mode_buffer, "station", (size_t)mode_req_sz) == 0) {
        // opmode = WIFI_MODE_STA_ONLY;
        wifi_config.opmode = WIFI_MODE_STA_ONLY;
        // port = WIFI_PORT_STA;
        strcpy((char *)wifi_config.sta_config.ssid, ssid_buffer);
        wifi_config.sta_config.ssid_length = strlen(ssid_buffer);
        strcpy((char *)wifi_config.sta_config.password, password_buffer);
        wifi_config.sta_config.password_length = strlen(password_buffer);

      } else if (strncmp (mode_buffer, "ap", (size_t)mode_req_sz) == 0) {
        wifi_config.opmode = WIFI_MODE_AP_ONLY;
        // port = WIFI_PORT_AP;
      }

      xTaskCreate(wifi_initial_task, "WifiInitTask", 2048, NULL, 1, NULL);

      jerry_api_release_object(&mode);
      jerry_api_release_object(&ssid);
      jerry_api_release_object(&password);

    }
  }

  ret_val_p->type = JERRY_API_DATA_TYPE_OBJECT;
  ret_val_p->v_object = args_p[0].v_object;
  return true;
}

void ml_wifi_init (void) {
  REGISTER_HANDLER(__wifi);
  REGISTER_HANDLER(__wifiActive);
}