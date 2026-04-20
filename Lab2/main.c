#include <msp430.h>
#include <stdbool.h>
#include "peripherals.h"

/*
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
For this Lab, the following are not needed and we're not used to complete the lab
*/

/*
 * main.c
 */

typedef enum {
    MENU,
    START,
    PLAY,
    WIN,
    GAMEOVER
} state;

/* PWM period values used by BuzzerOn() */
#define A   74
#define AS  70
#define B   66
#define C1  62
#define CS  59
#define D   55
#define DS  52
#define E   49
#define F   47
#define FS  44
#define G   42
#define GS  39
#define A2  37

/* Function prototypes */
void startTimer(void);
void initializeButtons(void);
void initLaunchpadLEDs(void);
void setLaunchpadLEDs(unsigned char led1On, unsigned char led2On);
void cycleDelay(unsigned int delay);
unsigned char play(unsigned char currButt);
unsigned char checkButtons(void);
unsigned char ledToLight(unsigned char value);
void gameOver(void);
void winGame(void);
void menuState(unsigned char currKey);
void initialize(void);
void resetToMenu(void);
void uintToStr(unsigned int value, char *buf);

/* Globals */
char startKey = '*';
state State = MENU;

volatile unsigned long millis = 0;
volatile unsigned long noteDurationTimer = 0;
volatile unsigned long delayTimerFinished = 0;

volatile int currPos = 0;          /* Byte index into noteSequence[] */
volatile int menuDisplayed = 0;
volatile unsigned char didGood = 0;
volatile unsigned int score = 0;   /* Tracks successful note hits */

/*
 * noteSequence format:
 * [duration0, pitch0, duration1, pitch1, ...]
 * duration units are multiplied by 10 timer ticks in this program
 *
 * Updated to include at least 8 distinct pitches:
 * A, B, C1, CS, D, E, F, G
 */
unsigned char noteSequence[] = {
    80, G,
    80, A,
    80, CS,
    80, G,
    80, E,
    80, F,
    80, D,
    80, D,
    80, C1,
    80, E,
    80, G,
    80, A,
    80, C1,
    80, A,
    80, C1,
    80, D,
    80, B,
    80, A,
    80, G,
    80, G,
    80, D,
    80, C1,
    80, A,
    80, C1,
    80, A,
    80, C1,
    80, D,
    80, B,
    80, A,
    80, G,
    80, G,
    80, D,
    80, C1,
    80, A,
    80, C1,
    80, A,
    80, C1,
    80, D,
    80, B,
    80, A,
    80, G,
    80, G,
    80, D,
    80, C1,
    80, A,
    80, C1,
    80, A,
    80, C1,
    80, D,
    80, B,
    80, A,
    80, G,
    80, G,
    80, D,
    80, C1
};

void startTimer(void) {
    TA2CTL = TASSEL_1 | MC_1 | ID_0;   /* ACLK, up mode, divide by 1 */
    TA2CCR0 = 0x20;
    TA2CCTL0 = CCIE;
}

#pragma vector = TIMER2_A0_VECTOR
__interrupt void TIMER2_A0_ISR(void) {
    millis++;
}

void initializeButtons(void) {
    /* S1: P7.0, S2: P3.6, S3: P2.2, S4: P7.4 */
    P7SEL &= ~(BIT0 | BIT4);
    P7DIR &= ~(BIT0 | BIT4);
    P7REN |=  (BIT0 | BIT4);
    P7OUT |=  (BIT0 | BIT4);

    P3SEL &= ~BIT6;
    P3DIR &= ~BIT6;
    P3REN |=  BIT6;
    P3OUT |=  BIT6;

    P2SEL &= ~BIT2;
    P2DIR &= ~BIT2;
    P2REN |=  BIT2;
    P2OUT |=  BIT2;
}

