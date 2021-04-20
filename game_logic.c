#include <stdio.h>
#include <string.h>
#include "ch.h"
#include "hal.h"
#include <main.h>
#include <game_logic.h>

#define MAX_NUM_TURN	8

#define GAME_CONTINUES	0
#define GAME_WON		1
#define GAME_OVER		2

#define NUMBER_OF_PINS	3

static char code2break[NUMBER_OF_PINS] ;
static unsigned int turnCounter = 0;


void setGamecode(gameCode code){
	code2break[0] = code.pin1;
	code2break[1] = code.pin2;
	code2break[2] = code.pin3;
}

void resetTurnCounter(){
	turnCounter=0;
}

hintPins guessCode (gameCode attempt){
	turnCounter++;

	hintPins key;  //initialize the return structure
	key.b_key = 0; //good color+position
	key.w_key = 0; //good color wrong position

	if (turnCounter > MAX_NUM_TURN) {
		key.victory_state=GAME_OVER;
		return key;
	}

	char attempt_[NUMBER_OF_PINS];
	attempt_[0] = attempt.pin1;
	attempt_[1] = attempt.pin2;
	attempt_[2] = attempt.pin3;

	for (int i=0; i < NUMBER_OF_PINS; i++)
	{
		if (attempt_[i]==code2break[i])
			key.b_key++;
		else if ((attempt_[i]==code2break[(i+1)%3]) || (attempt_[i]==code2break[(i+2)%3]))
			key.w_key++;
	}

	if (key.b_key == NUMBER_OF_PINS) {
		key.victory_state=GAME_WON;
		return key;
	}
	else {
		key.victory_state=GAME_CONTINUES;
		return key;
	}
}
