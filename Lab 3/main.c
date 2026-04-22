/*
 * This program is designed for the MSP430 microcontroller.
 * Expected hardware setup:
 *  - Graphics LCD display compatible with the Graphics library (g_sContext).
 *  - Two push buttons: Left (P2.1) and Right (P1.1).
 *  - Scroll wheel (analog input) connected to ADC12 channel 0 (P6.0).
 *  - Internal temperature sensor (ADC12 channel 10).
 *  - ACLK running at 32.768 kHz for Timer A2.
 *  - Required: peripherals.h with display configuration and Graphics context.
 */

#include "peripherals.h"
#include <msp430.h>
#include <stdbool.h>

#ifndef CALADC12_25V_30C
#define CALADC12_25V_30C (*((const unsigned short *)0x1A22))
#endif

#ifndef CALADC12_25V_85C
#define CALADC12_25V_85C (*((const unsigned short *)0x1A24))
#endif

#define TEMP_BUFFER_SIZE 36

typedef enum { PAGE_DATE, PAGE_TIME, PAGE_TEMP_C, PAGE_TEMP_F } DisplayPage;

typedef enum { FIELD_MONTH, FIELD_DAY, FIELD_HOUR, FIELD_MINUTE, FIELD_SECOND} EditField;

static const char monthAbbrev[12][4] = {"JAN", "FEB", "MAR", "APR",
                                        "MAY", "JUN", "JUL", "AUG",
                                        "SEP", "OCT", "NOV", "DEC"};

static const unsigned char monthDays[12] = {31, 28, 31, 30, 31, 30,
                                            31, 31, 30, 31, 30, 31};

static const char *fieldNames[5] = {"MONTH", "DAY", "HOUR", "MIN", "SEC"};

volatile unsigned long globalTime = 0;
volatile bool secondElapsed = false;
volatile bool refreshDisplay = true;

static unsigned char currentPage = PAGE_DATE;
static unsigned char pageSecondCount = 0;

static bool editMode = false;
static unsigned char currentField = FIELD_MONTH;

static unsigned char editMonth = 1;
static unsigned char editDay = 1;
static unsigned char editHour = 0;
static unsigned char editMinute = 0;
static unsigned char editSecond = 0;

static bool prevLeftPressed = false;
static bool prevRightPressed = false;

static unsigned int tempRawBuffer[TEMP_BUFFER_SIZE];
static unsigned char tempBufIndex = 0;
static unsigned long tempRawSum = 0;
static float avgTempC = 0.0f;

/*
 * Converts an unsigned int value to a null-terminated decimal string.
 * Supports values up to 5 digits.
 */
void uintToStr(unsigned int value, char *buf) {
  char reversedDigits[6];
  unsigned char digitCount = 0;
  unsigned char outputIndex = 0;

  if (value == 0) {
    buf[0] = '0';
    buf[1] = '\0';
    return;
  }

  while (value > 0 && digitCount < 5) {
    reversedDigits[digitCount++] = (value % 10) + '0';
    value /= 10;
  }

  while (digitCount > 0) {
    buf[outputIndex++] = reversedDigits[--digitCount];
  }

  buf[outputIndex] = '\0';
}

/*
 * Converts an integer to a fixed two-digit string.
 * Example: 5 -> "05"
 */
void twoDigitToStr(unsigned int value, char *buf) {
  buf[0] = '0' + ((value / 10) % 10);
  buf[1] = '0' + (value % 10);
  buf[2] = '\0';
}

/*
 * Builds a temperature string in the format ddd.f X
 * Example: 24.7 C
 */
void buildFixed1Temp(float temp, char *buf, char unit) {
  int wholePart;
  int tenthsPart;
  char wholePartStr[6];
  unsigned char sourceIndex = 0;
  unsigned char destIndex = 0;

  if (temp < 0) {
    buf[destIndex++] = '-';
    temp = -temp;
  }

  wholePart = (int)temp;
  tenthsPart = (int)((temp - wholePart) * 10.0f + 0.5f);

  if (tenthsPart >= 10) {
    wholePart += 1;
    tenthsPart = 0;
  }

  uintToStr((unsigned int)wholePart, wholePartStr);

  while (wholePartStr[sourceIndex] != '\0') {
    buf[destIndex++] = wholePartStr[sourceIndex++];
  }

  buf[destIndex++] = '.';
  buf[destIndex++] = '0' + tenthsPart;
  buf[destIndex++] = ' ';
  buf[destIndex++] = unit;
  buf[destIndex] = '\0';
}

