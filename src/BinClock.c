/*
 * BinClock.c
 * Jarrod Olivier
 * Modified for EEE3095S/3096S by Keegan Crankshaw
 * August 2019
 * JFFMAH001 NDKNIC001
 * Date
*/

#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <stdio.h> //For printf functions
#include <stdlib.h> // For system functions
#include <math.h>
#include <softPwm.h>
#include <errno.h>

#include "BinClock.h"
#include "CurrentTime.h"

//Global variables
int hours, mins, secs;
long lastInterruptTime = 0;
int RTC; //Holds the RTC instance
int HH,MM,SS;
void initGPIO(void){
	/*
	 * Sets GPIO using wiringPi pins. see pinout.xyz for specific wiringPi pins
	 * You can also use "gpio readall" in the command line to get the pins
	 * Note: wiringPi does not use GPIO or board pin numbers (unless specifically set to that mode)
	 */
	printf("Setting up\n");
	wiringPiSetup(); //This is the default mode. If you want to change pinouts, be aware

	RTC = wiringPiI2CSetup(RTCAddr); //Set up the RTC

	//Set up the LEDS 
	for(int i=0; i < sizeof(LEDS)/sizeof(LEDS[0]); i++){
	    pinMode(LEDS[i], OUTPUT);
		digitalWrite(LEDS[i],0);
	}


	//Set Up the Seconds LED for PWM
	pinMode(1, PWM_OUTPUT);

	printf("LEDS done\n");

	//Set up the Buttons
	for(int j=0; j < sizeof(BTNS)/sizeof(BTNS[0]); j++){
		pinMode(BTNS[j], INPUT);
		pullUpDnControl(BTNS[j], PUD_UP);
	}
	//interupts
	//Attach interrupts to Buttons w/ debouncing
	unsigned long interrupt_time = millis();
  // If interrupts come faster than 200ms, assume it's a bounce and ignore
  		if (interrupt_time - lastInterruptTime > 200) 
  		{
    		wiringPiISR (27, INT_EDGE_FALLING, &hourInc); //function must still be added
 		}
  		lastInterruptTime = interrupt_time;

  	 unsigned long interrupt_time = millis();
  // If interrupts come faster than 200ms, assume it's a bounce and ignore
  		if (interrupt_time - lastInterruptTime > 200) 
  		{
   			wiringPiISR (25, INT_EDGE_FALLING, &minInc) ; //function must still be added
 		}
  		lastInterruptTime = interrupt_time;

	printf("BTNS done\n");
	printf("Setup done\n");
}

/*
 * The main function
 * This function is called, and calls all relevant functions we've written
 */
int main(void){
	initGPIO();
	toggleTime();

	// Repeat this until we shut down
	for (;;){
		//Fetch the time from the RTC
		hours = wiringPiI2CReadReg8(RTC, HOUR);
		mins = wiringPiI2CReadReg8(RTC, MIN);
		secs = wiringPiI2CReadReg8(RTC, SEC);
		secs=secs-0x80;

		hours=hexCompensation(hours);
		hours=decCompensation(hFormat(hours));
		printf("The current time is: %x:%x:%x\n", hours, mins, secs);

		//Write your logic her
		//Function calls to toggle LEDs
		lightHours(hours);
		lightMins(mins);
		secPWM(secs);

		// Print out the time we have stored on our RTC

		//using a delay to make our program "less CPU hungry"
		delay(1000); //milliseconds		printf("The current time is: %x:%x:%x\n", HH, MM, SS);

	}
	cleanup();
	return 0;
}

/*
 * Change the hour format to 12 hours
 */
int hFormat(int hours){
	/*formats to 12h*/
	if (hours >= 24){
		hours = 0;
	}
	else if (hours > 12){
		hours -= 12;
	}
	return (int)hours;
}

/*
 * Turns on corresponding LED's for hours
 */
void lightHours(int units){
	// Write your logic to light up the hour LEDs here
	int temp;
	for (int i = 0; i <=3; i++){
			temp = pow(2, 3-i); //first 4 leds being used 
			digitalWrite(LEDS[i], units/temp);
			units = units % temp;
	}
}

/*
 * Turn on the Minute LEDs
 */
void lightMins(int units){
	//Light up the minute LEDs
	int temp;
	for (int i = 4; i <=9; i++){
			temp = pow(2,9-i); //6 leds being used
			digitalWrite(LEDS[i], units/temp);
			units = units % temp;
	}

}

