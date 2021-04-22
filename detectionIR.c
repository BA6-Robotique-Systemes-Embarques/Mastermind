/*
 * detectionIR.c
 *
 *  Created on: 20 Apr 2021
 *      Author: cyrilmonette
 */
#include "ch.h"
#include "hal.h"
#include <math.h>
#include <usbcfg.h>
#include <chprintf.h>

#include <main.h>
#include <detectionIR.h>
#include <sensors/proximity.h>
#include <leds.h>

#define BACK_PROX_SENSOR	3
#define MIN_DIST_PROX		20

static THD_WORKING_AREA(waDetectionIR, 256);
static THD_FUNCTION(DetectionIR, arg) {

    chRegSetThreadName(__FUNCTION__);
    (void)arg;

    while(1)
    {
    	//Activate led if object very close to back proximity sensor
    	if (get_prox(BACK_PROX_SENSOR) > MIN_DIST_PROX)
    	{
    		set_front_led(1);
    	}
    	else {set_front_led(0);}
   	    chThdSleepMilliseconds(100);
    }
}

void start_thd_IR(void){
	chThdCreateStatic(waDetectionIR, sizeof(waDetectionIR), NORMALPRIO, DetectionIR, NULL);
}




