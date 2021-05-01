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

#define SPEED_BASE 200 //la vitesse nominale des moteurs
#define POSITION_MOTEUR_CHAMP_VISION 1100 //la distance à laquelle la caméra voit convertie en position de moteur
#define POSITION_MOTEUR_ROTATION180 700
#define LEFT 0
#define RIGHT 1

static char etat = 'R'; //N = lighe noir, R = pastille rouge (lire carte),
						//B = pastille bleue (arrêt après 3 lectures), P = pause (open-loop)

static bool objectInFront = 0;
static bool cardScanned = 0; //informs run.c that a card has been processed
static uint8_t currentCard;

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
	move_dist(-1100);

	//Turn towards the line to follow
	turn_dist(POSITION_MOTEUR_ROTATION180/2);
}

void scan_move(bool orientation){
	//open loop forward and turn
	move_dist(POSITION_MOTEUR_CHAMP_VISION);
	if (orientation==LEFT){turn_dist(-1*POSITION_MOTEUR_ROTATION180/2);}
	else if (orientation==RIGHT){turn_dist(POSITION_MOTEUR_ROTATION180/2);}

	while(!cardScanned) //waits for processing of card colors
	{
		chThdSleepMilliseconds(100);
	}
	setAttemptPin(currentCard);
	cardScanned=0;

	//open loop turn
	if (orientation==LEFT){turn_dist(POSITION_MOTEUR_ROTATION180/2);}
	else if (orientation==RIGHT){turn_dist(-1*POSITION_MOTEUR_ROTATION180/2);}
	objectInFront = 0;
}


void break_move(void){//appellée lorsque le robot voit une ligne rouge
	left_motor_set_pos(0);
	right_motor_set_pos(0);
	right_motor_set_speed(400);
	left_motor_set_speed(400);
	while(left_motor_get_pos()<POSITION_MOTEUR_CHAMP_VISION)//correspondant à ~8 cm
	{
		__asm__ volatile ("nop");
	}
	right_motor_set_speed(0);
	left_motor_set_speed(0);

	//Turn 180°
	left_motor_set_pos(0);
	right_motor_set_pos(0);
	right_motor_set_speed(400);
	left_motor_set_speed(-400);
	while (right_motor_get_pos()<POSITION_MOTEUR_ROTATION180){
		__asm__ volatile ("nop");
	}
	right_motor_set_speed(0);
	left_motor_set_speed(0);
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

    int Kp=2;
    int Ki=0.02;
    int Kd=0.5;

    while(1){
        time = chVTGetSystemTime();
        //chprintf((BaseSequentialStream *)&SDU1, "% etat  %-7c\r\n", etat);
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
        else if(etat=='R')
        {
        	if(getTurnCounter()==0 || getTurnCounter()==1) //cards are to the left during first 6 scans
        		scan_move(LEFT);
			else if (getTurnCounter()>1) ////cards are to the right when the robot scans from the Paused placement
				scan_move(RIGHT);

    		erreur_precedente=0;
        	erreurtot=0;


        }
        else if(etat=='B')
        {
        	right_motor_set_speed(0);
        	left_motor_set_speed(0);
        	break_move();
        	etat='P';
        		/*
        		right_motor_set_speed(0);
        	    left_motor_set_speed(0);
        		erreur_precedente=0;
        		erreurtot=0;
        	    	//boucle ouverte*/
        }

        //100Hz
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
		case 'P' : etat = 'P';break;
		case 'N' : etat = 'N';break;
		case 'R' : etat = 'R';break;
		case 'G' : etat = 'G';break;
		case 'B' : etat = 'B';break;
	}
}

bool get_objectInFront(void){
	return objectInFront;
}

void set_objectInFront(bool object){
	objectInFront=object;
}

void set_currentCard(uint8_t leftColor, uint8_t rightColor){
	if (leftColor==RED || rightColor==RED) {currentCard=COLOR_RED_RED;
	set_rgb_led(LED2, 254, 0, 0);}
	else if (leftColor==BLUE || rightColor==BLUE) {currentCard=COLOR_BLUE_BLUE;
	set_rgb_led(LED2, 0, 0, 254);}
	else if (leftColor==GREEN || rightColor==GREEN) {currentCard=COLOR_GREEN_GREEN;
	set_rgb_led(LED2, 0, 254, 0);}
	else if (leftColor==RED || rightColor==GREEN) {currentCard=COLOR_RED_GREEN;
	set_rgb_led(LED2, 254, 254, 0);}
	else if (leftColor==RED || rightColor==BLUE) {currentCard=COLOR_RED_BLUE;
	set_rgb_led(LED2, 255, 0, 254);}
	else {//chprintf((BaseSequentialStream *)&SDU1, "Wrong color of card");
	set_led(LED7, 1);}

	cardScanned=1;
}
