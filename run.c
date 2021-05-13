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
#include <game_logic.h>
#include <leds.h>

#define SPEED_BASE 400 //la vitesse nominale des moteurs pour le suivi de ligne
#define POSITION_MOTEUR_CHAMP_VISION 730 //la distance à laquelle la caméra voit convertie en position de moteur
#define POSITION_MOTEUR_ROTATION180 660
#define LEFT 0
#define RIGHT 1

static char etat = ETAT_FOLLOW; //ETAT_FOLLOW = N = lighe noir, R = ligne rouge (arrêt après 3 lectures),
						//ETAT_SCAN = B = ligne bleue (lire carte), P = pause

static bool ReadytoScan = false;
static bool objectInFront = false; //Updated by rom detectionIR.c to confirm that an card is in front of the e-puck2
static bool cardScanned = false; //informs run.c that a card has been processed
static uint8_t currentCard;

static bool ignoreSCAN = false;

void move_dist(int motorPos){ //position positive = avancer, négative = reculer
	left_motor_set_pos(0);
	right_motor_set_pos(0);
	if (motorPos>0){
		right_motor_set_speed(400);
		left_motor_set_speed(400);
		while(left_motor_get_pos()<motorPos) {__asm__ volatile ("nop");}
	}
	else if (motorPos<=0){
		right_motor_set_speed(-400);
		left_motor_set_speed(-400);
		while(left_motor_get_pos()>motorPos) {__asm__ volatile ("nop");}
	}
	right_motor_set_speed(0);
	left_motor_set_speed(0);
}

void turn_dist(int motorPos){ //position positive = clockwise, negative = counterclockwise
	left_motor_set_pos(0);
	right_motor_set_pos(0);
	if (motorPos>0){
		right_motor_set_speed(-400);
		left_motor_set_speed(400);
		while(left_motor_get_pos()<motorPos) {__asm__ volatile ("nop");}
	}
	else if (motorPos<=0){
		right_motor_set_speed(400);
		left_motor_set_speed(-400);
		while(left_motor_get_pos()>motorPos) {__asm__ volatile ("nop");}
	}
	right_motor_set_speed(0);
	left_motor_set_speed(0);
}

void starting_move(void){
	//Get out of the beginning slot
	move_dist(10);
	move_dist(-1250);

	//Turn towards the line to follow
	turn_dist(-POSITION_MOTEUR_ROTATION180/2);
	move_dist(-300);
}

void scan_move(bool orientation){ //Large function to handle the entire scanning process
	//open loop forward and turn
	move_dist(POSITION_MOTEUR_CHAMP_VISION);
	if (orientation==RIGHT) turn_dist(-1*POSITION_MOTEUR_ROTATION180/2);
	else if (orientation==LEFT) turn_dist(POSITION_MOTEUR_ROTATION180/2);
	move_dist(120);

	ReadytoScan = true;

	while(!cardScanned){
		chThdSleepMilliseconds(10); //waits for processing of card colors
	}

	//Resets booleans used to communicate to other threads, turning them off
	ReadytoScan = false;
	objectInFront = false;
	cardScanned=false;
	setAttemptPin(currentCard);

	//open loop turn
	move_dist(-120);
	if (orientation==RIGHT) turn_dist(POSITION_MOTEUR_ROTATION180/2);
	else if (orientation==LEFT) turn_dist(-1*POSITION_MOTEUR_ROTATION180/2);
}

void turnAround_move(void){
	turn_dist(POSITION_MOTEUR_ROTATION180);
}

void break_move(void){ //appellée lorsque le robot voit une ligne rouge
	move_dist(POSITION_MOTEUR_CHAMP_VISION);
	turn_dist(POSITION_MOTEUR_ROTATION180);//Turn 180°
}

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

    float Kp=2.8;
    float Ki=0.011;
    //float Kd=1;
    float Kd=0;

    while(1){
        time = chVTGetSystemTime();
        //chprintf((BaseSequentialStream *)&SDU1, "% etat  %-7c\r\n", etat);
        if(etat==ETAT_FOLLOW){
        		//alors suit la ligne noir
        		erreur=getPos(); //Erreur entre -320 et 320
        	    erreurtot+=erreur;

        	    if(erreurtot>700.){//Anti Wind-up
        	        erreurtot=700.;
             }
    	        else if(erreurtot<-700.){//Anti Wind-up
    	        		erreurtot=-700.;
    	         }

    	        speedR = (int)(SPEED_BASE-(Kp*erreur+Ki*erreurtot+Kd*(erreur-erreur_precedente)));
    	        speedL = (int)(SPEED_BASE+(Kp*erreur+Ki*erreurtot+Kd*(erreur-erreur_precedente)));

    	        right_motor_set_speed(speedR);
        		left_motor_set_speed(speedL);

        		erreur_precedente=erreur;
        }
        else if(etat==ETAT_SCAN){
        		if(getTurnCounter()==0 || getTurnCounter()==1) //(cards are to the left during first 6 scans)
        			scan_move(LEFT);
        		else if (getTurnCounter()>1){ //(cards are to the right when the robot scans from the Paused placement)
        			unsigned int TC_beforeScan = getTurnCounter();
        			scan_move(RIGHT);
        			//Moves back to pause spot once the turn's card have all been scanned
        			if (TC_beforeScan!=getTurnCounter()){
        				turnAround_move();
        				ignoreSCAN = true;
        			}
        		}

        		//reset PID :
        		erreur_precedente=0;
        		erreurtot=0;

        		set_body_led(0);
        		etat=ETAT_FOLLOW;
        }

        else if(etat==ETAT_GAMEHINT){
        		right_motor_set_speed(0);
        		left_motor_set_speed(0);
        		break_move();

        		ignoreSCAN = false; //turns back on the ability to detect scanning spots

        		//reset PID :
        		erreur_precedente=0;
        		erreurtot=0;

        		etat=ETAT_PAUSE;
        }

        //100 Hz :
        chThdSleepUntilWindowed(time, time + MS2ST(10));
    }
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
		case ETAT_PAUSE : etat = ETAT_PAUSE; break;
		case ETAT_FOLLOW : etat = ETAT_FOLLOW; break;
		case ETAT_SCAN : etat = ETAT_SCAN; break;
		case ETAT_GAMEHINT : etat = ETAT_GAMEHINT; break;
	}
}

bool get_objectInFront(void){
	return objectInFront;
}

void set_objectInFront(bool object){
	objectInFront=object;
}

bool getReadytoScan(void){
	return ReadytoScan;
}

void set_currentCard(uint8_t card){
	chprintf((BaseSequentialStream *)&SDU1, "% Card = %-7d\r\n", card);
	if(card != COLOR_WRONG){
		currentCard = card;
		cardScanned=1;
	}
}

bool getIgnoreScan(void){
	return ignoreSCAN;
}
