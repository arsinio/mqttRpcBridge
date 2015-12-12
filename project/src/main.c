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
#define CXA_LOG_LEVEL				CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>
#include <cxa_uniqueId.h>
#include <cxa_esp8266_gpio.h>
#include <cxa_esp8266_usart.h>
#include <cxa_esp8266_timeBase.h>
#include <cxa_esp8266_wifiManager.h>
#include <cxa_esp8266_network_factory.h>

#include <cxa_connectionManager.h>
#include <cxa_mqtt_rpc_node_bridge_single.h>
#include <cxa_mqtt_rpc_node_root.h>

#include <user_interface.h>


// ******** local macro definitions ********


// ******** local function prototoypes ********


// ******** local variable declarations ********
static cxa_esp8266_gpio_t led0;

static cxa_esp8266_usart_t usart_log;
static cxa_esp8266_usart_t usart_system;

static cxa_timeBase_t tb_generalPurpose;

static cxa_mqtt_rpc_node_root_t rpcNode_root;
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

	// setup our general-purpose timebase
	cxa_esp8266_timeBase_init(&tb_generalPurpose);

	// setup our logging system
	cxa_logger_setGlobalTimeBase(&tb_generalPurpose);
	cxa_logger_setGlobalIoStream(ios_log);

	// setup our system usart (to communicate with the rest of the system)
	// AND turn off system logging
	system_set_os_print(0);
	cxa_esp8266_usart_init_noHH(&usart_system, CXA_ESP8266_USART_0_ALTPINS, 9600, 1);

	// setup our connection manager
	cxa_connManager_init(&tb_generalPurpose, &led0.super);

	// setup our node root node bridge
	cxa_mqtt_rpc_node_root_init(&rpcNode_root, cxa_connManager_getMqttClient(), "/dev/beerSmart", cxa_uniqueId_getHexString());
	cxa_mqtt_rpc_node_bridge_single_init(&rpcNode_bridge, &rpcNode_root.super, cxa_usart_getIoStream(&usart_system.super), &tb_generalPurpose, "sys");
}


void loop(void)
{
	cxa_connManager_update();
	cxa_mqtt_rpc_node_bridge_update(&rpcNode_bridge.super);
}


// ******** local function implementations ********
