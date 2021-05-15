#include <stdio.h>
#include <string.h>
#include "ch.h"
#include "hal.h"
#include <chprintf.h>
#include <leds.h>
#include <main.h>
#include <game_logic.h>

#define CODING_TURN				0
#define FIRST_GUESSING_TURN		1
#define GUESSING_TURN			2

#define MAX_NUM_TURN	 		8
#define NUMBER_OF_PINS			3

//--------------Game data (static variables)-------------
static unsigned int turnCounter = 0; // coding of the code2break is counted as a turn !
static uint8_t code2break[NUMBER_OF_PINS];
static hintPins key;
static gameCode attempt;
static uint8_t pin_num_from_attempt = 0;


//Function called after each codebreaker attempt to guess the gamecode
//Updates the key given an attempt and a code2break
static void guessCode (void){
	turnCounter++;

	if (turnCounter > MAX_NUM_TURN+1) {
		key.victory_state=GAME_OVER;
	}
	else{
		uint8_t b_key = 0; //good color good position
		uint8_t w_key = 0; //good color wrong position

		uint8_t attempt_[NUMBER_OF_PINS];
		attempt_[0] = attempt.pin1;
		attempt_[1] = attempt.pin2;
		attempt_[2] = attempt.pin3;

		for(uint8_t i=0; i < NUMBER_OF_PINS; i++){
			if (attempt_[i]==code2break[i])
				b_key++;
			else if ((attempt_[i]==code2break[(i+1)%3]) && (attempt_[(i+1)%3]!=code2break[(i+1)%3]))
				w_key++;
			else if ((attempt_[i]==code2break[(i+2)%3]) && (attempt_[(i+2)%3]!=code2break[(i+2)%3]))
				w_key++;
		}

		if (b_key == NUMBER_OF_PINS){
			key.victory_state=GAME_WON;
			key.b_key = b_key;
			key.w_key = w_key;
		}
		else {
			key.victory_state=GAME_CONTINUES;
			key.b_key = b_key;
			key.w_key = w_key;
		}
	}
}

static void displayCard(uint8_t card){//displays the given card with RGB LEDs
	if(card == COLOR_BLUE_RED){
		set_rgb_led(LED6, 0, 0, 255);
		set_rgb_led(LED4, 255, 0, 0);
	}
	else if(card ==COLOR_GREEN_RED){
		set_rgb_led(LED6, 0, 255, 0);
		set_rgb_led(LED4, 255, 0, 0);
	}
	else if(card ==COLOR_RED_BLUE){
		set_rgb_led(LED6, 255, 0, 0);
		set_rgb_led(LED4, 0, 0, 255);
	}
	else if(card == COLOR_RED_GREEN){
		set_rgb_led(LED6, 255, 0, 0);
		set_rgb_led(LED4,0,255,0);
	}
	else if(card == COLOR_RED_RED){
		set_rgb_led(LED6, 255, 0, 0);
		set_rgb_led(LED4,255,0,0);
	}
}


//-----------------GETTERS AND SETTERS------------------------------

void setAttemptPin(uint8_t currentPin){
	if (turnCounter==CODING_TURN){
		//displayCard(currentPin);
		chprintf((BaseSequentialStream *)&SD3, "% Current pin for code :  %-7d\r\n", currentPin);
		switch (pin_num_from_attempt+1) {
		case 1:
			code2break[pin_num_from_attempt]=currentPin;
			pin_num_from_attempt++;
			break;
		case 2:
			code2break[pin_num_from_attempt]=currentPin;
			pin_num_from_attempt++;
			break;
		case 3:
			code2break[pin_num_from_attempt]=currentPin;
			pin_num_from_attempt=0;

			//other initialisations :
			key.b_key=0;
			key.w_key=0;
			key.victory_state=0;

			attempt.pin1 =0;
			attempt.pin2 =0;
			attempt.pin3 =0;
			turnCounter++;
			break;
		}
	}
	else if(turnCounter==FIRST_GUESSING_TURN){
		displayCard(currentPin);
		chprintf((BaseSequentialStream *)&SD3, "% Current pin for first guess :  %-7d\r\n", currentPin);
		switch (pin_num_from_attempt+1){
		case 1:
			attempt.pin3=currentPin;
			pin_num_from_attempt++;
			break;
		case 2:
			attempt.pin2=currentPin;
			pin_num_from_attempt++;
			break;
		case 3:
			attempt.pin1=currentPin;
			pin_num_from_attempt=0;
			guessCode();
			break;
		}
	}
	else if (turnCounter>=GUESSING_TURN){
		displayCard(currentPin);
		chprintf((BaseSequentialStream *)&SD3, "% Current pin for guesses :  %-7d\r\n", currentPin);
		switch (pin_num_from_attempt+1) {
		case 1:
			attempt.pin1=currentPin;
			pin_num_from_attempt++;
			break;
		case 2:
			attempt.pin2=currentPin;
			pin_num_from_attempt++;
			break;
		case 3:
			attempt.pin3=currentPin;
			pin_num_from_attempt=0;
			guessCode();
			break;
		}
	}

}

hintPins getHints(void){
	return key;
}

void setGamecode(uint8_t pin1_, uint8_t pin2_, uint8_t pin3_){//used for initializing a full code2break without scanning
	code2break[0] = pin1_;
	code2break[1] = pin2_;
	code2break[2] = pin3_;
}

unsigned int getTurnCounter(void){
	return turnCounter;
}

void resetTurnCounter(){
	pin_num_from_attempt=0;
	turnCounter=0;
}

void setRandomGamecode(void){
	uint32_t randomNum = chVTGetSystemTime(); //SystemTime is our basis for randomness
	uint8_t increment = randomNum%20;
	uint8_t pin1 = increment%5;
	uint8_t pin2 = (2*increment)%5;
	uint8_t pin3 = (increment%10)/2;

	setGamecode(pin1, pin2, pin3);
}
