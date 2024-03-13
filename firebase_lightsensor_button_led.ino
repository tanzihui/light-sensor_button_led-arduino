#define LIGHT_SENSOR_PIN 2 //ESP32 pin GPIO32, which connected to the light sensor
#define BUTTON_PIN  18 // ESP32 pin GPIO18, which connected to button
#define LED_PIN     9 // ESP32 pin GPIO21, which connected to led
#define ANALOG_THRESHOLD 200
#define PWMChannel 0

const int freq = 5000;
const int resolution = 8;
int pwmValue = 0;

#include <Arduino.h>
#if defined(ESP32) || defined(ARDUINO_RASPBERRY_PI_PICO_W)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#elif __has_include(<WiFiNINA.h>)
#include <WiFiNINA.h>
#elif __has_include(<WiFi101.h>)
#include <WiFi101.h>
#elif __has_include(<WiFiS3.h>)
#include <WiFiS3.h>
#endif

#include <Firebase_ESP_Client.h>

// Provide the token generation process info.
#include <addons/TokenHelper.h>

// Provide the RTDB payload printing info and other helper functions.
#include <addons/RTDBHelper.h>

/* 1. Define the WiFi credentials */
#define WIFI_SSID "WIFI_AP"
#define WIFI_PASSWORD "WIFI_PASSWORD"

// For the following credentials, see examples/Authentications/SignInAsUser/EmailPassword/EmailPassword.ino

/* 2. Define the API Key */
#define API_KEY "API_KEY"

/* 3. Define the RTDB URL */
#define DATABASE_URL "URL" //<databaseName>.firebaseio.com or <databaseName>.<region>.firebasedatabase.app

/* 4. Define the user Email and password that alreadey registerd or added in your project */
#define USER_EMAIL "USER_EMAIL"
#define USER_PASSWORD "USER_PASSWORD"

// Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;

unsigned long count = 0;

#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
WiFiMulti multi;
#endif

// variables will change:
bool led_state = false;    // the current state of LED
int button_state;       // the current state of button
int last_button_state;  // the previous state of button

void setup()
{

  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP); // set ESP32 pin to input pull-up mode
  pinMode(LED_PIN, OUTPUT);          // set ESP32 pin to output mode
  ledcSetup(PWMChannel, freq,resolution);
  ledcAttachPin(LIGHT_SENSOR_PIN, PWMChannel);

  button_state = digitalRead(BUTTON_PIN);

#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
  multi.addAP(WIFI_SSID, WIFI_PASSWORD);
  multi.run();
#else
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
#endif

  Serial.print("Connecting to Wi-Fi");
  unsigned long ms = millis();
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
    if (millis() - ms > 10000)
      break;
#endif
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the user sign in credentials */
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

  // Or use legacy authenticate method
  // config.database_url = DATABASE_URL;
  // config.signer.tokens.legacy_token = "<database secret>";

  // To connect without auth in Test Mode, see Authentications/TestMode/TestMode.ino

  //////////////////////////////////////////////////////////////////////////////////////////////
  // Please make sure the device free Heap is not lower than 80 k for ESP32 and 10 k for ESP8266,
  // otherwise the SSL connection will fail.
  //////////////////////////////////////////////////////////////////////////////////////////////

  // Comment or pass false value when WiFi reconnection will control by your code or third party library e.g. WiFiManager
  Firebase.reconnectNetwork(true);

  // Since v4.4.x, BearSSL engine was used, the SSL buffer need to be set.
  // Large data transmission may require larger RX buffer, otherwise connection issue or data read time out can be occurred.
  fbdo.setBSSLBufferSize(4096 /* Rx buffer size in bytes from 512 - 16384 */, 1024 /* Tx buffer size in bytes from 512 - 16384 */);

  // Limit the size of response payload to be collected in FirebaseData
  fbdo.setResponseSize(2048);

  Firebase.begin(&config, &auth);

  // The WiFi credentials are required for Pico W
  // due to it does not have reconnect feature.
#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
  config.wifi.clearAP();
  config.wifi.addAP(WIFI_SSID, WIFI_PASSWORD);
