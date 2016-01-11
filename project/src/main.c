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
 *
 * @author Christopher Armenio
 */
#include <cxa_assert.h>
#include <cxa_connectionManager.h>
#define CXA_LOG_LEVEL				CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>
#include <cxa_uniqueId.h>
#include <cxa_esp8266_gpio.h>
#include <cxa_esp8266_usart.h>
#include <cxa_esp8266_timeBase.h>
#include <cxa_esp8266_wifiManager.h>
#include <cxa_esp8266_network_factory.h>

#include <cxa_mqtt_messageFactory.h>
#include <cxa_mqtt_rpc_node_bridge_single.h>
#include <cxa_mqtt_rpc_node_root.h>
#include <cxa_protocolParser_mqtt.h>

#include <user_interface.h>


// ******** local macro definitions ********


// ******** local function prototoypes ********
static cxa_mqtt_rpc_node_bridge_authorization_t authCb_sys(char *const clientIdIn, size_t clientIdLen_bytes,
														   char *const usernameIn, size_t usernameLen_bytesIn,
														   uint8_t *const passwordIn, size_t passwordLen_bytesIn,
														   void *userVarIn);


// ******** local variable declarations ********
static cxa_esp8266_gpio_t led0;
static cxa_esp8266_gpio_t gpio_provisioned;

static cxa_esp8266_usart_t usart_log;
static cxa_esp8266_usart_t usart_system;

static cxa_mqtt_rpc_node_root_t rpcNode_root;

static cxa_protocolParser_mqtt_t mpp;
static cxa_mqtt_rpc_node_bridge_single_t rpcNode_bridge;


// ******** global function implementations ********
void setup(void)
{
	// setup our assert LED
	cxa_esp8266_gpio_init_output(&led0, 0, CXA_GPIO_POLARITY_INVERTED, 0);
	cxa_assert_setAssertGpio(&led0.super);

	// setup our logging usart
	cxa_esp8266_usart_init_noHH(&usart_log, CXA_ESP8266_USART_1, 115200, 0);
	cxa_ioStream_t* ios_log = cxa_usart_getIoStream(&usart_log.super);
	cxa_assert_setIoStream(ios_log);

	// setup our timebase
	cxa_esp8266_timeBase_init();

	// setup our logging system
	cxa_logger_setGlobalIoStream(ios_log);

	// setup our system usart (to communicate with the rest of the system)
	// AND turn off system logging
	system_set_os_print(0);
	cxa_esp8266_usart_init_noHH(&usart_system, CXA_ESP8266_USART_0_ALTPINS, 9600, 1);

	// setup our provision gpio flag
	cxa_esp8266_gpio_init_output(&gpio_provisioned, 14, CXA_GPIO_POLARITY_NONINVERTED, 0);

	// setup our connection manager
	cxa_connManager_init(&led0.super);

	// setup our MQTT protocol parser
	cxa_mqtt_message_t* msg = cxa_mqtt_messageFactory_getFreeMessage_empty();
	cxa_assert(msg);
	cxa_protocolParser_mqtt_init(&mpp, cxa_usart_getIoStream(&usart_system.super), cxa_mqtt_message_getBuffer(msg));

	// setup our node root node and bridge
	cxa_mqtt_rpc_node_root_init(&rpcNode_root, cxa_connManager_getMqttClient(), true, "/dev/beerSmart/%s", cxa_uniqueId_getHexString());
	cxa_mqtt_rpc_node_bridge_single_init(&rpcNode_bridge, &rpcNode_root.super, &mpp, "sys");
	cxa_mqtt_rpc_node_bridge_single_setAuthCb(&rpcNode_bridge, authCb_sys, NULL);
}


void loop(void)
{
	cxa_connManager_update();
	cxa_protocolParser_mqtt_update(&mpp);
	cxa_mqtt_rpc_node_root_update(&rpcNode_root);
}


// ******** local function implementations ********
static cxa_mqtt_rpc_node_bridge_authorization_t authCb_sys(char *const clientIdIn, size_t clientIdLen_bytes,
														   char *const usernameIn, size_t usernameLen_bytesIn,
														   uint8_t *const passwordIn, size_t passwordLen_bytesIn,
														   void *userVarIn)
{
	// let our attached device know that they have been provisioned
	cxa_gpio_setValue(&gpio_provisioned.super, 1);

	// allow all nodes to connect (really should only be one)
	return CXA_MQTT_RPC_NODE_BRIDGE_AUTH_ALLOW;
}
