//
// Created by petru on 02.01.2021.
//

#ifndef DS3231_NEW_DS3231_H
#define DS3231_NEW_DS3231_H

#include <Arduino.h>
#include <Wire.h>

#define DS3231_ADDRESS 0x68
#define EEPROM_ADDRESS 0x57

/*-----------------------------------------------------------------------------
                            * 0x00 -> seconds
                            * 0x01 -> minutes
                            * 0x02 -> hours
                            * 0x03 -> day (1-7)
                            * 0x04 -> date (01-31)
                            * 0x05 -> month
                            * 0x06 -> year
 ------------------------------------------------------------------------------*/

#define REG_TIME 0x00
#define REG_DAY 0x03
#define REG_DATE 0x04

/*-----------------------------------------------------------------------------
                            * Each alarm has 4 registers:
                            * -seconds
                            * -minutes
                            * -hours
                            * -date
 ------------------------------------------------------------------------------*/

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

/// @brief Struct that holds time & date information of main registers.
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


/// @brief Struct that holds values for alarm time and day registers.
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

/**
 * @brief This is the main class of the library.
 *
 * The class contains the methods which interact with the RTC module
 * to read and write to the time-keeping registers, to control the alarm states,
 * to read temperature and to control the square wave output from SQW pin.
 */
class DS3231 {
private:
    //Private Class Members
    /// false -> enables 32KHz SQW; true -> allows A1F & A2F to set INT/SQW pin low in alarm condition.
    bool INTCtr;
    /// holds last read clock data.
    RTCdata clockTime;
    /// holds alarm1 information.
    RTCalarm alarm1;
    /// holds alarm2 information.
    RTCalarm alarm2;
private:
    //Private Class Methods
    /**
     * Method to set a specific bit of a byte to 1.
     * @param byte The byte that needs to be modified
     * @param bit The bit that needs modified (0-7)
     * @return Returns the modified byte
     */
    static uint8_t setHigh(uint8_t byte, uint8_t bit);
    /**
      * Method to set a specific bit of a byte to 0.
      * @param byte The byte that needs to be modified
      * @param bit The bit that needs modified (0-7)
      * @return Returns the modified byte
      */
    static uint8_t setLow(uint8_t byte, uint8_t bit);
    ///@brief Method to convert binary coded decimal to binary.
    static uint8_t BCDtoDEC(const uint8_t bcd);
    ///@brief Method to convert binary to binary coded decimal.
    static uint8_t DECtoBCD(const uint8_t dec);
    /**
     * Method to read a specific register of the device.
     * @param reg The register's address
     * @param byteBuffer A byte buffer that holds the data that's being read
     * @param bytes The number of bytes that need to be read (max 7!)
     */
    static void readRegister(uint8_t reg, uint8_t byteBuffer[], const uint16_t bytes);
    /**
     * Method to write data to a specific register of the device.
     * @param reg The register's address
     * @param byteBuffer A byte buffer that holds the data that's being transferred
     * @param bytes The number of bytes that need to be written (max 7!)
     */
    static void writeRegister(const uint8_t reg, const uint8_t byteBuffer[], const uint8_t bytes);
    /**
     * Method to read data written to a specific address on the EEPROM chip of the device.
     * @param address The address where data is being stored
     * @param byteBuffer A byte buffer that holds the data
     * @param bytes The number of bytes that need to be read.
     */
    static void readEEPROM(uint8_t address, uint8_t byteBuffer[], const uint16_t bytes);
    /**
     * Method to write data to a specific address on the EEPROM chip of the device.
     * @param address address The address where data is being stored
     * @param byteBuffer A byte buffer that holds the data
     * @param bytes The number of bytes that need to be written.
     */
    static void writeEEPROM(uint8_t address, uint8_t byteBuffer[], const uint16_t bytes);
    ///Method to toggle the INTCN bit (bit 2 of control register).
    void writeINTCtr(bool enable);
public:
    /// By default, no SQW is outputted.
    DS3231(bool INTCtr = true);
    /// Starts the library.
    ///
    /// Disables alarm flags, sets the INTCtr bit, restores alarms from memory, sets time.
    void begin();
    /// converts dayOfWeek data to string
    static const char* dayStr(const dayOfWeek day);
    /// converts Month data to string
    static const char* monthStr(const Month month);

    /*--------------------------------------------------------------------------------------------------------------------
     *                                   Methods to interact with the clock time
     ---------------------------------------------------------------------------------------------------------------------*/

    /**
     * Method to check the running state of the clock.
     * @return True if clock runs in 12 hour mode and false otherwise.
     */
    bool is_12();
    /// Changes clock to run in 12 hour mode.
    void set_12();
    /// Changes clock to run in 24 hour mode.
    void set_24();
    /**
     * Method to change one item related to time (hour, minutes or seconds).
     * @param number Represents the time item. 0 -> hour, 1 -> minute, 2->seconds
     * @param value New value of the item
     */
    void setTime(uint8_t number, uint8_t value);
     ///Method to change all items related to time at once.
    void setTime(const uint8_t hours, const uint8_t minutes, const uint8_t seconds);
    /**
     * Method to change one item related to date (day, date, month or year).
     * @param number Represents the date item. 0->day, 1->date, 2->month, 3->year
     * @param value New value of the item
     */
    void setDate(uint8_t number, uint16_t value);
    ///Method to change all items related to date at once.
    void setDate(const dayOfWeek day, const Month month, const uint8_t date, const uint16_t year);
    /**
     * Method to read the time keeping registers of the device.
     * @return Returns an RTCdata object that holds the information from the registers
     */
    RTCdata readTime();

