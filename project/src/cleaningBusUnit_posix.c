/*
 * CleaningBusUnit.c
 *
 * Created: 5/25/2015 8:25:20 PM
 *  Author: Christopher Armenio
 */ 
#include <cxa_assert.h>
#include <cxa_posix_gpioConsole.h>
#include <cxa_posix_timeBase.h>
#include <cxa_timeDiff.h>
#include <cxa_delay.h>
#include <cxa_logger_implementation.h>
#include <bs_cleaningChannel.h>
#include <cxa_rpc_node.h>
#include <cxa_rpc_nodeRemote.h>
#include <cxa_ioStream_loopback.h>
#include <cxa_backgroundUpdater.h>


// ******** local macro definitions ********
#define SERIAL_NUM		"55b84724-5bbd-4426-80e8-681eca09633b"


// ******** local function prototoypes ********
void sysInit(void);


// ******** local variable declarations ********
static cxa_timeBase_t timeBase_genPurp;

static cxa_posix_gpioConsole_t led_run;
static cxa_posix_gpioConsole_t led_error;

static cxa_posix_gpioConsole_t sol_tap0;
static cxa_posix_gpioConsole_t sol_tap0_byp;
static cxa_posix_gpioConsole_t sol_tap1;
static cxa_posix_gpioConsole_t sol_tap1_byp;
static cxa_posix_gpioConsole_t sol_tap2;
static cxa_posix_gpioConsole_t sol_tap2_byp;
static cxa_posix_gpioConsole_t sol_tap3;
static cxa_posix_gpioConsole_t sol_tap3_byp;

static cxa_ioStream_loopback_t ioStreamInput;
static cxa_rpc_nodeRemote_t nr_main;
static cxa_rpc_node_t node_localRoot;
static bs_cleaningChannel_t chan0;
static bs_cleaningChannel_t chan1;
static bs_cleaningChannel_t chan2;
static bs_cleaningChannel_t chan3;


// ******** global function implementations ********
int main(void)
{
	sysInit();
	
	while(1)
	{
		// manually update each of our channels/nodes
		cxa_rpc_nodeRemote_update(&nr_main);

		bs_cleaningChannel_update(&chan0);
		bs_cleaningChannel_update(&chan1);
		bs_cleaningChannel_update(&chan2);
		bs_cleaningChannel_update(&chan3);

		cxa_backgroundUpdater_update();
	}
}


// ******** local function implementations ********
void sysInit()
{
	// setup our assert system
	cxa_posix_gpioConsole_init_output(&led_error, "ledError", 0);
	cxa_assert_setAssertGpio(&led_error.super);
	cxa_assert_setFileDescriptor(stdout);
	cxa_logger_setGlobalFd(stdout);
	
	printf("<boot>" CXA_LINE_ENDING);
	
	// setup our timing-related systems
	cxa_posix_timeBase_init(&timeBase_genPurp);
	cxa_backgroundUpdater_init();
	
	// now setup our application-specific components
	cxa_posix_gpioConsole_init_output(&led_run, "ledRun", 0);
	cxa_posix_gpioConsole_init_output(&sol_tap0, "sol_tap0", 0);
	cxa_posix_gpioConsole_init_output(&sol_tap0_byp, "sol_tap0_byp", 0);
	cxa_posix_gpioConsole_init_output(&sol_tap1, "sol_tap1", 0);
	cxa_posix_gpioConsole_init_output(&sol_tap1_byp, "sol_tap1_byp", 0);
	cxa_posix_gpioConsole_init_output(&sol_tap2, "sol_tap2", 0);
	cxa_posix_gpioConsole_init_output(&sol_tap2_byp, "sol_tap2_byp", 0);
	cxa_posix_gpioConsole_init_output(&sol_tap3, "sol_tap3", 0);
	cxa_posix_gpioConsole_init_output(&sol_tap3_byp, "sol_tap3_byp", 0);

	// setup our loopback ioStream for testing
	cxa_ioStream_loopback_init(&ioStreamInput);

	// setup our local root node and its downstream component
	cxa_rpc_node_init(&node_localRoot, &timeBase_genPurp, SERIAL_NUM);
	cxa_assert(cxa_rpc_nodeRemote_init_downstream(&nr_main, &ioStreamInput.endPoint1, &node_localRoot));

	// now each of our cleaning channels
	cxa_assert( bs_cleaningChannel_init(&chan0, 0, &sol_tap0.super, &sol_tap0_byp.super, &timeBase_genPurp) );
	cxa_assert( bs_cleaningChannel_init(&chan1, 1, &sol_tap1.super, &sol_tap1_byp.super, &timeBase_genPurp) );
	cxa_assert( bs_cleaningChannel_init(&chan2, 2, &sol_tap2.super, &sol_tap2_byp.super, &timeBase_genPurp) );
	cxa_assert( bs_cleaningChannel_init(&chan3, 3, &sol_tap3.super, &sol_tap3_byp.super, &timeBase_genPurp) );

	// add them to our local root
	cxa_assert( cxa_rpc_node_addSubNode(&node_localRoot, bs_cleaningChannel_getRpcNode(&chan0)) );
	cxa_assert( cxa_rpc_node_addSubNode(&node_localRoot, bs_cleaningChannel_getRpcNode(&chan1)) );
	cxa_assert( cxa_rpc_node_addSubNode(&node_localRoot, bs_cleaningChannel_getRpcNode(&chan2)) );
	cxa_assert( cxa_rpc_node_addSubNode(&node_localRoot, bs_cleaningChannel_getRpcNode(&chan3)) );
}