/* Launchpad LED1 = P1.0, Launchpad LED2 = P4.7 */
void initLaunchpadLEDs(void) {
    P1SEL &= ~BIT0;
    P1DIR |= BIT0;
    P1OUT &= ~BIT0;

    P4SEL &= ~BIT7;
    P4DIR |= BIT7;
    P4OUT &= ~BIT7;
}

void setLaunchpadLEDs(unsigned char led1On, unsigned char led2On) {
    if (led1On) {
        P1OUT |= BIT0;
    } else {
        P1OUT &= ~BIT0;
    }

    if (led2On) {
        P4OUT |= BIT7;
    } else {
        P4OUT &= ~BIT7;
    }
}

void cycleDelay(unsigned int delay) {
    delayTimerFinished = millis + ((unsigned long)delay * 10UL);
    while (millis <= delayTimerFinished) {
        /* Busy wait */
    }
}

/* Returns the system to the initial menu state. */
void resetToMenu(void) {
    BuzzerOff();
    setLeds(0);
    setLaunchpadLEDs(0, 0);
    currPos = 0;
    didGood = 0;
    score = 0;
    menuDisplayed = 0;
    State = MENU;
    Graphics_clearDisplay(&g_sContext);
    Graphics_flushBuffer(&g_sContext);
}

/* Converts an unsigned integer to a string for display. */
void uintToStr(unsigned int value, char *buf) {
    char temp[6];
    int i = 0;
    int j = 0;

    if (value == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }

    while (value > 0 && i < 5) {
        temp[i++] = (value % 10) + '0';
        value /= 10;
    }

    while (i > 0) {
        buf[j++] = temp[--i];
    }

    buf[j] = '\0';
}

/*
 * Return values:
 * 0 = still playing
 * 1 = win
 * 2 = game over
 */
unsigned char play(unsigned char currButt) {
    unsigned int noteSequenceLength = sizeof(noteSequence);
    unsigned char expected = ledToLight(noteSequence[currPos + 1]);

    /* Evaluate the current note once its time window expires. */
    if (millis >= noteDurationTimer) {
        if (didGood == 1) {
            currPos += 2; /* Advance to the next duration/pitch pair */
        } else {
            return 2;     /* Player missed the note */
        }

        /* Check bounds before accessing the next pitch value. */
        if ((currPos + 1) >= noteSequenceLength) {
            BuzzerOff();
            setLeds(0);
            return 1;
        }

        /*
         * Reset note-hit state before loading the next note so that
         * a correct press does not carry over between notes.
         */
        didGood = 0;
        BuzzerOff();
        setLeds(0);

        noteDurationTimer = millis + ((unsigned long)noteSequence[currPos] * 10UL);
        BuzzerOn(noteSequence[currPos + 1]);
        setLeds(ledToLight(noteSequence[currPos + 1]));
        expected = ledToLight(noteSequence[currPos + 1]);
    }

    /*
     * Record a successful hit only once per note.
     * This prevents the score from increasing repeatedly while a correct
     * button remains held.
     */
    if (!didGood && currButt == expected) {
        didGood = 1;
        score++;
    }

    /* Any incorrect nonzero input ends the game. */
    if ((currButt != 0) && (currButt != expected)) {
        return 2;
    }

    return 0;
}

unsigned char checkButtons(void) {
    unsigned char state = 0;

    /* Active-low buttons because of pull-ups */
    if ((P7IN & BIT0) == 0) {  /* S1 */
        state |= 0x01;
    }
    if ((P3IN & BIT6) == 0) {  /* S2 */
        state |= 0x02;
    }
    if ((P2IN & BIT2) == 0) {  /* S3 */
        state |= 0x04;
    }
    if ((P7IN & BIT4) == 0) {  /* S4 */
        state |= 0x08;
    }

    return state;
}