/*
 * Returns the number of days in a given month.
 * Note: This function does not handle leap years; February is always 28 days.
 */
unsigned int daysInMonth(unsigned int month) {
  if (month < 1 || month > 12) {
    return 31;
  }
  return monthDays[month - 1];
}

/*
 * Converts month/day/hour/minute/second into total seconds since
 * the start of the year.
 */
unsigned long dateTimeToSeconds(unsigned int month, unsigned int day,
                                unsigned int hour, unsigned int minute,
                                unsigned int second) {
  unsigned long totalDays = 0;
  unsigned int m;

  for (m = 1; m < month; m++) {
    totalDays += monthDays[m - 1];
  }

  totalDays += (day - 1);

  return (totalDays * 86400UL) + ((unsigned long)hour * 3600UL) +
         ((unsigned long)minute * 60UL) + second;
}

/*
 * Converts a second count within the year into month/day/hour/minute/second.
 * Note: This function does not handle leap years; February is always 28 days.
 */
void secondsToDateTime(unsigned long inTime, unsigned int *month,
                       unsigned int *day, unsigned int *hour,
                       unsigned int *minute, unsigned int *second) {
  unsigned long remainingDays;
  unsigned int m = 1;

  *second = inTime % 60;
  inTime /= 60;

  *minute = inTime % 60;
  inTime /= 60;

  *hour = inTime % 24;
  inTime /= 24;

  remainingDays = inTime;

  while ((m < 12) && (remainingDays >= monthDays[m - 1])) {
    remainingDays -= monthDays[m - 1];
    m++;
  }

  *month = m;
  *day = (unsigned int)remainingDays + 1;
}

/*
 * Formats a date string as "MMM DD".
 */
void formatDateString(unsigned long inTime, char *dateStr) {
  unsigned int month, day, hour, minute, second;
  char dayStr[3];

  secondsToDateTime(inTime, &month, &day, &hour, &minute, &second);
  twoDigitToStr(day, dayStr);

  dateStr[0] = monthAbbrev[month - 1][0];
  dateStr[1] = monthAbbrev[month - 1][1];
  dateStr[2] = monthAbbrev[month - 1][2];
  dateStr[3] = ' ';
  dateStr[4] = dayStr[0];
  dateStr[5] = dayStr[1];
  dateStr[6] = '\0';
}

/*
 * Formats a time string as "HH:MM:SS".
 */
void formatTimeString(unsigned int hour, unsigned int minute,
                      unsigned int second, char *timeStr) {
  char hourStr[3];
  char minuteStr[3];
  char secondStr[3];

  twoDigitToStr(hour, hourStr);
  twoDigitToStr(minute, minuteStr);
  twoDigitToStr(second, secondStr);

  timeStr[0] = hourStr[0];
  timeStr[1] = hourStr[1];
  timeStr[2] = ':';
  timeStr[3] = minuteStr[0];
  timeStr[4] = minuteStr[1];
  timeStr[5] = ':';
  timeStr[6] = secondStr[0];
  timeStr[7] = secondStr[1];
  timeStr[8] = '\0';
}

/*
 * Clears the LCD and displays two centered text lines.
 */
void drawTwoLineScreen(const char *title, const char *value) {
  Graphics_clearDisplay(&g_sContext);
  Graphics_drawStringCentered(&g_sContext, title, AUTO_STRING_LENGTH, 48, 24,
                              TRANSPARENT_TEXT);
  Graphics_drawStringCentered(&g_sContext, value, AUTO_STRING_LENGTH, 48, 40,
                              TRANSPARENT_TEXT);
  Graphics_flushBuffer(&g_sContext);
}

/*
 * Displays the current date in MMM DD format.
 */
