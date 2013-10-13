
#define PULSE_FACTOR 250000000        // 1000000 / 4 pulses per knot

elapsedMicros last_tick;
elapsedMillis ten_hz;


#define SAMPLE_BUFFER_SIZE 16
volatile unsigned int sample_buffer[SAMPLE_BUFFER_SIZE];
volatile int write_pos = 0;
volatile int broadcast_pos = 0;

unsigned int last_speed = 0;

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

    
    Serial.begin(38400);
}

void loop() {

    while( broadcast_pos != write_pos ) {
        Serial.print("$SPP,");
        Serial.print(sample_buffer[broadcast_pos]);
        Serial.print("*\r\n");

        broadcast_pos=(broadcast_pos+1)%SAMPLE_BUFFER_SIZE;
    }

    if ( ten_hz >= 200 ) {
        ten_hz = 0;

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

        //TODO: send over spi
        //broadcast speed
        Serial.print("$SPVHW,,,,,");
        Serial.print(speed_in_hundredths/100.0); //TODO
        Serial.print(",N,,*");
        Serial.print("\r\n");

        //acceleration
        Serial.print("$SPA,");
        Serial.print((speed_in_hundredths-last_speed)/100.0);
        Serial.print("\r\n");
        last_speed = speed_in_hundredths;

        //TODO temp
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
