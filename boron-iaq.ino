//Libraries
#include <Adafruit_BMP3XX.h>
#include <ASDL_SCD40.h>
#include <Arduino.h>
#include <Wire.h>

//Subsystem Definitions
#define SEALEVELPRESSURE_HPA (1013.25)
SensirionI2CScd4x scd4x;
Adafruit_BMP3XX BMP388; // I2C

//System Definitions
#define PUBLISH_INTERVAL (10 * 60 * 1000) //data push interval in minutes to milliseconds
#define Num_Sensor_Avg 5
#define SENSOR_INTERVAL  (5 * 1000) //How often the sensors are read (in S)
#define BLINK_INTERVAL  (1000) //How often the sensors are read (in S)
#define PARTICLE_MAX_CHARGE_CURRENT 500 //in mA
#define SOLAR_PEAK_POWER_VOLTAGE 4700 //in mV do not Exceed 4.84V 
SYSTEM_MODE(AUTOMATIC);
SYSTEM_THREAD(ENABLED);

#define LED1 8
#define LED2 7
#define LED3 6

//Global Variables
uint16_t  SCD_CO2;
uint32_t  SCD_CO2_SUM;
float     SCD_Tmp;
float     SCD_Tmp_SUM;
float     SCD_Hum;
float     SCD_Hum_SUM;

double    BMP_Tmp;
double    BMP_Tmp_SUM;
double    BMP_Prs;
double    BMP_Prs_SUM;
double    BMP_Alt;
double    BMP_Alt_SUM;

double    SCD_CO2_Last;
double    SCD_Tmp_Last;
double    SCD_Hum_Last;
double    BMP_Prs_Last;

double    solarVS;
double    solarVS_SUM;

double    batterySoc;
int       batteryState;

char      msg[256];        // Character array for the snprintf Publish Payload

uint32_t  publishTime = 0; //Keeps the timing 
uint32_t  sensorTime = 0;
int       counter; //Manages number of sensor readings 

const char * eventName =            "device_name";       // FIXME //  This must match the name of the event you chose for the WebHook



void setup() {
    set_PMIC(PARTICLE_MAX_CHARGE_CURRENT, SOLAR_PEAK_POWER_VOLTAGE);     //set custom charge rate
    pinMode(LED1, OUTPUT);
    pinMode(LED2, OUTPUT);
    pinMode(LED3, OUTPUT);
  
    analogWrite(LED1, 0);
    analogWrite(LED2, 0);
    analogWrite(LED3, 0);
    
    startSensors();
    
    publishTime = millis();
    sensorTime  = millis();
    
    Serial.begin(9600);
}


void loop() {
    
    manageLEDs();
//  if (abs(millis() - sensorTime) > SENSOR_INTERVAL) {
    
//     readSCD();
//     readBMP();
//     readSolar(); 
    
//     SCD_CO2_SUM = SCD_CO2_SUM + SCD_CO2;
//     SCD_Tmp_SUM = SCD_Tmp_SUM + SCD_Tmp;
//     SCD_Hum_SUM = SCD_Hum_SUM + SCD_Hum;
//     BMP_Tmp_SUM = BMP_Tmp_SUM + BMP_Tmp;
//     BMP_Prs_SUM = BMP_Prs_SUM + BMP_Prs;
//     BMP_Alt_SUM = BMP_Alt_SUM + BMP_Alt;
//     solarVS_SUM = solarVS_SUM + solarVS;
    
//     counter++;
//     sensorTime = millis();
//  }
    
 if (abs(millis() - publishTime) > PUBLISH_INTERVAL) {
    
    batterySoc = System.batteryCharge();
    batteryState = System.batteryState();
    
    readSensors(Num_Sensor_Avg);

    // Prepare the Publish Payload using snprintf to avoid Strings:
    snprintf(msg, sizeof(msg), "{\"1\":\"%.1f\", \"2\":\"%i\", \"3\":\"%.1f\", \"4\":\"%i\", \"5\":\"%.1f\", \"6\":\"%.1f\", \"7\":\"%.1f\"}",
    batterySoc, batteryState, solarVS_SUM/Num_Sensor_Avg, round(SCD_CO2_SUM/Num_Sensor_Avg), SCD_Tmp_SUM/Num_Sensor_Avg, SCD_Hum_SUM/Num_Sensor_Avg, BMP_Prs_SUM/Num_Sensor_Avg);
    // Note:    %.1f = 1 decimal place for a FLOAT number, many other options available.
    //          %.2f = 2 decimal places for a FLOAT
    //          %s   = String, normally just used for the ThingSpeak API Key
    // You can update all 8 Fields of a ThingSpeak Channel during 1 Publish, just add to the snprintf.
    
    Particle.publish(eventName, msg, PRIVATE, NO_ACK);  // perform the Publish
    
    
    SCD_CO2_SUM = 0;
    SCD_Tmp_SUM = 0;
    SCD_Hum_SUM = 0;
    BMP_Tmp_SUM = 0;
    BMP_Prs_SUM = 0;
    BMP_Alt_SUM = 0;
    solarVS_SUM = 0;
    
    counter = 0;
    publishTime = millis();                         // update the time of the last Publish.
    sensorTime  = millis();
 }
}

//   ______ _    _ _   _  _____ _______ _____ ____  _   _  _____ 
//  |  ____| |  | | \ | |/ ____|__   __|_   _/ __ \| \ | |/ ____|
//  | |__  | |  | |  \| | |       | |    | || |  | |  \| | (___  
//  |  __| | |  | | . ` | |       | |    | || |  | | . ` |\___ \ 
//  | |    | |__| | |\  | |____   | |   _| || |__| | |\  |____) |
//  |_|     \____/|_| \_|\_____|  |_|  |_____\____/|_| \_|_____/ 

