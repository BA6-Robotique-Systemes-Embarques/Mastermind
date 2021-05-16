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


void run_thd_start(void); // Starts the run thread
void starting_move(void);
void stopMotors(void);

//-----------------------GETTERS AND SETTERS-------------------------

char getEtat(void);
void setEtat(char c);

bool getReadytoScan(void);
bool getObjectInFront(void);
void setObjectInFront(bool object);
bool getIgnoreScan(void);
bool getSoloMode(void);
void setSoloMode(bool mode);
void setCurrentCard(uint8_t card);

#endif /* PI_REGULATOR_H */
