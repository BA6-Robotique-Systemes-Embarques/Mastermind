/*
 * detectionIR.c
 *
 *  Created on: 20 April 2021
 *      Author: Cyril Monette
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

#define FRONT_PROX_SENSOR	0
#define BACK_PROX_SENSOR		3
#define MIN_DIST_PROX		50
#define MIN_DIST_PROX_CARD	50

static THD_WORKING_AREA(waDetectionIR, 256);
static THD_FUNCTION(DetectionIR, arg) {

    chRegSetThreadName(__FUNCTION__);
    (void)arg;
    uint8_t compteurBack=0, compteurFront=0;

    int calibration_front=get_prox(FRONT_PROX_SENSOR);
    int calibration_back=get_prox(BACK_PROX_SENSOR);
    systime_t time;

    while(1){
        time=chVTGetSystemTime();
    		if(getEtat()==ETAT_PAUSE && ((get_prox(BACK_PROX_SENSOR)-calibration_back) > MIN_DIST_PROX)){
    			compteurBack++;
    		}
    		if(compteurBack>4){
    			compteurBack=0;//The hand needs to be kept close to the sensor for at least 1 second
    			setEtat(ETAT_FOLLOW);
    		}


    		if(getEtat()==ETAT_SCAN && ((get_prox(FRONT_PROX_SENSOR)-calibration_front) > MIN_DIST_PROX_CARD)){//Makes sure that a card is in front of the robot when scanning
    			compteurFront++;
    		}
    		if(compteurFront>9){
    			setObjectInFront(true);
    			compteurFront=0;//The card needs to remain in front of the robot for at least 2 seconds
    		}

    		/*For visualisation : activate led if object very close to back proximity sensor
    		//chprintf((BaseSequentialStream *)&SDU1, "% proximite %-7d %-7d\r\n", get_prox(BACK_PROX_SENSOR),get_prox(FRONT_PROX_SENSOR));
    		if((get_prox(BACK_PROX_SENSOR)-calibration_back) > MIN_DIST_PROX){
    			set_led(LED3,1);
    		}
    		else{
    			set_led(LED3,0);
    		}

    		if((get_prox(FRONT_PROX_SENSOR)-calibration_front) > MIN_DIST_PROX_CARD){
    			set_led(LED7,1);
    	    }
    	    else{
    	    		set_led(LED7,0);
    	    }*/
    		chprintf((BaseSequentialStream *)&SD3, "% Temps detectionIR = %-7d\r\n", chVTGetSystemTime()-time);
    		chThdSleepMilliseconds(200);
    	}
}

void IR_thd_start(void){
	chThdCreateStatic(waDetectionIR, sizeof(waDetectionIR), NORMALPRIO, DetectionIR, NULL);
}

