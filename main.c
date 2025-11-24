const int pir_pin = 2;
const int hall_pin =3;
const int mmwave_pin = 4;
const int led_pin = 13;

enum system_state{
  monitor,
  pre_off,
  disabled
};

system_state current_state = monitor;

enum hall_sequence{
  low1,
  high,
  low2 //low for latched 
};

hall_sequence hall_state = low1;

unsigned long preoff_start = 0;
const unsigned long preoff_duration = 10000;//10 secs of waiting for pir

void setup(){
  pinMode(pir_pin, INPUT);
  pinMode(hall_pin, INPUT);
  pinMode(mmwave_pin,INPUT);
  pinMode(led_pin, OUTPUT);

  Serial.begin(9600);
  Serial.println("START...")

}

bool checkHallSequence() {
  int hallvalue = digitalRead(hall_pin);

  switch (hall_state) {
    case low1:
      if (hallvalue == HIGH) {
        Serial.println("latched");
        hall_state = high;
      }
      break;

    case high:
      if (hallvalue == LOW) {
        Serial.println("unlatched");
        hall_state = low2;
      }
      break;

    case low2:
      if (hallvalue == HIGH) {
        Serial.println("latched again");
        hall_state = low1; // reset for next time
        return true; // sequence completed
      }
      break;
  }

  return false;
}

void loop(){
  switch (current_state) {

    case monitor:
    digitalWrite(led_pin,HIGH);
    Serial.println("Monitoring");

    if (checkHallSequence()){
        Serial.println("Hall trigger");
        current_state = pre_off;
        preoff_start = millis();
      }

    delay(200); //debounce , just adding for safety
    break;

    case pre_off:
    digitalWrite(led_pin, (millis() / 500) % 2); //blinking led
    Serial.println("PIR check in pre_off");

    if(digitalRead(pir_pin)==HIGH || digitalRead(mmwave_pin)==HIGH){
        Serial.println("PIR or mmWave motion detected - monitoring starts again");
        current_state = monitor;
        delay(200);
        break;
      }
    else{
        Serial.println("NO motion in PIR");
      }
    
   if (millis() - preoff_start >= preoff_duration) {
        Serial.println("No motion detected for 10 secs");
        current_state = disabled;
      }

      delay(300);
      break;

    case disabled:
      Serial.println("Disabled");
      digitalWrite(led_pin, LOW);
      delay(500);   
      break;
  }
}
  }

}
