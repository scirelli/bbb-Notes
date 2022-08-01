/* //////////////////////////////////////
//	blinkExternalLED.pru0.c
//	Blinks one LED wired to P9_14 by writing to memory using the PRU
//	Wiring:	P9_14 connects to the plus lead of an LED.  The negative lead of the
//			LED goes to a 220 Ohm resistor.  The other lead of the resistor goes
//			to ground.
//	Setup:	Set the direction to out on P9_14
//	See:	prugpio.h to see to which ports the P8 and P9 pins are attached
//	PRU:	pru0
//

//  Refernce:
//  * http://cdn.sparkfun.com/datasheets/Components/LED/WS2812.pdf or https://cdn-shop.adafruit.com/datasheets/WS2812.pdf
//  * https://cdn-shop.adafruit.com/datasheets/WS2812B.pdf
WS2812 and WS2812B have slightly different times. Other versions do as well.

Data Transfer time (TH+TL=1.25μs +-600ns) (WS2812)
┏━━━━━━━━━━━━━━━━━━━━┯━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┯━━━━━━━━━━━━━━━━━━━━┯━━━━━━━━━━━━━━━━━━━━┓
┃  T0H               │ 0 code, high voltage time         │ 0.35μs             │ +-150ns            ┃
┠────────────────────┼───────────────────────────────────┼────────────────────┼────────────────────┨
┃  T1H               │ 1 code, high voltage time         │ 0.7μs              │ +-150ns            ┃
┠────────────────────┼───────────────────────────────────┼────────────────────┼────────────────────┨
┃  T0L               │ 0 code, low voltage time          │ 0.8μs              │ +-150ns            ┃
┠────────────────────┼───────────────────────────────────┼────────────────────┼────────────────────┨
┃  T0L               │ 1 code, low voltage time          │ 0.6μs              │ +-150ns            ┃
┠────────────────────┼───────────────────────────────────┼────────────────────┼────────────────────┨
┃  RES               │ low voltage time                  │ Above 50μs         │                    ┃
┗━━━━━━━━━━━━━━━━━━━━┷━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┷━━━━━━━━━━━━━━━━━━━━┷━━━━━━━━━━━━━━━━━━━━┛

Data Transfer time (TH+TL=1.25μs +-600ns) (WS2812B)
┏━━━━━━━━━━━━━━━━━━━━┯━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┯━━━━━━━━━━━━━━━━━━━━┯━━━━━━━━━━━━━━━━━━━━┓
┃  T0H               │ 0 code, high voltage time         │ 0.4μs              │ +-150ns            ┃
┠────────────────────┼───────────────────────────────────┼────────────────────┼────────────────────┨
┃  T1H               │ 1 code, high voltage time         │ 0.8μs              │ +-150ns            ┃
┠────────────────────┼───────────────────────────────────┼────────────────────┼────────────────────┨
┃  T0L               │ 0 code, low voltage time          │ 0.85μs             │ +-150ns            ┃
┠────────────────────┼───────────────────────────────────┼────────────────────┼────────────────────┨
┃  T0L               │ 1 code, low voltage time          │ 0.45μs             │ +-150ns            ┃
┠────────────────────┼───────────────────────────────────┼────────────────────┼────────────────────┨
┃  RES               │ low voltage time                  │ Above 50μs         │                    ┃
┗━━━━━━━━━━━━━━━━━━━━┷━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┷━━━━━━━━━━━━━━━━━━━━┷━━━━━━━━━━━━━━━━━━━━┛

Sequence Chart
         ┏━━━━━━━━━┓     T0L    ┃
         ┃<------->┃<---------->┃
0 code   ┃   T0H   ┗━━━━━━━━━━━━┛


         ┏━━━━━━━━━━━━┓   T1L   ┃
         ┃<---------->┃<------->┃
1 code   ┃   T1H      ┗━━━━━━━━━┛


         ┃       Treset         ┃
RET code ┃<-------------------->┃
         ┗━━━━━━━━━━━━━━━━━━━━━━┛

Cascade method:

D1  ┏━━━━━━━━━━┓ D2   ┏━━━━━━━━━━┓ D3   ┏━━━━━━━━━━┓ D4
--->┃DIN     D0┃----->┃DIN     D0┃----->┃DIN     D0┃----->
    ┃   PIX1   ┃      ┃   PIX2   ┃      ┃   PIX3   ┃
    ┗━━━━━━━━━━┛      ┗━━━━━━━━━━┛      ┗━━━━━━━━━━┛

Data Transmission method:
                                            reset code
                                             >= 50μs
    │                                 ---->│          │<----                                       │
    │<-- Data refresh cycle 1 ------------>│          │<----------- Data refresh cycle 2 --------->│
    ├────────────┬────────────┬────────────┤          ├────────────┬────────────┬────────────┐     │
    │ 1st 24bits │ 2nd 24bits │ 3rd 24bits │          │ 1st 24bits │ 2nd 24bits │ 3rd 24bits │     │
D1 ─┼────────────┴────────────┴────────────┼──────────┼────────────┴────────────┴────────────┴─────┼──
    │                                      │          │                                            │
    │            ┌────────────┬────────────┤          │            ┌────────────┬────────────┐     │
    │            │ 2nd 24bits │ 3rd 24bits │          │            │ 2nd 24bits │ 3rd 24bits │     │
D2 ─┼────────────┴────────────┴────────────┼──────────┼────────────┴────────────┴────────────┴─────┼──
    │                                      │          │                                            │
    │                         ┌────────────┤          │                         ┌────────────┐     │
    │                         │ 3rd 24bits │          │                         │ 3rd 24bits │     │
D3 ─┼─────────────────────────┴────────────┼──────────┼─────────────────────────┴────────────┴─────┼──
    │                                      │          │                                            │
    │                                      │          │                                            │
    │                                      │          │                                            │
D4 ─┼──────────────────────────────────────┼──────────┼────────────────────────────────────────────┼──

Note: The data of D1 is send by the MCU. D2, D3, and D4 through pixel internal reshaping amplification to
transmit.

Composition of 24bit data:
┏━━━━┯━━━━┯━━━━┯━━━━┯━━━━┯━━━━┯━━━━┯━━━━┯━━━━┯━━━━┯━━━━┯━━━━┯━━━━┯━━━━┯━━━━┯━━━━┯━━━━┯━━━━┯━━━━┯━━━━┯━━━━┯━━━━┯━━━━┯━━━━┓
┃ G7 │ G6 │ G5 │ G4 │ G3 │ G2 │ G1 │ G0 │ R7 │ R6 │ R5 │ R4 │ R3 │ R2 │ R1 │ R0 │ B7 │ B6 │ B5 │ B4 │ B3 │ B2 │ B1 │ B0 ┃
┗━━━━┷━━━━┷━━━━┷━━━━┷━━━━┷━━━━┷━━━━┷━━━━┷━━━━┷━━━━┷━━━━┷━━━━┷━━━━┷━━━━┷━━━━┷━━━━┷━━━━┷━━━━┷━━━━┷━━━━┷━━━━┷━━━━┷━━━━┷━━━━┛
Note: Follow the order of GRB to sent data and the high bit sent at first. Big-endian
////////////////////////////////////// */
#include <stdint.h>
#include <pru_cfg.h>
#include "resource_table_empty.h"
#include "prugpio.h"

