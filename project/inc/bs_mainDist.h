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
#ifndef BS_MAIN_DIST_H_
#define BS_MAIN_DIST_H_


// ******** includes ********
#include <stdbool.h>
#include <cxa_logger_header.h>
#include <cxa_gpio.h>
#include <cxa_timeBase.h>
#include <cxa_timeDiff.h>
#include <cxa_rpc_node.h>
#include <cxa_rpc_nodeRemote.h>
#include <cxa_stateMachine.h>


// ******** global macro definitions ********


// ******** global type definitions *********
/**
 * @public
 * @brief "Forward" declaration of the bs_mainDist_t object
 */
typedef struct bs_mainDist bs_mainDist_t;


/**
 * @private
 */
typedef enum
{
	BS_MD_CLEANTYPE_1CHAN,
	BS_MD_CLEANTYPE_FULLSYS
}bs_mainDist_cleanType_t;


/**
 * @private
 */
typedef enum
{
	BS_MD_FULLSYS_CLEANDIR_FORWARD,
	BS_MD_FULLSYS_CLEANDIR_REVERSE
}bs_mainDist_fullSys_cleanDir_t;


/**
 * @private
 */
struct bs_mainDist
{
	cxa_stateMachine_t stateMachine;
	cxa_logger_t logger;

	cxa_rpc_node_t rpcNode;
	cxa_rpc_nodeRemote_t nr_cbu;
	cxa_rpc_nodeRemote_t nr_dtu;

	cxa_gpio_t* sol_co2;
	cxa_gpio_t* sol_dosingPump;
	cxa_gpio_t* sol_h2o;

	cxa_gpio_t* led_cloud;
	cxa_gpio_t* led_run;
	cxa_gpio_t* led_config;
	cxa_gpio_t* led_error;

	cxa_timeDiff_t td_transition;
	bs_mainDist_cleanType_t cleanType;
	uint8_t singleCleanChanIndex;
	bs_mainDist_fullSys_cleanDir_t fullSysCleanDir;
};


// ******** global function prototypes ********
bool bs_mainDist_init(bs_mainDist_t *const mdIn, cxa_timeBase_t *const timeBaseIn, cxa_ioStream_t *const ioStreamIn,
					  cxa_gpio_t *const sol_co2In, cxa_gpio_t *const sol_dosingPumpIn, cxa_gpio_t *const sol_h2oIn,
					  cxa_gpio_t *const led_cloudIn, cxa_gpio_t *const led_runIn, cxa_gpio_t *const led_configIn, cxa_gpio_t *const led_errorIn);

cxa_rpc_node_t* bs_mainDist_getRpcNode(bs_mainDist_t *const mdIn);

bool bs_mainDist_startChannelClean(bs_mainDist_t *const mdIn, uint8_t chanIndexIn);
bool bs_mainDist_startSystemClean(bs_mainDist_t *const mdIn);

void bs_mainDist_update(bs_mainDist_t *const mdIn);

#endif // BS_MAIN_DIST_H_
