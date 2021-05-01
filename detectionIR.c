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
#include <run.h>

#define FRONT_PROX_SENSOR	7
#define BACK_PROX_SENSOR	3
#define MIN_DIST_PROX		50
#define MIN_DIST_PROX_CARD	30

static THD_WORKING_AREA(waDetectionIR, 256);
static THD_FUNCTION(DetectionIR, arg) {

    chRegSetThreadName(__FUNCTION__);
    (void)arg;
    unsigned int compteurBack=0;
    unsigned int compteurFront=0;

    while(1){
    		if(getEtat()=='P'&& get_prox(BACK_PROX_SENSOR) > MIN_DIST_PROX){
    			compteurBack++;
    		}
    		if(compteurBack>9){
    			compteurBack=0;//Il faut garder la main pendant 1 seconde à côté du détecteur IR pour le relancer
    			setEtat('N');
    		}


    		if(getEtat()=='R' && (get_prox(FRONT_PROX_SENSOR) > MIN_DIST_PROX_CARD)){//Makes sure that a card is in front of the robot when scanning
    			compteurFront++;
    		}
    		if(compteurFront>9){
    			set_objectInFront(true);
    			compteurFront=0;//Il faut garder la main pendant 1 seconde à côté du détecteur IR pour le relancer
    		}

    		//For visualisation : activate led if object very close to back proximity sensor
    		//chprintf((BaseSequentialStream *)&SDU1, "% proximite  %-7d\r\n", get_prox(BACK_PROX_SENSOR));
    		if (get_prox(BACK_PROX_SENSOR) > MIN_DIST_PROX){
    			set_led(LED3,1);
    		}
    		else{
    			set_led(LED3,0);
    		}

    		if (get_prox(FRONT_PROX_SENSOR) > MIN_DIST_PROX_CARD){
    	    	set_led(LED6,1);
    	    }
    	    else{
    	    	set_led(LED6,0);
    	    }
    		chThdSleepMilliseconds(100);
    	}
}

void IR_thd_start(void){
	chThdCreateStatic(waDetectionIR, sizeof(waDetectionIR), NORMALPRIO, DetectionIR, NULL);
}

