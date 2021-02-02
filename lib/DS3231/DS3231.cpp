//
// Created by petru on 02.01.2021.
//

#include "DS3231.h"


DS3231::DS3231(bool INTCtr) {
    // sets the INT bit
    this->INTCtr = INTCtr;
    clockTime.seconds = 0;
    clockTime.minutes = 0;
    clockTime.hour = 0;
    clockTime.day = FRIDAY;
    clockTime.date = 1;
    clockTime.month = JANUARY;
    clockTime.year = 2021;
    // alarms are disabled at start
    // A1IE & A2IE bits are set low in the begin method
    alarm1.enabled = false;
    alarm2.enabled = false;
}

/**
 * @details This method reads the alarms stored in memory and sets them accordingly.
 * It also disables the alarm flags in case the power was lost while alarm was triggered.
 */
initialize the DS3231 RTC
void DS3231::begin(){
    Wire.begin(); // initializes the library
    DS3231::writeINTCtr(true); // enables INTCN bit from Control register
    // sets the alarm interrupts and disables any alarm flags
    snoozeAlarm();
    // restores the alarm in case of power loss
    alarm1 = readAlarmEEPROM(1);
    if(alarm1.day == DAILY)
        setAlarmDaily(1,alarm1.hour, alarm1.minutes);
    else
        setAlarmWeekly(1, alarm1.hour, alarm1.minutes, alarm1.day);
    alarm2 = readAlarmEEPROM(2);
    if(alarm2.day == DAILY)
        setAlarmDaily(2,alarm2.hour, alarm2.minutes);
    else
        setAlarmWeekly(2, alarm2.hour, alarm2.minutes, alarm2.day);
    toggleAlarm(1,alarm1.enabled);
    toggleAlarm(2,alarm2.enabled);
}

// converts binary coded decimal to decimal
uint8_t DS3231::BCDtoDEC(const uint8_t code) {
    uint8_t digit1, digit2;
    digit1 = (code & 0xF0) >> 4;
    digit2 = (code & 0x0F);
    return (digit1 * 10 + digit2);
}

// converts decimal to binary
uint8_t DS3231::DECtoBCD(const uint8_t dec) {
    uint8_t code, digit1, digit2;
    digit1 = ((dec / 10) << 4) | 0x0F;
    digit2 = (dec % 10) | 0xF0;
    code = digit1 & digit2;
    return code;
}

/**
 * @details This method is private because it is used by other methods (mainly the alarm methods).
 */
void DS3231::writeINTCtr(bool enable) {
    //reads from control register
    uint8_t byte[1];
    DS3231::readRegister(REG_CONTROL,byte,1);
    //modifies the INTCN bit
    if(enable){
        byte[0] = DS3231::setHigh(byte[0],2);
        INTCtr = true;
    }
    else{
        byte[0] = DS3231::setLow(byte[0],2);
        INTCtr = false;
    }
    //writes to control register
    DS3231::writeRegister(REG_CONTROL,byte,1);
}

/*--------------------------------------------------------------------------------------------------------------------
 *                                          READ FROM DS3231
---------------------------------------------------------------------------------------------------------------------*/

/**
 * @details This method uses the Wire library to communicate with the device via I2C and read
 * the desired register of the device.
 */
void DS3231::readRegister(const uint8_t reg, uint8_t byteBuffer[], const uint16_t bytes) {
    Wire.beginTransmission(DS3231_ADDRESS);
    Wire.write(reg);// specifies what register to read from
    uint8_t check = Wire.endTransmission(false);
    if(check == 0){
        uint8_t length = Wire.requestFrom((int)DS3231_ADDRESS, (int)bytes, (int)true); // requests the content of the register
        if(length == bytes){
            for(uint8_t i=0; i<bytes; i++)
                byteBuffer[i] = Wire.read();
        }
    }
}

/*--------------------------------------------------------------------------------------------------------------------
 *                                           WRITE TO DS3231
---------------------------------------------------------------------------------------------------------------------*/

/**
 * @details This method uses the Wire library to communicate with the device via I2C and write
 * information to the specified register.
 */
void DS3231::writeRegister(const uint8_t reg, const uint8_t* value, const uint8_t bytes) {
    Wire.beginTransmission(DS3231_ADDRESS);
    Wire.write(reg); // specifies what register to write to
    //Wire.write(value); // sends the byte to that register
    for(uint8_t i = 0; i<bytes; i++){
        Wire.write(value[i]);
    }
    Wire.endTransmission(true);
}

