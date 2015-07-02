/*
 * CleaningBusUnit.c
 *
 * Created: 5/25/2015 8:25:20 PM
 *  Author: Christopher Armenio
 */ 
#include <cxa_assert.h>
#include <cxa_posix_timeBase.h>
#include <cxa_timeDiff.h>
#include <cxa_delay.h>
#include <cxa_logger_implementation.h>
#include <cxa_backgroundUpdater.h>
#include <bs_mainDist.h>

#include <cxa_posix_gpioConsole.h>
#include <cxa_posix_usart.h>


// ******** local macro definitions ********


// ******** local function prototoypes ********
static void sysInit(void);


// ******** local variable declarations ********
static cxa_timeBase_t timeBase_genPurp;

static cxa_posix_gpioConsole_t sol_co2;
static cxa_posix_gpioConsole_t sol_dosing;
static cxa_posix_gpioConsole_t sol_h2o;

static cxa_posix_gpioConsole_t led_cloud;
static cxa_posix_gpioConsole_t led_run;
static cxa_posix_gpioConsole_t led_error;
static cxa_posix_gpioConsole_t led_config;

static cxa_posix_usart_t usart_rs485;
static bs_mainDist_t mainDist;


// ******** global function implementations ********
int main(void)
{
	sysInit();
	
	cxa_timeDiff_t td_start;
	cxa_timeDiff_init(&td_start, &timeBase_genPurp, true);

	bool didStart = false;
	while(1)
	{
		if( !didStart && cxa_timeDiff_isElapsed_ms(&td_start, 10000) )
		{
			didStart = true;
			bs_mainDist_startSystemClean(&mainDist);
		}

		bs_mainDist_update(&mainDist);
	}
}


// ******** local function implementations ********
static void sysInit()
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
	cxa_posix_gpioConsole_init_output(&led_cloud, "ledCloud", 0);
	cxa_posix_gpioConsole_init_output(&led_config, "ledConfig", 0);

	//cxa_posix_usart_init_noHH(&usart_rs485, "/dev/null", 9600);

	cxa_assert( bs_mainDist_init(&mainDist, &timeBase_genPurp, cxa_usart_getIoStream(&usart_rs485.super),
								 &sol_co2.super, &sol_dosing.super, &sol_h2o.super,
								 &led_cloud.super, &led_run.super, &led_config.super, &led_error.super) );
}
