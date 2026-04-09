#include <msp430.h> 
#include <stdbool.h>
#include "peripherals.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
// define type state, used for states. mainly stored as its own type for readability
typedef enum{
    MENU,
    START,
    PLAY,
    WIN,
    GAMEOVER,
} state;
// define type button, with 4 possible values. These will be used for the sequence.
/**
 * main.c
 */

#define A 74
#define AS 70
#define B 66
#define C1 62
#define CS 59
#define D 55
#define DS 52
#define E 49
#define F 47
#define FS 44
#define G 42
#define GS 39
#define A2 37

char startKey = '*';
state State = MENU;
volatile unsigned long timeOut, timeOutTimer;
volatile int currStage, currPos;
volatile int gameSpeed = 20;
volatile int sequence[32]; // limited to 32
volatile int menuDisplayed;
volatile char didGood;
const int low = 150;
const int med = 200;
const int high = 250;
const int vhigh = 300; // Buzzer frequencies
volatile unsigned long millis, buttonDownMillis, noteDurrationTimer, delayTimerFinished;
// note sequence is a series of chars, first byte is the duration and second byte is the pitch.
// This means that each note in sequence takes up 2 bytes of ram.
// Duration is scaled across 0ms to 1023ms
// Pitch is period as determined by
unsigned char noteSequence[] = {80, G,
                                80, A,
                                80, A,
                                80, G,
                                80, E,
                                80, E,
                                80, D,
                                80, D,
                                80, C,
                                80, E,
                                80, G,
        80, A,
        80, C,
        80, A,
        80, C,
        80, D,
        80, B,
        80, A,
        80, G,
        80, G,
        80, D,
        80, C,
        80, A,
        80, C,
        80, A,
        80, C,
        80, D,
        80, B,
        80, A,
        80, G,
        80, G,
        80, D,
        80, C
        80, A,
        80, C,
        80, A,
        80, C,
        80, D,
        80, B,
        80, A,
        80, G,
        80, G,
        80, D,
        80, C,
        80, A,
        80, C,
        80, A,
        80, C,
        80, D,
        80, B,
        80, A,
        80, G,
        80, G,
        80, D,
        80, C
        };
void startTimer(void){
    TA2CTL = TASSEL_1 | MC_1 | ID_0;
    TA2CCR0 = 0x20;
    TA2CCTL0 = CCIE;
}
#pragma vector=TIMER2_A0_VECTOR
__interrupt void TIMER2_A0_ISR(void){
    millis++;
}


void initializeButtons(void){
    P7SEL &= ~(BIT0|BIT4);
    P7DIR &= ~(BIT0|BIT4);
    P7REN |= (BIT0|BIT4);
    P7OUT |= (BIT0|BIT4);

    P3SEL &= ~(BIT6);
    P3DIR &= ~(BIT6);
    P3REN |= (BIT6);
    P3OUT |= (BIT6);

    P2SEL &= ~(BIT2);
    P2DIR &= ~(BIT2);
    P2REN |= (BIT2);
    P2OUT |= (BIT2); // configure 7-0, 4, 3-6, and 2-2 as inputs, pull up.
}


void cycleDelay(int delay){
    delayTimerFinished = millis + delay*10;
    while(millis <= delayTimerFinished){

    }
}
void waitMillis(int delay){
    delayTimerFinished = millis + delay;
    while(millis <= delayTimerFinished){

    }
}

// return 0 if game is still in play
// return 1 if win
// return 2 if game over
// check current time to see if next note is needed
// check how long button is held down, consider failure if wrong button pressed or pressed to early
unsigned char play(char currButt){
    if (millis >= noteDurrationTimer){
        if (didGood == 1){
            currPos+= 2; // advance place in sequence
        } else {
            return 2; // game over condition
        }

        if(currPos > sizeof(noteSequence)){
            return 1; // win condition
        } else {
            BuzzerOff();
            setLeds(0);
            cycleDelay(50);
            buttonDownMillis = 0;
            noteDurrationTimer = millis + (noteSequence[currPos] * 10);
            BuzzerOn(noteSequence[currPos + 1]);
            setLeds(ledToLight(noteSequence[currPos + 1]));
        }
    }
    if (currButt == ledToLight(noteSequence[currPos + 1])){
        didGood = 1;
    }
    if (currButt != 0 && currButt !=  ledToLight(noteSequence[currPos + 1])){
        return 2; // game over condition
    }
    return 0;

}
unsigned char checkButtons(void){
    char buttonVal;
    char S1 = (P7IN & (BIT0)) << 3; // 0000x000
    char S2 = ((P3IN >> 5) & (BIT1)) << 1; //0x000000 >> 00000x00
    char S3 = (P2IN  & (BIT2)) >> 1;// 000000x0
    char S4 = ((P7IN >> 1) & (BIT3)) >> 3;  //000x0000 >> 0000000x

    buttonVal = ~(BIT7 | BIT6 | BIT5 |BIT4 | S1|S2|S3|S4);
    return buttonVal;
}