/*
 * PWM on the Seconds LED
 * The LED should have 60 brightness levels
 * The LED should be "off" at 0 seconds, and fully bright at 59 seconds
 */
void secPWM(int units){
	// Write your logic here
	int dc = units*(1024/59); //duty cycle
	pwmWrite(SECS,dc);
}

/*
 * hexCompensation
 * This function may not be necessary if you use bit-shifting rather than decimal checking for writing out time values
 */
int hexCompensation(int units){
	/*Convert HEX or BCD value to DEC where 0x45 == 0d45 
	  This was created as the lighXXX functions which determine what GPIO pin to set HIGH/LOW
	  perform operations which work in base10 and not base16 (incorrect logic) 
	*/
	int unitsU = units%0x10;

	if (units >= 0x50){
		units = 50 + unitsU;
	}
	else if (units >= 0x40){
		units = 40 + unitsU;
	}
	else if (units >= 0x30){
		units = 30 + unitsU;
	}
	else if (units >= 0x20){
		units = 20 + unitsU;
	}
	else if (units >= 0x10){
		units = 10 + unitsU;
	}
	return units;
}


/*
 * decCompensation
 * This function "undoes" hexCompensation in order to write the correct base 16 value through I2C
 */
int decCompensation(int units){
	int unitsU = units%10;

	if (units >= 50){
		units = 0x50 + unitsU;
	}
	else if (units >= 40){
		units = 0x40 + unitsU;
	}
	else if (units >= 30){
		units = 0x30 + unitsU;
	}
	else if (units >= 20){
		units = 0x20 + unitsU;
	}
	else if (units >= 10){
		units = 0x10 + unitsU;
	}
	return units;
}


/*
 * hourInc
 * Fetch the hour value off the RTC, increase it by 1, and write back
 * Be sure to cater for there only being 23 hours in a day
 * Software Debouncing should be used
 */
void hourInc(void){
	//Debounce
	long interruptTime = millis();

	if (interruptTime - lastInterruptTime>200){
		printf("Interrupt 1 triggered, %x\n", hours);
		hours = wiringPiI2CReadReg8(RTC, HOUR); //Fetch RTC Time
		hours =hexCompensation(hours)+1;
		if (hours>12){
			hours=1;
		}
		//Increase hours by 1, ensuring not to overflow
		//Write hours back to the RTC
		wiringPiI2CWriteReg8(RTC, HOUR, decCompensation(hours));
	}
	lastInterruptTime = interruptTime;
}

/* 
 * minInc
 * Fetch the minute value off the RTC, increase it by 1, and write back
 * Be sure to cater for there only being 60 minutes in an hour
 * Software Debouncing should be used
 */
void minInc(void){
	long interruptTime = millis();

	if (interruptTime - lastInterruptTime>200){
		printf("Interrupt 2 triggered, %x\n", mins);
		mins = hexCompensation(wiringPiI2CReadReg8(RTC, MIN)); //Fetch RTC Time
		mins = mins + 1;//Increase minutes by 1, ensuring not to overflow
		//taking into account overflow -- go to next hour
		if (mins > 59){
			mins = 0;
			hourInc();
		}
		//Write minutes back to the RTC
		wiringPiI2CWriteReg8(RTC, MIN, decCompensation(mins));
	}
	lastInterruptTime = interruptTime;
}
//This interrupt will fetch current time from another script and write it to the clock registers
//This functions will toggle a flag that is checked in main
void toggleTime(void){
	long interruptTime = millis();

	if (interruptTime - lastInterruptTime>200){
		HH = getHours();
		MM = getMins();
		SS = getSecs();

		HH = hFormat(HH);
		HH = decCompensation(HH);
		wiringPiI2CWriteReg8(RTC, HOUR, HH);

		MM = decCompensation(MM);
		wiringPiI2CWriteReg8(RTC, MIN, MM);

		SS = decCompensation(SS);
		wiringPiI2CWriteReg8(RTC, SEC, 0b10000000+SS);

	}
	lastInterruptTime = interruptTime;
}
void cleanup(void){
	pinMode(SECS,INPUT);

	for(int i=0; i < sizeof(LEDS)/sizeof(LEDS[0]); i++){
	    pinMode(LEDS[i], INPUT);
		digitalWrite(LEDS[i],0);
	}
	printf("clean up done");
}