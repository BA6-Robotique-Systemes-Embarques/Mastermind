#include "ch.h"
#include "hal.h"
#include <chprintf.h>
#include <usbcfg.h>

#include <main.h>
#include <camera/po8030.h>

#include <process_image.h>
#include <run.h>


static float distance_cm = 0;
static int pos;


//GETTERS :
float getDistanceCM(void){
	return distance_cm;
}

int getPos(void){
	return pos;
}

void convert(float width){
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

	//pour éviter les bugs :
	if(abs(pos-((right+left)/2-IMAGE_BUFFER_SIZE/2))<200  || pos==-IMAGE_BUFFER_SIZE/2){
		pos=(right+left)/2-IMAGE_BUFFER_SIZE/2; // axe des x centré au milieu de l'image (pixel N° 320)
	}
	//convert((float)width);
	//chprintf((BaseSequentialStream *)&SDU1, "% Left= %-7d % Right= %-7d\r\n", left, right);
}

//semaphore
static BSEMAPHORE_DECL(image_ready_sem, TRUE);

static THD_WORKING_AREA(waCaptureImage, 256);
static THD_FUNCTION(CaptureImage, arg) {

    chRegSetThreadName(__FUNCTION__);
    (void)arg;

	//Takes pixels 0 to IMAGE_BUFFER_SIZE of the line 470 + 471 (minimum 2 lines because reasons)
	po8030_advanced_config(FORMAT_RGB565, 0, 470, IMAGE_BUFFER_SIZE, 2, SUBSAMPLING_X1, SUBSAMPLING_X1);
	dcmi_enable_double_buffering();
	dcmi_set_capture_mode(CAPTURE_ONE_SHOT);
	dcmi_prepare();
	//int time_start;
	//int total_time;
    while(1){
        //starts a capture
		dcmi_capture_start();
		//waits for the capture to be done
		wait_image_ready();
		//signals an image has been captured
		chBSemSignal(&image_ready_sem);
    }
}


static THD_WORKING_AREA(waProcessImage, 4*1024);
static THD_FUNCTION(ProcessImage, arg) {

    chRegSetThreadName(__FUNCTION__);
    (void)arg;

	uint8_t *img_buff_ptr;
	uint8_t imageR[IMAGE_BUFFER_SIZE] = {0};
	uint8_t imageG[IMAGE_BUFFER_SIZE] = {0};
	uint8_t imageB[IMAGE_BUFFER_SIZE] = {0};
	bool envoi =0;
	//uint8_t *imagePython;


    while(1){
    		//waits until an image has been captured
        chBSemWait(&image_ready_sem);
		//gets the pointer to the array filled with the last image in RGB565    
		img_buff_ptr = dcmi_get_last_image_ptr();

		if(getEtat()=='N'){
			float meanR=0;
			float meanG=0;
			float meanB=0;

			//Extracts only the red pixels
			for(uint16_t i=0; i<IMAGE_BUFFER_SIZE; i++){
				//extracts first 5bits of the first byte
				//takes nothing from the second byte
				imageR[i] =((uint8_t)(img_buff_ptr[2*i]) & 0xF8)>>3;
				//chprintf((BaseSequentialStream *)&SDU1, "% imageRed  %-7d\r\n", imageR[i]);
				meanR+=imageR[i/2];
				}

			//Extracts only the green pixels
			for(uint16_t i=0; i<IMAGE_BUFFER_SIZE; i++){
				imageG[i] = ((*(img_buff_ptr+2*i) & 0b00000111)<<3) | ((*(img_buff_ptr+2*i+1)) >>5);
				meanG+=imageG[i];
			}

			//Extracts only the blue pixels
			for(uint16_t i=0; i<(2*IMAGE_BUFFER_SIZE); i+=2){
				//extracts las 5bits of the second byte
				//takes nothing from the first byte
				imageB[i/2] = (uint8_t)img_buff_ptr[i+1]&0x1F;
				meanB+=imageB[i/2];
			}

			meanR/=IMAGE_BUFFER_SIZE;
			meanG/=IMAGE_BUFFER_SIZE;
			meanB/=IMAGE_BUFFER_SIZE;
			//chprintf((BaseSequentialStream *)&SDU1, "% MeanBlue= %-7d\r\n", (int)meanG);

			pos_width(imageG, meanG);

			if((imageB[pos+IMAGE_BUFFER_SIZE/2]<meanB) && (imageR[pos+IMAGE_BUFFER_SIZE/2]>meanR)){
				setEtat('R');//si au milieu de la ligne on a un du rouge
			}
			else if((imageB[pos+IMAGE_BUFFER_SIZE/2]>meanB) && (imageR[pos+IMAGE_BUFFER_SIZE/2]<meanR)){
				setEtat('B');//si au milieu de la ligne on a un du bleu
			}
		}

		//affichage ordinateur :
		/*if (envoi){
			SendUint8ToComputer(imageB, IMAGE_BUFFER_SIZE);
		}
		envoi = !envoi;*/
    }
}

void process_image_start(void){
	chThdCreateStatic(waProcessImage, sizeof(waProcessImage), NORMALPRIO, ProcessImage, NULL);
	chThdCreateStatic(waCaptureImage, sizeof(waCaptureImage), NORMALPRIO, CaptureImage, NULL);
}
