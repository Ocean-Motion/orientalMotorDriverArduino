#include <Arduino.h>

// Define pins for motor driver control
const int motorForwardPin = 8;    // Pin for forward direction
const int motorReversePin = 7;    // Pin for reverse direction

void setup() {
  // Initialize motor control pins as outputs
  pinMode(motorForwardPin, OUTPUT);
  pinMode(motorReversePin, OUTPUT);

  // Initialize Serial communication if needed
  Serial.begin(9600);
}

void loop() {
  // Sweep in one direction (clockwise)
  sweepMotor(1); // Sweep in forward direction (forward = 1)

  // Sweep in the opposite direction (counterclockwise)
  sweepMotor(0); // Sweep in reverse direction (forward = 0)

}

// Function to sweep the motor in a direction
void sweepMotor(int forward) {
  if (forward == 1) {
    digitalWrite(motorReversePin, LOW);
    digitalWrite(motorForwardPin, HIGH);
  } else {
    digitalWrite(motorForwardPin, LOW);
    digitalWrite(motorReversePin, HIGH);
  }

  // Motor Goes in one direction for 2 seconds
  delay(2000);
}