#define WS2812 1

/*
PRU runs at 200Mhz
200,000,000 cycles
------------------
       1s
*/
#define CPU_FRQ 200000000
#define SEC_PER_CYCLE (1 / CPU_FRQ)  // ~5e9 s/cycle
#define NANO_SEC_PER_SEC = 1e9
#define NANO_SEC_PER_CYCLE = ((1 / CPU_FRQ) * NANO_SEC_PER_SEC) // ~5 ns/cycle

#ifdef WS2812
#define T0H_NS (0.35/*μs*/ * 1000 /*ns/μs*/)
#define T0L_NS (0.8 * 1000)
#define T1H_NS (0.7 * 1000)
#define T1L_NS (0.6 * 1000)
#define RES_NS (60 * 1000 ) // Docs say anything above 50μs use 60μs
#endif

#ifdef WS2812B
#define T0H_NS (0.4/*μs*/ * 1000 /*ns/μs*/)
#define T0L_NS (0.85 * 1000)
#define T1H_NS (0.8 * 1000)
#define T1L_NS (0.45 * 1000)
#define RES_NS (60 * 1000 ) // Docs say anything above 50μs use 60μs
#endif

//Will use __delay_cycles since the PRU will just be dedicated to talking to the WS2812x.
#define T0H_CYC (T0H_NS / NANO_SEC_PER_CYCLE)
#define T0L_CYC (T0L_NS / NANO_SEC_PER_CYCLE)
#define T1H_CYC (T1H_NS / NANO_SEC_PER_CYCLE)
#define T1L_CYC (T1L_NS / NANO_SEC_PER_CYCLE)


