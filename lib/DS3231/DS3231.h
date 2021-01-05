//
// Created by petru on 02.01.2021.
//

#ifndef DS3231_NEW_DS3231_H
#define DS3231_NEW_DS3231_H

#include <Arduino.h>
#include <Wire.h>

#define DS3231_ADDRESS 0x68
#define EEPROM_ADDRESS 0x57

#define REG_TIME 0x00
#define REG_DAY 0x03
#define REG_DATE 0x04
/****************************************************************************
                            * 0x00 -> seconds
                            * 0x01 -> minutes
                            * 0x02 -> hours
                            * 0x03 -> day (1-7)
                            * 0x04 -> date (01-31)
                            * 0x05 -> month
                            * 0x06 -> year
 *****************************************************************************/

#define REG_ALARM1_SEC 0x07
#define REG_ALARM1_MIN 0x08
#define REG_ALARM1_H 0x09
#define REG_ALARM1_D 0x0A
#define REG_ALARM2_MIN 0x0B
#define REG_ALARM2_H 0x0C
#define REG_ALARM2_D 0x0D

#define A1_MASK_DAYLY 0b1000
#define A1_MASK_WEEKLY 0b0000
#define A2_MASK_DAYLY 0b1000
#define A2_MASK_WEEKLY 0b0000

/*****************************************************************************
                            * Each alarm has 4 registers:
                            * -seconds
                            * -minutes
                            * -hours
                            * -date
 ******************************************************************************/

#define REG_CONTROL 0x0E
#define REG_STATUS 0x0F
#define REG_TEMP_INT 0x11
#define REG_TEMP_FLOAT 0x12


enum dayOfWeek : uint8_t{
    DAILY = 0,
    MONDAY = 1,
    TUESDAY = 2,
    WEDNESDAY = 3,
    THURSDAY = 4,
    FRIDAY = 5,
    SATURDAY = 6,
    SUNDAY = 7
};

enum Month : uint8_t{
    JANUARY = 1,
    FEBRUARY = 2,
    MARCH = 3,
    APRIL = 4,
    MAY = 5,
    JUNE = 6,
    JULY = 7,
    AUGUST = 8,
    SEPTEMBER = 9,
    OCTOBER= 10,
    NOVEMBER = 11,
    DECEMBER = 12
};

// values for main time and date registers
struct RTCdata{
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hour;
    dayOfWeek day;
    Month month;
    uint16_t year;
    uint8_t date;
    bool operator == (RTCdata clock){
        return (this->seconds == clock.seconds && this->minutes == clock.minutes && this->hour == clock.hour &&
                this->day == clock.day && this->month == clock.month && this->year == clock.year &&
                this->date == clock.date);
    }
    RTCdata operator = (const RTCdata& clock){
        this->seconds = clock.seconds;
        this->minutes = clock.minutes;
        this->hour = clock.hour;
        this->date = clock.date;
        this->day = clock.day;
        this->month = clock.month;
        this->year = clock.year;

        return *this;
    }
};


// values for alarm time and day registers

struct RTCalarm{
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hour;
    dayOfWeek day;
    bool enabled;
    RTCalarm operator = (const RTCalarm& alarm) {
        this->seconds = alarm.seconds;
        this->minutes = alarm.minutes;
        this->hour = alarm.hour;
        this->day = alarm.day;
        this->enabled = alarm.enabled;

        return *this;
    }
};

class DS3231 {
private:
    //Private Class Members
    bool INTCtr; // false -> enables 32KHz SQW; true -> allows A1F & A2F to set INT/SQW pin low in alarm condition
    RTCdata clockTime; // holds last read clock data
    RTCalarm alarm1; // holds alarm1 information
    RTCalarm alarm2; // holds alarm2 information
private:
    //Private Class Methods
    static uint8_t setHigh(uint8_t byte, uint8_t bit);
    static uint8_t setLow(uint8_t byte, uint8_t bit);
    static uint8_t BCDtoDEC(const uint8_t bcd);
    static uint8_t DECtoBCD(const uint8_t dec);
    static void readRegister(uint8_t reg, uint8_t byteBuffer[], const uint16_t bytes);
    static void writeRegister(const uint8_t reg, const uint8_t byteBuffer[], const uint8_t bytes);
    static void readEEPROM(uint8_t address, uint8_t byteBuffer[], const uint16_t bytes);
    static void writeEEPROM(uint8_t address, uint8_t byteBuffer[], const uint16_t bytes);
    uint8_t writeINTCtr(bool enable);
public:
    DS3231(bool INTCtr = true); // by default the clock has SQW disabled
    void begin(); // disables alarm flags, sets the INTCtr bit, disables alarms, sets time
    static const char* dayStr(const dayOfWeek day);
    static const char* monthStr(const Month month);
    //Methods to interact with the clock time
    void setTime(uint8_t number, uint8_t value); // 0->hour, 1->minutes, 2->seconds
    void setTime(const uint8_t hours, const uint8_t minutes, const uint8_t seconds); // sets time (used by begin())
    void setDate(uint8_t number, uint8_t value); // 0->day, 1->date, 2->month, 3->year
    void setDate(const dayOfWeek day, const Month month, const uint8_t date, const uint16_t year); // sets date (used by begin())
    RTCdata readTime(); // reads the time from DS3231 chip
    //Methods to interact with the alarms
    uint8_t checkAlarmflag(); // returns 0 if both alarm flags are off, 1 if alarm 1 flag is on, 2 if alarm 2 flag is on
    void setAlarm(uint8_t alarmNumber, RTCalarm& alarm);
    void storeAlarmEEPROM(uint8_t alarmNumber);
    RTCalarm readAlarmEEPROM(uint8_t alarmNumber);
    bool alarmState(uint8_t alarmNumber) const; // returns alarm 1/2 enable member;
    RTCalarm readAlarm(uint8_t alarmNumber); // returns alarm 1/2 information
    void setAlarmDaily(uint8_t alarmNumber, uint8_t hour, const uint8_t minute); // sets alarm 1/2 to be triggered every day
    void setAlarmWeekly(uint8_t alarmNumber, uint8_t hour, const uint8_t minute, const dayOfWeek day); // sets alarm 1/2 to be triggered once a week
    void toggleAlarm(uint8_t alarmNumber, bool enable); // 1 -> alarm 1/ 2 -> alarm 2 || true -> enables the alarm; false -> disables the alarm
    void snoozeAlarm(); // disables alarm 1 and 2 flags || both at the same time
    //Methods to read the temperature
    float readCelcius();
    float readFahrenheit();
    float readKelvin();
    //Methods to output sqw
    void toggleSQW(bool enable);
    void setSQW(uint8_t mode); //0 : 1Hz, 1 : 1kHz, 2: 4kHz, 3 : 8kHz
    void toggle32kHz(bool enable);
};


#endif //DS3231_NEW_DS3231_H
