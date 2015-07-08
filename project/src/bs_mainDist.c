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
#include "bs_mainDist.h"


/**
 * @author Christopher Armenio
 */


// ******** includes ********
#include <stdio.h>
#include <string.h>
#include <cxa_assert.h>
#include <cxa_logger_implementation.h>
#include <cxa_rpc_message.h>
#include <cxa_rpc_messageFactory.h>
#include <cxa_config.h>


// ******** local macro definitions ********
#define NODE_NAME					"MainDist"
#define SETUP_TIME_MS				5000
#define RINSE_TIME_MS				20000
#define DOSE_TIME_MS				20000
#define PURGE_TIME_MS				20000

#define UNIT_CBU					"CBU"
#define UNIT_DTU					"DTU"


// ******** local type definitions ********
typedef enum
{
	BS_MD_STATE_IDLE,

	BS_MD_STATE_1CHAN_SETUP,
	BS_MD_STATE_FULLSYS_SETUP_FORWARD,
	BS_MD_STATE_FULLSYS_SETUP_REVERSE,

	BS_MD_STATE_RINSE1,
	BS_MD_STATE_DOSE,
	BS_MD_STATE_RINSE2,
	BS_MD_STATE_PURGE
}bs_mainDist_state_t;


typedef enum
{
	BS_CLEANCHAN_STATE_UNKNOWN,
	BS_CLEANCHAN_STATE_IDLE,
	BS_CLEANCHAN_STATE_BUS,
	BS_CLEANCHAN_STATE_DRAIN_ADJACENT
}bs_cleaningChannel_state_t;


// ******** local function prototypes ********
static void setChannelState(bs_mainDist_t *const mdIn, char *const unitIn, uint8_t indexIn, bs_cleaningChannel_state_t stateIn);

static void stateCb_idle_enter(cxa_stateMachine_t *const smIn, void *userVarIn);
static void stateCb_1chanSetup_enter(cxa_stateMachine_t *const smIn, void *userVarIn);
static void stateCb_fullSysSetupForward_enter(cxa_stateMachine_t *const smIn, void *userVarIn);
static void stateCb_fullSysSetupReverse_enter(cxa_stateMachine_t *const smIn, void *userVarIn);
static void stateCb_rinse_enter(cxa_stateMachine_t *const smIn, void *userVarIn);
static void stateCb_dose_enter(cxa_stateMachine_t *const smIn, void *userVarIn);
static void stateCb_purge_enter(cxa_stateMachine_t *const smIn, void *userVarIn);
static void stateCb_purge_state(cxa_stateMachine_t *const smIn, void *userVarIn);


// ********  local variable declarations *********


