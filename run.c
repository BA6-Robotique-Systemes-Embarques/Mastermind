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

#define SPEED_BASE 					400 //la vitesse nominale des moteurs
#define POSITION_MOTEUR_CHAMP_VISION 770 //Distance seen by the camera converted into motor position
#define POSITION_MOTEUR_ROTATION180 	660
#define RIGHT 0
#define LEFT 1

#define STARTUP_BACKUP_DIST 		-1210
#define STARTUP_BACKUP_SAFETY 	10
#define STARTUP_BACKUP_SETUP		-400
#define SCANNING_CLOSEUP_DIST	100

static char etat = ETAT_FOLLOW;
//ETAT_FOLLOW = following the black line, ETAT_GAMEHINT = (red spot) goes into break after scanning 3 cards
//ETAT_SCAN = (blue spot) scan a card, ETAT_PAUSE = wait for hand signal, ETAT_STOP = used for reset

static bool ReadytoScan = false;
static bool objectInFront = false; //Updated by rom detectionIR.c to confirm that an card is in front of the e-puck2
static bool cardScanned = false; //informs run.c that a card has been processed
static uint8_t currentCard;

static bool ignoreSCAN = false;

//-------------BASIC MOTOR CONTROL----------------

void move_dist(int motorPos){ //positive position = go forward, negative = go backwards
	left_motor_set_pos(0);
	right_motor_set_pos(0);
	if (motorPos>0){
		right_motor_set_speed(SPEED_BASE);
		left_motor_set_speed(SPEED_BASE);
		while(left_motor_get_pos()<motorPos) {__asm__ volatile ("nop");}
	}
	else if (motorPos<=0){
		right_motor_set_speed(-SPEED_BASE);
		left_motor_set_speed(-SPEED_BASE);
		while(left_motor_get_pos()>motorPos) {__asm__ volatile ("nop");}
	}
	right_motor_set_speed(0);
	left_motor_set_speed(0);
}

void turn_dist(int motorPos){ //positive position = clockwise, negative = counterclockwise
	left_motor_set_pos(0);
	right_motor_set_pos(0);
	if (motorPos>0){
		right_motor_set_speed(-SPEED_BASE);
		left_motor_set_speed(SPEED_BASE);
		while(left_motor_get_pos()<motorPos) {__asm__ volatile ("nop");}
	}
	else if (motorPos<=0){
		right_motor_set_speed(SPEED_BASE);
		left_motor_set_speed(-SPEED_BASE);
		while(left_motor_get_pos()>motorPos) {__asm__ volatile ("nop");}
	}
	right_motor_set_speed(0);
	left_motor_set_speed(0);
}

void stopMotors(void){
	right_motor_set_speed(0);
	left_motor_set_speed(0);
	left_motor_set_pos(0);
	right_motor_set_pos(0);
}

void turnAround_move(void){
	turn_dist(POSITION_MOTEUR_ROTATION180);
}

//-------------COMPLEX MOTOR FUNCTIONS----------------

void starting_move(void){
	//Get out of the beginning slot
	move_dist(STARTUP_BACKUP_SAFETY);
	move_dist(STARTUP_BACKUP_DIST);

	//Turn towards the line to follow
	turn_dist(-POSITION_MOTEUR_ROTATION180/2);
	move_dist(STARTUP_BACKUP_SETUP);
}

void scan_move(bool orientation){ //Large function to handle the entire scanning process

	//open loop move forward to the spot, turn and get close to the card
	move_dist(POSITION_MOTEUR_CHAMP_VISION);
	set_body_led(OFF);
	if (orientation==LEFT) turn_dist(-1*POSITION_MOTEUR_ROTATION180/2);
	else if (orientation==RIGHT) turn_dist(POSITION_MOTEUR_ROTATION180/2);
	move_dist(SCANNING_CLOSEUP_DIST);

	ReadytoScan = true; //signals to other threads that they can start scanning the front of the robot
	while(!cardScanned){
		chThdSleepMilliseconds(10); //waits for processing of card colors
	}

	//Resets booleans used to communicate to other threads, turning them off
	ReadytoScan = false;
	objectInFront = false;
	cardScanned=false;
	setAttemptPin(currentCard);//communicate what card was just scanned

	//open loop move back onto the black line
	move_dist(-SCANNING_CLOSEUP_DIST);
	if (orientation==LEFT) turn_dist(POSITION_MOTEUR_ROTATION180/2);
	else if (orientation==RIGHT) turn_dist(-1*POSITION_MOTEUR_ROTATION180/2);
}

void break_move(void){ //called when a red spot is spotted
	move_dist(POSITION_MOTEUR_CHAMP_VISION);
	turnAround_move();
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
    float Kd=0;

    while(1){
        time = chVTGetSystemTime();

        if(etat==ETAT_FOLLOW){
        		erreur=getPos(); //Error between -320 and 320
        		erreurtot+=erreur;

        		if(erreurtot>700.)       erreurtot= 700.;	//Anti Wind-up
        		else if(erreurtot<-700.) erreurtot=-700.;	//Anti Wind-up

        		speedR = SPEED_BASE-(Kp*erreur+Ki*erreurtot+Kd*(erreur-erreur_precedente));
        		speedL = SPEED_BASE+(Kp*erreur+Ki*erreurtot+Kd*(erreur-erreur_precedente));

        		right_motor_set_speed(speedR);
        		left_motor_set_speed(speedL);

        		erreur_precedente=erreur;
        }
        else if(etat==ETAT_SCAN){
        		if(getTurnCounter()==0 || getTurnCounter()==1) //cards are to the right during first 6 scans
        			scan_move(RIGHT);
        		else if (getTurnCounter()>1){ //cards are to the left when the robot scans from the red spot (PAUSE)
        			unsigned int TC_beforeScan = getTurnCounter();
        			scan_move(LEFT);
        			//Moves back to pause spot once the 3 cards have been scanned
        			if (TC_beforeScan!=getTurnCounter()){
        				turnAround_move();
        				ignoreSCAN = true;
        			}
        		}

        		//reset PID :
        		erreur_precedente=0;
        		erreurtot=0;

        		set_body_led(OFF);
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


//-------------------------GETTERS AND SETTERS----------------------------
char getEtat(void){
	return etat;
}

void setEtat(char c){
	switch(c){
		case ETAT_PAUSE : etat = ETAT_PAUSE; break;
		case ETAT_FOLLOW : etat = ETAT_FOLLOW; break;
		case ETAT_SCAN : etat = ETAT_SCAN; break;
		case ETAT_GAMEHINT : etat = ETAT_GAMEHINT; break;
		case ETAT_STOP : etat = ETAT_STOP; break;
	}
}

bool getObjectInFront(void){
	return objectInFront;
}

void setObjectInFront(bool object){
	objectInFront=object;
}

bool getReadytoScan(void){
	return ReadytoScan;
}

void setCurrentCard(uint8_t card){
	//chprintf((BaseSequentialStream *)&SDU1, "% Card = %-7d\r\n", card);
	if(card != COLOR_WRONG){
		currentCard = card;
		cardScanned=true;
	}
}

bool getIgnoreScan(void){
	return ignoreSCAN;
}
