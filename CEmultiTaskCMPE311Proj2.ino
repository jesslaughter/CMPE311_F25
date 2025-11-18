// PROJECT-CE
#define LED_PIN1 2
#define LED_PIN2 3

#include <Arduino.h>

unsigned long interval1 = 0;
unsigned long interval2 = 0;
unsigned long prevMillis1 = 0;
unsigned long prevMillis2 = 0;

bool state1 = LOW;
bool state2 = LOW;

int step = 0;      
int chosen_led = 0;

void TaskToggleLED1();
void TaskToggleLED2();
void TaskSerialInput();

typedef void (*TaskFunc)();
TaskFunc tasks[] = {
  TaskToggleLED1,
  TaskToggleLED2,
  TaskSerialInput,
};
const uint8_t NUM_TASKS = sizeof(tasks) / sizeof(TaskFunc);

void setup() {
  Serial.begin(9600);
  pinMode(LED_PIN1, OUTPUT);
  pinMode(LED_PIN2, OUTPUT);
  digitalWrite(LED_PIN1, LOW);
  digitalWrite(LED_PIN2, LOW);
  Serial.println("What LED? (1 or 2)");
}

void loop() {
  // Round-robin task manager
  for (uint8_t i = 0; i < NUM_TASKS; ++i) {
    tasks[i]();
  }
}


void TaskToggleLED1() {
  unsigned long now = millis();
  if (interval1 > 0 && (now - prevMillis1 >= interval1)) {
    prevMillis1 = now;
    state1 = !state1;
    digitalWrite(LED_PIN1, state1 ? HIGH : LOW);
  }
}

void TaskToggleLED2() {
  unsigned long now = millis();
  if (interval2 > 0 && (now - prevMillis2 >= interval2)) {
    prevMillis2 = now;
    state2 = !state2;
    digitalWrite(LED_PIN2, state2 ? HIGH : LOW);
  }
}

void flushSerialInput() {
  while (Serial.available()) Serial.read();
}

void TaskSerialInput() {
  if (Serial.available()) {
    if (step == 0) {
      // Read chosen LED number
      int val = Serial.parseInt();
      flushSerialInput();
      if (val == 1 || val == 2) {
        chosen_led = val;
        step = 1;
        Serial.println("What interval (in msec)?");
      } else {
        Serial.println("Invalid LED. Please enter 1 or 2:");
      }
    } else if (step == 1) {
      // Read interval
      long val = Serial.parseInt();
      flushSerialInput();
      if (val > 0) {
        if (chosen_led == 1) {
          interval1 = (unsigned long) val;
          prevMillis1 = millis(); 
        } else {
          interval2 = (unsigned long) val;
          prevMillis2 = millis();
        }
        Serial.print("LED ");
        Serial.print(chosen_led);
        Serial.print(" interval set to ");
        Serial.print(val);
        Serial.println(" ms");
        step = 0;
        Serial.println("What LED? (1 or 2)");
      }
    }
  }
}

