const int h_pin = 2;
void setup(){
  Serial.begin(9600);
  pinMode(h_pin,INPUT_PULLUP);

}

  void loop(){
    int op = digitalRead(h_pin);

    Serial.println(op);
delay(500);
  }

