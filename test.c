#include <SoftwareSerial.h> // Include library for sensor communication

// --- PINS ---
const int pir_pin = 2;
const int hall_pin = 3;
const int mm_rx_pin = 4; // Connect to Sensor TX
const int mm_tx_pin = 5; // Connect to Sensor RX
const int led_pin = 13;

SoftwareSerial mmWave(mm_rx_pin, mm_tx_pin);

const int LIGHT_THRESHOLD = 5; 

enum system_state{
  monitor,
  pre_off,
  disabled,
  idle
};

system_state current_state = idle;

enum hall_sequence{
  low1,
  high,
  low2 // latched
};

hall_sequence hall_state = low1;

unsigned long preoff_start = 0;
const unsigned long preoff_duration = 10000; 

// Reads the serial buffer to see if a person is currently detected
bool isMmWaveDetecting() {
  bool personFound = false;
  
  // Read all data currently in the buffer
  while (mmWave.available()) {
    String line = mmWave.readStringUntil('\n');
    
    // Check if it is a Distance Mode Packet ($DFDMD)
    if (line.startsWith("$DFDMD")) {
      int firstComma = line.indexOf(',');
      if (firstComma != -1) {
        char targetCount = line.charAt(firstComma + 1);
        if (targetCount != '0') {
          personFound = true;
        }
      }
    }
  }
  return personFound;
}

void setup(){
  pinMode(pir_pin, INPUT);
  pinMode(hall_pin, INPUT);
  pinMode(led_pin, OUTPUT);

  Serial.begin(9600);
  mmWave.begin(9600); // Start the sensor stream
  
  Serial.println("STARTING SYSTEM...");
}

bool checkHallSequence() {
  int hallvalue = digitalRead(hall_pin);
  switch (hall_state) {
    case low1:
      if (hallvalue == HIGH) { hall_state = high; }
      break;
    case high:
      if (hallvalue == LOW) { hall_state = low2; }
      break;
    case low2:
      if (hallvalue == HIGH) {
        hall_state = low1; 
        return true; 
      }
      break;
  }
  return false;
}

void loop(){

  int light_value = analogRead(A0);

  switch (current_state) {
    case idle:
      Serial.print("Idle. Light value: ");
      Serial.println(light_value);
      digitalWrite(led_pin, LOW);
      
      if (light_value > LIGHT_THRESHOLD) {
        Serial.println("Light detected. Entering Monitor state.");
        current_state = monitor;
      }
      delay(500);
      break;

    case monitor:
      digitalWrite(led_pin, HIGH);
      Serial.println("Monitoring");

      if (light_value <= LIGHT_THRESHOLD) {
        Serial.println("Light is low, switching to idle");
        current_state = idle;
        break;
      }

      if (checkHallSequence()){
        Serial.println("Hall triggered (Door Logic)");
        current_state = pre_off;
        preoff_start = millis();
        while(mmWave.available()) mmWave.read(); 
      }
      delay(200);
      break;

    case pre_off:
      digitalWrite(led_pin, (millis() / 500) % 2); 
      Serial.println("Pre-off, checking for motion (PIR + mmWave)");

      // Check PIR OR Check mmWave Serial Stream
      if(digitalRead(pir_pin) == HIGH || isMmWaveDetecting() == true){
        Serial.println("Motion detected (Person still in room). Returning to Monitor state");
        current_state = monitor;
        break;
      }
    
      if (millis() - preoff_start >= preoff_duration) {
        Serial.println("No motion for 10 seconds. Disabling system");
        current_state = disabled;
      }
      // Reduced delay slightly to ensure we catch Serial data faster
      delay(200); 
      break;

    case disabled:
      Serial.println("Disabled");
      digitalWrite(led_pin, LOW);
    
      if (light_value <= LIGHT_THRESHOLD) {
        Serial.println("Resetting to idle");
        current_state = idle;
      }
      delay(500);   
      break;
  }
}
