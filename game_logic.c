#include <stdio.h>
#include <string.h>
#include "ch.h"
#include "hal.h"
#include <main.h>
#include <game_logic.h>

#define MAX_NUM_TURN	 8

#define GAME_CONTINUES	0
#define GAME_WON		1
#define GAME_OVER		2

#define NUMBER_OF_PINS	3

static unsigned int turnCounter = 0;

static uint8_t code2break[NUMBER_OF_PINS];
static hintPins key;
static gameCode attempt;


//Called each time the codebreaker attempts to guess the gamecode :
//Updates the key given an attempt and a code2break
static void guessCode (void){
	turnCounter++;

	if (turnCounter > MAX_NUM_TURN) {
		key.victory_state=GAME_OVER;
	}
	else{
		uint8_t b_key = 0; //good color+position
		uint8_t w_key = 0; //good color wrong position

		uint8_t attempt_[NUMBER_OF_PINS];
		attempt_[0] = attempt.pin1;
		attempt_[1] = attempt.pin2;
		attempt_[2] = attempt.pin3;

		for(uint8_t i=0; i < NUMBER_OF_PINS; i++){
			if (attempt_[i]==code2break[i])
				b_key++;
			else if ((attempt_[i]==code2break[(i+1)%3]) || (attempt_[i]==code2break[(i+2)%3]))
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

//-----------------Getters and setters------------------------------
hintPins getHints(void){
	return key;
}

void setAttempt(uint8_t pin1, uint8_t pin2, uint8_t pin3){
	if(pin1>=COLOR_RED_RED && pin2>=COLOR_RED_RED && pin3>=COLOR_RED_RED &&
		pin1 <= COLOR_RED_BLUE && pin2 <= COLOR_RED_BLUE && pin3 <= COLOR_RED_BLUE){
		attempt.pin1=pin1;
		attempt.pin2=pin2;
		attempt.pin3=pin3;
	}
	guessCode();
}

void setGamecode(gameCode code){
	code2break[0] = code.pin1;
	code2break[1] = code.pin2;
	code2break[2] = code.pin3;

	//autres initialisations :
	key.b_key=0;
	key.w_key=0;
	key.victory_state=0;

	attempt.pin1 =0;
	attempt.pin2 =0;
	attempt.pin3 =0;
}

void resetTurnCounter(){
	turnCounter=0;
}
