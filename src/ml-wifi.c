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

#include "jerry-api.h"
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
  jerry_value_t eval_ret;
  eval_ret = jerry_eval(script, strlen (script), false);
  jerry_release_value (eval_ret);
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
  wifi_config_set_radio(jerry_get_number_value(args_p[0]));
  return jerry_create_boolean (true);
}

DELCARE_HANDLER(__wifi) {
  if (jerry_value_is_object(args_p[0])) {

    // mode
    jerry_value_t prop_mode = jerry_create_string ((const jerry_char_t *) "mode");
    jerry_value_t mode = jerry_get_property(args_p[0], prop_mode);
    // ssid
    jerry_value_t prop_ssid = jerry_create_string ((const jerry_char_t *) "ssid");
    jerry_value_t ssid = jerry_get_property(args_p[0], prop_ssid);
    // password
    jerry_value_t prop_password = jerry_create_string ((const jerry_char_t *) "password");
    jerry_value_t password = jerry_get_property(args_p[0], prop_password);

    if (jerry_value_is_string(mode)) {
      /* ssid */
      jerry_size_t ssid_req_sz = jerry_get_string_size (ssid);
      jerry_char_t ssid_buffer[ssid_req_sz];
      jerry_string_to_char_buffer (ssid, ssid_buffer, ssid_req_sz);
      ssid_buffer[ssid_req_sz] = '\0';
      /* mode */
      jerry_size_t mode_req_sz = jerry_get_string_size (mode);
      jerry_char_t mode_buffer[mode_req_sz];
      jerry_string_to_char_buffer (mode, mode_buffer, mode_req_sz);
      mode_buffer[mode_req_sz] = '\0';
      /* password */
      jerry_size_t password_req_sz = jerry_get_string_size (password);
      jerry_char_t password_buffer[password_req_sz];
      jerry_string_to_char_buffer (password, password_buffer, password_req_sz);
      password_buffer[password_req_sz] = '\0';

      wifi_connection_register_event_handler(WIFI_EVENT_IOT_INIT_COMPLETE , _wifi_event_handler);
      wifi_connection_register_event_handler(WIFI_EVENT_IOT_CONNECTED, _wifi_event_handler);
      wifi_connection_register_event_handler(WIFI_EVENT_IOT_PORT_SECURE, _wifi_event_handler);
      wifi_connection_register_event_handler(WIFI_EVENT_IOT_DISCONNECTED, _wifi_event_handler);

      if (strncmp (mode_buffer, "station", (size_t)mode_req_sz) == 0) {
        wifi_config.opmode = WIFI_MODE_STA_ONLY;
        strcpy((char *)wifi_config.sta_config.ssid, ssid_buffer);
        wifi_config.sta_config.ssid_length = ssid_req_sz;
        strcpy((char *)wifi_config.sta_config.password, password_buffer);
        wifi_config.sta_config.password_length = password_req_sz;

      } else if (strncmp (mode_buffer, "ap", (size_t)mode_req_sz) == 0) {
        wifi_config.opmode = WIFI_MODE_AP_ONLY;
      }

      xTaskCreate(wifi_initial_task, "WifiInitTask", 2048, NULL, 1, NULL);

    }

    jerry_release_value(prop_mode);
    jerry_release_value(prop_ssid);
    jerry_release_value(prop_password);
    jerry_release_value(mode);
    jerry_release_value(ssid);
    jerry_release_value(password);

  }

  return jerry_create_boolean (true);
}

void ml_wifi_init (void) {
  REGISTER_HANDLER(__wifi);
  REGISTER_HANDLER(__wifiActive);
}