void displayDate(unsigned long inTime) {
  char dateStr[8];
  formatDateString(inTime, dateStr);
  drawTwoLineScreen("Date", dateStr);
}

/*
 * Displays the current time in HH:MM:SS format.
 */
void displayClock(unsigned long inTime) {
  unsigned int month, day, hour, minute, second;
  char timeStr[9];

  secondsToDateTime(inTime, &month, &day, &hour, &minute, &second);
  formatTimeString(hour, minute, second, timeStr);
  drawTwoLineScreen("Time", timeStr);
}

/*
 * Displays the average temperature in Celsius.
 */
void displayTempC(float inAvgTempC) {
  char tempStr[12];
  buildFixed1Temp(inAvgTempC, tempStr, 'C');
  drawTwoLineScreen("Temp C", tempStr);
}

/*
 * Displays the average temperature in Fahrenheit.
 */
void displayTempF(float inAvgTempC) {
  char tempStr[12];
  float tempF = (inAvgTempC * 9.0f / 5.0f) + 32.0f;
  buildFixed1Temp(tempF, tempStr, 'F');
  drawTwoLineScreen("Temp F", tempStr);
}

/*
 * Displays the edit-mode screen showing the selected field,
 * date, and time.
 */
void displayEditScreen(void) {
  char dayStr[3];
  char dateStr[8];
  char timeStr[9];

  twoDigitToStr(editDay, dayStr);

  dateStr[0] = monthAbbrev[editMonth - 1][0];
  dateStr[1] = monthAbbrev[editMonth - 1][1];
  dateStr[2] = monthAbbrev[editMonth - 1][2];
  dateStr[3] = ' ';
  dateStr[4] = dayStr[0];
  dateStr[5] = dayStr[1];
  dateStr[6] = '\0';

  formatTimeString(editHour, editMinute, editSecond, timeStr);

  Graphics_clearDisplay(&g_sContext);
  Graphics_drawStringCentered(&g_sContext, "EDIT MODE", AUTO_STRING_LENGTH, 48,
                              10, TRANSPARENT_TEXT);
  Graphics_drawStringCentered(&g_sContext, fieldNames[currentField],
                              AUTO_STRING_LENGTH, 48, 22, TRANSPARENT_TEXT);
  Graphics_drawStringCentered(&g_sContext, dateStr, AUTO_STRING_LENGTH, 48, 40,
                              TRANSPARENT_TEXT);
  Graphics_drawStringCentered(&g_sContext, timeStr, AUTO_STRING_LENGTH, 48, 55,
                              TRANSPARENT_TEXT);
  Graphics_flushBuffer(&g_sContext);
}

/* ---------------- Timer A2 ---------------- */

/*
 * Assumes ACLK is running at 32.768 kHz.
 * TA2CCR0 = 32767 yields a 1-second interval (32768 counts).
 */
void initTimerA2_1sec(void) {
  TA2CTL = TASSEL_1 | MC_1 | ID_0;
  TA2CCR0 = 32767;
  TA2CCTL0 = CCIE;
}

#pragma vector = TIMER2_A0_VECTOR
__interrupt void Timer_A2_ISR(void) {
  if (!editMode) {
    globalTime++;
  }
  secondElapsed = true;
}

/*
 * Configures the left and right Launchpad buttons with pull-up resistors.
 * Left button = P2.1
 * Right button = P1.1
 */
void initLaunchpadButtons(void) {
  P2SEL &= ~BIT1;
  P2DIR &= ~BIT1;
  P2REN |= BIT1;
  P2OUT |= BIT1;

  P1SEL &= ~BIT1;
  P1DIR &= ~BIT1;
  P1REN |= BIT1;
  P1OUT |= BIT1;
}

/*
 * Returns true when the left button is pressed.
 */
bool leftButtonPressed(void) {
  return ((P2IN & BIT1) == 0);
}

/*
 * Returns true when the right button is pressed.
 */
bool rightButtonPressed(void) {
  return ((P1IN & BIT1) == 0);
}

/*
 * Configures ADC12 for manual single-channel conversions.
 * Uses 2.5 V internal reference for the internal temperature sensor.
 */
