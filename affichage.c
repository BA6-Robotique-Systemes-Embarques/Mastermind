/*
 * affichage.c
 *
 *  Created on: 29 April 2021
 *      Author: Cyril Monette
 */

#include "ch.h"
#include "hal.h"
#include <math.h>
#include <usbcfg.h>
#include <chprintf.h>
#include <leds.h>

#include <game_logic.h>
#include <main.h>
#include <run.h>

static const uint8_t seq_game_over[8][4] = {
	{0, 0, 0, 1}, // ON1
	{0, 0, 1, 1}, // ON3
	{0, 1, 1, 1}, // ON5
	{1, 1, 1, 1}, // ON7
	{1, 1, 1, 0}, // OFF1
	{1, 1, 0, 0}, // OFF3
	{1, 0, 0, 0}, // OFF5
	{0, 0, 0, 0}, // OFF7
};

static void LEDs_update(const uint8_t *out){
	out[3] ? set_led(LED3,0) : set_led(LED3,1);
	out[2] ? set_led(LED5,0) : set_led(LED5,1);
	out[1] ? set_led(LED7,0) : set_led(LED7,1);
	out[0] ? set_led(LED1,0) : set_led(LED1,1);
}


static THD_WORKING_AREA(waDisplay, 256);
static THD_FUNCTION(Display, arg){
    chRegSetThreadName(__FUNCTION__);
    (void)arg;

    systime_t time;
    hintPins hintpins;
    uint8_t sequence_pos=0;

    while(1){
    		time = chVTGetSystemTime();

    		hintpins=getHints();

    		if(getEtat()==ETAT_PAUSE && hintpins.b_key+hintpins.w_key<=3){
    			uint8_t compteur_led=0;
    			set_rgb_led(LED6, 0, 0, 0);
    			set_rgb_led(LED4, 0, 0, 0);
    			if(hintpins.victory_state==GAME_WON){
    				set_body_led(ON);
    				setEtat(ETAT_STOP);
    			}
    			else if(hintpins.victory_state==GAME_OVER){
    				LEDs_update(seq_game_over[sequence_pos]);
    				sequence_pos++;
    				sequence_pos %=8;
    				setEtat(ETAT_STOP);
    			}
    			else{
    				switch(hintpins.b_key){
    				case 1 :
    					compteur_led++;
    					set_led(LED5,ON);
    					break;
    				case 2 :
    					compteur_led+=2;
    					set_led(LED5,ON);
    					set_led(LED7,ON);
    					break;
    				}

    				switch(hintpins.w_key){
    				case 1 :
    					if(compteur_led==0) set_led(LED5,TOGGLE);
    					if(compteur_led==1) set_led(LED7,TOGGLE);
    					if(compteur_led==2) set_led(LED1,TOGGLE);
    					compteur_led++;
    					break;
    				case 2 :
    					if(compteur_led==0){
    						set_led(LED5,TOGGLE);
    						set_led(LED7,TOGGLE);
    					}
    					if(compteur_led==1){
    						set_led(LED7,TOGGLE);
    						set_led(LED1,TOGGLE);
    					}
    					compteur_led+=2;
    					break;
    				case 3 :
    					set_led(LED5,TOGGLE);
    					set_led(LED7,TOGGLE);
    					set_led(LED1,TOGGLE);
    					break;
    				}
    			}
    		}
    		else{
    			set_led(LED5,OFF);
    			set_led(LED7,OFF);
    			set_led(LED1,OFF);
    			set_led(LED3,OFF);
    		}
        chThdSleepUntilWindowed(time, time + MS2ST(500));
    }
}

void display_start(void){
	chThdCreateStatic(waDisplay, sizeof(waDisplay), NORMALPRIO, Display, NULL);
}

