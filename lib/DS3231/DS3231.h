//
// Created by petru on 02.01.2021.
//

#ifndef DS3231_NEW_DS3231_H
#define DS3231_NEW_DS3231_H

#include <Arduino.h>
#include <Wire.h>

#define DS3231_ADDRESS 0x68
#define EEPROM_ADDRESS 0x57

/****************************************************************************
                            * 0x00 -> seconds
                            * 0x01 -> minutes
                            * 0x02 -> hours
                            * 0x03 -> day (1-7)
                            * 0x04 -> date (01-31)
                            * 0x05 -> month
                            * 0x06 -> year
 *****************************************************************************/

#define REG_TIME 0x00
#define REG_DAY 0x03
#define REG_DATE 0x04

/*****************************************************************************
                            * Each alarm has 4 registers:
                            * -seconds
                            * -minutes
                            * -hours
                            * -date
 ******************************************************************************/

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

/// Structer that holds time & date information of main registers.
struct RTCdata{
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hour;
    dayOfWeek day;
    Month month;
    uint16_t year;
    uint8_t date;
    bool pm;
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


/// Structer that holds values for alarm time and day registers.
struct RTCalarm{
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hour;
    dayOfWeek day;
    bool enabled;
    bool pm;
    RTCalarm operator = (const RTCalarm& alarm) {
        this->seconds = alarm.seconds;
        this->minutes = alarm.minutes;
        this->hour = alarm.hour;
        this->day = alarm.day;
        this->enabled = alarm.enabled;

        return *this;
    }
};

/***********************************************************************************
 * This is the main class of the library.
 * The class contains the methods which interact with the RTC module
 * to read and write to the time-keeping registers, to control the alarm states,
 * to read temperature and to control the square wave output from SQW pin.
 **********************************************************************************/
class DS3231 {
private:
    //Private Class Members
    bool INTCtr; /// false -> enables 32KHz SQW; true -> allows A1F & A2F to set INT/SQW pin low in alarm condition
    RTCdata clockTime; /// holds last read clock data
    RTCalarm alarm1; /// holds alarm1 information
    RTCalarm alarm2; /// holds alarm2 information
private:
    //Private Class Methods
    static uint8_t setHigh(uint8_t byte, uint8_t bit); /// sets desired bit to 1
    static uint8_t setLow(uint8_t byte, uint8_t bit); /// sets desired bit to 0
    static uint8_t BCDtoDEC(const uint8_t bcd); /// converts binary coded decimal to binary
    static uint8_t DECtoBCD(const uint8_t dec); /// converts binary to binary coded decimal
    static void readRegister(uint8_t reg, uint8_t byteBuffer[], const uint16_t bytes); /// reads max. 7 bytes from 'reg' register
    static void writeRegister(const uint8_t reg, const uint8_t byteBuffer[], const uint8_t bytes); /// writes max 7. bytes to 'reg' register
    static void readEEPROM(uint8_t address, uint8_t byteBuffer[], const uint16_t bytes); /// reads a number of bytes stored at the given address of the EEPROM
    static void writeEEPROM(uint8_t address, uint8_t byteBuffer[], const uint16_t bytes); /// writes a number of bytes to a given address of the EEPROM
    void writeINTCtr(bool enable); /// toggles INTCN (bit 2 of control register)
    RTCdata readTime_12();
public:
    DS3231(bool INTCtr = true); /// By default, no SQW is outputted
    void begin(); /// Disables alarm flags, sets the INTCtr bit, restores alarms from memory, sets time
    static const char* dayStr(const dayOfWeek day); /// converts dayOfWeek data to string
    static const char* monthStr(const Month month); /// converts Month data to string

    /***********************************************************************************************
     *                  Methods to interact with the clock time
     **********************************************************************************************/

    bool is_12(); /// true -> runs in 12 hour mode; false -> runs in 24 hour mode
    void set_12(); /// Changes clock to run in 12 hour mode
    void set_24(); /// Changes clock to run in 24 hour mode
    void setTime(uint8_t number, uint8_t value); /// 0->hour, 1->minutes, 2->seconds
    void setTime(const uint8_t hours, const uint8_t minutes, const uint8_t seconds); // sets time (used by begin())
    void setDate(uint8_t number, uint16_t value); /// 0->day, 1->date, 2->month, 3->year
    void setDate(const dayOfWeek day, const Month month, const uint8_t date, const uint16_t year); // sets date (used by begin())
    RTCdata readTime(); /// Reads the time from DS3231 chip and returns RTCdata object

    /***********************************************************************************************
     *                  Methods to interact with the alarms
     **********************************************************************************************/

    uint8_t checkAlarmFlag(); /// returns 0 if both alarm flags are off, 1 if alarm 1 flag is on, 2 if alarm 2 flag is on
    void setAlarm(uint8_t alarmNumber, RTCalarm& alarm); /// alarmNumber = 1 -> changes alarm 1; alarmNumber = 2 -> changes alarm 2
    void storeAlarmEEPROM(uint8_t alarmNumber); /// stores one of the alarms in memory
    RTCalarm readAlarmEEPROM(uint8_t alarmNumber); /// reads one of the alarms from memory
    bool alarmState(uint8_t alarmNumber) const; /// Returns alarm 1/2 enable member (whether alarm is ON or OFF);
    RTCalarm readAlarm(uint8_t alarmNumber); /// returns alarm 1/2 information in form of RTCalarm object
    void setAlarmDaily(uint8_t alarmNumber, uint8_t hour, const uint8_t minute); /// sets alarm 1/2 to be triggered every day
    void setAlarmWeekly(uint8_t alarmNumber, uint8_t hour, const uint8_t minute, const dayOfWeek day); /// sets alarm 1/2 to be triggered once a week
    void toggleAlarm(uint8_t alarmNumber, bool enable); /// alarmNumber = 1 -> alarm 1; alarmNumber = 2 -> alarm 2 | true -> enables the alarm; false -> disables the alarm
    void snoozeAlarm(); /// disables alarm 1 and 2 flags (both at the same time)

    /***********************************************************************************************
     *                  Methods to read the temperature
     **********************************************************************************************/

    float readCelcius(); /// reads temperature registers and returns value in Celcius
    float readFahrenheit(); /// uses readCelcius method, converts value to Fahrenheit
    float readKelvin(); /// uses readCelcius method, converts value to Kelvin

    /***********************************************************************************************
     *                  Methods to output sqw
     **********************************************************************************************/

    void toggleSQW(bool enable); /// when enabled, INTCN bit is set low and SQW is outputted (no alarm can be triggered)
    void setSQW(uint8_t mode); /// Sets the frequency of the SQW; 0 : 1Hz, 1 : 1kHz, 2: 4kHz, 3 : 8kHz
    void toggle32kHz(bool enable); /// enables or disables the 32kHz square wave outputted at pin 32K
    void enableOSC(bool enable);  /// starts or stops the internal oscillator (effect only in battery mode)
};


#endif //DS3231_NEW_DS3231_H
