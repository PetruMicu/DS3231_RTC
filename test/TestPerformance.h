//
// Created by micup on 2/1/2021.
//

#ifndef DS3231_RTC_TESTPERFORMANCE_H
#define DS3231_RTC_TESTPERFORMANCE_H
#include <Arduino.h>

class TestPerformance{
private:
    char* functionName;
    bool running;
public:
    TestPerformance(char* functionName){
        this->functionName = functionName;
        running = false;
    }
    void startCount(){
        if(running){
            Serial.print("Clock running!\n");
            return;
        }
        TCCR1A = 0;
        TCCR1B = bit(CS10);
        TCNT1 = 0;
        running = true;
    }
    void stopCount(){
        if(!running){
            Serial.print("Clock is not running!\n");
            return;
        }
        Serial.print(functionName);
        Serial.print(" took ");
        Serial.print((float)(TCNT1 - 1) / 16);
        Serial.print(" microseconds\n");
        running = false;
    }
};

#endif //DS3231_RTC_TESTPERFORMANCE_H