#define LED_COUNT 8
#define BITS_PER_LED 24
#define TOTAL_BITS_IN_STRAND (LED_COUNT * BITS_PER_LED)

// Writes to R30 control the General Purpose Output pins, and reads allow the user to determine the current state of those pins
// R31 is used to read General Purpose Input pins as well as the status of the two PRU host interrupts (bits 30 and 31)
// Writes to R31 are used to generate interrupts - see the device-specific TRM for more information on how these work.
/*
    5.7.2 Global Register Variables https://www.ti.com/lit/ug/spruhv7c/spruhv7c.pdf?ts=1658240188236
        The C/C++ compiler extends the C language by adding a special convention to the register storage class
        specifier to allow the allocation of global registers. This special global declaration has the form:
        register type regid

        The regid parameter can be __R30 and __R31. The identifiers _ _R30 and _ _R31 are each bound to
        their corresponding register R30 and R31, respectively. These are control registers and should always be
        declared as volatile. For example:

            volatile register unsigned int __R31;
            __R31 |= 1;

    Edit /boot/uEnv.txt make sure these lines are uncommented. Restart of needed.
        disable_uboot_overlay_video=1
        disable_uboot_overlay_audio=1

    Then run:
        $ config-pin P9_31 pruout
*/
volatile register unsigned int __R30; //Output
volatile register unsigned int __R31; //Input
// ---------------------------------------------------------------------------

void turnOffAllLEDS(void);

void main(void) {
	// Points to the GPIO port that is used
	uint32_t *gpio1 = (uint32_t *)GPIO1; //Memory map used as GPIO1 Registers (0x4804_C000 - 0x4804_CFFF) 4KB; Table 2-3 L4_PER Peripheral Memory Map

    unsigned int i;
    unsigned int leds;
    unsigned int x;


	/* Clear SYSCFG[STANDBY_INIT] to enable OCP master port */
	CT_CFG.SYSCFG_bit.STANDBY_INIT = 0;

    turnOffAllLEDS();

	leds = LED_COUNT;

	while(x<19000) {
		for(i=0; i<leds; i++){
			gpio1[GPIO_SETDATAOUT]   = P9_14;	// Turn on leds one by one
			__delay_cycles(180);		// Wait 900 ns
			gpio1[GPIO_CLEARDATAOUT] = P9_14;
			__delay_cycles(70);
		}

		__delay_cycles(800);

		leds++;

		if(leds > TOTAL_BITS_IN_STRAND){
			leds = 0;
		}

		__delay_cycles(150000);

		for(i=0; i<leds; i++){
			gpio1[GPIO_SETDATAOUT]   = P9_14;	// Turn on leds
			__delay_cycles(70);
			gpio1[GPIO_CLEARDATAOUT] = P9_14;
			__delay_cycles(180);
		}

		x++;
	}

    turnOffAllLEDS();

	__halt();
}

void turnOffAllLEDS(void) {
    // Turns off all leds
	for (int i=0; i<TOTAL_BITS_IN_STRAND; i++){
		gpio1[GPIO_SETDATAOUT] = P9_14;
		__delay_cycles(T0H_CYC);    // Wait 350ns = 0.35μs
		gpio1[GPIO_CLEARDATAOUT] = P9_14; //off
		__delay_cycles(T0L_CYC);	// Wait 800ns = 0.8μs //180 Wait 900ns = 0.9μs
	}
}

// Set direction of P9_14 (which is port 1 pin 18)
#pragma DATA_SECTION(init_pins, ".init_pins")
#pragma RETAIN(init_pins)
const char init_pins[] =
	"/sys/class/gpio/gpio50/direction\0out\0" \
	"/sys/devices/platform/ocp/ocp:P9_14_pinmux/state\0gpio\0" \
	"\0\0";

// The export doesn't have to be done on the Black since
//	GPIOs are already exported
	// "/sys/class/gpio/export\0 177\0"
