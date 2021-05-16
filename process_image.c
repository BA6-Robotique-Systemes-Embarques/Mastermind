/*
 *  process_image.c
 *
 *  Created on: 20 April 2021
 *      Author: Emile Chevrel
 */

#include "ch.h"
#include "hal.h"
#include <usbcfg.h>

#include <main.h>
#include <camera/po8030.h>

#include <process_image.h>
#include <run.h>
#include <leds.h>
#include <game_logic.h>

#define IMAGE_BUFFER_SIZE		640

#define LEFT_COEF	0.3
#define RIGHT_COEF	0.7

static int pos; //Center of the black line, relative to the middle of the image (pixel N° 320)


//---------------Image Calculations---------------
static int findPosition(uint8_t* image, float mean){
	int left=0; //left and right points defining the width of the black line
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

	//safety checks
	if(abs(pos-((right+left)/2-IMAGE_BUFFER_SIZE/2))<200  || pos==-IMAGE_BUFFER_SIZE/2){
		return (right+left)/2-IMAGE_BUFFER_SIZE/2; // x axis' origin is located at pixel N° 320
	}
	else return 0;
}

uint8_t colorOfCard(uint8_t* Pixel1, uint8_t* Pixel2){
	uint8_t mean1=(Pixel1[RED]+Pixel1[GREEN]+Pixel1[BLUE])/3;
	uint8_t mean2=(Pixel2[RED]+Pixel2[GREEN]+Pixel2[BLUE])/3;

	if(1.4*Pixel1[RED]<Pixel2[RED]){
		if(Pixel1[BLUE]>mean1) return COLOR_BLUE_RED;
		else return COLOR_GREEN_RED;
	}
	else if(Pixel1[RED]>1.4*Pixel2[RED]){
		if(Pixel2[BLUE]>mean2) return COLOR_RED_BLUE;
		else return COLOR_RED_GREEN;
	}
	else return COLOR_RED_RED;
}

//---------------Semaphore---------------
static BSEMAPHORE_DECL(image_ready_sem, TRUE);

static THD_WORKING_AREA(waCaptureImage, 256);
static THD_FUNCTION(CaptureImage, arg) {

    chRegSetThreadName(__FUNCTION__);
    (void)arg;
    bool capture = true;

	//Takes pixels 0 to IMAGE_BUFFER_SIZE of the line 470 + 471
	po8030_advanced_config(FORMAT_RGB565, 0, 470, IMAGE_BUFFER_SIZE, 2, SUBSAMPLING_X1, SUBSAMPLING_X1);
	dcmi_enable_double_buffering();
	dcmi_set_capture_mode(CAPTURE_ONE_SHOT);
	dcmi_prepare();
    while(1){
    		if(capture){
    			//starts a capture
    			dcmi_capture_start();
    			//waits for the capture to be done
    			wait_image_ready();
    			//signals an image has been captured

    			chBSemSignal(&image_ready_sem);
    		}
    		else chThdYield();
    		capture = !capture;
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

    while(1){
        chBSemWait(&image_ready_sem); //waits until an image has been captured

		img_buff_ptr = dcmi_get_last_image_ptr(); //gets the pointer to the array filled with the last image in RGB565

		if(getEtat()==ETAT_FOLLOW){
			float meanR=0, meanG=0, meanB=0;

			//Extracts only the red pixels
			for(uint16_t i=0; i<IMAGE_BUFFER_SIZE; i++){
				//extracts first 5bits of the first byte
				//takes nothing from the second byte
				imageR[i] =((uint8_t)(img_buff_ptr[2*i]) & 0xF8)>>3;
				meanR+=imageR[i/2];
			}

			//Extracts only the green pixels
			for(uint16_t i=0; i<IMAGE_BUFFER_SIZE; i++){
				imageG[i] = ((*(img_buff_ptr+2*i) & 0b00000111)<<3) | ((*(img_buff_ptr+2*i+1)) >>5);
				meanG+=imageG[i];
			}

			//Extracts only the blue pixels
			for(uint16_t i=0; i<(2*IMAGE_BUFFER_SIZE); i+=2){
				//extracts last 5bits of the second byte
				//takes nothing from the first byte
				imageB[i/2] = (uint8_t)img_buff_ptr[i+1]&0x1F;
				meanB+=imageB[i/2];
			}

			meanR/=IMAGE_BUFFER_SIZE;
			meanG/=IMAGE_BUFFER_SIZE;
			meanB/=IMAGE_BUFFER_SIZE;

			pos=findPosition(imageG, meanG); //finds the position of the black line uing the green information

			if(((float)imageB[pos+IMAGE_BUFFER_SIZE/2]<0.7*meanB) && ((float)imageR[pos+IMAGE_BUFFER_SIZE/2]>1.1*meanR)){
				setEtat(ETAT_GAMEHINT);//if the middle of the line contains red
			}
			else if(((float)imageB[pos+IMAGE_BUFFER_SIZE/2]>0.9*meanB) && ((float)imageR[pos+IMAGE_BUFFER_SIZE/2]<0.8*meanR) && !getIgnoreScan()){
				setEtat(ETAT_SCAN);//if the middle of the line contains blue
				if(getTurnCounter()!=0 || !getSoloMode()) set_body_led(ON);
			}
		}
		else if(getEtat()==ETAT_SCAN && getObjectInFront() && getReadytoScan()){
			//Extract the colors of 2 pixels in order to find the colors of the card
			uint8_t leftPixel[RGB] = {0};
			uint8_t rightPixel[RGB] = {0};

			unsigned int i = LEFT_COEF * IMAGE_BUFFER_SIZE; // left position
			leftPixel[RED]= ((uint8_t)(img_buff_ptr[2*i]) & 0xF8)>>3;
			leftPixel[GREEN] = (((*(img_buff_ptr+2*i) & 0b00000111)<<3) | ((*(img_buff_ptr+2*i+1)) >>5))/2;
			leftPixel[BLUE] = (uint8_t)img_buff_ptr[(2*i)+1]&0x1F;

			i = RIGHT_COEF * IMAGE_BUFFER_SIZE; // right position
			rightPixel[RED]=((uint8_t)(img_buff_ptr[2*i]) & 0xF8)>>3;
			rightPixel[GREEN] = (((*(img_buff_ptr+2*i) & 0b00000111)<<3) | ((*(img_buff_ptr+2*i+1)) >>5))/2;
			rightPixel[BLUE] = (uint8_t)img_buff_ptr[(2*i)+1] & 0x1F;

			//send the color information to run module
			setCurrentCard(colorOfCard(leftPixel, rightPixel));
		}
    }
}

void process_image_start(void){
	chThdCreateStatic(waProcessImage, sizeof(waProcessImage), NORMALPRIO, ProcessImage, NULL);
	chThdCreateStatic(waCaptureImage, sizeof(waCaptureImage), NORMALPRIO, CaptureImage, NULL);
}

//---------------GETTERS AND SETTERS---------------
int getPos(void){
	return pos;
}