/*--------------------------------------------------------------------------------------------------------------------
 *                                           Interact with the EEPROM
---------------------------------------------------------------------------------------------------------------------*/

void DS3231::writeEEPROM(uint8_t address, uint8_t byteBuffer[], const uint16_t bytes){
    int remainingBytes = bytes;   // bytes left to write
    int offsetDataBuffer = 0;           // current offset in dataBuffer pointer
    int offsetPage;                     // current offset in page
    int nextByte = 0;                   // next n bytes to write
    int pageSize = 32;

    // write all bytes in multiple steps
    while (remainingBytes > 0) {
        // calc offset in page
        offsetPage = address % pageSize;
        // maximal 30 bytes to write
        nextByte = min(min(remainingBytes, 30), pageSize - offsetPage);
        Wire.beginTransmission(EEPROM_ADDRESS);
        if (Wire.endTransmission() == 0) {
            Wire.beginTransmission(EEPROM_ADDRESS);
            Wire.write(address >> 8);
            Wire.write(address & 0xFF);
            byte *adr = byteBuffer + offsetDataBuffer;
            Wire.write(adr, bytes);
            Wire.endTransmission();
            delay(10);
        }
        //write(address, dataBuffer, offsetDataBuffer, nextByte);
        remainingBytes -= nextByte;
        offsetDataBuffer += nextByte;
        address += nextByte;
    }
}

void DS3231::readEEPROM(uint8_t address, uint8_t byteBuffer[], const uint16_t bytes) {
    int remainingBytes = bytes;
    int offsetDataBuffer = 0;
    // read until are bytes bytes read
    while (remainingBytes > 0) {
        // read maximum 32 bytes
        int nextByte = remainingBytes;
        if (nextByte > 32) { nextByte = 32;}
        Wire.beginTransmission(EEPROM_ADDRESS);
        if (Wire.endTransmission() == 0) {
            Wire.beginTransmission(EEPROM_ADDRESS);
            Wire.write(address >> 8);
            Wire.write(address & 0xFF);
            if (Wire.endTransmission() == 0) {
                int r = 0;
                Wire.requestFrom(EEPROM_ADDRESS, bytes);
                while (Wire.available() > 0 && r < bytes) {
                    byteBuffer[offsetDataBuffer + r] = (byte)Wire.read();
                    r++;
                }
            }
        }
        address += nextByte;
        offsetDataBuffer += nextByte;
        remainingBytes -= nextByte;
    }
}

/*--------------------------------------------------------------------------------------------------------------------
 *                                            EDIT SINGLE BITS
---------------------------------------------------------------------------------------------------------------------*/

// sets a bit (0 to 7) to 1 and returns the modified byte
uint8_t DS3231::setHigh(uint8_t byte, uint8_t bit) {
    uint8_t value = 0x01 << bit;
    return byte | value;
}

//sets a bit (0 to 7) to 1 and returns the modified byte
uint8_t DS3231::setLow(uint8_t byte, uint8_t bit) {
    uint8_t value = (0x01 << bit) ^ 0xFF;
    return byte & value;
}

/*--------------------------------------------------------------------------------------------------------------------
 *                                            CONVERTS DAY/DATE TO STRINGS
---------------------------------------------------------------------------------------------------------------------*/

const char* DS3231::dayStr(const dayOfWeek day){
    switch (day) {
        case 0:
            return "DAILY";
        case 1:
            return "MON";
        case 2:
            return "TUE";
        case 3:
            return "WED";
        case 4:
            return "THU";
        case 5:
            return "FRI";
        case 6:
            return "SAT";
        case 7:
            return "SUN";

        default:
            return "?";
    }
}


const char* DS3231::monthStr(const Month month){
    switch (month) {
        case 1:
            return "JANUARY";
        case 2:
            return "FEBRUARY";
        case 3:
            return "MARCH";
        case 4:
            return "APRIL";
        case 5:
            return "MAY";
        case 6:
            return "JUNE";
        case 7:
            return "JULY";
        case 8:
            return "AUGUST";
        case 9:
            return "SEPTEMBER";
        case 10:
            return "OCTOBER";
        case 11:
            return "NOVEMBER";
        case 12:
            return "DECEMBER";

        default:
            return "?";
    }
}

/*--------------------------------------------------------------------------------------------------------------------
 *                                            EDIT TIME
---------------------------------------------------------------------------------------------------------------------*/

bool DS3231::is_12() {
    uint8_t byte[1];
    DS3231::readRegister(REG_TIME + 2, byte, 1);
    return byte[0] >> 6; // return 6th bit
}

