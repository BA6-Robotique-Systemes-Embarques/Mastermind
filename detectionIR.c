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


static THD_WORKING_AREA(waDetectionIR, 256);
static THD_FUNCTION(DetectionIR, arg) {

    chRegSetThreadName(__FUNCTION__);
    (void)arg;
    messagebus_topic_t *prox_topic = messagebus_find_topic_blocking(&bus, "/proximity");
    proximity_msg_t prox_values;

    while(1){
    		//wait for new measures to be published
    	     messagebus_topic_wait(prox_topic, &prox_values, sizeof(prox_values));

    	     //if (SDU1.config->usbp->state != USB_ACTIVE) {continue;} // Skip printing if port not opened.

    	     int i=2;
    	     //for (uint8_t i = 0; i < PROXIMITY_NB_CHANNELS; i++)
    	     //{
    	     chprintf((BaseSequentialStream *)&SD3, "%4d,", prox_values.ambient[i]);
    	     chprintf((BaseSequentialStream *)&SD3, "%4d,", prox_values.reflected[i]);
    	     chprintf((BaseSequentialStream *)&SD3, "%4d", prox_values.delta[i]);
    	     chprintf((BaseSequentialStream *)&SD3, "\r\n");
    	     //}
    	     chprintf((BaseSequentialStream *)&SD3, "\r\n");

    	     chThdSleepMilliseconds(100);

    }
}

void start_thd_IR(void){
	chThdCreateStatic(waDetectionIR, sizeof(waDetectionIR), NORMALPRIO, DetectionIR, NULL);
}




