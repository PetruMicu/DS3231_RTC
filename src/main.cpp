//
// Created by petru on 30.12.2020.
//

#include "../lib/DS3231/DS3231.h"
#include "../lib/LiquidCrystal/LiquidCrystal.h"
#include <Arduino.h>

//Pins used for lcd display

#define RS_pin 7
#define E_pin 8
#define D4_pin 9
#define D5_pin 10
#define D6_pin 11
#define D7_pin 12

//Buzzer pin

#define BUZZ_pin 6

//Pins used for DS3231 RTC clock

uint8_t INT_pin = 2; // LOW when alarm condition is met
uint8_t SNOOZE_pin =5;
uint8_t UP_pin = 4; // increment / toggle alarm1 / set alarm1 (while not in edit mode)
uint8_t DOWN_pin = 3; // decrement / toggle alarm2 / set alarm2 (while not in edit mode)

// variables for alarm management

bool AlarmState = false;
bool alarmIgnored[2] = {false, false};
uint8_t alarmIgnoredCount[2] = {0,0};

//vectors that hold the number of days in each month;
const uint8_t normalYear[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
const uint8_t leapYear[12] = {31,29,31,30,31,30,31,31,30,31,30,31};

//4->celcius; 5->fahrenheit; 6->kelvin;
uint8_t checkTemperature = 4; // keep track of the temperature measure unit

//Variables used for button interface

long buttonTimer = 0;
long buttonPressTime = 700;
bool buttonActive = false;
bool buttonHoldActive = false;


//Custom characters for alarm status
//each character is 5 pix wide and 8 pix high

//          *
//        * * *
//        * * *

// alarm 1 ON

#define A1ON 1

byte alarm1ON[8] = {
        B00100,
        B01110,
        B01110,
        B00000,
        B00000,
        B00000,
        B00000,
        B00000
};

//alarm 2 ON

#define A2ON 2

byte alarm2ON[8] = {
        B00000,
        B00000,
        B00000,
        B00000,
        B00000,
        B01110,
        B01110,
        B00100
};

//both alarms are enabled

#define BothON 3

byte bothAlarms[8] = {
        B00100,
        B01110,
        B01110,
        B00000,
        B00000,
        B01110,
        B01110,
        B00100
};

//small 'A' for alarm symbol
//inspired by guy from youtube -----> put his name at the beginning of this code

#define ASymbol 0

byte alarmSymbol[8] = {
        B00000,
        B00000,
        B00100,
        B01010,
        B01110,
        B10001,
        B00000,
        B00000
};

#define CELCIUS 4

byte celcius[8]{
        B10110,
        B01001,
        B01000,
        B01000,
        B01000,
        B01001,
        B00110,
        B00000
};

#define FAHRENHEIT 5

byte fahrenheit[8]{
        B11111,
        B01000,
        B01111,
        B01000,
        B01000,
        B01000,
        B01000,
        B00000
};

#define KELVIN 6

byte kelvin[8]{
        B10000,
        B10010,
        B10100,
        B11000,
        B11000,
        B10100,
        B10010,
        B00000
};



//instantiating LCD object using the pins from above

LiquidCrystal lcd(RS_pin,E_pin,D4_pin,D5_pin,D6_pin,D7_pin);

//instantiating RTC object

DS3231 rtc;

bool greater9(uint8_t value){
    return value > 9;
}

bool isLeapYear(uint8_t year){
    if((year + 2000) % 400 == 0)
        return true;
    if((year + 2000) % 100 == 0)
        return false;
    if((year + 2000) % 4 == 0)
        return true;

    return false;
}

//used for printing values in 0X format
void print0X2LCD(uint8_t value){
    if(greater9(value)){
        lcd.print(value);
    }
    else{
        lcd.print(0);
        lcd.print(value);
    }
}
// prints the time to the lcd display
void printTime2LCD(){
    //lcd.clear();
    RTCdata clockTime = rtc.readTime();
    lcd.setCursor(0,0);
    print0X2LCD(clockTime.hour);
    lcd.print(':');
    print0X2LCD(clockTime.minutes);
    lcd.print(":");
    print0X2LCD(clockTime.seconds);
    //print temperature ---> change later to print different temp
    switch (checkTemperature) {
        case CELCIUS: // celcius
            lcd.print("  ");
            lcd.print(rtc.readCelcius());
            lcd.write(byte(CELCIUS));
            break;
        case FAHRENHEIT: // fahrenheit
            lcd.print("  ");
            lcd.print(rtc.readFahrenheit());
            lcd.write(byte(FAHRENHEIT));
            break;
        case KELVIN: // kelvin
            lcd.print(" ");
            lcd.print(rtc.readKelvin());
            lcd.write(byte(KELVIN));
            break;
    }
    //start printing on the next row
    lcd.setCursor(0,1);
    //prints the day
    lcd.print(DS3231::dayStr(clockTime.day));
    lcd.print(' ');
    print0X2LCD(clockTime.date);
    lcd.print('/');
    print0X2LCD(clockTime.month);
    lcd.print('/');
    lcd.print(clockTime.year);
    //TEST CUSTOM CHARACTERS ---> DELETE LATER
    lcd.write(byte(ASymbol));
    if(rtc.alarmState(1) && rtc.alarmState(2)){ // both alarms enabled
        lcd.write(byte(BothON));
    }
    else if(rtc.alarmState(1) && !rtc.alarmState(2)){ // alarm 1 enabled and alarm 2 disabled
        lcd.write(byte(A1ON));
    }
    else if(!rtc.alarmState(1) && rtc.alarmState(2)){ // alarm 1 disabled and alarm 2 enabled
        lcd.write(byte(A2ON));
    }
    else{ // no alarm enabled
        lcd.print("-");
    }
}

//prints the blinking alarm to the lcd
void printALarm2LCD(RTCalarm &alarm){
    lcd.clear();
    lcd.setCursor(6,0);
    print0X2LCD(alarm.hour);
    lcd.print(":");
    print0X2LCD(alarm.minutes);
    lcd.setCursor(6,1);
    lcd.print("ALARM!");
    delay(500);
    lcd.clear();
    delay(500);
}

//prints alarm information to the lcd
void displayAlarm2LCD(RTCalarm &alarm){
    lcd.noBlink();
    lcd.clear();
    lcd.setCursor(4,0);
    print0X2LCD(alarm.hour);
    lcd.print(":");
    print0X2LCD(alarm.minutes);
    lcd.print(" ");
    if(alarm.enabled)
        lcd.print("ON");
    else
        lcd.print("OFF");
    lcd.setCursor(6,1);
    lcd.print(DS3231::dayStr(alarm.day));
}

//is called when the alarm condition is met
void alarm(uint8_t alarmNumber){
    uint8_t passedSeconds = 0;
    lcd.noCursor();
    lcd.noBlink();
    RTCalarm alarm = rtc.readAlarm(alarmNumber);
    rtc.toggleSQW(true); // make LED blink while alarm is ringing
    while(digitalRead(SNOOZE_pin) == LOW && passedSeconds < 60){
        tone(BUZZ_pin,1245,500);
        printALarm2LCD(alarm);
        passedSeconds++;
    }
    if(passedSeconds >= 60 && !alarmIgnored[alarmNumber-1]){
        rtc.storeAlarmEEPROM(alarmNumber); // store current alarm in the memory
        alarmIgnored[alarmNumber-1] = true; // alarm was ignored
    }
    if(passedSeconds >= 60 && alarmIgnored[alarmNumber-1]){
        //sets new alarm to trigger 5 min from now;
        alarmIgnoredCount[alarmNumber-1]++;
        alarm.minutes += 5;
        if(alarm.minutes > 59){
            alarm.minutes %= 60;
            alarm.hour++;
            if(alarm.hour > 23){
                alarm.hour %= 24;
                if(alarm.day != DAILY){
                    uint8_t value = (uint8_t)alarm.day + 1;
                    if(value == 8)
                        alarm.day = (dayOfWeek)1;
                    else
                        alarm.day = (dayOfWeek)value;
                }
            }
        }
        if(alarm.day == DAILY){
            rtc.setAlarmDaily(alarmNumber, alarm.hour, alarm.minutes);
        }
        else{
            rtc.setAlarmWeekly(alarmNumber,alarm.hour,alarm.minutes,alarm.day);
        }
    }
    if(passedSeconds < 60 && alarmIgnoredCount[alarmNumber-1]){
        alarmIgnored[alarmNumber-1] = false; // alarm was ignored before, but not this time
        alarmIgnoredCount[alarmNumber-1] = 0;
        // restore the alarm that was stored in the memory
        alarm = rtc.readAlarmEEPROM(alarmNumber);
        if(alarm.day == DAILY){
            rtc.setAlarmDaily(alarmNumber, alarm.hour, alarm.minutes);
        }
        else{
            rtc.setAlarmWeekly(alarmNumber,alarm.hour,alarm.minutes,alarm.day);
        }
    }
    if(passedSeconds < 60 && !alarmIgnoredCount[alarmNumber-1]){
        //alarm was disabled correctly;
    }
    //if it was ignored 5 times in a row, ignore the alarm and restore the one stored in the memory
    if(alarmIgnoredCount[alarmNumber-1] == 5){
        alarmIgnored[alarmNumber-1] = false;
        alarmIgnoredCount[alarmNumber-1] = 0;
        // restore the alarm that was stored in the memory
        alarm = rtc.readAlarmEEPROM(alarmNumber);
        if(alarm.day == DAILY){
            rtc.setAlarmDaily(alarmNumber, alarm.hour, alarm.minutes);
        }
        else{
            rtc.setAlarmWeekly(alarmNumber,alarm.hour,alarm.minutes,alarm.day);
        }
    }
    rtc.toggleSQW(false); // turn off SQW
    rtc.snoozeAlarm(); // disable alarm flags
    AlarmState = false; // disable the interrupt variable
}

//is called to change clock values
// 1-hour; 2-minutes; 3-temperature measure unit;
// 4-DOW, 5-date; 6-month; 7-year;
void changeValue(uint8_t changeItem){
    uint8_t value = 1;
    RTCdata time = rtc.readTime();
    uint8_t maxDays;
    if(isLeapYear(time.year)){
        maxDays = leapYear[time.month-1];
    }
    else maxDays = normalYear[time.month-1];
    /*
     * setTime method:
     * 0 -> change hour; 1 -> change minutes; 2 -> change seconds;
     * setDate method:
     * 0 -> change day; 1 -> change date; 2 -> change month; 3 -> change year;
     */
    if(digitalRead(UP_pin)){
        switch (changeItem) {
            case 1:
                if (time.hour + value == 24) {
                    time.hour = 0;
                    rtc.setTime(0, time.hour);
                } else {
                    rtc.setTime(0, time.hour + value);
                }
                break;
            case 2:
                if (time.minutes + value == 60) {
                    time.minutes = 0;
                    rtc.setTime(1, time.minutes);
                } else rtc.setTime(1, time.minutes + value);
                break;
            case 3:
                if(checkTemperature == KELVIN) // 6
                    checkTemperature = CELCIUS; // 4
                else
                    checkTemperature++;
                break;
            case 4:
                //1-7
                if ((int8_t)time.day + value == 8) {
                    time.day = MONDAY;
                    rtc.setDate(0, time.day);
                } else rtc.setDate(0, time.day + value);
                break;
            case 5:
                if(time.date == maxDays)
                    rtc.setDate(1, 1);
                else rtc.setDate(1,time.date + 1);
                break;
            case 6:
                if ((int8_t)time.month == 12) {
                    time.month = JANUARY;
                    rtc.setDate(2, time.month);
                } else {
                    time.month = Month((uint8_t)time.month + value);
                    rtc.setDate(2, (int8_t)time.month);
                }
                if(isLeapYear(time.year)){
                    maxDays = leapYear[time.month-1];
                }
                else maxDays = normalYear[time.month-1];
                if(time.date > maxDays)
                    rtc.setDate(1,maxDays);
                break;
            case 7:
                if (time.year == 2199) {
                    time.year = 2000;
                    rtc.setDate(3, time.year);
                } else rtc.setDate(3, time.year + value);
                break;
            case 8:
                // toggle alarm 1
                rtc.toggleAlarm(1, !rtc.alarmState(1));
                break;
        }
    }
    else{
        switch (changeItem) {
            case 1:
                if (time.hour == 0) {
                    time.hour = 23;
                    rtc.setTime(0, time.hour);
                } else {
                    rtc.setTime(0, time.hour - value);
                }
                break;
            case 2:
                if (time.minutes == 0) {
                    time.minutes = 59;
                    rtc.setTime(1, time.minutes);
                }
                else rtc.setTime(1, time.minutes - value);
                break;
            case 3:
                if(checkTemperature == CELCIUS) // 6
                    checkTemperature = KELVIN; // 4
                else
                    checkTemperature--;
                break;
            case 4:
                //1-7
                if ((int8_t) time.day  == 1) {
                    time.day = SUNDAY;
                    rtc.setDate(0, time.day);
                }
                else rtc.setDate(0, (int8_t)time.day - value);
                break;
            case 5:
                if(time.date == 1)
                    rtc.setDate(1, maxDays);
                else rtc.setDate(1,time.date - 1);
                break;
            case 6:
                if ((int8_t) time.month == 1) {
                    time.month = DECEMBER;
                    rtc.setDate(2, time.month);
                }
                else {
                    time.month = Month((uint8_t)time.month - value);
                    rtc.setDate(2, (int8_t)time.month);
                }
                if(isLeapYear(time.year)){
                    maxDays = leapYear[time.month-1];
                }
                else maxDays = normalYear[time.month-1];
                if(time.date > maxDays)
                    rtc.setDate(1,maxDays);
                break;
            case 7:
                if (time.year == 2000) {
                    time.year = 2199;
                    rtc.setDate(3, time.year);
                }
                else rtc.setDate(3, time.year - value);
                break;
            case 8:
                //toggle alarm2;
                rtc.toggleAlarm(2, !(rtc.alarmState(2)));
                break;
        }
    }
    printTime2LCD();
    delay(200);
}

//edit interface using buttons
void editClock(){
    printTime2LCD();
    uint8_t cursorColPosition = 1; // 0 - 15
    uint8_t cursorRowPosition = 0; // 0 - 1
    uint8_t timesPressed = 1;
    lcd.setCursor(cursorColPosition,cursorRowPosition);
    lcd.blink();
    lcd.noCursor();
    delay(200);
    //runs until button is hold
    while(true){
        //check if SNOOZE button has been pressed
        if(cursorColPosition > 15){
            if(cursorRowPosition == 0){
                cursorRowPosition++;
                cursorColPosition = 2;
            }
            else{
                cursorRowPosition = 0;
                cursorColPosition = 1;
            }
            lcd.setCursor(cursorColPosition,cursorRowPosition);
            //lcd.blink();
        }
        //***********************************
       if(digitalRead(SNOOZE_pin) == HIGH){
           if(buttonActive == false){
               buttonActive = true;
               buttonTimer = millis();
           }
           if((millis() - buttonTimer > buttonPressTime) && buttonHoldActive == false){
               // button has been held down -> exit the loop
               buttonHoldActive = false;
               buttonActive = false;
               lcd.clear();
               lcd.setCursor(1,0);
               lcd.noBlink();
               lcd.noCursor();
               lcd.print("EXIT EDIT MENU");
               break;
           }
       }
       else{
           //button has only been pressed, not held down
           if(buttonActive == true){
               //increment the cursor / update the display
               printTime2LCD();
               buttonActive = false;
               timesPressed++;
               if(timesPressed == 9)
                   timesPressed = 1;
               switch (cursorRowPosition) {
                   case 0: // cursor on the first row
                       if(timesPressed == 3){ // on the 4th press ...
                           cursorColPosition += 11; // jump to the temperature measure unit
                       }
                       else cursorColPosition += 3;
                       break;
                   case 1: // cursor on the second row
                        if(timesPressed == 7){
                            cursorColPosition += 5; // jumps to the year
                        }
                        else if(timesPressed == 8){
                            cursorColPosition++; // jumps to the alarm symbol
                            //set up alarms using 2 buttons, 1 for each alarm
                        }
                        else cursorColPosition += 3;
                       break;
               }
           }
       }
       //increment or decrement the selected/blinking value;
       if(digitalRead(UP_pin) == HIGH || digitalRead(DOWN_pin) == HIGH){
           changeValue(timesPressed);
       }
        // check if alarm condition is met
        if(AlarmState){
            uint8_t alarmNumber = rtc.checkAlarmflag();
            if(alarmNumber == 0) // both alarm at the same time
                alarmNumber = 1; // only deal with alarm 1, ignore alarm 2;
            alarm(alarmNumber);
            delay(700); // debounce time
            printTime2LCD(); // update the screen since we've exited the alarm state
        }
        lcd.setCursor(cursorColPosition,cursorRowPosition);
        lcd.blink();
    }
}

//
void changeValueForAlarm(uint8_t alarmNumber, uint8_t changeItem) {
    RTCalarm alarm = rtc.readAlarm(alarmNumber);
    int8_t value, mode; // mode = 0 -> daily; mode = 1 -> weekly
    if(alarm.day == DAILY)
        mode = 0;
    else mode = 1;
    if (digitalRead(UP_pin) == HIGH)
        value = 1;
    else value = -1;
    switch (changeItem) {
        case 1: // change hour
            if ((int8_t)alarm.hour + value == 24)
                alarm.hour = 0;
            else if ((int8_t)alarm.hour + value == -1)
                alarm.hour = 23;
            else alarm.hour = (uint8_t)((int8_t) alarm.hour + value);
            if(mode == 0){
                rtc.setAlarmDaily(alarmNumber,alarm.hour,alarm.minutes);
            }
            else rtc.setAlarmWeekly(alarmNumber, alarm.hour, alarm.minutes, alarm.day);
            break;
        case 2: // change minute
            if ((int8_t)alarm.minutes + value == 60)
                alarm.minutes = 0;
            else if ((int8_t) alarm.minutes + value == -1)
                alarm.minutes = 59;
            else alarm.minutes = (uint8_t)((int8_t) alarm.minutes + value);
            if(mode == 0){
                rtc.setAlarmDaily(alarmNumber,alarm.hour,alarm.minutes);
            }
            else rtc.setAlarmWeekly(alarmNumber, alarm.hour, alarm.minutes, alarm.day);
            break;
        case 3: // toggle alarm on or off
            rtc.toggleAlarm(alarmNumber,!rtc.alarmState(alarmNumber));
            break;
        case 4: // change mode / day 0-7
            if((int8_t)alarm.day + value == 8) {
                alarm.day = DAILY; // 0;
            }
            else if((int8_t)alarm.day + value == -1){
                alarm.day = SUNDAY; // 7
            }
            else alarm.day = (dayOfWeek)((int8_t)alarm.day + value);
            if(alarm.day == DAILY)
                rtc.setAlarmDaily(alarmNumber,alarm.hour,alarm.minutes);
            else
                rtc.setAlarmWeekly(alarmNumber,alarm.hour,alarm.minutes,alarm.day);
            break;
    }
}

//1 for alarm 1 and 2 for alarm 2
void editAlarm(uint8_t alarmNumber){
    RTCalarm alarmTime = rtc.readAlarm(alarmNumber);
    uint8_t cursorColPosition = 5; // 0 - 15
    uint8_t cursorRowPosition = 0; // 0 - 1
    uint8_t timesPressed = 1;
    displayAlarm2LCD(alarmTime);
    lcd.setCursor(cursorColPosition,cursorRowPosition);
    lcd.blink();
    lcd.noCursor();
    delay(200);
    while(true){
        //***********************************
        if(digitalRead(SNOOZE_pin) == HIGH){
            if(buttonActive == false){
                buttonActive = true;
                buttonTimer = millis();
            }
            if((millis() - buttonTimer > buttonPressTime) && buttonHoldActive == false){
                // button has been held down -> exit the loop
                buttonHoldActive = false;
                buttonActive = false;
                lcd.clear();
                lcd.setCursor(1,0);
                lcd.noBlink();
                lcd.noCursor();
                lcd.print("EXIT EDIT MENU");
                break;
            }
        }
        else{
            //button has only been pressed, not held down
            if(buttonActive == true){
                //increment the cursor / update the display
                buttonActive = false;
                timesPressed++;
                switch (timesPressed) {
                    case 2:
                        cursorColPosition += 3;
                        break;
                    case 3:
                        if(!alarmTime.enabled)
                            cursorColPosition += 4;
                        else cursorColPosition += 3;
                        break;
                    case 4: // next row
                        if(alarmTime.day == DAILY)
                            cursorColPosition = 10;
                        else cursorColPosition = 8;
                        cursorRowPosition = 1;
                        break;
                    case 5:
                        timesPressed = 1; // restart the cycle;
                        cursorRowPosition = 0;
                        cursorColPosition = 5;
                        break;
                }
            }
        }
        if(digitalRead(UP_pin) == HIGH || digitalRead(DOWN_pin) == HIGH){
            changeValueForAlarm(alarmNumber, timesPressed);
            alarmTime = rtc.readAlarm(alarmNumber); // update alarmTime
            if(timesPressed == 3 && cursorColPosition == 12)
                cursorColPosition--;
            else if(timesPressed == 3 && cursorColPosition == 11)
                cursorColPosition++;
            if(timesPressed == 4 && cursorColPosition == 10)
                cursorColPosition -= 2;
            else if(timesPressed == 4 && alarmTime.day == DAILY)
                cursorColPosition += 2;
            displayAlarm2LCD(alarmTime);
            delay(200);
        }
        if(AlarmState){
            uint8_t alarmNumber = rtc.checkAlarmflag();
            if(alarmNumber == 0) // both alarmTime at the same time
                alarmNumber = 1; // only deal with alarmTime 1, ignore alarmTime 2;
            alarm(alarmNumber);
            displayAlarm2LCD(alarmTime);
            delay(700); // debounce time
        }
        lcd.setCursor(cursorColPosition,cursorRowPosition);
        lcd.blink();
    }
    // When editing the alarm, it counts as not ignored!
    alarmIgnored[alarmNumber-1] = false;
    alarmIgnoredCount[alarmNumber-1] = 0;
    rtc.storeAlarmEEPROM(alarmNumber);
}

void _ISR(){
    AlarmState = true;
}

void setup(){
    //initialize lcd library for the display ( 2 rows of 16 characters each )
    lcd.begin(16,2);
    //disable all pins that might cause alarm interrupts ---> might change this later
    rtc.begin();
    pinMode(INT_pin,INPUT);
    attachInterrupt(digitalPinToInterrupt(2),_ISR,FALLING); // activating the interrupt when going from high to low
    pinMode(SNOOZE_pin,INPUT);
    pinMode(UP_pin,INPUT);
    pinMode(DOWN_pin,INPUT);
    pinMode(BUZZ_pin,OUTPUT);
    //create custom characters (up to 8 characters)
    lcd.createChar(ASymbol,alarmSymbol);
    lcd.createChar(A1ON,alarm1ON);
    lcd.createChar(A2ON,alarm2ON);
    lcd.createChar(BothON,bothAlarms);
    lcd.createChar(CELCIUS,celcius);
    lcd.createChar(FAHRENHEIT,fahrenheit);
    lcd.createChar(KELVIN,kelvin);
}

void loop(){
    //alarm condition is met
    if(AlarmState){
        uint8_t alarmNumber = rtc.checkAlarmflag();
        if(alarmNumber == 0) // both alarm at the same time
            alarmNumber = 1; // only deal with alarm 1, ignore alarm 2;
        alarm(alarmNumber);
        delay(700); // debounce time
    }
    // enter edit mode
    if(digitalRead(SNOOZE_pin) == HIGH){
        editClock();
        delay(500); // debounce time
    }

    // edit alarm1
    if(digitalRead(UP_pin) == HIGH){
        editAlarm(1);
        delay(500);
    }

    //edit alarm2
    if(digitalRead(DOWN_pin) == HIGH){
        editAlarm(2);
        delay(500);
    }

    //enter SWQ edit mode
    printTime2LCD();
}
