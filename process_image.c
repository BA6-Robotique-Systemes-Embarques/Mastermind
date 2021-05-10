#include "ch.h"
#include "hal.h"
#include <chprintf.h>
#include <usbcfg.h>

#include <main.h>
#include <camera/po8030.h>

#include <process_image.h>
#include <run.h>
#include <leds.h>
#include <game_logic.h>

static int pos; //Centre de la ligne noire, axe centré au milieu de l'image (pixel N° 320)


//---------------GETTERS---------------
int getPos(void){
	return pos;
}


//---------------Image Calculations---------------
int pos_width(uint8_t* image, float mean){
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
		return (right+left)/2-IMAGE_BUFFER_SIZE/2; // axe des x centré au milieu de l'image (pixel N° 320)
	}
	else{
		return 0;
	}
	//chprintf((BaseSequentialStream *)&SDU1, "% Left= %-7d % Right= %-7d\r\n", left, right);
}

uint8_t colorOfPixel(uint8_t* Pixel){
	bool redB=0, greenB=0, blueB=0;

	uint8_t mean=(Pixel[RED]+Pixel[GREEN]+Pixel[BLUE])/3;

	if (Pixel[RED]>mean) redB=true;
	if (Pixel[GREEN]>mean) greenB=true;
	if (Pixel[BLUE]>mean) blueB=true;

	if (redB && greenB){
		if (Pixel[RED]>Pixel[GREEN]) return RED;
		else return GREEN;
	}

	if (redB && blueB){
		if (Pixel[RED]>Pixel[BLUE]) return RED;
		else return BLUE;
	}

	if (blueB && greenB){
		if (Pixel[BLUE]>Pixel[GREEN]) return BLUE;
		else return GREEN;
	}

	if (redB) return RED;
	else if (greenB) return GREEN;
	else if (blueB) return BLUE;
	else return RED;			//ARBITRARY COLOR, THIS RETURN IS IF NO COLOR WAS DETECTED
}

uint8_t colorOfCard(uint8_t* Pixel1, uint8_t* Pixel2){
	if(Pixel1[RED]-Pixel2[RED]<5 && Pixel1[GREEN]-Pixel2[GREEN]<5 && Pixel1[BLUE]-Pixel2[BLUE]<5){
		return COLOR_RED_RED;//si on observe une cloche pour les 3 couleurs
	}
	else{
		uint8_t leftColor=colorOfPixel(Pixel1);
		uint8_t rightColor=colorOfPixel(Pixel2);
		if (leftColor==RED && rightColor==GREEN){
			return COLOR_RED_GREEN;
			//chprintf((BaseSequentialStream *)&SDU1, "RedGreen");
			//set_rgb_led(LED2, 255, 255, 0);
		}
		else if (leftColor==GREEN && rightColor==RED){
			return COLOR_GREEN_RED;
			//chprintf((BaseSequentialStream *)&SDU1, "RedGreen");
			//set_rgb_led(LED2, 255, 255, 0);
		}
		else if (leftColor==RED && rightColor==BLUE){
			return COLOR_RED_BLUE;
			//chprintf((BaseSequentialStream *)&SDU1, "RedBlue");
			//set_rgb_led(LED2, 255, 0, 255);
		}
		else if (leftColor==BLUE && rightColor==RED){
			return COLOR_BLUE_RED;
			//chprintf((BaseSequentialStream *)&SDU1, "RedBlue");
			//set_rgb_led(LED2, 255, 0, 255);
		}
		else{
			//chprintf((BaseSequentialStream *)&SDU1, "Wrong color of card");
			set_body_led(2); //Toggle body LED to
			return COLOR_WRONG;
		}
	}
}

