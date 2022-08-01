////////////////////////////////////////
//	neopixelStatic.pru0.c
//	Control a ws2812 (NeoPixel) display, green, red, blue, green, ...
//	Wiring:	The NeoPixel Data In goes to P9_29, the plus lead to P9_3 or P9_4
//			and the ground to P9_1 or P9_2.  If you have more then 40 some
//			NeoPixels you will need and external supply.
//	Setup:	config_pin P9_29 pruout
//	See:
//	PRU:	pru0
////////////////////////////////////////
#include <stdint.h>
#include <pru_cfg.h>
#include <math.h>
#include "resource_table_empty.h"
#include "prugpio.h"

#define NANO_SEC_PER_CYCLE 5
#define STR_LEN 8
#define	oneCyclesOn		700/NANO_SEC_PER_CYCLE   // Stay on 700ns
#define oneCyclesOff	800/NANO_SEC_PER_CYCLE
#define zeroCyclesOn	350/NANO_SEC_PER_CYCLE
#define zeroCyclesOff	600/NANO_SEC_PER_CYCLE
#define resetCycles		60000/NANO_SEC_PER_CYCLE // Must be at least 50u, use 60u
/*
PRU runs at 200Mhz
200,000,000 cycles
------------------
       1s
*/

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
volatile register uint32_t __R30; //Output
volatile register uint32_t __R31; //Input

typedef uint32_t color_t;

void writeToAllPixels(color_t colors[3], size_t colorsSz, uint32_t gpio);
color_t hsvToRgb(float hue, float saturation, float value);

#define BLACK 0x000000
#define LVL_0 0xF26824
#define LVL_1 0xF9A640
#define LVL_2 0xF9B94A
#define LVL_3 0xFCEAAB
#define LVL_4 0xFEF2D2

void main(void)
{
	/* Clear SYSCFG[STANDBY_INIT] to enable OCP master port */
	CT_CFG.SYSCFG_bit.STANDBY_INIT = 0;
	//							 green,     red,    blue
	color_t colors[STR_LEN] = {LVL_0, LVL_1, LVL_2, LVL_3, LVL_4, 0x00FF00, BLACK};
	color_t black[] = {BLACK};

	writeToAllPixels(colors, 6, P9_29);
	__delay_cycles((unsigned long)(resetCycles * 1000000ul));
	writeToAllPixels(black, 1, P9_29);

	__halt();
}

/*
	Write a color to the NeoPixel
	params:
		uint32_t color[]: 24bit RGB colors in GRB order. Cycle through each color in this array.
		uint32_t gpio: Select which pins to output to.  Example: P9_29, These are all on pru1_1
*/
void writeToAllPixels(color_t colors[], size_t colorsSz, uint32_t gpio) {
	int i, j;
	for(j=0; j<STR_LEN; j++) {	// Loop for each LED in string
		for(i=23; i>=0; i--) {
			if(colors[j%colorsSz] & (0x1<<i)) {	// Pick off one bit at a time
				__R30 |= gpio;		// Set the GPIO pin to 1
				__delay_cycles(oneCyclesOn-1);
				__R30 &= ~gpio;		// Clear the GPIO pin
				__delay_cycles(oneCyclesOff-2);
			} else {
				__R30 |= gpio;		// Set the GPIO pin to 1
				__delay_cycles(zeroCyclesOn-1);
				__R30 &= ~gpio;	// Clear the GPIO pin
				__delay_cycles(zeroCyclesOff-2);
			}
		}
	}
	// Send Reset
	__R30 &= ~gpio;	// Clear the GPIO pin
	__delay_cycles(resetCycles);
}

/*
hsvToRgb
params:
    float h: hue angle (degrees) 0 to 359
    float s: saturation percentage 0 to 100
    float v: value percentage 0 to 100
returns a color_t in grb form
*/
color_t hsvToRgb(float h, float s, float v){
	float C, hh, X, m, r=0.f, g=0.f, b=0.f;

	if(h<0.f) h=0.f;
	if(s<0.f) s=0.f;
	if(v<0.f) v=0.f;
	if(h>=360.f) h=359.f;
	if(s>100.f) s=100.f;
	if(v>100.f) v=100.f;
	s/=100.f;
	v/=100.f;
	C = v*s;
	hh = h/60.0f;
	X = C*(1.0f-fabsf((fmod(hh,2.f))-1.0f));

	if(hh >= 0.f && hh < 1.f){
		r = C;
		g = X;
	} else if(hh >= 1.f && hh < 2.f){
		r = X;
		g = C;
	} else if(hh >= 2.f && hh < 3.f){
		g = C;
		b = X;
	} else if(hh >= 3.f && hh < 4.f){
		g = X;
		b = C;
	} else if(hh >= 4.f && hh < 5.f){
		r = X;
		b = C;
	} else{
		r = C;
		b = X;
	}
	m = v-C;
	r += m;
	g += m;
	b += m;
	r *= 255.f;
	g *= 255.f;
	b *= 255.f;
	r = roundf(r);
	g = roundf(g);
	b = roundf(b);
	return (color_t)(g*65536.f + r*256.f + b);
}

// Sets pinmux
#pragma DATA_SECTION(init_pins, ".init_pins")
#pragma RETAIN(init_pins)
const char init_pins[] =
	"/sys/devices/platform/ocp/ocp:P9_29_pinmux/state\0pruout\0" \
	"\0\0";