#endif

  Firebase.setDoubleDigits(5);

  config.timeout.serverResponse = 10 * 1000;

  
}

void loop()
{
  // Firebase.ready() should be called repeatedly to handle authentication tasks.

  if (Firebase.ready() && (millis() - sendDataPrevMillis > 5000 || sendDataPrevMillis == 0))
  {
    sendDataPrevMillis = millis();
    int analogValue = analogRead(LIGHT_SENSOR_PIN); //read the value on analog pin
    int last_button_state = button_state;      // save the last state
    int button_state = digitalRead(BUTTON_PIN); // read new state
    //if brightness of surroundings/analogValue lower than ANALOG_THRESHOLD set on light sensor
    if (analogValue < ANALOG_THRESHOLD){
      //set and get string from firebase
      Serial.printf("Set string... %s\n", Firebase.RTDB.setString(&fbdo, F("/test/string"), F("It is dark!Please on the light.")) ? "ok" : fbdo.errorReason().c_str());
      Serial.printf("Get string... %s\n", Firebase.RTDB.getString(&fbdo, F("/test/string")) ? fbdo.to<const char *>() : fbdo.errorReason().c_str());
      //Serial.println("It is dark!Please on the light.");
      if (last_button_state == HIGH && button_state == LOW) {
        // toggle state of LED
        led_state = !led_state;

        // control LED arccoding to the toggled state
        digitalWrite(LED_PIN, led_state);
        //set and get bool from firebase
        Serial.printf("Set bool... %s\n", Firebase.RTDB.setBool(&fbdo, F("/test/led state"), led_state) ? "ok" : fbdo.errorReason().c_str());
        Serial.printf("Get bool... %s\n", Firebase.RTDB.getBool(&fbdo, FPSTR("/test/led state")) ? fbdo.to<bool>() ? "true" : "false" : fbdo.errorReason().c_str());
        bool bVal;
        Serial.printf("Get bool ref... %s\n", Firebase.RTDB.getBool(&fbdo, F("/test/led state"), &bVal) ? bVal ? "true" : "false" : fbdo.errorReason().c_str());

      }
      
    }
    //if brightness of surroundings/analogValue higher than the ANALOG_THRESHOLD set on the light sensor
    if (analogValue> ANALOG_THRESHOLD){
      //set and get bool from firebase
      Serial.printf("Set string... %s\n", Firebase.RTDB.setString(&fbdo, F("/test/string"), F("It is bright! No lights needed.")) ? "ok" : fbdo.errorReason().c_str());
      Serial.printf("Get string... %s\n", Firebase.RTDB.getString(&fbdo, F("/test/string")) ? fbdo.to<const char *>() : fbdo.errorReason().c_str());
      //turn off LED light
      digitalWrite(LED_PIN, LOW); 
      led_state=false;
      //set and get bool from firebase
      Serial.printf("Set bool... %s\n", Firebase.RTDB.setBool(&fbdo, F("/test/led state"), led_state) ? "ok" : fbdo.errorReason().c_str());
      Serial.printf("Get bool... %s\n", Firebase.RTDB.getBool(&fbdo, FPSTR("/test/led state")) ? fbdo.to<bool>() ? "true" : "false" : fbdo.errorReason().c_str());
      bool bVal;
      Serial.printf("Get bool ref... %s\n", Firebase.RTDB.getBool(&fbdo, F("/test/led state"), &bVal) ? bVal ? "true" : "false" : fbdo.errorReason().c_str());
    }
    //get data from firebase to esp32;
    if(Firebase.RTDB.getBool(&fbdo,"/test/led state")){
      if(fbdo.dataType() =="boolean"){
        led_state = fbdo.boolData();
        Serial.println("Successful READ from " + fbdo.dataPath() + ": " + led_state + " (" + fbdo.dataType() + ")");
        digitalWrite(LED_PIN,led_state);
      }
    }else{
      Serial.println("FAILED: " + fbdo.errorReason());
    }
    
    
    FirebaseJson json;
    
    
    Serial.println();



    
    
  }
}

