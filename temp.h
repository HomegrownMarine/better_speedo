#ifndef _Temp_h
#define _Temp_h

#include "Arduino.h"

// Airmar thermister equation coefficients
#define A 0.0011321253
#define B 0.000233722
#define C 0.0000000886

#define ANALOG_READ_REFERENCE_VOLTAGE 5.0
#define ANALOG_READ_RANGE 1023.0

class Temp {
private:
    int pin_number;
public:
    Temp(int pin) {
        pin_number = pin;
    }

    int read() {
        int voltage_read = analogRead(pin_number) * (ANALOG_READ_REFERENCE_VOLTAGE/ANALOG_READ_RANGE); 
        double lnr =  log( (ANALOG_READ_REFERENCE_VOLTAGE * 10000.0 / voltage_read) - 10000.0 ); //get resistence of thermister
        int t = (int) (100 / (A + (B * lnr) + (C * lnr * lnr * lnr)) - 27315);
        
        return t;
    }
};

#endif
