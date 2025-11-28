const int pir_pin = 2;
const int hall_pin = 3;
const int mmwave_pin = 4;
const int led_pin = 13;


const int LIGHT_THRESHOLD = 5;

enum system_state {
  monitor,
  pre_off,
  disabled,
  idle
};

system_state current_state = idle;


enum hall_sequence {
  wait_first_low,
  wait_first_high,
  wait_second_low
};

hall_sequence hall_state = wait_first_low;

unsigned long preoff_start = 0;
const unsigned long preoff_duration = 10000;

void setup() {
  pinMode(pir_pin, INPUT);
  pinMode(hall_pin, INPUT_PULLUP);
  pinMode(mmwave_pin, INPUT);
  pinMode(led_pin, OUTPUT);

  Serial.begin(9600);
  Serial.println("STARTING SYSTEM...");
}


bool checkHallSequence() {
  int hallvalue = digitalRead(hall_pin);

  switch (hall_state) {
    case wait_first_low:
      // Waiting for the start of the first pass (magnet close)
      if (hallvalue == LOW) {
        Serial.println("First pass started (magnet detected).");
        hall_state = wait_first_high;
      }
      break;

    case wait_first_high:
      // Waiting for the end of the first pass (magnet removed)
      if (hallvalue == HIGH) {
        Serial.println("First pass complete (magnet removed). Waiting for second pass.");
        hall_state = wait_second_low;
      }
      break;

    case wait_second_low:
      // Waiting for the start of the second pass (magnet close)
      if (hallvalue == LOW) {
        Serial.println("Second pass started (magnet detected). TRIGGERING.");
        hall_state = wait_first_low; 
        return true;                 
      }
      break;
  }
  return false;
}

void loop() {

  int light_value = analogRead(A0);

  switch (current_state) {
    case idle:
      Serial.print("Idle. Light value: ");
      Serial.println(light_value);
      digitalWrite(led_pin, LOW);


      if (light_value > LIGHT_THRESHOLD) {
        Serial.println("Bright light detected. Entering Monitor state.");
        current_state = monitor;
      }
      delay(500);
      break;

    case monitor:
      digitalWrite(led_pin, HIGH);
      


      if (light_value <= LIGHT_THRESHOLD) {
        Serial.println("Low light. Returning to Idle state.");
        // Reset the hall sequence if the main system resets
        hall_state = wait_first_low;
        current_state = idle;
        break;
      }

      if (checkHallSequence()) {
        Serial.println("Hall trigger sequence detected. Entering Pre-off.");
        current_state = pre_off;
        preoff_start = millis();
      }
      delay(200);
      break;

    case pre_off:
      digitalWrite(led_pin, (millis() / 500) % 2);
      Serial.println("Pre-off, checking for motion.");

      if (digitalRead(pir_pin) == HIGH || digitalRead(mmwave_pin) == HIGH) {
        Serial.println("Motion detected. Returning to Monitor state.");
        current_state = monitor;
        break;
      }

      if (millis() - preoff_start >= preoff_duration) {
        Serial.println("No motion for 10 seconds. Disabling system.");
        current_state = disabled;
      }
      delay(300);
      break;

    case disabled:
      Serial.println("Disabled. Waiting for low light to reset.");
      digitalWrite(led_pin, LOW);

      // Reset to idle when the light goes out, ready for the next day/cycle
      if (light_value <= LIGHT_THRESHOLD) {
        Serial.println("Resetting to Idle state.");
        current_state = idle;
      }
      delay(500);
      break;
  }
}
