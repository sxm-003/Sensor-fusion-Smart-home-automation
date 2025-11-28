#include <SoftwareSerial.h>

// --- PINS ---
const int pir_pin = 2;
const int hall_pin = 3;
const int mm_rx_pin = 4;
const int mm_tx_pin = 5;
const int led_pin = 13;

SoftwareSerial mmWave(mm_rx_pin, mm_tx_pin);

const int LIGHT_THRESHOLD = 5;

enum system_state
{
    monitor,
    pre_off,
    disabled,
    idle
};
system_state current_state = idle;

enum hall_sequence
{
    wait_first_low,
    wait_first_high,
    wait_second_low
};
hall_sequence hall_state = wait_first_low;

unsigned long preoff_start = 0;
const unsigned long preoff_duration = 10000;
unsigned long last_telemetry = 0;

// --- mmWave Helper ---
bool isMmWaveDetecting()
{
    bool personFound = false;
    while (mmWave.available())
    {
        String line = mmWave.readStringUntil('\n');
        if (line.startsWith("$DFDMD"))
        {
            int firstComma = line.indexOf(',');
            if (firstComma != -1)
            {
                char targetCount = line.charAt(firstComma + 1);
                if (targetCount != '0')
                    personFound = true;
            }
        }
    }
    return personFound;
}

void setup()
{
    pinMode(pir_pin, INPUT);
    pinMode(hall_pin, INPUT_PULLUP);
    pinMode(led_pin, OUTPUT);
    Serial.begin(9600);
    mmWave.begin(9600);
    Serial.println("SYSTEM_START");
}

bool checkHallSequence()
{
    int hallvalue = digitalRead(hall_pin);
    switch (hall_state)
    {
    case wait_first_low:
        if (hallvalue == LOW)
        {
            Serial.println("DOOR: MAGNET_DETECTED");
            hall_state = wait_first_high;
        }
        break;
    case wait_first_high:
        if (hallvalue == HIGH)
        {
            Serial.println("DOOR: OPENED");
            hall_state = wait_second_low;
        }
        break;
    case wait_second_low:
        if (hallvalue == LOW)
        {
            Serial.println("DOOR: CLOSED_TRIGGER");
            hall_state = wait_first_low;
            return true;
        }
        break;
    }
    return false;
}

void loop()
{
    int light_value = analogRead(A0);

    // --- TELEMETRY: Sends Light Data EVERY 500ms ---
    if (millis() - last_telemetry > 500)
    {
        Serial.print("TELEMETRY|LIGHT:");
        Serial.print(light_value);
        Serial.println();
        last_telemetry = millis();
    }

    switch (current_state)
    {
    case idle:
        digitalWrite(led_pin, LOW);
        if (light_value > LIGHT_THRESHOLD)
        {
            Serial.println("EVENT: HIGH_LIGHT_DETECTED");
            current_state = monitor;
        }
        break;

    case monitor:
        digitalWrite(led_pin, HIGH);
        if (light_value <= LIGHT_THRESHOLD)
        {
            Serial.println("EVENT: LOW_LIGHT_DETECTED");
            hall_state = wait_first_low;
            current_state = idle;
            break;
        }
        if (checkHallSequence())
        {
            Serial.println("EVENT: DOOR_TRIGGER_ACTIVATED");
            current_state = pre_off;
            preoff_start = millis();
            while (mmWave.available())
                mmWave.read(); // Flush buffer
        }
        delay(50);
        break;

    case pre_off:
        digitalWrite(led_pin, (millis() / 500) % 2);

        // --- SEPARATED SENSOR CHECK ---
        bool pir_active = (digitalRead(pir_pin) == HIGH);
        bool mm_active = isMmWaveDetecting();

        if (pir_active || mm_active)
        {
            Serial.print("MOTION_STOP|SOURCE:");
            if (pir_active && mm_active)
                Serial.println("BOTH");
            else if (pir_active)
                Serial.println("PIR");
            else
                Serial.println("MMWAVE");

            current_state = monitor;
            break;
        }

        if (millis() - preoff_start >= preoff_duration)
        {
            Serial.println("EVENT: TIMER_EXPIRED");
            current_state = disabled;
        }
        delay(100);
        break;

    case disabled:
        digitalWrite(led_pin, LOW);
        if (light_value <= LIGHT_THRESHOLD)
        {
            Serial.println("EVENT: RESET_TO_IDLE");
            current_state = idle;
        }
        delay(200);
        break;
    }
}