/**
 * @details This method reads the hour register of the clock and modifies
 * bits 6 and 5 so that the clock will run in 12 hour mode.
 */
void DS3231::set_12() {
    if(DS3231::is_12())
        return;

    uint8_t byte[1];
    uint8_t hour;
    bool pm = false;
    DS3231::readRegister(REG_TIME + 2, byte, 1);
    hour = DS3231::BCDtoDEC(byte[0]);
    if(hour == 0){
        hour = 12;
        pm = true;
    }
    else if(hour == 12)
        pm = false;
    else{
        if(hour > 12)
            pm = true;
        hour %= 12;
    }
    byte[0] = DS3231::DECtoBCD(hour);
    byte[0] = DS3231::setHigh(byte[0], 6);
    if(pm) byte[0] = DS3231::setHigh(byte[0], 5);
    DS3231::writeRegister(REG_TIME + 2, byte, 1);
}

/**
 * @details This method reads the hour register of the clock and modifies
 * bits 6 and 5 so that the clock will run in 24 hour mode.
 */
void DS3231::set_24() {
    if(!DS3231::is_12())
        return;
    uint8_t byte[1];
    DS3231::readRegister(REG_TIME + 2, byte, 1);
    // first disable unwanted bits, then convert
    byte[0] = DS3231::setLow(byte[0], 6);
    bool pm = (bool)(byte[0] >> 5);
    byte[0] = DS3231::setLow(byte[0], 5);
    uint8_t hour = DS3231::BCDtoDEC(byte[0]);
    if(hour == 12 && pm)
        hour = 0;
    else if(pm)
        hour += 12;
    DS3231::writeRegister(REG_TIME + 2, byte, 1);
}

//change one unit of time
// 0-> hour, 1-> minutes, 2->seconds
// only for 24 hour mode
/**
 * @details As of now, this method only works in 24 hour mode.
 */
void DS3231::setTime(uint8_t number, uint8_t value){
    uint8_t byte[1];
    uint8_t reg;
    switch (number) {
        case 0:
            reg = REG_TIME+2;
            clockTime.hour = value % 24;
            value %= 24;
            break;
        case 1:
            reg = REG_TIME+1;
            clockTime.minutes = value % 60;
            value %= 60;
            break;
        case 2:
            reg = REG_TIME;
            clockTime.seconds = value % 60;
            value %= 60;
            break;
    }
    byte[0] = DS3231::DECtoBCD(value);
    DS3231::writeRegister(reg, byte,1);
}

/**
 * @details This method converts the parameters to BCD and then writes the values
 * to the device registers.
 */
void DS3231::setTime(const uint8_t hours, const uint8_t minutes, const uint8_t seconds) {
    uint8_t bytes[3];
    bytes[0] = DS3231::DECtoBCD(seconds % 60);
    bytes[1] = DS3231::DECtoBCD(minutes % 60);
    bytes[2] = DS3231::DECtoBCD(hours % 24);
    if(DS3231::is_12()){
        bytes[2] = DS3231::setHigh(bytes[2], 6);
        if(clockTime.pm)
            bytes[2] = DS3231::setHigh(bytes[2], 5);
    }
    DS3231::writeRegister(REG_TIME, bytes, 3);
}

// 0->day, 1->date, 2->month, 3->year


void DS3231::setDate(uint8_t number, uint16_t value) {
    uint8_t byte[1];
    uint8_t reg;
    switch (number) {
        case 0:
            reg = REG_DAY;
            clockTime.day = dayOfWeek(value);
            break;
        case 1:
            reg = REG_DATE;
            clockTime.date = value;
            break;
        case 2:
            reg = REG_DATE+1;
            clockTime.month = Month(value);
            break;
        case 3:
            reg = REG_DATE+2;
            clockTime.year = value;
            value -= 2000;
            break;
    }
    // if we are changing the year
    // we have to check the month register and modify
    // the century bit ( MSB )
    if(number == 3){
        uint8_t monthByte[1];
        //read month register
        DS3231::readRegister(REG_DATE+1,monthByte,1);
        if(value > 99){ // activate the century bit
            monthByte[0] = DS3231::setHigh(monthByte[0],7);
            value -= 100;
        }
        else{
            // just in case it was active before, deactivate century bit
            monthByte[0] = DS3231::setLow(monthByte[0],7);
        }
        // write the changed byte to the register
        DS3231::writeRegister(REG_DATE+1,monthByte,1);
    }
    // convert byte to bcd format
    byte[0] = DS3231::DECtoBCD(value);
    DS3231::writeRegister(reg, byte,1);
}