void readSensors(int numAvg){
    for (int i = 1; i<= numAvg; i++){
        readSCD();
        readBMP();
        readSolar(); 
        
        if (SCD_CO2 > 3000) {
            SCD_CO2 = SCD_CO2_Last;
            SCD_Tmp = SCD_Tmp_Last;
            SCD_Hum = SCD_Hum_Last;
            BMP_Prs = BMP_Prs_Last;
        }
        
        SCD_CO2_SUM = SCD_CO2_SUM + SCD_CO2;
        SCD_Tmp_SUM = SCD_Tmp_SUM + (SCD_Tmp * 9.0 / 5.0) + 32.0;
        SCD_Hum_SUM = SCD_Hum_SUM + SCD_Hum;
        BMP_Prs_SUM = BMP_Prs_SUM + BMP_Prs;
        solarVS_SUM = solarVS_SUM + solarVS;
        
        SCD_CO2_Last = SCD_CO2;
        SCD_Tmp_Last = (SCD_Tmp * 9.0 / 5.0) + 32.0;
        SCD_Hum_Last = SCD_Hum;
        BMP_Prs_Last = BMP_Prs;
        
    }
}


void startSensors() {
    startSCD();
    startBMP();
    //Start any additional sensors here
    pinMode(A1, INPUT);
}

void startSCD() { //DOES NOT INCLUDE ANY ERROR LOGGING - UPDATE THIS
    Wire.begin();
    scd4x.begin(Wire);
    scd4x.startPeriodicMeasurement();
}

void startBMP() { //DOES NOT INCLUDE ANY ERROR LOGGING - UPDATE THIS
    BMP388.begin();
    BMP388.setTemperatureOversampling(BMP3_OVERSAMPLING_8X);
    BMP388.setPressureOversampling(BMP3_OVERSAMPLING_4X);
    BMP388.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_3);
}

void readSCD() {
    scd4x.readMeasurement(SCD_CO2, SCD_Tmp, SCD_Hum);
    
    Serial.print("SCD CO2:"); Serial.println(SCD_CO2);
    Serial.print("SCD Tmp:"); Serial.println(SCD_Tmp);
    Serial.print("SCD Hum:"); Serial.println(SCD_Hum);
}

void readBMP() {
    BMP388.performReading();
    BMP_Prs = BMP388.pressure / 100.0;
    BMP_Tmp = BMP388.temperature;
    BMP_Alt = BMP388.readAltitude(SEALEVELPRESSURE_HPA);
    
    Serial.print("BMP Prs:"); Serial.println(BMP_Prs);
    Serial.print("BMP Tmp:"); Serial.println(BMP_Tmp);
}
    
void readSolar() {
    solarVS = 6.97*analogRead(A5)/4095; //Convert to V
    Serial.print("Input Voltage:"); Serial.println(solarVS);
}

void lowSOC(int threshold, double currentSOC){ //To run if the state of charge is low, set by threshold (20-80) and comparred to currentSOC
    if (threshold < 20){threshold = 20;}
    else if (threshold > 80){threshold = 80;}
    
    if (currentSOC < threshold){
        //Write Sleep & Repeat Code Here
    }
}

void set_PMIC(int maxA, double minV){
    SystemPowerConfiguration solarSettings;
    solarSettings.batteryChargeCurrent(maxA);
    solarSettings.powerSourceMinVoltage(minV);
}

void manageLEDs(){
    if (System.batteryState() == 2){ //Charging
        blinkLED(BLINK_INTERVAL, 127, 50, LED1);
    }
    else if (System.batteryState() == 3) { //Charged
        analogWrite(LED1, 127);
    }
    else if (System.batteryState() == 6) { //disconnected
        blinkLED(250, 255, 50, LED1);
    }
    else {
        analogWrite(LED1, 0);
    }

    if (Particle.connected()){
        blinkLED(BLINK_INTERVAL, 127, 50, LED2);}
    else {
        analogWrite(LED2, 0);
    }
    
    blinkLED(BLINK_INTERVAL, 127, 50, LED3);
}


void blinkLED(int iVal, int brightness, int dCycle, int LED_pin) {
    if ((millis()%iVal) >= (iVal*(100-dCycle)/100)) {
        analogWrite(LED_pin, brightness);
        }
    else {
        analogWrite(LED_pin, 0);
        }
}

/*
  Create a Particle Webhook manually at  https://console.particle.io/integrations/webhooks/create
    Click [CUSTOM TEMPLATE]
    Copy/Paste the following into the CODE window:

  {
  "event": "Charge",
  "url": "https://api.thingspeak.com/update",
  "requestType": "POST",
  "form": {
  "api_key": "{{k}}",
  "field1": "{{1}}",
  "field2": "{{2}}",
  "field3": "{{3}}",
  "field4": "{{4}}",
  "field5": "{{5}}",
  "field6": "{{6}}",
  "field7": "{{7}}",
  "field8": "{{8}}",
  "lat": "{{a}}",
  "long": "{{o}}",
  "elevation": "{{e}}",
  "status": "{{s}}"
  },
  "mydevices": true,
  "noDefaults": true
  }

  CLICK: "CREATE WEBHOOK"
  
    You will change the "event" in the first line of the JSON to whatever event name you wish:
    The Example names the WebHook's event: "TEST",
    So you make sure the CODE (for Photon or Electron) below under [Specific Values for each Installation] is :
    const char * eventName = { Must match the WebHook event name }
    Note: 1 webhook will work for multiple devices.  Use a different eventName if you want to track how many publishes any particular device makes daily/monthly. 

*/









