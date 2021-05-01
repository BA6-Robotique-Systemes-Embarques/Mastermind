#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

//code de couleurs par moitié de carte :
#define COLOR_RED_RED 0
#define COLOR_GREEN_GREEN 1
#define COLOR_BLUE_BLUE 2
#define COLOR_RED_GREEN 3
#define COLOR_RED_BLUE 4

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

void setAttempt(uint8_t pin1, uint8_t pin2, uint8_t pin3); //sets the guess attempt
void setGamecode(gameCode code); //Sets the code that has to be guessed
hintPins getHints(void);

void resetTurnCounter(void); //self-explanatory
unsigned int getTurnCounter(void);

void setAttemptPin(uint8_t currentPin);//dynamically adds the color of the pin seen by the camera to the attempt

#endif
