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
#include <selector.h>
#include "spi_comm.h"

#include <detectionIR.h>
#include <process_image.h>
#include <run.h>
#include <main.h>
#include <affichage.h>
#include <game_logic.h>

#define START_GAME		1
#define STOP_GAME		2
#define RESTART_GAME	3
#define SOLO_GAME		15

messagebus_t bus;
MUTEX_DECL(bus_lock);
CONDVAR_DECL(bus_condvar);

parameter_namespace_t parameter_root, aseba_ns;


/*void SendUint8ToComputer(uint8_t* data, uint16_t size){
	chSequentialStreamWrite((BaseSequentialStream *)&SD3, (uint8_t*)"START", 5);
	chSequentialStreamWrite((BaseSequentialStream *)&SD3, (uint8_t*)&size, sizeof(uint16_t));
	chSequentialStreamWrite((BaseSequentialStream *)&SD3, (uint8_t*)data, size);
}*/

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

    serial_start();//Starts the serial communication
    usb_start();//Starts the USB communication
    //Starts the camera :
    dcmi_start();
	po8030_start();
	po8030_set_awb(false); //Disables the camera's automatic white balance

	//Initialises the motors and starts moving
	motors_init();
	spi_comm_start(); //Starts the spi communication for the RGB LEDs

	set_rgb_led(LED4, 0, 200, 200);
	set_rgb_led(LED6, 0, 200, 200);

	uint8_t selector=0, old_selector=0;
    while (1) {
    		selector= get_selector();
    		if(selector != old_selector && selector == START_GAME){ //First start
    			old_selector=selector;
    			starting_move();
    			initializethreads();
    		}
    		else if(selector != old_selector && selector == STOP_GAME){ //Stops the threads
    			old_selector=selector;
    			setEtat(ETAT_STOP);
    			stopMotors();
    			set_body_led(0);
    			set_rgb_led(LED4, 0, 0, 0);//Clears the RGB LEDs
    			set_rgb_led(LED6, 0, 0, 0);
    		}
    		else if(selector != old_selector && selector == RESTART_GAME){ //Restart
    			old_selector=selector;
    			resetTurnCounter();
    			starting_move();
    			setEtat(ETAT_FOLLOW);
    		}
    		if(selector != old_selector && selector == SOLO_GAME){
    			old_selector=selector;

    			setsoloMode(true);
    			starting_move();
    			initializethreads();
    		}

        chThdSleepMilliseconds(1000); //Waits 1 second
    }
}

void initializethreads(void){
	//Starts the IR sensors and thread
	proximity_start();
	calibrate_ir();

	//Starts the thread used later
	IR_thd_start();//Proximity detection, used for start signal detection and card recognition
	process_image_start();//Image capture and analysis
	run_thd_start();//Main thread : responsible of motors and thread coordination
	display_start();//affichage des indices de jeu sur les LEDS
}

#define STACK_CHK_GUARD 0xe2dee396
uintptr_t __stack_chk_guard = STACK_CHK_GUARD;

void __stack_chk_fail(void){
    chSysHalt("Stack smashing detected");
}
