#ifndef CONFIG_OVERRIDE_H
#define CONFIG_OVERRIDE_H

// OPTIONAL: Assign default values here.
#define WIFI_SSID           "" // Leave unset for wireless autoconfig. Note that these values will be lost
#define WIFI_PASS           "" // when updating, but that's probably OK because they will be saved in EEPROM.

////////////////////////////////////////////////////////////////////////////////////////////////////
// These defaults may be overwritten with values saved by the web interface
#define MQTT_SRV            ""
#define MQTT_PORT           ""
#define MQTT_USER           ""
#define MQTT_PASS           ""
#define HASP_NODE           "plate01"
#define GROUP_NAME          "plates"
#define WEB_USER            "admin"
#define WEB_PASS            ""
#define MOTION_PIN          "0"

#endif