void DS3231::setDate(const dayOfWeek day, const Month month, const uint8_t date, uint16_t year) {
    uint8_t bytes[4];
    // converts data to BCD and stores it in bytes buffer
    bytes[0] = DS3231::DECtoBCD((uint8_t)day);
    bytes[1] = DS3231::DECtoBCD(date);
    bytes[2] = DS3231::DECtoBCD((uint8_t)month);
    if(year > 2099 && year < 2200){
        bytes[2] = DS3231::setHigh(bytes[2], 7); //activate century bit
        year -= 100;
    }
    bytes[3] = DS3231::DECtoBCD(year - 2000);
    //writes the bytes from the bytes buffer to the DS3231 starting with day register
    DS3231::writeRegister(0x03,bytes,4);
}

/*--------------------------------------------------------------------------------------------------------------------
 *                                             READ TIME
---------------------------------------------------------------------------------------------------------------------*/

RTCdata DS3231::readTime() {
    // reads the clockTime and date registers
    uint8_t bytes[7];
    DS3231::readRegister(REG_TIME,bytes,7);
    clockTime.seconds = DS3231::BCDtoDEC(bytes[0]);
    clockTime.minutes = DS3231::BCDtoDEC(bytes[1]);
    //check if clock runs in 12 hour mode
    if(bytes[2] >> 6){
        bytes[2] = DS3231::setLow(bytes[2], 6);
        if(bytes[2] >> 5){
            clockTime.pm = true;
            bytes[2] = DS3231::setLow(bytes[2], 5);
        }
        else clockTime.pm = false;
        clockTime.hour = DS3231::BCDtoDEC(bytes[2]);
    }
    else
        clockTime.hour = DS3231::BCDtoDEC(bytes[2]);
    clockTime.day = dayOfWeek(DS3231::BCDtoDEC(bytes[3]));
    clockTime.date = DS3231::BCDtoDEC(bytes[4]);
    uint8_t century = 0;
    if(bytes[5] >> 7 == 1){
        century = 100;
        bytes[5] = DS3231::setLow(bytes[5],7);
    }
    clockTime.month = Month(DS3231::BCDtoDEC(bytes[5] & 0b00011111));
    clockTime.year = DS3231::BCDtoDEC(bytes[6]) + 2000 + century;
    return clockTime;
}

/*--------------------------------------------------------------------------------------------------------------------
 *                                             EDIT ALARMS
---------------------------------------------------------------------------------------------------------------------*/

uint8_t DS3231::checkAlarmFlag() {
    uint8_t byte[1];
    DS3231::readRegister(REG_STATUS,byte,1);
    bool alarm1Flag, alarm2Flag;
    alarm1Flag = (bool)(byte[0] & 0x01);
    alarm2Flag = (bool)(byte[0] & 0x02);
    if(alarm1Flag && alarm2Flag)
        return 0;
    else if(alarm1Flag && !alarm2Flag)
        return 1;
    else if(!alarm1Flag && alarm2Flag)
        return 2;
    return 3; // if both on
}

RTCalarm DS3231::readAlarm(const uint8_t alarmNumber) {
    switch (alarmNumber) {
        case 1:
            return alarm1;
        case 2:
            return alarm2;
    }
    //in case of wrong alarmNumber
    RTCalarm emptyAlarm;
    return emptyAlarm;
}

bool DS3231::alarmState(uint8_t alarmNumber) const {
    switch (alarmNumber) {
        case 1:
            return alarm1.enabled;
        case 2:
            return alarm2.enabled;
    }

    return false;
}

void DS3231::toggleAlarm(const uint8_t alarmNumber, bool enable) {
    snoozeAlarm(); // in case alarm flag were activated but the alarm interrupts were off
    uint8_t byte[1];
    DS3231::readRegister(REG_CONTROL, byte, 1);
    switch (alarmNumber) {
        case 1:
            alarm1.enabled = enable;
            if(enable){
                byte[0] = DS3231::setHigh(byte[0], 0); // set A1IE to 1
            }
            else{
                byte[0] = DS3231::setLow(byte[0], 0); // set A1IE to 0
            }
            break;
        case 2:
            alarm2.enabled = enable;
            if(enable){
                byte[0] = DS3231::setHigh(byte[0], 1); // set A12E to 1
            }
            else{
                byte[0] = DS3231::setLow(byte[0], 1); // set A2IE to 0
            }
            break;
    }
    if(!INTCtr)
        writeINTCtr(true); // enables INTC bit in case it's disabled
    DS3231::writeRegister(REG_CONTROL, byte, 1);
}

