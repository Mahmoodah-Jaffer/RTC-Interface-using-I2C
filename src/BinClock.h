#ifndef BINCLOCK_H
#define BINCLOCK_H

//Some reading (if you want)
//https://stackoverflow.com/questions/1674032/static-const-vs-define-vs-enum

// Function definitions
void initGPIO(void);
void hour_interrupt_handler(void);
void minute_interrupt_handler(void);
int hFormat(int hours);
void lightHours(int units);
void lightMins(int units);
void secPWM(int units);
int hexCompensation(int units);
int decCompensation(int units);
void hourInc(void);
void minInc(void);
void toggleTime(void);
void cleanup(void);

// define constants
const char RTCAddr = 0x6f;
const char SEC = 0x00; // see register table in datasheet
const char MIN = 0x01;
const char HOUR = 0x02;
const char TIMEZONE = 2; // +02H00 (RSA)

// define pins
const int LEDS[] = {26,6,5,4,23,22,21,3,2,0}; //H0-H4, M0-M5
const int SECS = 1;
const int BTNS[] = {27,25}; // B0, B1

#endif