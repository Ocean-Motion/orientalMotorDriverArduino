/*Angle And Speed Motor Contol Code
DAC Value = (V/5) x 4095 , 
Speed (in Angle Per Sec) = 10 * the voltage

Speed Explanation: 
    Speed = Gain * Set Volt + Offset, NOTE: Sets the speed command per 1 VDC of the input voltage
    Offset = 0, Volt = whatever, Gain = 750 r/min
    750 r/min = 4500 deg/sec, note our gear ratio is 450:1
    4500/450 = 10 deg/sec per volt

Examples
5V   : 4095   : 50 degrees
4.5V : 3685.5 : 45 degrees
4V   : 3276   : 40 degrees
3V   : 2457   : 30 degrees
2V   : 1638   : 20 degrees
1V   : 819    : 10 degrees
*/


#include <Wire.h>
#include <Adafruit_MCP4725.h>
#include <SD.h>

#define PI 3.1415
#define TWOPI 6.283

Adafruit_MCP4725 dac;

const int dacAddress = 0x60;
const int Fwd = 2;
const int Rev = 3;

// Adjustable variables
float speed = 45; // Speed in degrees/second
float targetAngle = 90; // Target angle for oscillation
unsigned long pause1 = 1*1000; // Pause after forward movement
unsigned long pause2 = 1*1000; // Pause after backward movement
int NumCycles = 4; // Number of cycles to run
int direction = 1;  // Direction of motor (1 for forward, -1 for backward)
float totalTime = 0.0; // Total time accumulated (seconds)

void clearSerialBuffer() {
    while (Serial.available() > 0) {
        Serial.read(); // Read and discard all available characters
    }
         delay(100);
}

void promptUserForInputs() {

    Serial.println("Enter the direction (1 for forward, -1 for reverse):");
    clearSerialBuffer();
    while (Serial.available() == 0) {
        delay(100);
    }
    direction = Serial.parseInt();
    
    if (direction != 1 && direction != -1) {
        Serial.println("Invalid direction. Setting to forward (1).");
        direction = 1;
    }
}

void setup() {
    Serial.begin(9600);
    while (!Serial); // Wait for the serial port to connect
    delay(100);

    if (!dac.begin(dacAddress)) {
        Serial.println("DAC initialization failed!");
        while (1); // Stop execution if DAC initialization fails
    }
    Serial.println("DAC initialized.");

    pinMode(Fwd, OUTPUT);
    pinMode(Rev, OUTPUT);
    digitalWrite(Fwd, LOW);
    digitalWrite(Rev, LOW);


promptUserForInputs();

}


void moveMotor(float targetAngle, int direction, unsigned long cycleStartTime) {
    unsigned long startTime = millis();
    float angleCovered = 0.0;
    float elapsedTime;
    unsigned long lastLogTime = startTime;
    const unsigned long logInterval = 100; // 100 ms logging interval

    dac.setVoltage((speed / 10.0) / 5.0 * 4095, false); // Update DAC for speed

    // Set direction
    if (direction == 1) {
        digitalWrite(Fwd, HIGH);
        digitalWrite(Rev, LOW);
    } else {
        digitalWrite(Fwd, LOW);
        digitalWrite(Rev, HIGH);
    }

    // Main loop for motor movement
    while (true) {
        unsigned long currentTime = millis();
        elapsedTime = (currentTime - startTime) / 1000.0; // Time in seconds
        angleCovered = speed * elapsedTime;

        // Check if target angle is reached or exceeded
        if (abs(angleCovered) >= abs(targetAngle)) {
            angleCovered = targetAngle; // Adjust to exact target angle
            break; // Exit the loop once target angle is reached
        }

        // Log data at regular intervals
        if (currentTime - lastLogTime >= logInterval) {
            Serial.print(elapsedTime, 3);
            Serial.print(", ");
            Serial.print(angleCovered, 2);
            Serial.print(", ");
            Serial.print((currentTime - cycleStartTime) / 1000.0, 2);
            Serial.print(", ");
            Serial.println(direction == 1 ? "Forward" : "Backward");

            lastLogTime = currentTime; // Update last log time
        }

        // Optional delay to manage loop responsiveness and CPU usage
       // delay(1); // Adjust delay for responsiveness if needed
    }

    // Stop the motor and reset control
    digitalWrite(Fwd, LOW);
    digitalWrite(Rev, LOW);
    dac.setVoltage(0, false);
}


void loop() {
                  
    unsigned long cycleStartTime;
        Serial.print("Elapsed Time, Angle Covered, degrees, Total Time, Direction\n");
    for (int i = 0; i < NumCycles; i++) {

        cycleStartTime = millis(); // Record the start time of the cycle

        moveMotor(targetAngle, direction, cycleStartTime);
        delay(pause1); // Pause after reaching target angle

        moveMotor(targetAngle, -direction, cycleStartTime);
        delay(pause2); // Pause after returning to original position
    }

    Serial.println("All cycles completed. Motor stopped.");
    while (true); // Stop further execution after all cycles are complete
}
