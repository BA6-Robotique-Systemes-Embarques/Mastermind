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


static BSEMAPHORE_DECL(robot_stopped_sem, TRUE);

static THD_WORKING_AREA(waDetectionIR, 256);
static THD_FUNCTION(DetectionIR, arg) {

    chRegSetThreadName(__FUNCTION__);
    (void)arg;

    while(1){
    		chBSemWait(&robot_stopped_sem);

    }
}

void run_thd_IR(void){
	chThdCreateStatic(waDetectionIR, sizeof(waDetectionIR), NORMALPRIO, DetectionIR, NULL);
}




