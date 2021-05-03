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

#define SPEED_BASE 250 //la vitesse nominale des moteurs pour le suivi de ligne
#define POSITION_MOTEUR_CHAMP_VISION 900 //la distance à laquelle la caméra voit convertie en position de moteur
#define POSITION_MOTEUR_ROTATION180 680
#define LEFT 0
#define RIGHT 1

static char etat = ETAT_FOLLOW; //N = lighe noir, R = pastille rouge (lire carte),
						//B = pastille bleue (arrêt après 3 lectures), P = pause (open-loop)

static bool objectInFront = 0; //Updayed by rom detectionIR.c to confirm that an card is in front of the e-puck2
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
	turn_dist(-POSITION_MOTEUR_ROTATION180/2);
	move_dist(-300);
}

void scan_move(bool orientation){
	//open loop forward and turn
	move_dist(POSITION_MOTEUR_CHAMP_VISION);
	if (orientation==RIGHT) turn_dist(-1*POSITION_MOTEUR_ROTATION180/2);
	else if (orientation==LEFT) turn_dist(POSITION_MOTEUR_ROTATION180/2);

	while(!cardScanned) //waits for processing of card colors
	{
		chThdSleepMilliseconds(10);
	}
	objectInFront = 0;
	cardScanned=0;
	setAttemptPin(currentCard);

	//open loop turn
	if (orientation==RIGHT) turn_dist(POSITION_MOTEUR_ROTATION180/2);
	else if (orientation==LEFT) turn_dist(-1*POSITION_MOTEUR_ROTATION180/2);
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

    float Kp=0.5;
    float Ki=0.1;
    float Kd=0.5;

    while(1){
        time = chVTGetSystemTime();
        chprintf((BaseSequentialStream *)&SDU1, "% etat  %-7c\r\n", etat);
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

    	        //chprintf((BaseSequentialStream *)&SDU1, "% speedRight  %-7d\r\n", speedR);
    	        //chprintf((BaseSequentialStream *)&SDU1, "% speedLeft  %-7d\r\n", speedL);
    	        right_motor_set_speed(speedR);
        		left_motor_set_speed(speedL);

        		erreur_precedente=erreur;
        }
        else if(etat==ETAT_SCAN){
        		if(getTurnCounter()==0 || getTurnCounter()==1) //cards are to the left during first 6 scans
        			scan_move(LEFT);
			else if (getTurnCounter()>1) ////cards are to the right when the robot scans from the Paused placement
				scan_move(RIGHT);

        		erreur_precedente=0;
        		erreurtot=0;
        		etat=ETAT_FOLLOW;
        }

        else if(etat==ETAT_GAMEHINT){
        		right_motor_set_speed(0);
        		left_motor_set_speed(0);
        		break_move();

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

void set_currentCard(uint8_t leftColor, uint8_t rightColor){
	//chprintf((BaseSequentialStream *)&SDU1, "% leftColor= %-7d % right Color = %-7d\r\n", leftColor, rightColor);
	if(leftColor==RED && rightColor==RED){
		currentCard=COLOR_RED_RED;
		chprintf((BaseSequentialStream *)&SDU1, "RedRed");
		//set_rgb_led(LED2, 255, 0, 0);
	}
	else if(leftColor==BLUE &&rightColor==BLUE){
		currentCard=COLOR_BLUE_BLUE;
		chprintf((BaseSequentialStream *)&SDU1, "BlueBlue");
		//set_rgb_led(LED2, 0, 0, 255);
	}
	else if (leftColor==GREEN && rightColor==GREEN){
		currentCard=COLOR_GREEN_GREEN;
		chprintf((BaseSequentialStream *)&SDU1, "GreenGreen");
		//set_rgb_led(LED2, 0, 255, 0);
	}
	else if (leftColor==RED && rightColor==GREEN){
		currentCard=COLOR_RED_GREEN;
		chprintf((BaseSequentialStream *)&SDU1, "RedGreen");
		//set_rgb_led(LED2, 255, 255, 0);
	}
	else if (leftColor==RED && rightColor==BLUE){
		currentCard=COLOR_RED_BLUE;
		chprintf((BaseSequentialStream *)&SDU1, "RedBlue");
		//set_rgb_led(LED2, 255, 0, 255);
	}
	else{
		chprintf((BaseSequentialStream *)&SDU1, "Wrong color of card");
		set_body_led(1);
	}

	cardScanned=1;
}