// ******** global function implementations ********
bool bs_mainDist_init(bs_mainDist_t *const mdIn, cxa_timeBase_t *const timeBaseIn, cxa_ioStream_t *const ioStreamIn,
					  cxa_gpio_t *const sol_co2In, cxa_gpio_t *const sol_dosingPumpIn, cxa_gpio_t *const sol_h2oIn,
					  cxa_gpio_t *const led_cloudIn, cxa_gpio_t *const led_runIn, cxa_gpio_t *const led_configIn, cxa_gpio_t *const led_errorIn)
{
	cxa_assert(mdIn);
	cxa_assert(timeBaseIn);
	cxa_assert(sol_co2In);
	cxa_assert(sol_dosingPumpIn);
	cxa_assert(sol_h2oIn);
	cxa_assert(led_cloudIn);
	cxa_assert(led_runIn);
	cxa_assert(led_configIn);
	cxa_assert(led_errorIn);

	// setup our internal state
	mdIn->sol_co2 = sol_co2In;
	mdIn->sol_dosingPump = sol_dosingPumpIn;
	mdIn->sol_h2o = sol_h2oIn;
	mdIn->led_cloud = led_cloudIn;
	mdIn->led_run = led_runIn;
	mdIn->led_config = led_configIn;
	mdIn->led_error = led_errorIn;
	cxa_timeDiff_init(&mdIn->td_transition, timeBaseIn, true);

	// setup our rpc node and remotes
	//@todo fix this
	cxa_rpc_protocolParser_init(&mdIn->rpp, 2, ioStreamIn);

	/*
	cxa_rpc_node_init(&mdIn->rpcNode, timeBaseIn, NODE_NAME);
	cxa_rpc_nodeRemote_init_upstream(&mdIn->nr_cbu, ioStreamIn, timeBaseIn);
	if( !cxa_rpc_node_addSubNode_remote(&mdIn->rpcNode, &mdIn->nr_cbu) ) return false;
	cxa_rpc_nodeRemote_init_upstream(&mdIn->nr_dtu, ioStreamIn, timeBaseIn);
	if( !cxa_rpc_node_addSubNode_remote(&mdIn->rpcNode, &mdIn->nr_dtu) ) return false;
	*/

	// now setup our logger
	cxa_logger_init(&mdIn->logger, NODE_NAME);

	// and our stateMachine
	cxa_stateMachine_init_timedStates(&mdIn->stateMachine, NODE_NAME, timeBaseIn);
	cxa_stateMachine_addState(&mdIn->stateMachine, BS_MD_STATE_IDLE, "idle", stateCb_idle_enter, NULL, NULL, (void*)mdIn);
	cxa_stateMachine_addState_timed(&mdIn->stateMachine, BS_MD_STATE_1CHAN_SETUP, "1chan_setup", BS_MD_STATE_RINSE1, SETUP_TIME_MS, stateCb_1chanSetup_enter, NULL, NULL, (void*)mdIn);
	cxa_stateMachine_addState_timed(&mdIn->stateMachine, BS_MD_STATE_FULLSYS_SETUP_FORWARD, "fullSys_fwd_setup", BS_MD_STATE_RINSE1, SETUP_TIME_MS, stateCb_fullSysSetupForward_enter, NULL, NULL, (void*)mdIn);
	cxa_stateMachine_addState_timed(&mdIn->stateMachine, BS_MD_STATE_FULLSYS_SETUP_REVERSE, "fullSys_rev_setup", BS_MD_STATE_RINSE1, SETUP_TIME_MS, stateCb_fullSysSetupReverse_enter, NULL, NULL, (void*)mdIn);
	cxa_stateMachine_addState_timed(&mdIn->stateMachine, BS_MD_STATE_RINSE1, "rinse1", BS_MD_STATE_DOSE, RINSE_TIME_MS, stateCb_rinse_enter, NULL, NULL, (void*)mdIn);
	cxa_stateMachine_addState_timed(&mdIn->stateMachine, BS_MD_STATE_DOSE, "dose", BS_MD_STATE_RINSE2, DOSE_TIME_MS, stateCb_dose_enter, NULL, NULL, (void*)mdIn);
	cxa_stateMachine_addState_timed(&mdIn->stateMachine, BS_MD_STATE_RINSE2, "rinse2", BS_MD_STATE_PURGE, RINSE_TIME_MS, stateCb_rinse_enter, NULL, NULL, (void*)mdIn);
	cxa_stateMachine_addState(&mdIn->stateMachine, BS_MD_STATE_PURGE, "purge", stateCb_purge_enter, stateCb_purge_state, NULL, (void*)mdIn);
	cxa_stateMachine_transition(&mdIn->stateMachine, BS_MD_STATE_IDLE);
	cxa_stateMachine_update(&mdIn->stateMachine);

	return true;
}


cxa_rpc_node_t* bs_mainDist_getRpcNode(bs_mainDist_t *const mdIn)
{
	cxa_assert(mdIn);

	return &mdIn->rpcNode;
}


bool bs_mainDist_startChannelClean(bs_mainDist_t *const mdIn, uint8_t chanIndexIn)
{
	cxa_assert(mdIn);
	if( cxa_stateMachine_getCurrentState(&mdIn->stateMachine) != BS_MD_STATE_IDLE ) return false;

	mdIn->singleCleanChanIndex = chanIndexIn;
	cxa_stateMachine_transition(&mdIn->stateMachine, BS_MD_STATE_1CHAN_SETUP);
	return true;
}


bool bs_mainDist_startSystemClean(bs_mainDist_t *const mdIn)
{
	cxa_assert(mdIn);
	if( cxa_stateMachine_getCurrentState(&mdIn->stateMachine) != BS_MD_STATE_IDLE ) return false;

	cxa_stateMachine_transition(&mdIn->stateMachine, BS_MD_STATE_FULLSYS_SETUP_FORWARD);
	return true;
}


