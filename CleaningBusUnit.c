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


// ******** local function prototoypes ********
void sysInit(void);


// ******** local variable declarations ********
static cxa_xmega_usart_t usart_debug;
static cxa_xmega_usart_t usart_rs485;

static cxa_xmega_timer32_t timer_timeBase;
static cxa_xmega_timeBase_t timeBase_genPurp;

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


// ******** global function implementations ********
int main(void)
{
	sysInit();
	
	//FILE* fd_rs485 = cxa_usart_getFileDescriptor(&usart_debug.super);
	FILE* fd_rs485 = cxa_usart_getFileDescriptor(&usart_rs485.super);
	while(1)
	{
		cxa_gpio_toggle(&led_run.super);
		cxa_delay_ms(100);
		
		int foo = fgetc(fd_rs485);
		if( foo != EOF ) fputc(foo, stdout);
		
		foo = fgetc(stdin);
		if( foo != EOF ) fputc(foo, fd_rs485);
		
		fflush(stdout);
	}
}


// ******** local function implementations ********
void sysInit()
{
	// make sure we at least have a GPIO for asserts
	cxa_xmega_gpio_init_output(&led_error, &PORTD, 5, CXA_GPIO_POLARITY_NONINVERTED, 0);
	cxa_assert_setAssertGpio(&led_error.super);
	
	// setup our base system functions
	cxa_xmega_pmic_init();
	cxa_xmega_clockController_init(CXA_XMEGA_CC_INTOSC_32MHZ);
	
	// now setup our debug serial console
	cxa_xmega_usart_init_noHH(&usart_debug, &USARTD0, 115200);
	stdin = stdout = stderr = cxa_usart_getFileDescriptor(&usart_debug.super);
	printf("<boot>\r\n");
	
	// setup our timing-related systems
	cxa_xmega_timer32_init_freerun(&timer_timeBase, CXA_XMEGA_TIMER16_TCE0, CXA_XMEGA_TIMER16_TCE1, CXA_XMEGA_TIMER16_CLOCKSRC_PERCLK_DIV1024);
	cxa_xmega_timeBase_init_timer32(&timeBase_genPurp, &timer_timeBase);
	
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
	
	cxa_xmega_gpio_init_output(&gpio_rs485_txEn, &PORTF, 0, CXA_GPIO_POLARITY_NONINVERTED, 0);
	cxa_xmega_usart_init_noHH(&usart_rs485, &USARTF0, 9600);
	
	// finally, enable interrupts
	cxa_xmega_pmic_enableInterruptLevel(CXA_XMEGA_PMIC_INTLEVEL_MED);
	cxa_xmega_pmic_enableGlobalInterrupts();
}
