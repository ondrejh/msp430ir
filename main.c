//******************************************************************************
// msp430 ir receiver
//
// author: Ondrej Hejda
// date:   10.2.2013
//
// resources:
//
//  some of my other repos
//
//
// hardware: MSP430G2553 (launchpad)
//
//                MSP4302553
//             -----------------
//        /|\ |                 |
//         |  |           P1.1,2|--> UART (debug output 9.6kBaud)
//          --|RST              |
//            |             P1.0|--> RED LED (active high)
//            |                 |
//            |             P1.6|--> UP LED (active high)
//            |             P1.7|--> DOWN LED (active high)
//            |                 |
//            |             P1.3|<-- UP BUTTON (active low, inverted, internal pullup)
//            |             P1.4|<-- DOWN BUTTON (active low, inverted, internal pullup)
//            |                 |
//            |             P1.5|<-- IR receiver
//            |                 |
//
// alternatively the MSP430G2452(20pin) or MSP430G2201(20pin)
//    can be used (no uart)
//
//******************************************************************************

// include section
#include <msp430g2553.h>
//#include <msp430g2452.h>
//#include <msp430g2201.h>

#ifdef DEBUG
#include "uart.h"
#endif

#include "irdecode.h"

// board (leds, button)
#define LED_INIT() {P1DIR|=0x41;P1OUT&=~0x41;}
#define LED_RED_ON() {P1OUT|=0x01;}
#define LED_RED_OFF() {P1OUT&=~0x01;}
#define LED_RED_SWAP() {P1OUT^=0x01;}
#define LED_GREEN_ON() {P1OUT|=0x40;}
#define LED_GREEN_OFF() {P1OUT&=~0x40;}
#define LED_GREEN_SWAP() {P1OUT^=0x40;}

#define BTN_INIT() {P1SEL&=~0x18;P1DIR&=~0x18;P1OUT|=0x18;P1REN|=0x18;}
#define BTN_UP ((P1IN&0x08)!=0)
#define BTN_DOWN ((P1IN&0x10)!=0)

#define LED_UP_PIN BIT6
#define LED_DOWN_PIN BIT7
#define LED_CRANE_INIT() {P1DIR|=(LED_UP_PIN|LED_DOWN_PIN);P1OUT&=~(LED_UP_PIN|LED_DOWN_PIN);}
#define LED_UP_ON() {P1OUT|=LED_UP_PIN;}
#define LED_UP_OFF() {P1OUT&=~LED_UP_PIN;}
#define LED_DOWN_ON() {P1OUT|=LED_DOWN_PIN;}
#define LED_DOWN_OFF() {P1OUT&=~LED_DOWN_PIN;}

#define IR_CODE_UP 3
#define IR_CODE_DOWN 4
#define TIME_CODE_RELAYING 32
#define TIME_SWITCH_TIMEOUT 64

// global variables
bool wdt_timer_flag = false;
int8_t code=0;

// leds and dco init
void board_init(void)
{
	// oscillator
	BCSCTL1 = CALBC1_1MHZ;		// Set DCO
	DCOCTL = CALDCO_1MHZ;

    // leds
	LED_INIT();
	LED_CRANE_INIT();

	// buttons
	BTN_INIT();
}


// init timer (wdt used)
void wdt_timer_init(void)
{
    WDTCTL = WDT_MDLY_8;                     // Set Watchdog Timer interval to ~8ms
    IE1 |= WDTIE;                             // Enable WDT interrupt
}


// main program body
int main(void)
{
	WDTCTL = WDTPW + WDTHOLD;	// Stop WDT

    #ifdef DEBUG
	uart_init();  // init uart (communication)
	#endif

	board_init(); // init dco and leds

	irdecode_init(); // init irdecode module
	wdt_timer_init(); // init wdt timer (used for led blinking)

	while(1)
	{
        __bis_SR_register(CPUOFF + GIE); // enter sleep mode (leave interrupt event)
        if (is_ircode_present())
        {
            #ifdef DEBUG
            // debug mode prints whole received code on uart
            LED_GREEN_ON();
            ircode_uart_send();
            //uart_putint8(ircode_decode());
            //uart_puts("\r\n");
            ircode_mark_used();
            LED_GREEN_OFF();
            #else
            // normal mode tries to decode received code
            code = ircode_decode();
            /*if (code>=0)
            {
                led_timeout = LED_TIMEOUT_PRESET;
                LED_GREEN_ON();
            }*/
            ircode_mark_used();
            #endif
        }
        if (wdt_timer_flag==true)
        {
            // clear timer flag
            wdt_timer_flag=false;

            static int time_counter = 0;
            if (time_counter!=0) time_counter--;

            static int8_t crane_status = 0;
            switch (crane_status)
            {
                case 0: // wait button or code (or timeout)
                    if (time_counter==0)
                    {
                        if ((code==IR_CODE_UP)||BTN_UP)
                        {
                            if (code==IR_CODE_UP) time_counter = TIME_CODE_RELAYING;
                            LED_DOWN_OFF();
                            LED_UP_ON();
                            crane_status = 1;
                        }
                        else if ((code==IR_CODE_DOWN)||BTN_DOWN)
                        {
                            if (code==IR_CODE_DOWN) time_counter = TIME_CODE_RELAYING;
                            LED_UP_OFF();
                            LED_DOWN_ON();
                            crane_status = 2;
                        }
                        else
                        {
                            LED_DOWN_OFF();
                            LED_UP_OFF();
                        }
                    }
                    break;
                case 1: // up
                    if (code==IR_CODE_UP) time_counter = TIME_CODE_RELAYING;
                    if ((time_counter==0)&&(!BTN_UP))
                    {
                        LED_DOWN_OFF();
                        LED_UP_OFF();
                        time_counter = TIME_SWITCH_TIMEOUT;
                        crane_status = 0;
                    }
                    break;
                case 2: // down
                    if (code==IR_CODE_DOWN) time_counter = TIME_CODE_RELAYING;
                    if ((time_counter==0)&&(!BTN_DOWN))
                    {
                        LED_DOWN_OFF();
                        LED_UP_OFF();
                        time_counter = TIME_SWITCH_TIMEOUT;
                        crane_status = 0;
                    }
                    break;
                default: // what the hell ?!?
                    LED_DOWN_OFF();
                    LED_UP_OFF();
                    time_counter = TIME_SWITCH_TIMEOUT;
                    crane_status = 0;
                    break;
            }

            code = 0; // clear code
        }
	}

	return -1;
}

// Watchdog Timer interrupt service routine
#pragma vector=WDT_VECTOR
__interrupt void watchdog_timer(void)
{
    /*if (led_timeout>0)
    {
        led_timeout--;
        if (led_timeout==0) LED_GREEN_OFF();
    }*/

    wdt_timer_flag = true;
    __bic_SR_register_on_exit(CPUOFF);  // Clear CPUOFF bit from 0(SR)
}
