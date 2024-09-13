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
const int chipSelect = 53; // Adjust to match your setup

// Adjustable variables
float speed = 0.0; // Speed in degrees/second
float targetAngle = 0.0; // Target angle for oscillation
unsigned long pause1 = 0; // Pause after forward movement
unsigned long pause2 = 0; // Pause after backward movement
int NumCycles = 0; // Number of cycles to run
int direction = 1; // Direction of motor (1 for forward, -1 for backward)
float totalTime = 0.0; // Total time accumulated (seconds)

void clearSerialBuffer() {
    while (Serial.available() > 0) {
        Serial.read(); // Read and discard all available characters
    }
}

void promptUserForInputs() {
    Serial.println("Enter the desired angle to move (degrees):");
    clearSerialBuffer();
    while (Serial.available() == 0) {
        delay(100);
    }
    targetAngle = Serial.parseFloat();
    Serial.println("Target Angle: " + String(targetAngle) + " degrees");

    Serial.println("Enter the desired speed (degrees per second):");
    clearSerialBuffer();
    while (Serial.available() == 0) {
        delay(100);
    }
    speed = Serial.parseFloat();
    Serial.println("Speed: " + String(speed) + " degrees per second");

    if (speed <= 0) {
        Serial.println("Invalid speed. Motor will not run.");
        while (true);
    }
    Serial.println("Time to reach angle: " + String(targetAngle/speed) + " seconds");

    Serial.println("Enter number of cycles:");
    clearSerialBuffer();
    while (Serial.available() == 0) {
        delay(100);
    }
    NumCycles = Serial.parseInt();
    if (NumCycles <= 0) {
        Serial.println("Invalid number of cycles.");
        while (true);
    }

    Serial.println("Enter the pause duration after forward movement (seconds):");
    clearSerialBuffer();
    while (Serial.available() == 0) {
        delay(100);
    }
    pause1 = Serial.parseInt() * 1000; // Convert to milliseconds

    Serial.println("Enter the pause duration after backward movement (seconds):");
    clearSerialBuffer();
    while (Serial.available() == 0) {
        delay(100);
    }
    pause2 = Serial.parseInt() * 1000; // Convert to milliseconds

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

    if (!SD.begin(chipSelect)) {
        Serial.println("SD card initialization failed! Check the wiring and the card.");
        while (1); // Stop execution if SD card initialization fails
    } else {
        Serial.println("SD card initialized successfully.");
    }

    // Clear old data.csv file
    if (SD.exists("data.csv")) {
        SD.remove("data.csv"); // Delete the old file
        Serial.println("Old data.csv file deleted.");
    }

    // Create a new file and write the header
    File dataFile = SD.open("data.csv", FILE_WRITE);
    if (dataFile) {
        dataFile.println("DAC Value,Speed,Angle,Total Time,Direction");
        dataFile.close();
    } else {
        Serial.println("Error creating data.csv!");
    }

    promptUserForInputs();
}

void logDataToSD(float time, float dacValue, float speed, float angle, float totalTime, int direction) {
    File dataFile = SD.open("data.csv", FILE_WRITE);
    if (dataFile) {
        dataFile.print(dacValue, 1);
        dataFile.print(",");
        dataFile.print(speed, 2);
        dataFile.print(",");
        dataFile.print(angle, 2);
        dataFile.print(",");
        dataFile.print(totalTime, 2);
        dataFile.print(",");
        dataFile.println(direction);
        dataFile.close();
    } else {
        Serial.println("Error opening data.csv in logDataToSD!");
    }
}

void moveMotor(float targetAngle, int direction, unsigned long cycleStartTime) {
    unsigned long startTime = millis();
    float angleCovered = 0.0;
    float elapsedTime;
    unsigned long lastLogTime = startTime;

    // Print the direction of motor movement
    Serial.print("Direction: ");
    Serial.println(direction == 1 ? "Forward" : "Backward");

    dac.setVoltage((speed / 10.0) / 5.0 * 4095, false); // Update DAC for speed

    // Set direction
    if (direction == 1) {
        digitalWrite(Fwd, HIGH);
        digitalWrite(Rev, LOW);
    } else {
        digitalWrite(Fwd, LOW);
        digitalWrite(Rev, HIGH);
    }

    while (abs(angleCovered) < abs(targetAngle)) {
        unsigned long currentTime = millis();
        elapsedTime = (currentTime - startTime) / 1000.0; // Time in seconds
        angleCovered = speed * elapsedTime;

        // Log data at 0.1-second intervals
        if (currentTime - lastLogTime >= 100) { // 100 ms = 0.1 seconds
            totalTime += 1; // Increment total time by 0.1 seconds
            Serial.print(elapsedTime, 3);
            Serial.print(", ");
            Serial.print(totalTime, 2);
            Serial.print(", ");
            Serial.print(angleCovered, 2);
            Serial.print(", ");
            Serial.println(direction == 1 ? "Forward" : "Backward");


            logDataToSD(elapsedTime, (speed / 10.0), speed, angleCovered, totalTime, direction);
            lastLogTime = currentTime; // Update last log time
        }


        
    }

    // Stop the motor and reset control
    digitalWrite(Fwd, LOW);
    digitalWrite(Rev, LOW);
    dac.setVoltage(0, false);
}


void loop() {
    unsigned long cycleStartTime;

    Serial.print("Elapsed Time(sec), Angle Covered (degrees), Total Time, Direction ");

    for (int i = 0; i < NumCycles; i++) {

        cycleStartTime = millis(); // Record the start time of the cycle
        
        // Move in the REV direction
        Serial.println("Moving to target angle...");
        moveMotor(targetAngle, direction, cycleStartTime);
        delay(pause1); // Pause after reaching target angle

        // Move back to the original position
        Serial.println("Returning to original position...");
        moveMotor(targetAngle, -direction, cycleStartTime);
        delay(pause2); // Pause after returning to original position
    }

    Serial.println("All cycles completed. Motor stopped.");
    while (true); // Stop further execution after all cycles are complete
}


