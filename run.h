#ifndef PI_REGULATOR_H
#define PI_REGULATOR_H

//start the PI regulator thread
void run_thd_start(void);
void starting_move(void);

//Getters et setters :
char getEtat(void);
void setEtat(char c);

#endif /* PI_REGULATOR_H */