unsigned char ledToLight(unsigned char value) {
    if (value >= 66 && value <= 74) {
        return 1;
    } else if (value >= 55 && value <= 65) {
        return 2;
    } else if (value >= 46 && value <= 54) {
        return 4;
    } else if (value >= 37 && value <= 45) {
        return 8;
    } else {
        return 0;
    }
}

void gameOver(void) {
    char scoreBuf[6];

    BuzzerOff();
    setLeds(0);
    setLaunchpadLEDs(0, 0);

    uintToStr(score, scoreBuf);

    Graphics_clearDisplay(&g_sContext);
    Graphics_drawStringCentered(&g_sContext, "YOU", AUTO_STRING_LENGTH, 48, 8, TRANSPARENT_TEXT);
    Graphics_drawStringCentered(&g_sContext, "SUCK", AUTO_STRING_LENGTH, 48, 18, TRANSPARENT_TEXT);
    Graphics_drawStringCentered(&g_sContext, "GAMEOVER", AUTO_STRING_LENGTH, 48, 30, TRANSPARENT_TEXT);
    Graphics_drawStringCentered(&g_sContext, "Score", AUTO_STRING_LENGTH, 48, 44, TRANSPARENT_TEXT);
    Graphics_drawStringCentered(&g_sContext, scoreBuf, AUTO_STRING_LENGTH, 48, 54, TRANSPARENT_TEXT);
    Graphics_flushBuffer(&g_sContext);

    cycleDelay(150);
    Graphics_clearDisplay(&g_sContext);
    Graphics_flushBuffer(&g_sContext);
}

void winGame(void) {
    char scoreBuf[6];

    BuzzerOff();
    setLeds(0);
    setLaunchpadLEDs(0, 0);

    uintToStr(score, scoreBuf);

    Graphics_clearDisplay(&g_sContext);
    Graphics_drawStringCentered(&g_sContext, "YOU", AUTO_STRING_LENGTH, 48, 8, TRANSPARENT_TEXT);
    Graphics_drawStringCentered(&g_sContext, "DONT SUCK", AUTO_STRING_LENGTH, 48, 18, TRANSPARENT_TEXT);
    Graphics_drawStringCentered(&g_sContext, "YAY", AUTO_STRING_LENGTH, 48, 30, TRANSPARENT_TEXT);
    Graphics_drawStringCentered(&g_sContext, "Score", AUTO_STRING_LENGTH, 48, 44, TRANSPARENT_TEXT);
    Graphics_drawStringCentered(&g_sContext, scoreBuf, AUTO_STRING_LENGTH, 48, 54, TRANSPARENT_TEXT);
    Graphics_flushBuffer(&g_sContext);

    cycleDelay(150);
    Graphics_clearDisplay(&g_sContext);
    Graphics_flushBuffer(&g_sContext);
}

void menuState(unsigned char currKey) {
    if (currKey == startKey) {
        State = START;
        menuDisplayed = 0;
    } else if (menuDisplayed == 0) {
        menuDisplayed = 1;
        Graphics_clearDisplay(&g_sContext);

        /* Welcome/reset screen shown on power-up and after reset */
        Graphics_drawStringCentered(&g_sContext, "Welcome To", AUTO_STRING_LENGTH, 48, 18, TRANSPARENT_TEXT);
        Graphics_drawStringCentered(&g_sContext, "MSP430 Hero", AUTO_STRING_LENGTH, 48, 28, TRANSPARENT_TEXT);
        Graphics_drawStringCentered(&g_sContext, "Press *", AUTO_STRING_LENGTH, 48, 40, TRANSPARENT_TEXT);
        Graphics_drawStringCentered(&g_sContext, "To Begin", AUTO_STRING_LENGTH, 48, 50, TRANSPARENT_TEXT);
        Graphics_drawStringCentered(&g_sContext, "Press #", AUTO_STRING_LENGTH, 48, 62, TRANSPARENT_TEXT);
        Graphics_drawStringCentered(&g_sContext, "To Reset", AUTO_STRING_LENGTH, 48, 72, TRANSPARENT_TEXT);

        Graphics_flushBuffer(&g_sContext);
    }
}

