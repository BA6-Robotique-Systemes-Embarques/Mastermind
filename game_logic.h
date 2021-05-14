#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

//Cards' color code, equivalent to their symbol:
#define COLOR_RED_GREEN 0
#define COLOR_GREEN_RED 1
#define COLOR_RED_BLUE 2
#define COLOR_BLUE_RED 3
#define COLOR_RED_RED 4
#define COLOR_WRONG 5

typedef struct {
	uint8_t pin1; //number between 0 and 4 for each color
	uint8_t pin2;
	uint8_t pin3;
}gameCode;	// Code structure whether it's a guess or the original code

typedef struct {
	uint8_t b_key; //black key peg is for each peg from the guess correct in both color and position.
	uint8_t w_key; //white key peg indicates existence of a correct color peg placed in the wrong position.

	uint8_t victory_state; //if 0 the game continues, if 1 the code has been guessed, if 2 its game over
}hintPins;

//dynamically adds the color of the pin seen by the camera to the attempt
void setAttemptPin(uint8_t currentPin);

//void setAttempt(uint8_t pin1, uint8_t pin2, uint8_t pin3);

void setGamecode(gameCode code); //Sets all the code that has to be guessed at once
hintPins getHints(void);
void resetTurnCounter(void);
unsigned int getTurnCounter(void);

#endif
