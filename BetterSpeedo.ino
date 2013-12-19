
//#include <Wire.h>

#define SPEEDO_PIN 2
#define SPEEDO_INTERRUPT 2
#define TEMP_PIN 0

// Airmar thermister coefficients
#define A 0.0011321253
#define B 0.000233722
#define C 0.0000000886

//pulse factor for ST300 with fins.  converts pulse time into milli-knots
#define PULSE_FACTOR 250000000        // 1000000 / 4 pulses per knot

//buffer for samples from speedo - 16 pulses per 1/5 second at 20 knots
#define SAMPLE_BUFFER_SIZE 16
volatile unsigned int sample_buffer[SAMPLE_BUFFER_SIZE];
volatile int write_pos = 0;
int broadcast_pos = 0;

//last speed for calculating acceleration
unsigned int last_speed = 0;

elapsedMicros last_tick;
elapsedMillis five_hz_time;
elapsedMillis one_hz_time;

elapsedMillis debug_tome;

void setup() {
    //initialize sample buffer
    for (int i=0; i < SAMPLE_BUFFER_SIZE; i++ ) {
        sample_buffer[i] = 0;    
    }
    
    //TODO: don't broadcast if no depth connected

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);

    //speedo input pin
    pinMode(SPEEDO_PIN, INPUT);
    attachInterrupt(SPEEDO_INTERRUPT, on_speedo_pulse, RISING);

//    Wire.begin();
    Serial.begin(38400);
}

#define DEBUG(x) Serial.println(x);

void send_data(char key, unsigned int value) {
    // Wire.beginTransmission(6);

    // Wire.wire('P');

    // //send 4 byte integer
    // Wire.wire(c & 0xFF);
    // Wire.wire((c>>8)& 0xFF;
    // Wire.wire((c>>16)& 0xFF;
    // Wire.wire((c>>24)& 0xFF);

    // // disable Slave Select        
    // Wire.endTransmission();

    Serial.print(key);
    Serial.print(" : ");
    Serial.print(value);
    Serial.print("\r\n");
}

void loop() {

    //for debugging purposes, we're going to send all pulse 
    //lengths to the mux.
    int current_write_pos = write_pos;
    while( broadcast_pos != write_pos ) {            
        send_data('P', sample_buffer[broadcast_pos]);

        broadcast_pos=(broadcast_pos+1)%SAMPLE_BUFFER_SIZE;
    }

    //5Hz, broadcast speed and acceleration 
    if ( five_hz_time >= 2000 ) {
        five_hz_time = 0;

        //calculate time 
        //use a variable width window to average the observed
        int last_sample_pos = (current_write_pos-1+SAMPLE_BUFFER_SIZE)%SAMPLE_BUFFER_SIZE;
        int samples = sample_buffer[last_sample_pos] / 625;      //sample size factor, tries to read about .4 seconds worth of samples
        if (samples < 3) samples = 3;
        if (samples > SAMPLE_BUFFER_SIZE) samples = SAMPLE_BUFFER_SIZE;

        
        int i = (last_sample_pos-samples+SAMPLE_BUFFER_SIZE)%SAMPLE_BUFFER_SIZE;
        unsigned int sample_sum = 0;
        for ( ; i != current_write_pos; i=(i+1)%SAMPLE_BUFFER_SIZE ) {
            sample_sum += sample_buffer[i];
        }
        
        //divide by # of samples and by 10 to turn milli knots into
        //centi-knots
        unsigned int speed_in_hundredths = sample_sum / (samples*10);
        int acceleration = speed_in_hundredths-last_speed;
        last_speed = speed_in_hundredths;


        //broadcast speed and acceleration
        send_data('S', speed_in_hundredths);
        send_data('A', acceleration);
    }

    if ( one_hz_time >= 2000 ) {
        one_hz_time = 0;
      
        double voltage_read = analogRead(TEMP_PIN)*(3.3/1023); 
        double lnr =  log( (33000.0 / voltage_read) - 10000.0 ); //get resistence of thermister
        int t = (int) (100 / (A + (B * lnr) + (C * lnr * lnr * lnr)) - 27315);
        
        send_data('T', t);
    }

    //handle no signal for a while.
    //1s is ~.3 kts -- just go to zero
    if ( last_tick > 1000000 ) {
        sample_buffer[write_pos] = 0;
        write_pos=(write_pos+1)%SAMPLE_BUFFER_SIZE;
        last_tick = 0;
        Serial.println("too long");
    }
}

void on_speedo_pulse() {
    unsigned int milliKnots = (PULSE_FACTOR/last_tick);
    sample_buffer[write_pos] = milliKnots;
    write_pos=(write_pos+1)%SAMPLE_BUFFER_SIZE;
    
    last_tick = 0;
}
