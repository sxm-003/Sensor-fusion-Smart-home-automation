const int pir_pin = 2;
const int hall_pin =3;
const int mmwave_pin = 4;
const int led_pin = 13;

// *** CHOOSE A REALISTIC THRESHOLD AFTER TESTING ***
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

void setup(){
  pinMode(pir_pin, INPUT);
  pinMode(hall_pin, INPUT);
  pinMode(mmwave_pin,INPUT);
  pinMode(led_pin, OUTPUT);

  Serial.begin(9600);
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
        Serial.println("Hall triggered");
        current_state = pre_off;
        preoff_start = millis();
      }
      delay(200);
      break;

    case pre_off:
      digitalWrite(led_pin, (millis() / 500) % 2); 
      Serial.println("Pre-off, checking for motion");

      if(digitalRead(pir_pin) == HIGH || digitalRead(mmwave_pin) == HIGH){
        Serial.println("Motion detected. Returning to Monitor state");
        current_state = monitor;
        break;
      }
    
      if (millis() - preoff_start >= preoff_duration) {
        Serial.println("No motion for 10 seconds. Disabling system");
        current_state = disabled;
      }
      delay(300);
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