void initADC12(void) {
  P6SEL |= BIT0;

  REFCTL0 &= ~REFMSTR;

  ADC12CTL0 = ADC12SHT0_9 | ADC12REFON | ADC12REF2_5V | ADC12ON;
  ADC12CTL1 = ADC12SHP;

  __delay_cycles(10000);
}

/*
 * Reads a single ADC12 conversion from the requested channel/reference.
 */
unsigned int readADC12Channel(unsigned int inch, unsigned int sref) {
  ADC12CTL0 &= ~ADC12ENC;
  ADC12MCTL0 = sref | inch;
  ADC12CTL0 |= ADC12ENC | ADC12SC;

  while (ADC12CTL1 & ADC12BUSY) {
    __no_operation();
  }

  return (ADC12MEM0 & 0x0FFF);
}

/*
 * Reads the raw ADC value from the scroll wheel.
 */
unsigned int readScrollWheelRaw(void) {
  return readADC12Channel(ADC12INCH_0, ADC12SREF_0);
}

/*
 * Reads the raw ADC value from the internal temperature sensor.
 */
unsigned int readTempRaw(void) {
  return readADC12Channel(ADC12INCH_10, ADC12SREF_1);
}

/*
 * Converts a raw ADC value to degrees Celsius using calibration data.
 * Returns -999.9f if calibration data appears invalid.
 */
float rawToTempC(unsigned int raw) {
  float rawF = (float)raw;
  float cal30 = (float)CALADC12_25V_30C;
  float cal85 = (float)CALADC12_25V_85C;

  if ((cal85 <= cal30) || (cal30 > 4095.0f) || (cal85 > 4095.0f)) {
    return -999.9f;
  }

  return ((rawF - cal30) * 55.0f / (cal85 - cal30)) + 30.0f;
}

/*
 * Initializes the moving-average temperature buffer with the first reading.
 */
void initTemperatureAverage(void) {
  unsigned char i;
  unsigned int firstRaw;

  for (i = 0; i < 8; i++) {
    readTempRaw();
    __delay_cycles(1000);
  }

  firstRaw = readTempRaw();

  tempRawSum = 0;
  tempBufIndex = 0;

  for (i = 0; i < TEMP_BUFFER_SIZE; i++) {
    tempRawBuffer[i] = firstRaw;
    tempRawSum += firstRaw;
  }

  avgTempC = rawToTempC(firstRaw);
}

/*
 * Updates the moving average of the last 36 temperature readings.
 */
void updateTemperatureAverage(void) {
  unsigned int newRaw = readTempRaw();
  unsigned int avgRaw;

  tempRawSum -= tempRawBuffer[tempBufIndex];
  tempRawBuffer[tempBufIndex] = newRaw;
  tempRawSum += newRaw;

  tempBufIndex++;
  if (tempBufIndex >= TEMP_BUFFER_SIZE) {
    tempBufIndex = 0;
  }

  avgRaw = (unsigned int)(tempRawSum / TEMP_BUFFER_SIZE);
  avgTempC = rawToTempC(avgRaw);
}

/*
 * Advances the display page every 3 seconds.
 */
void updateDisplayPage(void) {
  pageSecondCount++;

  if (pageSecondCount >= 3) {
    pageSecondCount = 0;
    currentPage++;

    if (currentPage > PAGE_TEMP_F) {
      currentPage = PAGE_DATE;
    }

    refreshDisplay = true;
  }
}

/*
 * Maps a raw ADC reading into an integer range [minVal, maxVal].
 * Full-scale 4095 maps to maxVal.
 */
unsigned int mapADCToRange(unsigned int raw, unsigned int minVal,
                           unsigned int maxVal) {
  unsigned long span = (unsigned long)(maxVal - minVal + 1);
  return minVal + (unsigned int)((raw * (span - 1)) / 4095UL);
}

/*
 * Loads the editable month/day/hour/minute/second fields from the current time.
 * A snapshot of globalTime is used for safety since the ISR may update it.
 */
