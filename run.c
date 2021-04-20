#include "ch.h"
#include "hal.h"
#include <math.h>
#include <usbcfg.h>
#include <chprintf.h>


#include <main.h>
#include <motors.h>
#include <process_image.h>
#include <run.h>

#define distanceObjectif 10
#define SPEED_BASE 100 //la vitesse nominale des moteurs

static THD_WORKING_AREA(waPiRegulator, 256);
static THD_FUNCTION(PiRegulator, arg) {

    chRegSetThreadName(__FUNCTION__);
    (void)arg;

    systime_t time;
    int16_t speedR = 0;
    int16_t speedL = 0;

    float erreur=0;
    float erreur_precedente=0;
    float erreurtot=0;

    int Kp=1.2;
    int Ki=0.05;
    int Kd=0.5;

    while(1){
        time = chVTGetSystemTime();
        if(getEtat()=='N'){
        		//alors suit la ligne noir
        		erreur=getPos(); //Erreur entre -320 et 320
        	    erreurtot+=erreur;

        	    if(erreurtot>700.){//Anti Wind-up
        	        erreurtot=700.;
             }
    	        else if(erreurtot<-700.){//Anti Wind-up
    	        		erreurtot=-700.;
    	         }

    	        speedR = SPEED_BASE+(Kp*erreur+Ki*erreurtot+Kd*(erreur-erreur_precedente));
    	        speedL = SPEED_BASE-(Kp*erreur+Ki*erreurtot+Kd*(erreur-erreur_precedente));

    	        /*
        	    if(abs(speedR)<20){//threshold
        	    		speedR=0;
             }
        	    if(abs(speedL)<20){//threshold
    	    			speedL=0;
        	    }
        	    */

    	        right_motor_set_speed(speedR);
        		left_motor_set_speed(speedL);

        		erreur_precedente=erreur;
        }
        //100Hz
        chThdSleepUntilWindowed(time, time + MS2ST(10));
    }
}

void pi_regulator_start(void){
	chThdCreateStatic(waPiRegulator, sizeof(waPiRegulator), NORMALPRIO, PiRegulator, NULL);
}
