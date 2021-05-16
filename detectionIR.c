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

#include <main.h>
#include <detectionIR.h>
#include <sensors/proximity.h>
#include <run.h>

#define FRONT_PROX_SENSOR	0
#define BACK_PROX_SENSOR		3

#define MIN_DIST_PROX		50
#define MIN_DIST_PROX_CARD	50

#define FRONT_PROX_CNT		9
#define BACK_PROX_CNT		4


//---------------------THREAD---------------------

static THD_WORKING_AREA(waDetectionIR, 256);
static THD_FUNCTION(DetectionIR, arg){

    chRegSetThreadName(__FUNCTION__);
    (void)arg;

    systime_t time;
    uint8_t compteurBack=0, compteurFront=0;

    int calibration_front=get_prox(FRONT_PROX_SENSOR);
    int calibration_back=get_prox(BACK_PROX_SENSOR);

    while(1){
    	time = chVTGetSystemTime();

    	if(getEtat()==ETAT_PAUSE && ((get_prox(BACK_PROX_SENSOR)-calibration_back) > MIN_DIST_PROX)){
    		compteurBack++;
    	}
    	if(compteurBack>BACK_PROX_CNT){
    		compteurBack=0;
    		setEtat(ETAT_FOLLOW);//The hand needs to be kept close to the sensor for at least 1 second
    	}

    	//Makes sure that a card is in front of the robot when scanning:
    	if(getEtat()==ETAT_SCAN && ((get_prox(FRONT_PROX_SENSOR)-calibration_front) > MIN_DIST_PROX_CARD)){
    		compteurFront++;
    	}
    	if(compteurFront>FRONT_PROX_CNT){
    		compteurFront=0;
    		setObjectInFront(true);//The card needs to remain in front of the robot for at least 2 seconds
    	}
    	chThdSleepUntilWindowed(time, time + MS2ST(200));
    }
}

void IR_thd_start(void){
	chThdCreateStatic(waDetectionIR, sizeof(waDetectionIR), NORMALPRIO-1, DetectionIR, NULL);
}