void loadEditFieldsFromGlobalTime(void) {
  unsigned int month, day, hour, minute, second;
  unsigned long timeSnapshot;

  __disable_interrupt();
  timeSnapshot = globalTime;
  __enable_interrupt();

  secondsToDateTime(timeSnapshot, &month, &day, &hour, &minute, &second);

  editMonth = (unsigned char)month;
  editDay = (unsigned char)day;
  editHour = (unsigned char)hour;
  editMinute = (unsigned char)minute;
  editSecond = (unsigned char)second;
}

/*
 * Updates the selected edit field based on the scroll wheel position.
 */
void updateEditFieldFromScrollWheel(void) {
  unsigned int raw = readScrollWheelRaw();

  switch (currentField) {
  case FIELD_MONTH:
    editMonth = (unsigned char)mapADCToRange(raw, 1, 12);
    if (editDay > daysInMonth(editMonth)) {
      editDay = (unsigned char)daysInMonth(editMonth);
    }
    break;

  case FIELD_DAY:
    editDay = (unsigned char)mapADCToRange(raw, 1, daysInMonth(editMonth));
    break;

  case FIELD_HOUR:
    editHour = (unsigned char)mapADCToRange(raw, 0, 23);
    break;

  case FIELD_MINUTE:
    editMinute = (unsigned char)mapADCToRange(raw, 0, 59);
    break;

  case FIELD_SECOND:
    editSecond = (unsigned char)mapADCToRange(raw, 0, 59);
    break;
  }
}

/*
 * Advances the selected field in edit mode.
 * Cycles back to month after seconds.
 */
void advanceEditField(void) {
  currentField++;

  if (currentField > FIELD_SECOND) {
    currentField = FIELD_MONTH;
  }
}

/*
 * Handles left/right button presses for entering edit mode,
 * advancing fields, and accepting edits.
 */
void handleEditButtons(void) {
  bool leftNow = leftButtonPressed();
  bool rightNow = rightButtonPressed();

  if (leftNow && !prevLeftPressed) {
    if (!editMode) {
      editMode = true;
      currentField = FIELD_MONTH;
      loadEditFieldsFromGlobalTime();
    } else {
      advanceEditField();
    }
    refreshDisplay = true;
  }

  if (rightNow && !prevRightPressed) {
    if (editMode) {
      __disable_interrupt();
      globalTime = dateTimeToSeconds(editMonth, editDay, editHour, editMinute,
                                     editSecond);
      __enable_interrupt();

      editMode = false;
      currentPage = PAGE_DATE;
      pageSecondCount = 0;
      refreshDisplay = true;
    }
  }

  prevLeftPressed = leftNow;
  prevRightPressed = rightNow;
}

/*
 * Main program entry point.
 * Initializes peripherals, starts the timer/ADC system,
 * and runs the display/edit loop forever.
 */
int main(void) {
  WDTCTL = WDTPW | WDTHOLD;

  configDisplay();
  initTimerA2_1sec();
  initLaunchpadButtons();
  initADC12();

  __disable_interrupt();
  globalTime = dateTimeToSeconds(1, 1, 0, 0, 0);
  __enable_interrupt();

  initTemperatureAverage();

  _enable_interrupts();

  while (1) {
    handleEditButtons();

    if (secondElapsed) {
      secondElapsed = false;

      updateTemperatureAverage();

      if (!editMode) {
        updateDisplayPage();
      }

      refreshDisplay = true;
    }

    if (editMode) {
      updateEditFieldFromScrollWheel();

      if (refreshDisplay) {
        displayEditScreen();
        refreshDisplay = false;
      }
    } else {
      if (refreshDisplay) {
        unsigned long timeSnapshot;

        __disable_interrupt();
        timeSnapshot = globalTime;
        __enable_interrupt();

        switch (currentPage) {
        case PAGE_DATE:
          displayDate(timeSnapshot);
          break;
        case PAGE_TIME:
          displayClock(timeSnapshot);
          break;
        case PAGE_TEMP_C:
          displayTempC(avgTempC);
          break;
        case PAGE_TEMP_F:
          displayTempF(avgTempC);
          break;
        }

        refreshDisplay = false;
      }
    }
  }
}