//copies alarm information
void DS3231::setAlarm(uint8_t alarmNumber, RTCalarm &alarm) {
    switch (alarmNumber) {
        case 1:
            alarm1 = alarm;
            break;
        case 2:
            alarm2 = alarm;
            break;
    }
}

void DS3231::setAlarmDaily(const uint8_t alarmNumber, uint8_t hour, const uint8_t minute) {
    uint8_t bytes[4];
    bytes[0] = 0x00; // alarm starts at seconds 00;
    bytes[1] = DECtoBCD(minute);
    bytes[2] = DECtoBCD(hour);
    //set mask bits to trigger when hour, minutes and seconds match
    for(uint8_t i = 0; i < 3; i++) {
        bytes[i] = DS3231::setLow(bytes[i], 7);
    }
    bytes[3] = DS3231::setHigh(bytes[3],7);

    switch (alarmNumber) {
        case 1:
            this->alarm1.seconds = 0;
            this->alarm1.minutes = minute;
            this->alarm1.hour = hour;
            this->alarm1.day = DAILY; // 0
            DS3231::writeRegister(REG_ALARM1_SEC,bytes,4);
            break;
        case 2:
            this->alarm2.seconds = 0;
            this->alarm2.minutes = minute;
            this->alarm2.hour = hour;
            this->alarm2.day = DAILY; // 0
            uint8_t bytes2[3];
            //ignores the seconds byte, since alarm 2 does not have a seconds register
            for(uint8_t i = 1; i < 4; i++){
                bytes2[i-1] = bytes[i];
            }
            DS3231::writeRegister(REG_ALARM2_MIN,bytes2,3);
            break;
    }
}

void DS3231::setAlarmWeekly(uint8_t alarmNumber, uint8_t hour, const uint8_t minute, const dayOfWeek day) {
    uint8_t bytes[4];
    bytes[0] = 0x00; // alarm starts at seconds 00;
    bytes[1] = DS3231::DECtoBCD(minute);
    bytes[2] = DS3231::DECtoBCD(hour);
    bytes[3] = DS3231::DECtoBCD(day);
    //set mask bits to trigger when hour, minutes and seconds and day match
    for(uint8_t i = 0; i < 4; i++) {
        bytes[i] = DS3231::setLow(bytes[i], 7);
    }
    bytes[3] = DS3231::setHigh(bytes[3],6);

    switch (alarmNumber) {
        case 1:
            this->alarm1.seconds = 0;
            this->alarm1.minutes = minute;
            this->alarm1.hour = hour;
            this->alarm1.day = day; // 0
            DS3231::writeRegister(REG_ALARM1_SEC,bytes,4);
            break;
        case 2:
            this->alarm2.seconds = 0;
            this->alarm2.minutes = minute;
            this->alarm2.hour = hour;
            this->alarm2.day = day; // 0
            uint8_t bytes2[3];
            //ignores the seconds byte, since alarm 2 does not have a seconds register
            for(uint8_t i = 1; i < 4; i++){
                bytes2[i-1] = bytes[i];
            }
            DS3231::writeRegister(REG_ALARM2_MIN,bytes2,3);
            break;
    }
}

//disable alarm flags
void DS3231::snoozeAlarm() {
    uint8_t byte[1];
    DS3231::readRegister(REG_STATUS,byte,1);
    byte[0] = DS3231::setLow(byte[0],0);
    byte[0] = DS3231::setLow(byte[0],1);
    DS3231::writeRegister(REG_STATUS,byte,1);
    //restore the INT bit state
    //DS3231::readRegister(REG_CONTROL,byte,1);
    //DS3231::writeINTCtr(INTCtr); // restore INTCN bit's state
}

void DS3231::storeAlarmEEPROM(uint8_t alarmNumber) {
    uint8_t bytes[5];
    uint8_t address;
    switch (alarmNumber) {
        case 1:
            bytes[0] = alarm1.seconds;
            bytes[1] = alarm1.minutes;
            bytes[2] = alarm1.hour;
            bytes[3] = (uint8_t)alarm1.day;
            bytes[4] = alarm1.enabled;
            address = 0x00;
            break;
        case 2:
            bytes[0] = alarm2.seconds;
            bytes[1] = alarm2.minutes;
            bytes[2] = alarm2.hour;
            bytes[3] = (uint8_t)alarm2.day;
            bytes[4] = alarm2.enabled;
            address = 0x05;
            break;
    }
    writeEEPROM(address,bytes,5);
}

