#include "ch.h"
#include "hal.h"
#include <math.h>
#include <usbcfg.h>
#include <chprintf.h>


#include <main.h>
#include <motors.h>
#include <process_image.h>
#include <run.h>
#include <detectionIR.h>

#define distanceObjectif 10
#define SPEED_BASE 200 //la vitesse nominale des moteurs

static char etat = 'N'; //N = lighe noir, R = pastille rouge (arrêt),
						//B = pastille bleue (Lire carte), P = pause (open-loop)

static THD_WORKING_AREA(waRun, 256);
static THD_FUNCTION(Run, arg) {

    chRegSetThreadName(__FUNCTION__);
    (void)arg;

    systime_t time;
    int16_t speedR = 0;
    int16_t speedL = 0;

    //PID :
    float erreur=0;
    float erreur_precedente=0;
    float erreurtot=0;

    int Kp=1;
    int Ki=0.02;
    int Kd=0.5;

    while(1){
        time = chVTGetSystemTime();
        if(etat=='N'){
        		//alors suit la ligne noir
        		erreur=getPos(); //Erreur entre -320 et 320
        	    erreurtot+=erreur;

        	    if(erreurtot>700.){//Anti Wind-up
        	        erreurtot=700.;
             }
    	        else if(erreurtot<-700.){//Anti Wind-up
    	        		erreurtot=-700.;
    	         }

    	        speedR = SPEED_BASE-(Kp*erreur+Ki*erreurtot+Kd*(erreur-erreur_precedente));
    	        speedL = SPEED_BASE+(Kp*erreur+Ki*erreurtot+Kd*(erreur-erreur_precedente));

    	        //chprintf((BaseSequentialStream *)&SDU1, "% speedRight  %-7d\r\n", speedR);
    	        //chprintf((BaseSequentialStream *)&SDU1, "% speedLeft  %-7d\r\n", speedL);
    	        right_motor_set_speed(speedR);
        		left_motor_set_speed(speedL);

        		erreur_precedente=erreur;
        }
        else if(etat=='R'){
        		chSysLock();//boucle ouverte


        		erreur_precedente=0;
        		erreurtot=0;

        		chSysUnlock();
        }
        else if(etat=='B'){
        		chSysLock();
        		erreur_precedente=0;
        		erreurtot=0;
        	    	//boucle ouverte
        		chSysUnlock();
        }

        //100Hz
        chThdSleepUntilWindowed(time, time + MS2ST(10));
    }
}

void starting_move(void){
	//Get out of the beginning slot
	left_motor_set_pos(0);
	right_motor_set_pos(0);
	right_motor_set_speed(-400);
	left_motor_set_speed(-400);
	while (left_motor_get_pos()>-1300){}
	right_motor_set_speed(0);
	left_motor_set_speed(0);

	//Turn towards the line to follow
	left_motor_set_pos(0);
	right_motor_set_pos(0);
	right_motor_set_speed(400);
	left_motor_set_speed(-400);
	while (left_motor_get_pos()>-350){}
	right_motor_set_speed(0);
	left_motor_set_speed(0);
}

void run_thd_start(void){
	chThdCreateStatic(waRun, sizeof(waRun), NORMALPRIO, Run, NULL);
}


//-------------------------Getters et setters :----------------------------
char getEtat(void){
	return etat;
}

void setEtat(char c){
	switch(c){
	case 'P' : etat = 'P';break;
	case 'N' : etat = 'N';break;
	case 'R' : etat = 'R';break;
	case 'G' : etat = 'G';break;
	case 'B' : etat = 'B';break;
	}
}
