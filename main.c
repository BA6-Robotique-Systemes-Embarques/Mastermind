#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "ch.h"
#include "hal.h"
#include <pal.h>
#include "memory_protection.h"
#include <usbcfg.h>
#include <motors.h>
#include <camera/po8030.h>
#include <chprintf.h>
#include <sensors/proximity.h>
#include <leds.h>

#include <detectionIR.h>
#include <process_image.h>
#include <run.h>
#include <main.h>
#include <affichage.h>
#include <game_logic.h>


messagebus_t bus;
MUTEX_DECL(bus_lock);
CONDVAR_DECL(bus_condvar);

parameter_namespace_t parameter_root, aseba_ns;


void SendUint8ToComputer(uint8_t* data, uint16_t size){
	chSequentialStreamWrite((BaseSequentialStream *)&SD3, (uint8_t*)"START", 5);
	chSequentialStreamWrite((BaseSequentialStream *)&SD3, (uint8_t*)&size, sizeof(uint16_t));
	chSequentialStreamWrite((BaseSequentialStream *)&SD3, (uint8_t*)data, size);
}

static void serial_start(void){
	static SerialConfig ser_cfg = {
	    115200,
	    0,
	    0,
	    0,
	};

	sdStart(&SD3, &ser_cfg); // UART3.
}

int main(void){
    halInit();
    chSysInit();
    mpu_init();

    messagebus_init(&bus, &bus_lock, &bus_condvar);
    parameter_namespace_declare(&parameter_root, NULL, NULL);

    //starts the serial communication
    serial_start();
    //start the USB communication
    usb_start();
    //starts the camera
    dcmi_start();
	po8030_start();
	//start the IR sensors and thread
	proximity_start();
	calibrate_ir();
	//init the motors
	motors_init();

	//starting_move();
	/*setEtat('P');
	gameCode code;
	code.pin1=COLOR_RED_RED;
	code.pin2=COLOR_GREEN_GREEN;
	code.pin3=COLOR_RED_BLUE;
	setGamecode(code);
	setAttempt(COLOR_RED_RED,COLOR_GREEN_GREEN,COLOR_RED_BLUE);*/
	//starts the thread used later
	IR_thd_start();//détection de proximité, utilisé notamment pour le départ avec le signal de la main
	run_thd_start();//thread générale du jeu : gère l'état du jeu et éventuellement les moteurs
	process_image_start();//gère la capture d'image et son analyse
	//affichage_start();//affichage des indices de jeu sur les LEDS

    //Infinite loop
    while (1) {
    	//Waits 1 second
        chThdSleepMilliseconds(1000);
    }
}

#define STACK_CHK_GUARD 0xe2dee396
uintptr_t __stack_chk_guard = STACK_CHK_GUARD;

void __stack_chk_fail(void){
    chSysHalt("Stack smashing detected");
}