//---------------Semaphore---------------
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


    while(1){
    		//waits until an image has been captured
        chBSemWait(&image_ready_sem);
		//gets the pointer to the array filled with the last image in RGB565    
		img_buff_ptr = dcmi_get_last_image_ptr();

		if(getEtat()==ETAT_FOLLOW){
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

			pos=pos_width(imageG, meanG); //axe des x centré au milieu de l'image (pixel N° 320)
			//chprintf((BaseSequentialStream *)&SDU1, "% position  %-7d\r\n", pos);
			//chprintf((BaseSequentialStream *)&SDU1, "% BlueCentre = %-7d % BleuComparaison = %-7d\r\n", imageB[pos+IMAGE_BUFFER_SIZE/2], (int)meanB);

			if(((float)imageB[pos+IMAGE_BUFFER_SIZE/2]<0.8*meanB) && ((float)imageR[pos+IMAGE_BUFFER_SIZE/2]>1.3*meanR)){
				setEtat(ETAT_GAMEHINT);//si au milieu de la ligne on a un du rouge
			}
			else if(((float)imageB[pos+IMAGE_BUFFER_SIZE/2]>1.1*meanB) && ((float)imageR[pos+IMAGE_BUFFER_SIZE/2]<0.8*meanR) && !getIgnoreScan()){
				setEtat(ETAT_SCAN);//si au milieu de la ligne on a un du bleu
			}
		}
		else if(getEtat()==ETAT_SCAN && get_objectInFront() && getReadytoScan()){
			//Extract the colors of 2 pixels in order to find the colors of the card
			uint8_t leftPixel[RGB] = {0};
			uint8_t rightPixel[RGB] = {0};

			/*float meanR=0;
			float meanG=0;
			float meanB=0;

			for(uint16_t i=0; i<IMAGE_BUFFER_SIZE/4; i+=4){		//RED mean
				meanR+=(((uint8_t)(img_buff_ptr[2*i]) & 0xF8)>>3)/2;}

			for(uint16_t i=0; i<IMAGE_BUFFER_SIZE/4; i+=4){		//GREEN mean
				meanG+= ((*(img_buff_ptr+2*i) & 0b00000111)<<3) | ((*(img_buff_ptr+2*i+1)) >>5);}

			for(uint16_t i=0; i<(2*IMAGE_BUFFER_SIZE/4); i+=8){	//BLUE mean
				meanB+=((uint8_t)img_buff_ptr[i+1] & 0x1F)/2;}

			meanR/=IMAGE_BUFFER_SIZE/4;
			meanG/=IMAGE_BUFFER_SIZE/4;
			meanB/=IMAGE_BUFFER_SIZE/4;*/

			unsigned int i = 1*IMAGE_BUFFER_SIZE/4; // left position
			leftPixel[RED]= ((uint8_t)(img_buff_ptr[2*i]) & 0xF8)>>3;
			leftPixel[GREEN] = (((*(img_buff_ptr+2*i) & 0b00000111)<<3) | ((*(img_buff_ptr+2*i+1)) >>5))/2;
			leftPixel[BLUE] = (uint8_t)img_buff_ptr[(2*i)+1]&0x1F;

			i = 3*IMAGE_BUFFER_SIZE/4; // right position
			rightPixel[RED]=((uint8_t)(img_buff_ptr[2*i]) & 0xF8)>>3;
			rightPixel[GREEN] = (((*(img_buff_ptr+2*i) & 0b00000111)<<3) | ((*(img_buff_ptr+2*i+1)) >>5))/2;
			rightPixel[BLUE] = (uint8_t)img_buff_ptr[(2*i)+1] & 0x1F;

			//send the color information to run module
			set_currentCard(colorOfCard(leftPixel, rightPixel));
		}
		//affichage ordinateur :
		if(envoi){
			SendUint8ToComputer(imageR, IMAGE_BUFFER_SIZE);
		}
		envoi = !envoi;
    }
}

void process_image_start(void){
	chThdCreateStatic(waProcessImage, sizeof(waProcessImage), NORMALPRIO, ProcessImage, NULL);
	chThdCreateStatic(waCaptureImage, sizeof(waCaptureImage), NORMALPRIO, CaptureImage, NULL);
	pos=0;
}