    /*--------------------------------------------------------------------------------------------------------------------
     *                                   Methods to interact with the alarms
     ---------------------------------------------------------------------------------------------------------------------*/

    /**
     * Method to check whether the alarms were triggered.
     * @return Returns 0 if both alarm flags are off, 1 if alarm 1 flag is on, 2 if alarm 2 flag is on
     */
    uint8_t checkAlarmFlag();
    /**
     * Method to change the alarm values.
     *
     * This method will only store information in memory. The alarm information will not be communicated to DS3231.
     * @param alarmNumber 1 -> changes affect alarm1; 2 -> changes affect alarm2
     * @param alarm RTCalarm object which holds information about the new alarm
     */
    void setAlarm(uint8_t alarmNumber, RTCalarm& alarm);
    /**
     * Method to store one of the two alarms in memory in case power is lost.
     * @param alarmNumber 1 -> Stores alarm1; 2 -> Stores alarm2
     */
    void storeAlarmEEPROM(uint8_t alarmNumber);
    /**
     * Method to read one of the two alarms from memory.
     * @param alarmNumber 1 -> Reads alarm1; 2 -> Reads alarm2
     */
    RTCalarm readAlarmEEPROM(uint8_t alarmNumber);
    /**
     * Method to check whether one of the two alarms has been triggered.
     *
     * Alarm flags are set to 1 every time the registers of the alarm match the registers of the clock (based on the alarm mask).
     * @param alarmNumber The number of the alarm (1 or 2)
     * @return True means alarm has been triggered, False means alarm has not been triggered
     */
    bool alarmState(uint8_t alarmNumber) const;
    /**
     * Method to read alarm information.
     * @param alarmNumber The number of the alarm (1 or 2)
     * @return Returns an RTCalarm object containing the information about the specified alarm
     */
    RTCalarm readAlarm(uint8_t alarmNumber);
    /**
     * Method to set the alarm to trigger every time day at the specified time.
     *
     * Alarm triggers when the hour and minute registers of the alarm match
     * the hour and minute registers of the clock.
     * @param alarmNumber Number of the alarm (1 or 2)
     */
    void setAlarmDaily(uint8_t alarmNumber, uint8_t hour, const uint8_t minute);
    /**
     * Method to set the alarm to trigger at the time and day of the week specified (once a week).
     * @param alarmNumber alarmNumber Number of the alarm (1 or 2)
     */
    void setAlarmWeekly(uint8_t alarmNumber, uint8_t hour, const uint8_t minute, const dayOfWeek day);
    /**
     * Method to toggle the alarm ON or OFF.
     * @param alarmNumber The number of the alarm (1 or 2)
     * @param enable True -> enables the alarm; False -> disables the alarm
     */
    void toggleAlarm(uint8_t alarmNumber, bool enable);
    ///Method to stop the alarm.
    ///
    ///This method sets both alarm flags to 0.
    void snoozeAlarm();

    /*--------------------------------------------------------------------------------------------------------------------
     *                                   Methods to read the temperature
     ---------------------------------------------------------------------------------------------------------------------*/

    /// Reads temperature registers and returns value in Celcius.
    float readCelcius();
    /// Uses readCelcius method, converts value to Fahrenheit.
    float readFahrenheit();
    /// Uses readCelcius method, converts value to Kelvin.
    float readKelvin();

    /*--------------------------------------------------------------------------------------------------------------------
     *                                   Methods to output sqw
     ---------------------------------------------------------------------------------------------------------------------*/

    /**
     * Method to control the state of the SQW pin.
     *
     * When enabled, INTCN bit is set low and a square wave is generated at SQW pin.
     * During this time, no alarm can be triggered, because one of the alarm conditions is
     * INTCN bit to be set to 1.
     * @param enable True -> square wave signal is generated; False -> SQW pin stays HIGH and alarms can be triggered
     */
    void toggleSQW(bool enable);
    /**
     * Method to modify the frequency of the square wave signal outputted by the SQW pin.
     *
     * The device can generate 4 frequencies : 1Hz, 1kHz, 4kHz and 8kHz.
     * @param mode 0 -> 1Hz; 1 -> 1kHz; 2 -> 4kHz; 3 -> 8kHz;
     */
    void setSQW(uint8_t mode);
    /**
     * Method to enable or disable the 32kHz square wave outputted at pin 32K.
     *
     * 32K pin goes in high impedance state when disabled.
     * @param enable true -> enables the square wave signal; false -> disables the square wave signal
     */
    void toggle32kHz(bool enable);
    /**
     * Method to enable or disable the internal oscillator.
     *
     * When powered by VCC, the oscillator stays on no matter the state of the control register.
     * This function only has effect when the device is powered by VBAT. While disabled, all registeres
     * are static!
     * @param enable True -> enables the oscillator; False -> disables the oscillator
     */
    void enableOSC(bool enable);
};


#endif //DS3231_NEW_DS3231_H
