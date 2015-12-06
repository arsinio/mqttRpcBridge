#ifndef _USER_CONFIG_H_
#define _USER_CONFIG_H_

#define CFG_HOLDER	0x00FF55A4	/* Change this value to load default configurations */
#define CFG_LOCATION	0x3C	/* Please don't change or if you know what you doing */
#define CLIENT_SSL_ENABLE

/*DEFAULT CONFIGURATIONS*/

#define STA_SSID "lcars"
#define STA_PASS "pineapple14"
#define STA_TYPE AUTH_WPA2_PSK

#define DEFAULT_SECURITY	0
#define QUEUE_BUFFER_SIZE		 		2048

#endif
