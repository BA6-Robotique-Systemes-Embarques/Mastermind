#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

// Code structure whether it's a guess or the original code
typedef struct {
	uint8_t pin1; //number between 0 and 4 for each color
	uint8_t pin2;
	uint8_t pin3;
}gameCode;

//
typedef struct {
	uint8_t b_key; //black key peg is for each peg from the guess correct in both color and position.
	uint8_t w_key; //white key peg indicates existence of a correct color peg placed in the wrong position.

	uint8_t victory_state; //if 0 the game continues, if 1 the code has been guessed, if 2 its game over
}hintPins;


void setGamecode(gameCode code); //Set the code that will be guessed

void resetTurnCounter(void); //self-explanatory

hintPins guessCode (gameCode attempt); //Called each time the codebreaker attempts to guess the gamecode.
									   //Gamecode needs to be set beforehand, otherwise it's {000}

#endif
