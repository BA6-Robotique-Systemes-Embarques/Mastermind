/*
 * run.h
 *
 *  Created on: 20 April 2021
 *      Author: Cyril Monette
 */

#ifndef PI_REGULATOR_H
#define PI_REGULATOR_H

#define ETAT_FOLLOW 'N'
#define ETAT_SCAN 'B'
#define ETAT_GAMEHINT 'R'
#define ETAT_PAUSE 'P'
#define ETAT_STOP 'S'


void run_thd_start(void); //starts the run thread
void starting_move(void);
void stopMotors(void);

//Getters and setters :
char getEtat(void);
void setEtat(char c);

bool getReadytoScan(void);
bool getObjectInFront(void);
bool getIgnoreScan(void);
void setObjectInFront(bool object);
void setSoloMode(bool mode);
bool getSoloMode(void);
void setCurrentCard(uint8_t card);

#endif /* PI_REGULATOR_H */