void initialize(void) {
    unsigned long nextTime;

    /* Countdown: 3 */
    Graphics_clearDisplay(&g_sContext);
    Graphics_drawStringCentered(&g_sContext, "3", AUTO_STRING_LENGTH, 48, 48, TRANSPARENT_TEXT);
    Graphics_flushBuffer(&g_sContext);
    setLaunchpadLEDs(1, 0);   /* LED1 on */
    nextTime = millis + 1000;
    while (millis < nextTime) {
        /* Timer A2-based wait */
    }

    /* Countdown: 2 */
    Graphics_clearDisplay(&g_sContext);
    Graphics_drawStringCentered(&g_sContext, "2", AUTO_STRING_LENGTH, 48, 48, TRANSPARENT_TEXT);
    Graphics_flushBuffer(&g_sContext);
    setLaunchpadLEDs(0, 1);   /* LED2 on */
    nextTime = millis + 1000;
    while (millis < nextTime) {
        /* Timer A2-based wait */
    }

    /* Countdown: 1 */
    Graphics_clearDisplay(&g_sContext);
    Graphics_drawStringCentered(&g_sContext, "1", AUTO_STRING_LENGTH, 48, 48, TRANSPARENT_TEXT);
    Graphics_flushBuffer(&g_sContext);
    setLaunchpadLEDs(1, 0);   /* LED1 on */
    nextTime = millis + 1000;
    while (millis < nextTime) {
        /* Timer A2-based wait */
    }

    /* Countdown: GO */
    Graphics_clearDisplay(&g_sContext);
    Graphics_drawStringCentered(&g_sContext, "GO", AUTO_STRING_LENGTH, 48, 48, TRANSPARENT_TEXT);
    Graphics_flushBuffer(&g_sContext);
    setLaunchpadLEDs(1, 1);   /* both on */
    nextTime = millis + 1000;
    while (millis < nextTime) {
        /* Timer A2-based wait */
    }

    Graphics_clearDisplay(&g_sContext);
    Graphics_flushBuffer(&g_sContext);
    setLaunchpadLEDs(0, 0);

    /* Reset per-game state before starting playback */
    currPos = 0;
    didGood = 0;
    score = 0;

    noteDurationTimer = millis + ((unsigned long)noteSequence[currPos] * 10UL);
    BuzzerOn(noteSequence[currPos + 1]);
    setLeds(ledToLight(noteSequence[currPos + 1]));
}

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;   /* Stop watchdog timer */

    unsigned char currKey = 0;
    unsigned char prevKey = 0;
    unsigned char currButt = 0;

    startTimer();
    _enable_interrupts();

    initializeButtons();
    initLeds();
    initLaunchpadLEDs();
    configDisplay();
    configKeypad();

    Graphics_clearDisplay(&g_sContext);
    Graphics_flushBuffer(&g_sContext);

    while (1) {
        prevKey = currKey;
        currKey = getKey();
        currButt = checkButtons();

        /* Simple keypad hold suppression */
        if (prevKey == currKey) {
            currKey = 0;
        }

        /* Allow '#' to return directly to the menu. */
        if (currKey == '#') {
            resetToMenu();
            continue;
        }

        switch (State) {
        case MENU:
            menuState(currKey);
            setLeds(currButt);
            break;

        case START:
            initialize();
            State = PLAY;
            break;

        case PLAY:
            switch (play(currButt)) {
            case 0:
                break;
            case 1:
                State = WIN;
                menuDisplayed = 0;
                break;
            case 2:
                State = GAMEOVER;
                menuDisplayed = 0;
                break;
            }
            break;

        case GAMEOVER:
            gameOver();
            State = MENU;
            break;

        case WIN:
            winGame();
            State = MENU;
            break;
        }
    }

    return 0;
}
