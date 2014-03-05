#ifndef _Speedo_h
#define _Speedo_h

#include "Arduino.h"
#include "elapsedMillis.h"

//pulse factor for ST300 with fins.  converts pulse time in microseconds into centi-knots 
#define PULSE_FACTOR ((100 * 1000000)/4)        // 100 (hundreths of a knot) * 1000000 microseconds/s / 4 pulses/s
#define CALIBRATION_FACTOR 1.23

//buffer for samples from speedo - 16 pulses per 1/5 second at 20 knots
#define SPEEDO_SAMPLE_BUFFER_SIZE 48

#define Define_Speedo(speed_object)                                             \
    void speed_object##_tick() {                                                \
        /* on pulse just record time since last pulse */                        \
        speed_object.tick();                                                    \
    }
  
#define Setup_Speedo(speed_object, pin, interrupt)                              \
    pinMode(pin, INPUT);                                                        \
    attachInterrupt(interrupt, speed_object##_tick, RISING);                    \

class Speedo {
protected:
    volatile unsigned int sample_buffer[SPEEDO_SAMPLE_BUFFER_SIZE];
    volatile int write_pos;
    volatile unsigned long last_tick;

    int broadcast_pos;
    
public:

    Speedo() {
        write_pos = 0;
        broadcast_pos = 0;

        for ( int i=0; i < SPEEDO_SAMPLE_BUFFER_SIZE; i++ ) {
            sample_buffer[i] = 0;
        }
    }

    void tick() {
        unsigned long t = micros();
        unsigned int milliKnots = (long(PULSE_FACTOR*CALIBRATION_FACTOR) / (t - last_tick) );

        last_tick = t;

        update(milliKnots);
    }

    void update(unsigned int milliKnots) {
        sample_buffer[write_pos] = milliKnots;
        write_pos=(write_pos+1)%SPEEDO_SAMPLE_BUFFER_SIZE;
    }
    
    void debug() {
        //make a non-volitile copy of the write_pos
        int current_write_pos = write_pos;

        //for debugging purposes, we're going to send all pulse 
        //lengths to the mux.
        while( broadcast_pos != current_write_pos ) {
            Serial.print("$P,");
            Serial.println(sample_buffer[broadcast_pos]);

            broadcast_pos=(broadcast_pos+1)%SPEEDO_SAMPLE_BUFFER_SIZE;
        }
    }

    unsigned int read() {
        //make a non-volitile copy of the write_pos
        int current_write_pos = write_pos;

        //calculate speed 
        //use a variable width window to average the observed pulses
        int last_sample_pos = (current_write_pos-1+SPEEDO_SAMPLE_BUFFER_SIZE)%SPEEDO_SAMPLE_BUFFER_SIZE;
        int samples = sample_buffer[last_sample_pos] * 4 / 100;      //sample size factor, tries to read about 1 second worth of samples
        if (samples < 3) samples = 3;     //minimum of 3 samples
        if (samples > SPEEDO_SAMPLE_BUFFER_SIZE) samples = SPEEDO_SAMPLE_BUFFER_SIZE;

        unsigned long sample_sum = 0;
        unsigned int sample_dividor = 0;

        for ( int i=0; i < samples; i++ ) {
            sample_sum += (2*samples-i) * sample_buffer[last_sample_pos];
            sample_dividor += (2*samples-i);

            last_sample_pos = (last_sample_pos-1+SPEEDO_SAMPLE_BUFFER_SIZE)%SPEEDO_SAMPLE_BUFFER_SIZE;
        }
        
        //divide by total weight of all samples
        unsigned int speed_in_hundredths = sample_sum / (sample_dividor);
        
        return speed_in_hundredths;
    }

    void reset() {
        //make a non-volitile copy of the last_tick
        unsigned long old_last_tick = last_tick;
        unsigned long t = micros();

        //handle no signal for a while.
        //1 s is ~.3 kts -- just go to zero
        if ( (t-old_last_tick) > 1000000 ) {
            update(0);
            last_tick = t;
        }
    }
};

#endif
