/**
 * Copyright 2015 opencxa.org
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <cxa_connectionManager.h>


// ******** includes ********
#include <stdlib.h>
#include <string.h>
#include <cxa_assert.h>
#include <cxa_esp8266_network_factory.h>
#include <cxa_esp8266_wifiManager.h>
#include <cxa_led.h>
#define CXA_LOG_LEVEL					CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>
#include <cxa_uniqueId.h>

#include <cxa_config.h>


// ******** local macro definitions ********
#define SSID							"lcars"
#define KEY								"pineapple14"

#define MQTT_SERVER						"m11.cloudmqtt.com"
#define MQTT_SERVER_PORTNUM				13164
#define MQTT_SERVER_USERNAME			"arsinio"
#define MQTT_SERVER_PASSWORD			"tmpPasswd"

#define BLINKPERIODMS_ON_CONFIG			100
#define BLINKPERIODMS_OFF_CONFIG		500
#define BLINKPERIODMS_ON_ASSOC			500
#define BLINKPERIODMS_OFF_ASSOC			500
#define BLINKPERIODMS_ON_CONNECTING		100
#define BLINKPERIODMS_OFF_CONNECTING	100
#define BLINKPERIODMS_ON_CONNECTED		5000
#define BLINKPERIODMS_OFF_CONNECTED		100


// ******** local type definitions ********


// ******** local function prototypes ********
static void wifiManCb_associated(const char *const ssidIn, void* userVarIn);
static void wifiManCb_lostAssociation(const char *const ssidIn, void* userVarIn);
static void wifiManCb_configMode_enter(void* userVarIn);

static void mqttClientCb_onConnect(cxa_mqtt_client_t *const clientIn, void* userVarIn);
static void mqttClientCb_onDisconnect(cxa_mqtt_client_t *const clientIn, void* userVarIn);


// ********  local variable declarations *********
static cxa_mqtt_client_network_t mqttClient;
static cxa_led_t led_conn;

static bool isInConnStandoff = false;
static cxa_timeDiff_t td_connStandoff;
static uint32_t connStandoff_ms;

static cxa_logger_t logger;


// ******** global function implementations ********
void cxa_connManager_init(cxa_timeBase_t *const timeBaseIn, cxa_gpio_t *const ledConnIn)
{
	cxa_assert(timeBaseIn);
	cxa_assert(ledConnIn);

	// save our references
	cxa_led_init(&led_conn, ledConnIn, timeBaseIn);

	// setup our connection standoff
	cxa_timeDiff_init(&td_connStandoff, timeBaseIn, true);
	srand(cxa_timeBase_getCount_us(timeBaseIn));

	// setup our logger
	cxa_logger_init(&logger, "connectionManager");
	cxa_logger_info(&logger, "associating");

	// setup our WiFi
	cxa_esp8266_wifiManager_init(NULL, timeBaseIn);
	cxa_esp8266_wifiManager_addListener(wifiManCb_configMode_enter, NULL, NULL, NULL, wifiManCb_associated, wifiManCb_lostAssociation, NULL, NULL);
	cxa_esp8266_wifiManager_addStoredNetwork(SSID, KEY);
	cxa_esp8266_wifiManager_start();

	// now setup our network connection
	cxa_esp8266_network_factory_init(timeBaseIn);

	// and our mqtt client
	cxa_mqtt_client_network_init(&mqttClient, timeBaseIn, cxa_uniqueId_getHexString());
	cxa_mqtt_client_addListener(&mqttClient.super, mqttClientCb_onConnect, NULL, mqttClientCb_onDisconnect, NULL);

	// start our LED
	cxa_led_blink(&led_conn, BLINKPERIODMS_ON_ASSOC, BLINKPERIODMS_OFF_ASSOC);
}


cxa_mqtt_client_t* cxa_connManager_getMqttClient(void)
{
	return &mqttClient.super;
}


void cxa_connManager_update(void)
{
	// handle our connection standoff (if needed)
	if( isInConnStandoff && cxa_timeDiff_isElapsed_ms(&td_connStandoff, connStandoff_ms) )
	{
		isInConnStandoff = false;
		if( cxa_esp8266_wifiManager_isAssociated() )
		{
			cxa_mqtt_client_network_connectToHost(&mqttClient, MQTT_SERVER, MQTT_SERVER_PORTNUM, MQTT_SERVER_USERNAME, (uint8_t*)MQTT_SERVER_PASSWORD, strlen(MQTT_SERVER_PASSWORD), false);
		}
	}

	// update everything
	cxa_led_update(&led_conn);
	cxa_esp8266_wifiManager_update();
	cxa_esp8266_network_factory_update();
	cxa_mqtt_client_update(&mqttClient.super);
}


// ******** local function implementations ********
static void wifiManCb_associated(const char *const ssidIn, void* userVarIn)
{
	cxa_logger_info(&logger, "associated");

	cxa_led_blink(&led_conn, BLINKPERIODMS_ON_CONNECTING, BLINKPERIODMS_OFF_CONNECTING);
	cxa_mqtt_client_network_connectToHost(&mqttClient, MQTT_SERVER, MQTT_SERVER_PORTNUM, MQTT_SERVER_USERNAME, (uint8_t*)MQTT_SERVER_PASSWORD, strlen(MQTT_SERVER_PASSWORD), false);
}


static void wifiManCb_lostAssociation(const char *const ssidIn, void* userVarIn)
{
	cxa_logger_info(&logger, "lost association");

	cxa_led_blink(&led_conn, BLINKPERIODMS_ON_ASSOC, BLINKPERIODMS_OFF_ASSOC);
	cxa_mqtt_client_network_internalDisconnect(&mqttClient);
}


static void wifiManCb_configMode_enter(void* userVarIn)
{
	cxa_logger_info(&logger, "configMode");

	cxa_led_blink(&led_conn, BLINKPERIODMS_ON_CONFIG, BLINKPERIODMS_OFF_CONFIG);
}


static void mqttClientCb_onConnect(cxa_mqtt_client_t *const clientIn, void* userVarIn)
{
	cxa_logger_info(&logger, "connected");

	cxa_led_blink(&led_conn, BLINKPERIODMS_ON_CONNECTED, BLINKPERIODMS_OFF_CONNECTED);
}


static void mqttClientCb_onDisconnect(cxa_mqtt_client_t *const clientIn, void* userVarIn)
{
	if( cxa_esp8266_wifiManager_isAssociated() )
	{
		// setup our standoff
		connStandoff_ms = rand() % 1000 + 500;
		isInConnStandoff = true;

		cxa_logger_info(&logger, "disconnected, retry after %d ms", connStandoff_ms);

		cxa_led_blink(&led_conn, BLINKPERIODMS_ON_CONNECTING, BLINKPERIODMS_OFF_CONNECTING);
	}
	else
	{
		cxa_led_blink(&led_conn, BLINKPERIODMS_ON_ASSOC, BLINKPERIODMS_OFF_ASSOC);
	}
}