int ledToLight(char value){
    if (value >= 66 && value <= 74){
        return 1;
    } else if (value >= 55 && value <= 65){
        return 2;
    } else if (value >= 46 && value <= 54){
        return 4;
    } else if (value >= 37 && value <= 45){
        return 8;
    } else{
        return 0;
    }
}

void drawNumber(char num){
    Graphics_clearDisplay(&g_sContext);
    // Show a number and play the buzzer
    switch(num){
    case '1':
        BuzzerOn(low);
        Graphics_drawStringCentered(&g_sContext, "1", AUTO_STRING_LENGTH, 48, 35, TRANSPARENT_TEXT);
        break;
    case '2':
        BuzzerOn(med);
        Graphics_drawStringCentered(&g_sContext, "2", AUTO_STRING_LENGTH, 48, 35, TRANSPARENT_TEXT);
        break;
    case '3':
        BuzzerOn(high);
        Graphics_drawStringCentered(&g_sContext, "3", AUTO_STRING_LENGTH, 48, 35, TRANSPARENT_TEXT);
        break;
    case '4':
        BuzzerOn(vhigh);
        Graphics_drawStringCentered(&g_sContext, "4", AUTO_STRING_LENGTH, 48, 35, TRANSPARENT_TEXT);
        break;
    }
    Graphics_flushBuffer(&g_sContext);
    cycleDelay(1);
    BuzzerOff();

}
void gameOver(){
        BuzzerOff();
        Graphics_clearDisplay(&g_sContext); // tell the player they suck and move on

        Graphics_drawStringCentered(&g_sContext, "YOU", AUTO_STRING_LENGTH, 48, 15, TRANSPARENT_TEXT);
        Graphics_drawStringCentered(&g_sContext, "SUCK", AUTO_STRING_LENGTH, 48, 25, TRANSPARENT_TEXT);
        Graphics_drawStringCentered(&g_sContext, "GAMEOVER", AUTO_STRING_LENGTH, 48, 35, TRANSPARENT_TEXT);
        Graphics_flushBuffer(&g_sContext);
        cycleDelay(150);
        Graphics_clearDisplay(&g_sContext);

}
void winGame(){
        BuzzerOff();
        Graphics_clearDisplay(&g_sContext); // tell the player they dont suck and move on

        Graphics_drawStringCentered(&g_sContext, "YOU", AUTO_STRING_LENGTH, 48, 15, TRANSPARENT_TEXT);
        Graphics_drawStringCentered(&g_sContext, "DONT SUCK", AUTO_STRING_LENGTH, 48, 25, TRANSPARENT_TEXT);
        Graphics_drawStringCentered(&g_sContext, "YAY", AUTO_STRING_LENGTH, 48, 35, TRANSPARENT_TEXT);
        Graphics_flushBuffer(&g_sContext);
        cycleDelay(150);
        Graphics_clearDisplay(&g_sContext);

}
void menuState(char currKey){
    if(currKey == startKey){ //  if the start key is pressed, start
        State = START;
        menuDisplayed = 0;
    } else if (menuDisplayed == 0) { // if the menu hasnt been displayed, play it to avoid flickering
        menuDisplayed = 1;
        Graphics_drawStringCentered(&g_sContext, "Welcome", AUTO_STRING_LENGTH, 48, 15, TRANSPARENT_TEXT);
        Graphics_drawStringCentered(&g_sContext, "to", AUTO_STRING_LENGTH, 48, 25, TRANSPARENT_TEXT);
        Graphics_drawStringCentered(&g_sContext, "HERO LMAO!", AUTO_STRING_LENGTH, 48, 35, TRANSPARENT_TEXT);
        Graphics_flushBuffer(&g_sContext);
        cycleDelay(10);
    }
}
int addStage(){
    // make it add stuff to the sequence
    int newStage = rand() % 4 + 1;
    return newStage;
}
void initialize(){
    // start the game
    // count down
    Graphics_clearDisplay(&g_sContext);
    Graphics_drawStringCentered(&g_sContext, "3", AUTO_STRING_LENGTH, 48, 25, TRANSPARENT_TEXT);
    Graphics_flushBuffer(&g_sContext);
    setLeds(7);
    cycleDelay(60);
    Graphics_clearDisplay(&g_sContext);
    Graphics_drawStringCentered(&g_sContext, "2", AUTO_STRING_LENGTH, 48, 25, TRANSPARENT_TEXT);
    Graphics_flushBuffer(&g_sContext);
    setLeds(3);
    cycleDelay(60);
    Graphics_clearDisplay(&g_sContext);
    Graphics_drawStringCentered(&g_sContext, "1", AUTO_STRING_LENGTH, 48, 25, TRANSPARENT_TEXT);
    setLeds(1);
    Graphics_flushBuffer(&g_sContext);
    cycleDelay(60);
    Graphics_clearDisplay(&g_sContext);
    // reset game values
    currStage = 0;
    currPos = 0;
    didGood = 0;
    buttonDownMillis = 0;
    noteDurrationTimer = millis + (noteSequence[currPos] * 10);
    BuzzerOn(noteSequence[currPos + 1]);
    setLeds(ledToLight(noteSequence[currPos + 1]));
}
void playSequence(){
    // display all steps in sequence alongside accompanying sound.
    volatile int i;
    for(i = 0; i <= currStage; i++){

        switch (sequence[i]){
                case 1:
                    setLeds(1);
                    BuzzerOn(low);
                    cycleDelay(gameSpeed);
                    break;
                case 2:
                    setLeds(2);
                    BuzzerOn(med);
                    cycleDelay(gameSpeed);
                    break;
                case 3:
                    setLeds(4);
                    BuzzerOn(high);
                    cycleDelay(gameSpeed);
                    break;
                case 4:
                    setLeds(8);
                    BuzzerOn(vhigh);
                    cycleDelay(gameSpeed);
                    break;
        }
        setLeds(0);
        // turn off leds and stop buzzer for the next loop
        BuzzerOff();
        cycleDelay(gameSpeed); // wait however long the gamespeed is


    }
}

