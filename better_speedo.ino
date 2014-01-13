
#include "Speedo.h"
#include "Temp.h"

#include <PString.h>

#define SPEEDO_PIN 2
#define SPEEDO_INTERRUPT 0
#define TEMP_PIN 0

Speedo speedo;
Define_Speedo(speedo);

Temp   temp(TEMP_PIN);

unsigned long five_hz_tick = 0;
unsigned long one_hz_tick = 0;

boolean enableSpeedo = false;

void setup() {
    
    //logging on PC, over USB
    Serial.begin(115200);
    
    Setup_Speedo(speedo, SPEEDO_PIN, SPEEDO_INTERRUPT);

    if ( analogRead(TEMP_PIN) > 0 )
        enableSpeedo = true;
}

void loop() {
    //run our own loop, since the method that calls loop doesn't do anything
    //this is an optimization
    while (1) {
        unsigned long now = millis();
        
        if ( enableSpeedo ) {
            speedo.debug();

            //5Hz, broadcast speed
            if ( now - five_hz_tick >= 200 ) {
                five_hz_tick = now;
                
                unsigned int speed_in_hundredths = speedo.read();

                create_sentence('S', speed_in_hundredths);
            }

            speedo.reset();
        }

        //1Hz, broadcast water temp
        if ( now - one_hz_tick >= 1000 ) {
            one_hz_tick = now;
          
            int t = temp.read();
            
            if ( t > 0 ) {
                enableSpeedo = true;    
                create_sentence('T', t);
            }
        }
    }
}

void create_sentence(char key, unsigned int value) {
    char buffer[60];
    PString vdr(buffer, 60);

    switch (key) {
    case 'S':
        vdr.print("$SPVHW,,,,,");
        vdr.print(value/100.0);
        vdr.print(",N,,*");
        break;
    case 'T':
        vdr.print("$SPMTW,");
        vdr.print(value/100.0, 1);
        vdr.print(",C*");
        break;
    default:
        return;
    }

    byte checksum = 0;
    //iterate from 1 -> length-1, so we don't
    //checksum the $ and *
    for ( byte i=1; i < vdr.length()-1; i++ ) {
        checksum ^= buffer[i];
    }

    vdr.print(checksum, HEX);
    vdr.print("\r\n");
    
    Serial.print(vdr);
}
