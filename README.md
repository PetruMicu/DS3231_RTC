# DS3231_RTC
Real Time Clock Driver + Sketch

This project contains a DS3231 driver as well as a sketch to test the library.
The code used is compatible with the ZS-042 RTC module that consits of a DS3231 chip
and an EEPROM.
The library uses the EEPROM only for storing the alarm information. This feature is used in
the sketch to restore the initial alarm after it was ignored 5 times in a row (if ignored alarm is to be triggered
again after 5 minutes). The methods used to communicate with the EEPROM were taken from the SimpleAlarmCLock.h
written by Ricardo Moreno Jr. in 2018. The link to his library: https://github.com/rmorenojr/SimpleAlarmClock.
