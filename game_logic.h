/*
 * game_logic.h
 *
 *  Created on: 20 April 2021
 *      Author: Emile Chevrel
 */

#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

//Cards' color code, equivalent to their symbol:
#define COLOR_RED_GREEN	 	0	//Square
#define COLOR_GREEN_RED	 	1	//Star
#define COLOR_RED_BLUE		2	//Triangle
#define COLOR_BLUE_RED 		3	//Moon
#define COLOR_RED_RED 		4	//Circle
#define COLOR_WRONG 			5

#define GAME_CONTINUES			0
#define GAME_WON					1
#define GAME_OVER				2

//Structure for both the guesses and the code2break:
typedef struct {
	uint8_t pin1; //Number between 0 and 4 corresponding to each color
	uint8_t pin2;
	uint8_t pin3;
}gameCode;

typedef struct {
	uint8_t b_key; //Black key peg is for each peg from the guess correct in both color and position.
	uint8_t w_key; //White key peg indicates existence of a correct color peg placed in the wrong position.

	uint8_t victory_state; //If 0 the game continues, if 1 the code has been guessed, if 2 the game is over
}hintPins;


//-----------------------GETTERS AND SETTERS-------------------------

void setAttemptPin(uint8_t currentPin);//Dynamically adds the card of the current pin scanned by the camera to the attempt
void setRandomGamecode(void); //Randomly sets a code
hintPins getHints(void);
uint8_t getTurnCounter(void);
void resetTurnCounter(void);

#endif
