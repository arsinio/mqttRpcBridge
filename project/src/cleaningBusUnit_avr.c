/*
 * CleaningBusUnit.c
 *
 * Created: 5/25/2015 8:25:20 PM
 *  Author: Christopher Armenio
 */ 
#include <avr/io.h>
#include <cxa_assert.h>
#include <cxa_xmega_gpio.h>
#include <cxa_xmega_timeBase.h>
#include <cxa_xmega_timer32.h>
#include <cxa_timeDiff.h>
#include <cxa_xmega_clockController.h>
#include <cxa_delay.h>
#include <cxa_xmega_pmic.h>
#include <cxa_xmega_usart.h>
#include <cxa_xmega_ioStream_toFile.h>
#include <bs_cleaningChannel.h>
#include <cxa_logger_implementation.h>
#include <cxa_rpc_node.h>
#include <cxa_rpc_nodeRemote.h>
#include <cxa_ioStream_loopback.h>
#include <cxa_backgroundUpdater.h>
#include <cxa_rpc_messageFactory.h>


// ******** local macro definitions ********
#define SERIAL_NUM		"CBU"


// ******** local function prototoypes ********
void sysInit(void);


// ******** local variable declarations ********
static cxa_xmega_usart_t usart_debug;
static cxa_xmega_usart_t usart_rs485;

static cxa_xmega_timer32_t timer_timeBase;
static cxa_timeBase_t timeBase_genPurp;

static cxa_xmega_gpio_t led_run;
static cxa_xmega_gpio_t led_error;
static cxa_xmega_gpio_t gpio_rs485_txEn;

static cxa_xmega_gpio_t sol_tap0;
static cxa_xmega_gpio_t sol_tap0_byp;
static cxa_xmega_gpio_t sol_tap1;
static cxa_xmega_gpio_t sol_tap1_byp;
static cxa_xmega_gpio_t sol_tap2;
static cxa_xmega_gpio_t sol_tap2_byp;
static cxa_xmega_gpio_t sol_tap3;
static cxa_xmega_gpio_t sol_tap3_byp;

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
	
	cxa_timeDiff_t td_blink;
	cxa_timeDiff_init(&td_blink, &timeBase_genPurp, true);

	while(1)
	{
		if( cxa_timeDiff_isElaped_recurring_ms(&td_blink, 500) )
		{
			cxa_gpio_toggle(&led_run.super);
		}
		
		uint8_t rxChar;
		if( cxa_ioStream_readByte(cxa_usart_getIoStream(&usart_rs485.super), &rxChar) )
		{
			cxa_gpio_toggle(&led_error.super);
			fputc(rxChar, stdout);
		}
		
		/*
		// manually update each of our channels/nodes
		cxa_rpc_nodeRemote_update(&nr_main);

		bs_cleaningChannel_update(&chan0);
		bs_cleaningChannel_update(&chan1);
		bs_cleaningChannel_update(&chan2);
		bs_cleaningChannel_update(&chan3);

		cxa_backgroundUpdater_update();
		*/
	}
}


// ******** local function implementations ********
void sysInit()
{
	// make sure we at least have a GPIO for asserts
	cxa_xmega_gpio_init_output(&led_error, &PORTD, 5, CXA_GPIO_POLARITY_NONINVERTED, 0);
	cxa_assert_setAssertGpio(&led_error.super);
	
	cxa_xmega_pmic_init();
	//cxa_xmega_clockController_init(CXA_XMEGA_CC_INTOSC_32MHZ);
	cxa_xmega_clockController_init(CXA_XMEGA_CC_INTOSC_2MHZ);
	
	// now setup our debug serial console
	cxa_xmega_usart_init_noHH(&usart_debug, &USARTD0, 115200);
	stdin = stdout = stderr = cxa_xmega_ioStream_toFile(cxa_usart_getIoStream(&usart_debug.super));
	cxa_assert_setFileDescriptor(stdout);
	cxa_logger_setGlobalFd(stdout);
	printf("<boot>\r\n");
	
	// setup our timing-related systems
	cxa_xmega_timer32_init_freerun(&timer_timeBase, CXA_XMEGA_TIMER16_TCE0, CXA_XMEGA_TIMER16_TCE1, CXA_XMEGA_TIMER16_CLOCKSRC_PERCLK_DIV1024);
	cxa_xmega_timeBase_init_timer32(&timeBase_genPurp, &timer_timeBase);
	
	// setup our RS-485 port
	cxa_xmega_gpio_init_output(&gpio_rs485_txEn, &PORTF, 0, CXA_GPIO_POLARITY_NONINVERTED, 0);
	cxa_xmega_usart_init_noHH(&usart_rs485, &USARTF0, 9600);
	
	// now setup our application-specific components
	cxa_xmega_gpio_init_output(&led_run, &PORTD, 4, CXA_GPIO_POLARITY_NONINVERTED, 0);
	cxa_xmega_gpio_init_output(&sol_tap0, &PORTC, 0, CXA_GPIO_POLARITY_NONINVERTED, 0);
	cxa_xmega_gpio_init_output(&sol_tap0_byp, &PORTC, 1, CXA_GPIO_POLARITY_NONINVERTED, 0);
	cxa_xmega_gpio_init_output(&sol_tap1, &PORTC, 2, CXA_GPIO_POLARITY_NONINVERTED, 0);
	cxa_xmega_gpio_init_output(&sol_tap1_byp, &PORTC, 3, CXA_GPIO_POLARITY_NONINVERTED, 0);
	cxa_xmega_gpio_init_output(&sol_tap2, &PORTC, 4, CXA_GPIO_POLARITY_NONINVERTED, 0);
	cxa_xmega_gpio_init_output(&sol_tap2_byp, &PORTC, 5, CXA_GPIO_POLARITY_NONINVERTED, 0);
	cxa_xmega_gpio_init_output(&sol_tap3, &PORTC, 6, CXA_GPIO_POLARITY_NONINVERTED, 0);
	cxa_xmega_gpio_init_output(&sol_tap3_byp, &PORTC, 7, CXA_GPIO_POLARITY_NONINVERTED, 0);

	// setup our local root node and its downstream component
	cxa_rpc_node_init(&node_localRoot, &timeBase_genPurp, SERIAL_NUM);
	cxa_assert(cxa_rpc_nodeRemote_init_downstream(&nr_main, cxa_usart_getIoStream(&usart_rs485.super), &node_localRoot));

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
	
	// finally, enable interrupts
	cxa_xmega_pmic_enableInterruptLevel(CXA_XMEGA_PMIC_INTLEVEL_MED);
	cxa_xmega_pmic_enableGlobalInterrupts();
}