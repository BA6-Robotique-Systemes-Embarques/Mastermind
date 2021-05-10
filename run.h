#ifndef PI_REGULATOR_H
#define PI_REGULATOR_H

#define ETAT_FOLLOW 'N'
#define ETAT_SCAN 'B'
#define ETAT_GAMEHINT 'R'
#define ETAT_PAUSE 'P'

//start the PI regulator thread
void run_thd_start(void);
void starting_move(void);

//Getters et setters :
char getEtat(void);
void setEtat(char c);

bool getReadytoScan(void);

bool get_objectInFront(void);
bool getIgnoreScan(void);
void set_objectInFront(bool object);

void set_currentCard(uint8_t card);

#endif /* PI_REGULATOR_H */