RTCalarm DS3231::readAlarmEEPROM(uint8_t alarmNumber) {
    uint8_t address;
    uint8_t byteBuffer[5];
    switch (alarmNumber) {
        case 1:
            address = 0x00;
            break;
        case 2:
            address = 0x05;
            break;
    }
    RTCalarm alarm;
    DS3231::readEEPROM(address, byteBuffer, 5);
    alarm.seconds = byteBuffer[0];
    alarm.minutes = byteBuffer[1];
    alarm.hour = byteBuffer[2];
    alarm.day = (dayOfWeek)byteBuffer[3];
    alarm.enabled = byteBuffer[4];

    return alarm;
}

/*--------------------------------------------------------------------------------------------------------------------
 *                                              TEMPERATURE
---------------------------------------------------------------------------------------------------------------------*/


float DS3231::readCelcius() {
    float temperature, floatTemp;
    uint8_t bytes[2];
    DS3231::readRegister(REG_TEMP_INT,bytes,2);
    temperature = bytes[0];
    bytes[1] >>= 6;
    floatTemp = bytes[1]*0.25;
    if(temperature >= 0)
        temperature += floatTemp;
    else
        temperature -= floatTemp;

    return temperature;
}

float DS3231::readFahrenheit() {
    float celcius = DS3231::readCelcius();
    float fahrenheit = celcius * 1.8 +32;
    return fahrenheit;
}

float DS3231::readKelvin() {
    float celcius = DS3231::readCelcius();
    float kelvin = celcius + 273.15;
    return kelvin;
}

/*--------------------------------------------------------------------------------------------------------------------
 *                                              CONTROL SQW
---------------------------------------------------------------------------------------------------------------------*/

void DS3231::toggleSQW(bool enable) {
     uint8_t byte[1];
     DS3231::readRegister(REG_CONTROL,byte,1);
     INTCtr = enable; // when 0 SQW is on
     if(enable)
         byte[0] = DS3231::setLow(byte[0],2);
     else
         byte[0] = DS3231::setHigh(byte[0],2);
     DS3231::writeRegister(REG_CONTROL,byte,1);
 }

 // this method sets the freq of SWQ but does not toggle it on of off
 /*
  *        RS2                  RS1             SQUARE-WAVE OUTPUT FREQUENCY
  *         0                    0                    1Hz
  *         0                    1                    1.024kHz
  *         1                    0                    4.096kHz
  *         1                    1                    8.192kHz
  */
 void DS3231::setSQW(uint8_t mode) { //0 : 1Hz, 1 : 1kHz, 2: 4kHz, 3 : 8kHz
    uint8_t byte[1];
    DS3231::readRegister(REG_CONTROL,byte,1);
    //of interest: bit 4 and bit 3
     switch (mode) {
         case 0:
             byte[0] = DS3231::setLow(byte[0],3);
             byte[0] = DS3231::setLow(byte[0],4);
             break;
         case 1:
             byte[0] = DS3231::setHigh(byte[0],3);
             byte[0] = DS3231::setLow(byte[0],4);
             break;
         case 2:
             byte[0] = DS3231::setLow(byte[0],3);
             byte[0] = DS3231::setHigh(byte[0],4);
             break;
         case 3:
             byte[0] = DS3231::setHigh(byte[0],3);
             byte[0] = DS3231::setHigh(byte[0],4);
             break;
     }
     DS3231::writeRegister(REG_CONTROL,byte,1);
}

//this method toggles the 32KHz pin
//status register bit3
void DS3231::toggle32kHz(bool enable) {
     uint8_t byte[1];
     DS3231::readRegister(REG_STATUS, byte,1);
     if(enable)
         byte[0] = DS3231::setHigh(byte[0],3);
     else
         byte[0] = DS3231::setLow(byte[0], 3);

     DS3231::writeRegister(REG_STATUS,byte,1);
 }

 //the oscillator can stop only if DS3231 is powered by the battery.
 // 1 -> turned off ; 0 -> turned on;
 void DS3231::enableOSC(bool enable) {
     uint8_t byte[1];
     DS3231::readRegister(REG_CONTROL, byte, 1);
     if(enable)
        byte[0] = DS3231::setLow(byte[0], 7);
     else
         byte[0] = DS3231::setHigh(byte[0], 7);
     DS3231::writeRegister(REG_CONTROL, byte, 1);
 }