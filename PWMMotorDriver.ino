// ===== PROJECT: PWM Motor Driver (Part I) =====

// Pin assignments
#define LED_PIN1 2
#define LED_PIN2 3
#define BTN_PIN 4
#define PWM_PIN 9   // Timer1 output (OC1A)

#include <Arduino.h>

// ------------ CE LED VARIABLES ------------
unsigned long interval1 = 0;
unsigned long interval2 = 0;
unsigned long prevMillis1 = 0;
unsigned long prevMillis2 = 0;

bool state1 = LOW;
bool state2 = LOW;

int step = 0;
int chosen_led = 0;

// ------------ PWM VARIABLES ------------
volatile uint8_t pwmDuty = 0;       // 0–255 duty cycle
volatile uint8_t brightnessIndex = 0;

// Duty cycle sequence: 0/8 → 2/8 → 4/8 → 6/8 → 8/8 → 6/8 → 4/8 → 2/8
const uint8_t dutySteps[] = {0, 2, 4, 6, 8, 6, 4, 2};
const uint8_t NUM_STATES = 8;


// ------------ Debounce ------------
unsigned long lastDebounce = 0;
const unsigned long debounceDelay = 40;
bool lastReading = HIGH;
bool stableState = HIGH;

// ------------ Task Manager ------------
void TaskToggleLED1();
void TaskToggleLED2();
void TaskSerialInput();
void TaskPWMButton();

typedef void (*TaskFn)();
TaskFn tasks[] = {TaskToggleLED1, TaskToggleLED2, TaskSerialInput, TaskPWMButton};
const uint8_t NUM_TASKS = 4;


// =====================================================
//                     SETUP
// =====================================================
void setup() {
  Serial.begin(9600);

  pinMode(LED_PIN1, OUTPUT);
  pinMode(LED_PIN2, OUTPUT);
  pinMode(BTN_PIN, INPUT_PULLUP);

  digitalWrite(LED_PIN1, LOW);
  digitalWrite(LED_PIN2, LOW);

  // --- SET UP TIMER1 FOR PWM ON PIN 9 ---
  pinMode(PWM_PIN, OUTPUT);

  noInterrupts();
  TCCR1A = 0;
  TCCR1B = 0;

  // Fast PWM mode, 8-bit
  TCCR1A = (1 << WGM10);
  TCCR1B = (1 << WGM12);

  // Clear OC1A on compare match (non-inverted PWM)
  TCCR1A |= (1 << COM1A1);

  // Clock prescaler 64 → ~976 Hz PWM
  TCCR1B |= (1 << CS11) | (1 << CS10);

  OCR1A = 0;   // start OFF
  interrupts();

  Serial.println("What LED? (1 or 2)");
}


// =====================================================
//                     MAIN LOOP
// =====================================================
void loop() {
  for (uint8_t i = 0; i < NUM_TASKS; i++) {
    tasks[i]();
  }
}


// =====================================================
//               TASK: TOGGLE LED #1
// =====================================================
void TaskToggleLED1() {
  unsigned long now = millis();
  if (interval1 > 0 && now - prevMillis1 >= interval1) {
    prevMillis1 = now;
    state1 = !state1;
    digitalWrite(LED_PIN1, state1);
  }
}

// =====================================================
//               TASK: TOGGLE LED #2
// =====================================================
void TaskToggleLED2() {
  unsigned long now = millis();
  if (interval2 > 0 && now - prevMillis2 >= interval2) {
    prevMillis2 = now;
    state2 = !state2;
    digitalWrite(LED_PIN2, state2);
  }
}

// =====================================================
//               SERIAL INPUT TASK
// =====================================================
void flushSerialInput() {
  while (Serial.available()) Serial.read();
}

void TaskSerialInput() {
  if (!Serial.available()) return;

  if (step == 0) {
    int val = Serial.parseInt();
    flushSerialInput();

    if (val == 1 || val == 2) {
      chosen_led = val;
      step = 1;
      Serial.println("What interval (msec)?");
    } else {
      Serial.println("Invalid LED. Enter 1 or 2:");
    }
  }

  else if (step == 1) {
    long v = Serial.parseInt();
    flushSerialInput();

    if (v > 0) {
      if (chosen_led == 1) interval1 = v;
      else interval2 = v;

      Serial.print("LED ");
      Serial.print(chosen_led);
      Serial.print(" interval set to ");
      Serial.println(v);

      step = 0;
      Serial.println("What LED? (1 or 2)");
    }
  }
}


// =====================================================
//               PWM BUTTON TASK
// =====================================================
void TaskPWMButton() {
  bool reading = digitalRead(BTN_PIN);

  if (reading != lastReading) lastDebounce = millis();

  if ((millis() - lastDebounce) > debounceDelay) {
    if (reading != stableState) {
      stableState = reading;

      if (stableState == LOW) {   // button press

        // Step through 8-state cycle
        brightnessIndex = (brightnessIndex + 1) % NUM_STATES;

        // Convert 0–8 to 0–255 PWM
        pwmDuty = map(dutySteps[brightnessIndex], 0, 8, 0, 255);

        OCR1A = pwmDuty;

        Serial.print("Duty step ");
        Serial.print(dutySteps[brightnessIndex]);
        Serial.print("/8  → PWM = ");
        Serial.println(pwmDuty);
      }
    }
  }

  lastReading = reading;
}
