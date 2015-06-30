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
#include "bs_cleaningChannel.h"


/**
 * @author Christopher Armenio
 */


// ******** includes ********
#include <stdio.h>
#include <string.h>
#include <cxa_assert.h>
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define NAME_PREFIX					"cleanChan"


// ******** local type definitions ********


// ******** local function prototypes ********
static cxa_rpc_method_retVal_t rpc_methodCb_setState(cxa_linkedField_t *const paramsIn, cxa_linkedField_t *const responseParamsIn, void *userVarIn);

static void stateCb_idle_enter(cxa_stateMachine_t *const smIn, void *userVarIn);
static void stateCb_bus_enter(cxa_stateMachine_t *const smIn, void *userVarIn);
static void stateCb_drainAdj_enter(cxa_stateMachine_t *const smIn, void *userVarIn);


// ********  local variable declarations *********


// ******** global function implementations ********
bool bs_cleaningChannel_init(bs_cleaningChannel_t *const ccIn, const uint8_t indexIn, cxa_gpio_t *const sol_busIn, cxa_gpio_t *const sol_bypassIn, cxa_timeBase_t *const tbIn)
{
	cxa_assert(ccIn);

	// setup our internal state
	ccIn->sol_bus = sol_busIn;
	ccIn->sol_bypass = sol_bypassIn;

	// setup our RPC node
	cxa_rpc_node_init(&ccIn->rpcNode, tbIn, "%s_%d", NAME_PREFIX, indexIn);
	if( !cxa_rpc_node_addMethod(&ccIn->rpcNode, "setState", rpc_methodCb_setState, (void*)ccIn) ) return false;

	// setup our internal state machine
	cxa_stateMachine_init(&ccIn->stateMachine, cxa_rpc_node_getName(&ccIn->rpcNode));
	cxa_stateMachine_addState(&ccIn->stateMachine, BS_CLEANCHAN_STATE_IDLE, "idle", stateCb_idle_enter, NULL, NULL, (void*)ccIn);
	cxa_stateMachine_addState(&ccIn->stateMachine, BS_CLEANCHAN_STATE_BUS, "bus", stateCb_bus_enter, NULL, NULL, (void*)ccIn);
	cxa_stateMachine_addState(&ccIn->stateMachine, BS_CLEANCHAN_STATE_DRAIN_ADJACENT, "drainAdj", stateCb_drainAdj_enter, NULL, NULL, (void*)ccIn);
	cxa_stateMachine_transition(&ccIn->stateMachine, BS_CLEANCHAN_STATE_IDLE);
	cxa_stateMachine_update(&ccIn->stateMachine);

	return true;
}


cxa_rpc_node_t* bs_cleaningChannel_getRpcNode(bs_cleaningChannel_t *const ccIn)
{
	cxa_assert(ccIn);

	return &ccIn->rpcNode;
}


void bs_cleaningChannel_setState(bs_cleaningChannel_t *const ccIn, bs_cleaningChannel_state_t newStateIn)
{
	cxa_assert(ccIn);

	cxa_stateMachine_transition(&ccIn->stateMachine, newStateIn);
}


bs_cleaningChannel_state_t bs_cleaningChannel_getState(bs_cleaningChannel_t *const ccIn)
{
	cxa_assert(ccIn);

	return cxa_stateMachine_getCurrentState(&ccIn->stateMachine);
}


void bs_cleaningChannel_update(bs_cleaningChannel_t *const ccIn)
{
	cxa_assert(ccIn);

	cxa_stateMachine_update(&ccIn->stateMachine);
}


// ******** local function implementations ********
static cxa_rpc_method_retVal_t rpc_methodCb_setState(cxa_linkedField_t *const paramsIn, cxa_linkedField_t *const responseParamsIn, void *userVarIn)
{
	bs_cleaningChannel_t* ccIn = (bs_cleaningChannel_t*)userVarIn;
	cxa_assert(ccIn);

	uint8_t newState_raw;
	if( !cxa_linkedField_get_uint8(paramsIn, 0, newState_raw) ) return CXA_RPC_METHOD_RETVAL_FAIL_INVALID_PARAMS;

	// validate our requested state
	bs_cleaningChannel_state_t newState = newState_raw;
	if( (newState != BS_CLEANCHAN_STATE_IDLE) || (newState != BS_CLEANCHAN_STATE_BUS) || (newState != BS_CLEANCHAN_STATE_DRAIN_ADJACENT) ) return CXA_RPC_METHOD_RETVAL_FAIL_INVALID_PARAMS;

	cxa_stateMachine_transition(&ccIn->stateMachine, newState);
	return CXA_RPC_METHOD_RETVAL_SUCCESS;
}


static void stateCb_idle_enter(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	bs_cleaningChannel_t* ccIn = (bs_cleaningChannel_t*)userVarIn;
	cxa_assert(ccIn);

	cxa_gpio_setValue(ccIn->sol_bus, 0);
	cxa_gpio_setValue(ccIn->sol_bypass, 0);
}


static void stateCb_bus_enter(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	bs_cleaningChannel_t* ccIn = (bs_cleaningChannel_t*)userVarIn;
	cxa_assert(ccIn);

	// always set 0 first
	cxa_gpio_setValue(ccIn->sol_bypass, 0);
	cxa_gpio_setValue(ccIn->sol_bus, 1);
}


static void stateCb_drainAdj_enter(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	bs_cleaningChannel_t* ccIn = (bs_cleaningChannel_t*)userVarIn;
	cxa_assert(ccIn);

	// always set 0 first
	cxa_gpio_setValue(ccIn->sol_bus, 0);
	cxa_gpio_setValue(ccIn->sol_bypass, 1);
}