bool bs_mainDist_isIdle(bs_mainDist_t *const mdIn)
{
	cxa_assert(mdIn);

	return (cxa_stateMachine_getCurrentState(&mdIn->stateMachine) == BS_MD_STATE_IDLE );
}


void bs_mainDist_update(bs_mainDist_t *const mdIn)
{
	cxa_assert(mdIn);

	cxa_stateMachine_update(&mdIn->stateMachine);

	//cxa_rpc_nodeRemote_update(&mdIn->nr_cbu);
	//cxa_rpc_nodeRemote_update(&mdIn->nr_dtu);
}


// ******** local function implementations ********
static void setChannelState(bs_mainDist_t *const mdIn, char *const unitIn, uint8_t indexIn, bs_cleaningChannel_state_t stateIn)
{
	cxa_assert(mdIn);
	cxa_assert(unitIn);

	char dest[64];
	sprintf(dest, "%s/cleanChan_%d", unitIn, indexIn);

	cxa_rpc_message_t* reqMsg = cxa_rpc_messageFactory_getFreeMessage_empty();
	uint8_t params[] = { (uint8_t)stateIn };
	cxa_assert( cxa_rpc_message_initRequest(reqMsg, dest, "setState", params, sizeof(params)) );
	cxa_assert( cxa_rpc_message_setId(reqMsg, 1) );
	cxa_assert( cxa_rpc_message_prependNodeNameToSource(reqMsg, "mainDist") );
	cxa_assert( cxa_rpc_message_prependNodeNameToSource(reqMsg, "/") );

	cxa_rpc_protocolParser_writeMessage(&mdIn->rpp, reqMsg);
	cxa_rpc_messageFactory_decrementMessageRefCount(reqMsg);
}


static void stateCb_idle_enter(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	bs_mainDist_t *const mdIn = (bs_mainDist_t*)userVarIn;
	cxa_assert(mdIn);

	cxa_gpio_setValue(mdIn->sol_co2, 0);
	cxa_gpio_setValue(mdIn->sol_dosingPump, 0);
	cxa_gpio_setValue(mdIn->sol_h2o, 0);

	// make sure all chans and dt are set to idle
	setChannelState(mdIn, UNIT_CBU, 0, BS_CLEANCHAN_STATE_IDLE);
	setChannelState(mdIn, UNIT_CBU, 1, BS_CLEANCHAN_STATE_IDLE);
	setChannelState(mdIn, UNIT_CBU, 2, BS_CLEANCHAN_STATE_IDLE);
	setChannelState(mdIn, UNIT_CBU, 3, BS_CLEANCHAN_STATE_IDLE);
	setChannelState(mdIn, UNIT_DTU, 0, BS_CLEANCHAN_STATE_IDLE);
}


static void stateCb_1chanSetup_enter(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	bs_mainDist_t *const mdIn = (bs_mainDist_t*)userVarIn;
	cxa_assert(mdIn);

	mdIn->cleanType = BS_MD_CLEANTYPE_1CHAN;

	// make sure the bus is off
	cxa_gpio_setValue(mdIn->sol_co2, 0);
	cxa_gpio_setValue(mdIn->sol_dosingPump, 0);
	cxa_gpio_setValue(mdIn->sol_h2o, 0);

	// turn the appropriate channel to bus
	setChannelState(mdIn, UNIT_CBU, mdIn->singleCleanChanIndex, BS_CLEANCHAN_STATE_BUS);
}


static void stateCb_fullSysSetupForward_enter(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	bs_mainDist_t *const mdIn = (bs_mainDist_t*)userVarIn;
	cxa_assert(mdIn);

	mdIn->fullSysCleanDir = BS_MD_FULLSYS_CLEANDIR_FORWARD;
	mdIn->cleanType = BS_MD_CLEANTYPE_FULLSYS;

	// make sure the bus is off
	cxa_gpio_setValue(mdIn->sol_co2, 0);
	cxa_gpio_setValue(mdIn->sol_dosingPump, 0);
	cxa_gpio_setValue(mdIn->sol_h2o, 0);

	// turn chan 0 to bus, chans 1-3 to drainAdj, dt to drainAdj
	setChannelState(mdIn, UNIT_CBU, 0, BS_CLEANCHAN_STATE_BUS);
	setChannelState(mdIn, UNIT_CBU, 1, BS_CLEANCHAN_STATE_DRAIN_ADJACENT);
	setChannelState(mdIn, UNIT_CBU, 2, BS_CLEANCHAN_STATE_DRAIN_ADJACENT);
	setChannelState(mdIn, UNIT_CBU, 3, BS_CLEANCHAN_STATE_DRAIN_ADJACENT);
	setChannelState(mdIn, UNIT_DTU, 0, BS_CLEANCHAN_STATE_DRAIN_ADJACENT);
}


