#include "ch.h"
#include "hal.h"
#include <chprintf.h>
#include <usbcfg.h>

#include <main.h>
#include <camera/po8030.h>

#include <process_image.h>


static float distance_cm = 0;
static unsigned int  width;
static int pos;
static char etat = 'N'; //N = lighe noir, R = pastille rouge (arrêt), B = pastille bleue (Lire carte)

//GETTERS :
float getDistanceCM(void){
	return distance_cm;
}

char getEtat(void){
	return etat;
}

int getPos(void){
	return pos;
}

void convert(float width){
	//distance_cm= 27.03 - 0.1226*width;
	distance_cm=2*738.1/width;
}

void pos_width(uint8_t* image, float mean){
	int left=0;
	int right=0;

	for(unsigned int i=0; i<IMAGE_BUFFER_SIZE; i++){
		if(image[i]>mean && left==0){
			if(image[i+5]<0.7*mean && image[i+10]<0.7*mean ){
				left=i+5;
				i+=10;
			}
		}
		if(image[i]>mean && left!=0 && right==0 && image[i+10]>mean){
			right=i;
			i=IMAGE_BUFFER_SIZE-1;
		}
	}

	width = right-left;
	pos=(right+left)/2-IMAGE_BUFFER_SIZE/2; // axe des x centré au milieu de l'image (pixel N° 320)
	convert((float)width);
	//chprintf((BaseSequentialStream *)&SDU1, "%Width=%-7d %Distance=%-7d\r\n", width, distance_cm);
}

//semaphore
static BSEMAPHORE_DECL(image_ready_sem, TRUE);

static THD_WORKING_AREA(waCaptureImage, 256);
static THD_FUNCTION(CaptureImage, arg) {

    chRegSetThreadName(__FUNCTION__);
    (void)arg;

	//Takes pixels 0 to IMAGE_BUFFER_SIZE of the line 10 + 11 (minimum 2 lines because reasons)
	po8030_advanced_config(FORMAT_RGB565, 0, 10, IMAGE_BUFFER_SIZE, 2, SUBSAMPLING_X1, SUBSAMPLING_X1);
	dcmi_enable_double_buffering();
	dcmi_set_capture_mode(CAPTURE_ONE_SHOT);
	dcmi_prepare();
	int time_start;
	int total_time;
    while(1){
    		//time_start = chVTGetSystemTime();
        //starts a capture
		dcmi_capture_start();
		//waits for the capture to be done
		wait_image_ready();
		//signals an image has been captured
		//total_time = chVTGetSystemTime()-time_start;
		chBSemSignal(&image_ready_sem);
		//chprintf((BaseSequentialStream *)&SDU1, "%Elapsed time=%-7d\r\n", total_time);
    }
}


static THD_WORKING_AREA(waProcessImage, 1024);
static THD_FUNCTION(ProcessImage, arg) {

    chRegSetThreadName(__FUNCTION__);
    (void)arg;

	uint8_t *img_buff_ptr;
	uint8_t image[IMAGE_BUFFER_SIZE] = {0};
	bool envoi =0;
	uint8_t *imagePython;


    while(1){
    		//waits until an image has been captured
        chBSemWait(&image_ready_sem);
		//gets the pointer to the array filled with the last image in RGB565    
		img_buff_ptr = dcmi_get_last_image_ptr();
		float mean=0;
		for(unsigned int i=0; i<IMAGE_BUFFER_SIZE; i++){
			image[i] = ((*(img_buff_ptr+2*i) & 0b00000111)<<3) | ((*(img_buff_ptr+2*i+1)) >>5);
			mean+=image[i];
		}
		mean/=IMAGE_BUFFER_SIZE;
		pos_width(image, mean);


		imagePython=&image[0];

		if (envoi){
			SendUint8ToComputer(imagePython, IMAGE_BUFFER_SIZE);
		}
		envoi = !envoi;
    }
}

void process_image_start(void){
	chThdCreateStatic(waProcessImage, sizeof(waProcessImage), NORMALPRIO, ProcessImage, NULL);
	chThdCreateStatic(waCaptureImage, sizeof(waCaptureImage), NORMALPRIO, CaptureImage, NULL);
}