int checkCorrect(char currKey, int currPos){
    timeOutTimer++; // count up timer
    if(timeOutTimer < timeOut){ // check timer to time out
            if (currKey  == '1' || currKey == '2' || currKey == '3' || currKey == '4'){ // check if one of the 4 acceptable keys is pressed
                if (currKey - 0x30 == sequence[currPos]){ // use math to find number to corresponding ascii character
                    timeOutTimer = 0; // reset timer if its good
                    return 1; // 1 means good score
            } else {
                return 0; // 0 means bad score
            }
        } else {
            return 2; // 2 means no response
        }
    } else {
        return 0; // time out, bad score
    }
}


int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer


    srand(time(NULL));


    unsigned char currKey, prevKey, currButt;
    startTimer();
    _enable_interrupts();
    initializeButtons();
    initLeds();

    configDisplay();
    configKeypad();


    Graphics_clearDisplay(&g_sContext);

    timeOut = 15000; // define timeout time



	while(1){
	    prevKey = currKey; // Determine previous key to prevent bounces and holding
	    currKey = getKey();
	    currButt = checkButtons();
	    if (prevKey == currKey){
	        currKey = 0; // disregard input if held for longer then a loop
	    }
	    int correctnessResult;
	    switch (State){
	    case MENU: // menu state
	        menuState(currKey);
	        setLeds(currButt);
	        break;
	    case START: // game is started
	        initialize(); // initialize first round
	        State = PLAY; //  Change state
	        break;
	    case PLAY:
	        // loop sequence of game play
	        switch (play(currButt)){
	        case 0:
	            break;
	        case 1:
	            State = WIN;
	            break;
	        case 2:
	            State = GAMEOVER;
	            break;
	        }
	        break;
	    case GAMEOVER:
	        gameOver(); // game over function
	        State = MENU;
	        break;
        case WIN:
            winGame(); // game over function
            State = MENU;
            break;
	    }
	}
	return 0;
}



