#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "wifi_api.h"
#include "wifi_scan.h"
#include "jerry.h"

#include "microlattice.h"

DELCARE_HANDLER(wifiActive) {
  wifi_config_set_radio(args_p[0].v_uint32);

  ret_val_p->type = JERRY_API_DATA_TYPE_BOOLEAN;
  ret_val_p->v_bool = true;
  return true;
}

DELCARE_HANDLER(wifi) {
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
      // g_httpcallback = jerry_api_create_object ();
      // g_httpcallback.v_object = args_p[1].v_object;
      // jerry_api_value_t
      // g_callbackvalue = args_p[1];

      /* ssid */
      int ssid_req_sz = jerry_api_string_to_char_buffer(ssid.v_string, NULL, 0);
      ssid_req_sz *= -1;
      char ssid_buffer [ssid_req_sz+1];
      ssid_req_sz = jerry_api_string_to_char_buffer (ssid.v_string, (jerry_api_char_t *) ssid_buffer, ssid_req_sz);
      ssid_buffer[ssid_req_sz] = '\0';

      /* mode */
      int mode_req_sz = jerry_api_string_to_char_buffer(mode.v_string, NULL, 0);
      mode_req_sz *= -1;
      char mode_buffer [mode_req_sz+1]; // 不能有*
      mode_req_sz = jerry_api_string_to_char_buffer (mode.v_string, (jerry_api_char_t *) mode_buffer, mode_req_sz);
      mode_buffer[mode_req_sz] = '\0';

      /* password */
      int password_req_sz = jerry_api_string_to_char_buffer(password.v_string, NULL, 0);
      password_req_sz *= -1;
      char password_buffer [password_req_sz+1];
      password_req_sz = jerry_api_string_to_char_buffer (password.v_string, (jerry_api_char_t *) password_buffer, password_req_sz);
      password_buffer[password_req_sz] = '\0';

      uint8_t opmode;
      uint8_t port;
      if (strncmp (mode_buffer, "station", (size_t)mode_req_sz) == 0) {
        opmode = WIFI_MODE_STA_ONLY;
        port = WIFI_PORT_STA;
      } else if (strncmp (mode_buffer, "ap", (size_t)mode_req_sz) == 0) {
        opmode  = WIFI_MODE_AP_ONLY;
        port = WIFI_PORT_AP;
      }
      uint8_t *ssid = ssid_buffer;
      wifi_auth_mode_t auth = WIFI_AUTH_MODE_WPA_PSK_WPA2_PSK;
      wifi_encrypt_type_t encrypt = WIFI_ENCRYPT_TYPE_TKIP_AES_MIX;
      uint8_t *password = password_buffer;
      uint8_t nv_opmode;

      if (wifi_config_init() == 0) {
        wifi_config_get_opmode(&nv_opmode);
        if (nv_opmode != opmode) {
            wifi_config_set_opmode(opmode);
        }
        wifi_config_set_ssid(port, ssid ,strlen(ssid));
        wifi_config_set_security_mode(port, auth, encrypt);
        wifi_config_set_wpa_psk_key(port, password, strlen(password));
        wifi_config_reload_setting();
        network_dhcp_start(opmode);
      }

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
  REGISTER_HANDLER(wifi);
  REGISTER_HANDLER(wifiActive);
}