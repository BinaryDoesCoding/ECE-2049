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
    SHOWSEQ,
    INPUT,
    GAMEOVER
} state;
// define type button, with 4 possible values. These will be used for the sequence.
/**
 * main.c
 */

char startKey = '*';
state State = MENU;
volatile unsigned long timeOut, timeOutTimer;
volatile int currStage, currPos;
volatile int gameSpeed = 20;
volatile int sequence[32]; // limited to 32
volatile int menuDisplayed;
const int low = 150;
const int med = 200;
const int high = 250;
const int vhigh = 300; // Buzzer frequencies

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

    BuzzerOff();
    srand(time(NULL));


    unsigned char currKey, prevKey;

    initLeds();

    configDisplay();
    configKeypad();


    Graphics_clearDisplay(&g_sContext);

    timeOut = 15000; // define timeout time

	

	while(1){
	    prevKey = currKey; // Determine previous key to prevent bounces and holding
	    currKey = getKey();
	    if (prevKey == currKey){
	        currKey = 0; // disregard input if held for longer then a loop
	    }
	    int correctnessResult;
	    switch (State){
	    case MENU: // menu state
	        menuState(currKey);
	        break;
	    case START: // game is started
	        initialize(); // initialize first round
	        State = SHOWSEQ; //  Change state
	        break;
	    case SHOWSEQ:
	        playSequence(); // play sequence to repeat
	        State = INPUT; // set state to waiting for input
	        break;
	    case INPUT:
	        cycleDelay(3); // delay to avoid waiting
	        drawNumber(currKey); // draw the pressed key on the screen
	        correctnessResult = checkCorrect(currKey, currPos); // check if the key pressed matches the correct position
	        if(correctnessResult == 1){ // if its correct, move on
	            currPos++;
	            if(currPos > currStage && currStage <= 32){ // if its the last move in the sequence and is right, move to a new stage
	                currStage++;
	                sequence[currStage] = addStage(sequence); // add a new stage
	                currPos = 0;
	                State = SHOWSEQ;
	                gameSpeed = 20 - currStage; // increase game speed (will be unplayable after like 15 rounds lol)
	            }
	        } else if (correctnessResult == 0 || currStage >= 32) { // if the result is wrong or the stage is 32, stop
	            State = GAMEOVER;
	        }
	        break;
	    case GAMEOVER:
	        gameOver(); // game over function
	        State = MENU;
	        break;
	    }
	}
	return 0;
}

void cycleDelay(char numLoops)
{
    // This function is a software delay. It performs
    // useless loops to waste a bit of time
    //
    // Input: numLoops = number of delay loops to execute
    // Output: none
    //
    // smj, ECE2049, 25 Aug 2013

    volatile unsigned int i,j;  // volatile to prevent removal in optimization
                                // by compiler. Functionally this is useless code

    for (j=0; j<numLoops; j++)
    {
        i = 5000 ;                 // SW Delay
        while (i > 0)               // could also have used while (i)
           i--;
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
        Graphics_clearDisplay(&g_sContext); // tell the player they suck and move on

        Graphics_drawStringCentered(&g_sContext, "YOU", AUTO_STRING_LENGTH, 48, 15, TRANSPARENT_TEXT);
        Graphics_drawStringCentered(&g_sContext, "SUCK", AUTO_STRING_LENGTH, 48, 25, TRANSPARENT_TEXT);
        Graphics_drawStringCentered(&g_sContext, "GAMEOVER", AUTO_STRING_LENGTH, 48, 35, TRANSPARENT_TEXT);
        Graphics_flushBuffer(&g_sContext);
        cycleDelay(30);
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
        Graphics_drawStringCentered(&g_sContext, "SIMON!", AUTO_STRING_LENGTH, 48, 35, TRANSPARENT_TEXT);
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
    cycleDelay(30);
    Graphics_clearDisplay(&g_sContext);
    Graphics_drawStringCentered(&g_sContext, "2", AUTO_STRING_LENGTH, 48, 25, TRANSPARENT_TEXT);
    Graphics_flushBuffer(&g_sContext);
    cycleDelay(30);
    Graphics_clearDisplay(&g_sContext);
    Graphics_drawStringCentered(&g_sContext, "1", AUTO_STRING_LENGTH, 48, 25, TRANSPARENT_TEXT);
    Graphics_flushBuffer(&g_sContext);
    Graphics_clearDisplay(&g_sContext);
    // reset game values
    currStage = 0;
    currPos = 0;
    sequence[0] = addStage();
    timeOutTimer = 0;
    timeOut = 15000;
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


