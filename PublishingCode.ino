
SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(SEMI_AUTOMATIC);

SystemSleepConfiguration config;

// Define whether (1) or not (0) to publish
#define PUBLISHING 0

// Global objects
FuelGauge batteryMonitor;
const char * eventName = "FloodorNo";

//------------------State variables
// not yet used but placeholders in case of additional states
enum State {
  DATALOG_STATE,
  PUBLISH_STATE,
  SLEEP_STATE
};
State state = DATALOG_STATE;

// Define whether (1) or not (0) to publish (PJB: redefining to 0 for initial testing sans cell)
#define PUBLISHING 0

unsigned long stateTime = 0;
char data[120];

// Various timing constants
//const unsigned long MAX_TIME_TO_PUBLISH_MS = 20000; // Only stay awake for this time trying to connect to the cloud and publish
// const unsigned long TIME_AFTER_PUBLISH_MS = 4000; // After publish, wait 4 seconds for data to go out
const unsigned long SECONDS_BETWEEN_MEASUREMENTS = 60; // What should sampling period be?
const unsigned long EARLYBIRD_SECONDS = 0; // how long before desired time should I wake up? 

void setup() {

    if (PUBLISHING==1) {
    Particle.connect();
  }
  else{
    Cellular.off(); // turn off cellular for prelim testing (uncomment)
  }

    Serial.begin(9600);
    Serial1.begin(9600); // Initialize serial communication
    pinMode(A0, OUTPUT);
    digitalWrite(A0, HIGH);

}

void loop(void) {
  // Enter state machine
  switch (state) {

    case DATALOG_STATE: {
        void loop() {

            digitalWrite(A0, LOW);

            delay(1000);

        //Serial.println(Serial1.readString());

        //delay(3000);

            digitalWrite(A0, HIGH);

            digitalWrite(A0, LOW);

            delay(1000);

            Serial.println(Serial1.readString()); //PJB: this doesn't "return" the answer, it just prints it to serial monitor. Therefore you won't be able to access it for publishing.

        //delay(3000);

            digitalWrite(A0, HIGH);

          //PJB: move sleep code to your sleep state.
            config.mode(SystemSleepMode::ULTRA_LOW_POWER)
            .gpio(A0, FALLING)
            .duration(60* 1000L); // Set seconds until wake    
        }

    float cellVoltage = batteryMonitor.getVCell();
    float stateOfCharge = batteryMonitor.getSoC();
    real_time = Time.now();

      // PJB: try to predict what this will save to the data variable. Is a "flood" or "no flood" response anywhere here? 
    snprintf(data, sizeof(data), "%li,%.5f,%.02f,%.02f", //,%.5f,%.5f,%.5f,%.5f,%.5f,%.02f,%.02f",
      real_time, // if it takes a while to connect, this time could be offset from sensor recording
      cellVoltage, stateOfCharge
    );

     // Print out data buffer
    Serial.println(data);

    if (PUBLISHING==1) {
      state = PUBLISH_STATE;
    }
    else{
      state = SLEEP_STATE;
    }
   
}

    break;

    case PUBLISH_STATE: {

    // Prep for cellular transmission
        bool isMaxTime = false;
        stateTime = millis();

        while (!isMaxTime) {
      //connect particle to the cloud
            if (Particle.connected() == false) {
                Particle.connect();
                Serial.print("Trying to connect");
            }

      // If connected, publish data buffer
        if (Particle.connected()) {

            Serial.println("publishing data");

        // bool (or Future) below requires acknowledgment to proceed
            bool success = Particle.publish(eventName, data, 60, PRIVATE, WITH_ACK);
            Serial.printlnf("publish result %d", success); 

            isMaxTime = true;
            state = SLEEP_STATE;
        }
      // If not connected after certain amount of time, go to sleep to save battery
        else {
        // Took too long to publish, just go to sleep
            if (millis() - stateTime >= MAX_TIME_TO_PUBLISH_MS) {
                isMaxTime = true;
                state = SLEEP_STATE;
                Serial.println("max time for publishing reached without success; go to sleep");
            }

            Serial.println("Not max time, try again to connect and publish");
            delay(500);
        }
    }
}

    break;

//Ready to sleep
    case SLEEP_STATE:{
      // PJB: move your config details from above down to here
        SystemSleepResult result = System.sleep(config); // Device sleeps here
    
    }
        
//PJB: integrate this soon. You currently have your sleep timing hard-coded. The following code will give you more robust timing.
     //int secondsUntilNextEvent() {

         //int current_seconds = Time.now();
         //int seconds_to_sleep = SECONDS_BETWEEN_MEASUREMENTS - EARLYBIRD_SECONDS; //- (current_seconds % SECONDS_BETWEEN_MEASUREMENTS);

         //Serial.print("Sleeping for ");
         //Serial.println(seconds_to_sleep);

         //return seconds_to_sleep;
  }
}
