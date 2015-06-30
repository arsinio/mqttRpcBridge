/**
 * @file
 *
 * @note This object should work across all architecture-specific implementations
 *
 *
 * #### Example Usage: ####
 *
 * @code
 * @endcode
 *
 *
 * @copyright 2015 opencxa.org
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @author Christopher Armenio
 */
#ifndef BS_CLEANING_CHANNEL_H_
#define BS_CLEANING_CHANNEL_H_


// ******** includes ********
#include <stdbool.h>
#include <cxa_logger_header.h>
#include <cxa_gpio.h>
#include <cxa_timeBase.h>
#include <cxa_rpc_node.h>
#include <cxa_stateMachine.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 * @brief "Forward" declaration of the bs_cleaningChannel_t object
 */
typedef struct bs_cleaningChannel bs_cleaningChannel_t;


typedef enum
{
	BS_CLEANCHAN_STATE_UNKNOWN,
	BS_CLEANCHAN_STATE_IDLE,
	BS_CLEANCHAN_STATE_BUS,
	BS_CLEANCHAN_STATE_DRAIN_ADJACENT
}bs_cleaningChannel_state_t;


/**
 * @private
 */
struct bs_cleaningChannel
{
	cxa_stateMachine_t stateMachine;
	cxa_rpc_node_t rpcNode;

	cxa_gpio_t* sol_bus;
	cxa_gpio_t* sol_bypass;
};


// ******** global function prototypes ********
bool bs_cleaningChannel_init(bs_cleaningChannel_t *const ccIn, const uint8_t indexIn, cxa_gpio_t *const sol_busIn, cxa_gpio_t *const sol_bypassIn, cxa_timeBase_t *const tbIn);

cxa_rpc_node_t* bs_cleaningChannel_getRpcNode(bs_cleaningChannel_t *const ccIn);

void bs_cleaningChannel_setState(bs_cleaningChannel_t *const ccIn, bs_cleaningChannel_state_t newStateIn);
bs_cleaningChannel_state_t bs_cleaningChannel_getState(bs_cleaningChannel_t *const ccIn);

void bs_cleaningChannel_update(bs_cleaningChannel_t *const ccIn);

#endif // BS_CLEANING_CHANNEL_H_
