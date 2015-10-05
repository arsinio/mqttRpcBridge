/*
 * CleaningBusUnit.c
 *
 * Created: 5/25/2015 8:25:20 PM
 *  Author: Christopher Armenio
 */ 
#include <mraa.h>

#include <cxa_assert.h>
#include <cxa_posix_timeBase.h>
#include <cxa_timeDiff.h>
#include <cxa_delay.h>
#include <cxa_logger_implementation.h>
#include <cxa_ioStream_fromFile.h>
#include <cxa_backgroundUpdater.h>
#include <cxa_libmraa_gpio.h>
#include <cxa_libmraa_usart.h>

#include <cxa_commandLineParser.h>
#include <bs_mainDist.h>


// ******** local macro definitions ********


// ******** local function prototoypes ********
static void sysInit(void);
static void clp_idle(cxa_commandLineParser_t *const clpIn, void * userVarIn);
static void clp_fullSystem(cxa_commandLineParser_t *const clpIn, void * userVarIn);
static void clp_chanClean_0(cxa_commandLineParser_t *const clpIn, void * userVarIn);
static void clp_chanClean_1(cxa_commandLineParser_t *const clpIn, void * userVarIn);
static void clp_chanClean_2(cxa_commandLineParser_t *const clpIn, void * userVarIn);
static void clp_chanClean_3(cxa_commandLineParser_t *const clpIn, void * userVarIn);


// ******** local variable declarations ********
static cxa_timeBase_t timeBase_genPurp;

static cxa_libmraa_gpio_t sol_co2;
static cxa_libmraa_gpio_t sol_dosing;
static cxa_libmraa_gpio_t sol_h2o;

static cxa_libmraa_gpio_t led_cloud;
static cxa_libmraa_gpio_t led_run;
static cxa_libmraa_gpio_t led_error;
static cxa_libmraa_gpio_t led_config;

static cxa_libmraa_usart_t usart_rs485;
static cxa_libmraa_gpio_t gpio_rs485_txen;

static bs_mainDist_t mainDist;

static cxa_commandLineParser_t clp;

static cxa_ioStream_fromFile_t ios_debug;


// ******** global function implementations ********
int main( int argc, char* argv[] )
{
	cxa_commandLineParser_init(&clp, "BeerSmart Demo", "Demo of BeerSmart prototype");
	cxa_commandLineParser_addOption_noArg(&clp, "i", "idle", "idle the system", false, clp_idle, NULL);
	cxa_commandLineParser_addOption_noArg(&clp, "fs", "fullSystem", "full system clean", false, clp_fullSystem, NULL);
	cxa_commandLineParser_addOption_noArg(&clp, "sc0", "singleChan_0", "single channel clean 0", false, clp_chanClean_0, NULL);
	cxa_commandLineParser_addOption_noArg(&clp, "sc1", "singleChan_1", "single channel clean 1", false, clp_chanClean_1, NULL);
	cxa_commandLineParser_addOption_noArg(&clp, "sc2", "singleChan_2", "single channel clean 2", false, clp_chanClean_2, NULL);
	cxa_commandLineParser_addOption_noArg(&clp, "sc3", "singleChan_3", "single channel clean 3", false, clp_chanClean_3, NULL);

	cxa_commandLineParser_parseOptions(&clp, argc, argv);
}


// ******** local function implementations ********
static void sysInit()
{
	// setup our assert system
	cxa_libmraa_gpio_init_output(&led_error, 25, false);
	cxa_assert_setAssertGpio(&led_error.super);

	// now setup our debug serial console
	cxa_ioStream_fromFile_init(&ios_debug, stdout);
	cxa_assert_setIoStream(&ios_debug.super);
	
	// setup our timing-related systems
	cxa_posix_timeBase_init(&timeBase_genPurp);
	cxa_backgroundUpdater_init();
	
	// setup our logger
	cxa_logger_setGlobalTimeBase(&timeBase_genPurp);
	cxa_logger_setGlobalIoStream(&ios_debug.super);

	// now setup our application-specific components
	cxa_libmraa_gpio_init_output(&led_run, 8, false);
	cxa_libmraa_gpio_init_output(&led_cloud, 6, false);
	cxa_libmraa_gpio_init_output(&led_config, 13, false);
	cxa_libmraa_gpio_init_output(&sol_co2, 54, false);
	cxa_libmraa_gpio_init_output(&sol_dosing, 40, false);
	cxa_libmraa_gpio_init_output(&sol_h2o, 41, false);

	cxa_libmraa_gpio_init_output(&gpio_rs485_txen, 36, false);
	cxa_assert(cxa_libmraa_usart_init_noHH(&usart_rs485, 0, 9600));
	cxa_gpio_setValue(&gpio_rs485_txen.super, true);

	cxa_assert( bs_mainDist_init(&mainDist, &timeBase_genPurp, cxa_usart_getIoStream(&usart_rs485.super),
								 &sol_co2.super, &sol_dosing.super, &sol_h2o.super,
								 &led_cloud.super, &led_run.super, &led_config.super, &led_error.super) );
}


static void clp_idle(cxa_commandLineParser_t *const clpIn, void * userVarIn)
{
	mraa_init();
	sysInit();

	bs_mainDist_update(&mainDist);
}


static void clp_fullSystem(cxa_commandLineParser_t *const clpIn, void * userVarIn)
{
	mraa_init();
	sysInit();

	bs_mainDist_startSystemClean(&mainDist);
	bs_mainDist_update(&mainDist);

	while( !bs_mainDist_isIdle(&mainDist) )
	{
		bs_mainDist_update(&mainDist);
	}
}


static void clp_chanClean_0(cxa_commandLineParser_t *const clpIn, void * userVarIn)
{
	mraa_init();
	sysInit();

	bs_mainDist_startChannelClean(&mainDist, 0);
	bs_mainDist_update(&mainDist);

	while( !bs_mainDist_isIdle(&mainDist) )
	{
		bs_mainDist_update(&mainDist);
	}
}


static void clp_chanClean_1(cxa_commandLineParser_t *const clpIn, void * userVarIn)
{
	mraa_init();
	sysInit();

	bs_mainDist_startChannelClean(&mainDist, 1);
	bs_mainDist_update(&mainDist);

	while( !bs_mainDist_isIdle(&mainDist) )
	{
		bs_mainDist_update(&mainDist);
	}
}


static void clp_chanClean_2(cxa_commandLineParser_t *const clpIn, void * userVarIn)
{
	mraa_init();
	sysInit();

	bs_mainDist_startChannelClean(&mainDist, 2);
	bs_mainDist_update(&mainDist);

	while( !bs_mainDist_isIdle(&mainDist) )
	{
		bs_mainDist_update(&mainDist);
	}
}


static void clp_chanClean_3(cxa_commandLineParser_t *const clpIn, void * userVarIn)
{
	mraa_init();
	sysInit();

	bs_mainDist_startChannelClean(&mainDist, 3);
	bs_mainDist_update(&mainDist);

	while( !bs_mainDist_isIdle(&mainDist) )
	{
		bs_mainDist_update(&mainDist);
	}
}
