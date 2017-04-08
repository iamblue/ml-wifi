#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "os.h"

#include "wifi_lwip_helper.h"
#include "wifi_api.h"

#include "jerry-api.h"
#include "microlattice.h"

void wifi_initial_task() {
  lwip_net_ready();

  char script [] = "global.eventStatus.emit('wifiConnect', true);";
  jerry_value_t eval_ret;
  eval_ret = jerry_eval(script, strlen (script), false);
  jerry_release_value (eval_ret);

  for (;;) {
      ;
  }
}

DELCARE_HANDLER(__wifi) {
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

      wifi_config_t config = {0};

      strcpy((char *)config.sta_config.ssid, ssid_buffer);
      strcpy((char *)config.sta_config.password, password_buffer);
      config.sta_config.ssid_length = strlen((const char *)config.sta_config.ssid);
      config.sta_config.password_length = strlen((const char *)config.sta_config.password);

      if (strncmp (mode_buffer, "station", (size_t)mode_req_sz) == 0) {
        config.opmode = WIFI_MODE_STA_ONLY;
      } else if (strncmp (mode_buffer, "ap", (size_t)mode_req_sz) == 0) {
        config.opmode = WIFI_MODE_AP_ONLY;
      }

      wifi_init(&config, NULL);
      lwip_network_init(config.opmode);
      lwip_net_start(config.opmode);

      xTaskCreate(wifi_initial_task, "WifiInitTask", 4096, NULL, 1, NULL);
    }

    jerry_release_value(prop_mode);
    jerry_release_value(prop_ssid);
    jerry_release_value(prop_password);
    jerry_release_value(mode);
    jerry_release_value(ssid);
    jerry_release_value(password);

    return jerry_create_boolean (true);
    /* Tcpip stack and net interface initialization,  dhcp client, dhcp server process initialization*/
}

void ml_wifi_init (void) {
  REGISTER_HANDLER(__wifi);
}