static void stateCb_fullSysSetupReverse_enter(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	bs_mainDist_t *const mdIn = (bs_mainDist_t*)userVarIn;
	cxa_assert(mdIn);

	mdIn->fullSysCleanDir = BS_MD_FULLSYS_CLEANDIR_REVERSE;
	mdIn->cleanType = BS_MD_CLEANTYPE_FULLSYS;

	// make sure the bus is off
	cxa_gpio_setValue(mdIn->sol_co2, 0);
	cxa_gpio_setValue(mdIn->sol_dosingPump, 0);
	cxa_gpio_setValue(mdIn->sol_h2o, 0);

	// turn chan 3 to bus, chans 0-2 to drainAdj, dt to drainAdj
	setChannelState(mdIn, UNIT_CBU, 0, BS_CLEANCHAN_STATE_DRAIN_ADJACENT);
	setChannelState(mdIn, UNIT_CBU, 1, BS_CLEANCHAN_STATE_DRAIN_ADJACENT);
	setChannelState(mdIn, UNIT_CBU, 2, BS_CLEANCHAN_STATE_DRAIN_ADJACENT);
	setChannelState(mdIn, UNIT_CBU, 3, BS_CLEANCHAN_STATE_BUS);
	setChannelState(mdIn, UNIT_DTU, 0, BS_CLEANCHAN_STATE_IDLE);
}


static void stateCb_rinse_enter(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	bs_mainDist_t *const mdIn = (bs_mainDist_t*)userVarIn;
	cxa_assert(mdIn);

	cxa_gpio_setValue(mdIn->sol_co2, 0);
	cxa_gpio_setValue(mdIn->sol_dosingPump, 0);
	cxa_gpio_setValue(mdIn->sol_h2o, 1);
}


static void stateCb_dose_enter(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	bs_mainDist_t *const mdIn = (bs_mainDist_t*)userVarIn;
	cxa_assert(mdIn);

	cxa_gpio_setValue(mdIn->sol_co2, 0);
	cxa_gpio_setValue(mdIn->sol_dosingPump, 1);
	cxa_gpio_setValue(mdIn->sol_h2o, 1);
}


static void stateCb_purge_enter(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	bs_mainDist_t *const mdIn = (bs_mainDist_t*)userVarIn;
	cxa_assert(mdIn);

	cxa_gpio_setValue(mdIn->sol_co2, 1);
	cxa_gpio_setValue(mdIn->sol_dosingPump, 0);
	cxa_gpio_setValue(mdIn->sol_h2o, 0);

	// so we know when to exit the purge state
	// (little more complex since we have a number of possible destination states)
	cxa_timeDiff_setStartTime_now(&mdIn->td_transition);
}


static void stateCb_purge_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	bs_mainDist_t *const mdIn = (bs_mainDist_t*)userVarIn;
	cxa_assert(mdIn);

	if( cxa_timeDiff_isElapsed_ms(&mdIn->td_transition, PURGE_TIME_MS) )
	{
		if( mdIn->cleanType == BS_MD_CLEANTYPE_1CHAN )
		{
			cxa_stateMachine_transition(&mdIn->stateMachine, BS_MD_STATE_IDLE);
			return;
		}

		// must be doing a full-system clean...see if we need to reverse
		if( mdIn->fullSysCleanDir == BS_MD_FULLSYS_CLEANDIR_FORWARD )
		{
			cxa_stateMachine_transition(&mdIn->stateMachine, BS_MD_STATE_FULLSYS_SETUP_REVERSE);
			return;
		}

		// if we made it here, we must be done
		cxa_stateMachine_transition(&mdIn->stateMachine, BS_MD_STATE_IDLE);
		return;
	}
}
