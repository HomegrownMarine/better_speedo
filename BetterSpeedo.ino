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

void setup() {
    //initialize sample buffer
    for (int i=0; i < SAMPLE_BUFFER_SIZE; i++ ) {
        sample_buffer[i] = 0;    
    }
    
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);

    //speedo input pin
    pinMode(12, INPUT);
    attachInterrupt(12, on_speedo_pulse, FALLING);

    //thermister input pin
    pinMode(8, INPUT);

    //
    SPI.begin();
}

void loop() {

    //for debugging purposes, we're going to send all pulse 
    //lengths to the mux.
    if ( broadcast_pos != write_pos ) {
        digitalWrite(SS, LOW);
                  
        while( broadcast_pos != write_pos ) {
            SPI.transfer('P');

            //send 4 byte integer
            SPI.transfer(c & 0xFF);
            SPI.transfer((c>>8)& 0xFF;
            SPI.transfer((c>>16)& 0xFF;
            SPI.transfer((c>>24)& 0xFF);

            broadcast_pos=(broadcast_pos+1)%SAMPLE_BUFFER_SIZE;
        }
    
        // disable Slave Select
        digitalWrite(SS, HIGH);
    }

    //5Hz, broadcast speed and acceleration 
    if ( five_hz_time >= 200 ) {
        five_hz_time = 0;

        //calculate time 
        //use a variable width window to average the observed
        int last_sample_index = (write_pos-1)%SAMPLE_BUFFER_SIZE;
        int samples = sample_buffer[last_sample_index] / 625;      //sample size factor, tries to read about .4 seconds worth of samples
        if (samples < 3) samples = 3;
        if (samples > SAMPLE_BUFFER_SIZE) samples = SAMPLE_BUFFER_SIZE;

        int sample_sum = 0;
        for ( int i=0, int c = last_sample_index; i < samples; i++, c = (c-1)%SAMPLE_BUFFER_SIZE ) {
            sample_sum += sample_buffer[c];
        }
        
        //divide by # of samples and by 10 to turn milli knots into
        //centi-knots
        unsigned int speed_in_hundredths = sample_sum / (samples*10);
        int acceleration = speed_in_hundredths-last_speed;
        last_speed = speed_in_hundredths;


        digitalWrite(SS, LOW);

        //broadcast speed and acceleration
        SPI.transfer('S');

        //send 4 byte integer
        SPI.transfer(speed_in_hundredths & 0xFF);
        SPI.transfer((speed_in_hundredths>>8)& 0xFF;
        SPI.transfer((speed_in_hundredths>>16)& 0xFF;
        SPI.transfer((speed_in_hundredths>>24)& 0xFF);

        //acceleration
        SPI.transfer('A');

        //send 4 byte integer
        SPI.transfer(acceleration & 0xFF);
        SPI.transfer((acceleration>>8)& 0xFF;
        SPI.transfer((acceleration>>16)& 0xFF;
        SPI.transfer((acceleration>>24)& 0xFF);
        
        digitalWrite(SS, HIGH);
    }

    if ( one_hz_time >= 1000 ) {
        double voltage_read = analogRead(0)*(3.3/1023); 
        double lnr =  log(33000.0 / voltage_read − 10000.0); //get resistence of thermister
        int t = (int) (100 / (A + (B * lnr) + (C * lnr * lnr * lnr)) - 27315);
        
        digitalWrite(SS, LOW);
        
        //time
        SPI.transfer('T');

        //send 4 byte integer
        SPI.transfer(t & 0xFF);
        SPI.transfer((t>>8)& 0xFF;
        SPI.transfer((t>>16)& 0xFF;
        SPI.transfer((t>>24)& 0xFF);
        
        digitalWrite(SS, HIGH);
    }

    //handle no signal for a while.
    //1s is ~.3 kts
    if ( last_tick > 1000000 ) {
        sample_buffer[write_pos] = 0;
        write_pos=(write_pos+1)%SAMPLE_BUFFER_SIZE;
        last_tick = 0;
    }
}

void on_speedo_pulse() {
    unsigned int milliKnots = (PULSE_FACTOR/last_tick);
    sample_buffer[write_pos] = milliKnots;
    write_pos=(write_pos+1)%SAMPLE_BUFFER_SIZE;
    last_tick = 0;
}
