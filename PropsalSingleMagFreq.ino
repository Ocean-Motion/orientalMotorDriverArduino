/** Most_Updated_OMTwaveArduinoSim
 *
 * Code will saend the BLE2D200-C Motor driver the analog
 * and digital signals that it needs to drive a BLM5200HPK motor with 450:1
 * gearbox to make sinusoidal speed of variable magnitude and frequency.
 *
 * This motor driver needs an analog signal for amplitude and 2 digital signals for
 * direction.  Operation is expected via the TI Code Composer
 *
 * Copyright 2023 Ocean Motion Tech, All rights reserved
 *
 * Math for rotational speed of motor relative to arm length, wave frequency,
 * and wave height 4095 counts = 3.3V = 2827 RPM, gear ratio is 450.8:1, so
 * output rpm = 6.27 rpm, .1045 Hz, 1 rotation over 9.57s Counted 5 turns for
 * 47.95.
 *
 * observed 9.59s per rotation, verified above math .1045 Hz / 4095 counts =
 * 25.5 uHz per count, or 39186 counts per Hz Modified this in the driver to be
 * 1212 rpm/V, so 4000 rpm max, 8.87 rpm at the shaft, .148 Hz .148 Hz / 4095
 * counts = 36.14 uHz per count, or 27669 counts per Hz linear speed of the
 * float at the end of the arm =
 * waveMagnitude*waveFrequency*2*pi*cos(waveFrequency*t) angular speed of the
 * motor = asin(waveMagnitude/armLength)*linear speed of the float at the end of
 * the arm.
 *
 */

#include <Wire.h>
#include <Adafruit_MCP4725.h>
#include <TimerOne.h>
#include <SD.h>

#define PI 3.1415
#define TWOPI 6.283

Adafruit_MCP4725 dac;

const int dacAddress = 0x60;
const int Fwd = 2;
const int Rev = 3;
const int chipSelect = 53;

// Adjustable variables
float waveMagnitude = 1; //
float waveFrequency = .24; // Desired frequency in Hz
float armLength = 1.0;

// Volatile variables for ISR access
volatile float anglePU = 0;
volatile uint16_t dacval = 0;
volatile float angSpeed = 0;
volatile bool dataReady = false;
volatile unsigned long lastTime = 0;
volatile unsigned long time = 0; // Time tracker in ms
volatile bool recordingComplete = false;
unsigned long recordingDuration = 500; // Default recording duration in ms

// Timer1 configuration
const int timerPeriodMicroseconds = 1000; // Timer period set to 1 ms (1000 us)

File dataFile;

void setup() {
    Serial.begin(9600);

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
        Serial.println("SD card initialization failed!");
        while (1); // Stop execution if SD card initialization fails
    }
    Serial.println("SD card initialized.");

    // Attempt to delete the file
    if (SD.remove("data.csv")) {
        Serial.println("data.csv deleted.");
    } else {
        Serial.println("Error deleting data.csv.");
    }

    // Create a new file and write header
    dataFile = SD.open("data.csv", FILE_WRITE); // Use FILE_WRITE
    if (dataFile) {
        dataFile.println("Time,AngularSpeed,DACValue,AnglePU"); // Write header
        dataFile.close(); // Close the file after writing
        Serial.println("New data.csv created.");
    } else {
        Serial.println("Error creating new data.csv.");
    }

    // Prompt user for recording duration
    Serial.println("Enter the recording duration in milliseconds:");
    while (Serial.available() == 0) {
        // Wait for user input
    }
    recordingDuration = Serial.parseInt(); // Read the input value
    Serial.print("Recording duration set to ");
    Serial.print(recordingDuration);
    Serial.println(" ms.");

    // Initialize the timer
    Timer1.initialize(timerPeriodMicroseconds);
    lastTime = micros(); // Record the start time
    Timer1.attachInterrupt(timerIsr); // Attach the timer ISR
}

void loop() {
    // Update DAC and SD card only if data is ready
    if (dataReady) {
        // Stop updating DAC if recording is complete
        if (!recordingComplete) {
            dac.setVoltage(dacval, false); }  // Update DAC output

        // Disable interrupts temporarily for safe SD card access
        Timer1.detachInterrupt();

        dataFile = SD.open("data.csv", FILE_WRITE); // Use FILE_WRITE
        if (dataFile) {
            dataFile.print(time);
            dataFile.print(",");
            dataFile.print(angSpeed);
            dataFile.print(",");
            dataFile.print(dacval);
            dataFile.print(",");
            dataFile.println(anglePU);
            dataFile.close(); // Close the file after writing
        } else {
            Serial.println("Error opening data.csv in loop!");   }

        Serial.print("Time: ");
        Serial.print(time);
        Serial.print(", Angular Speed: ");
        Serial.print(angSpeed);
        Serial.print(", DAC Value: ");
        Serial.print(dacval);
        Serial.print(", Angle PU: ");
        Serial.println(anglePU);

        // Reattach interrupt after SD card operations
        Timer1.attachInterrupt(timerIsr);

        dataReady = false; // Reset data ready flag

        // Check if time has reached the user-specified duration
        if (time >= recordingDuration && !recordingComplete) {
            recordingComplete = true;
            Timer1.stop(); // Stop the timer
            Serial.println("Recording complete. Appending to data file.");
            
            // Stop motor by setting DAC value to 0 and turning off both direction pins
            dac.setVoltage(0, false); // Set DAC to 0
            digitalWrite(Fwd, LOW);   // Stop the motor
            digitalWrite(Rev, LOW);   // Stop the motor

            // Open the file again to append magnitude and frequency
            dataFile = SD.open("data.csv", FILE_WRITE); // Use FILE_WRITE
            if (dataFile) {
                dataFile.print("Magnitude,");
                dataFile.println(waveMagnitude);
                dataFile.print("Frequency,");
                dataFile.println(waveFrequency);
                dataFile.close(); // Close the file after appending
                Serial.println("Magnitude and Frequency appended to data.csv.");
            } else {
                Serial.println("Error opening data.csv for appending!"); }
            while (1); // Halt the program to stop further execution
        }  } }

void timerIsr() {
    unsigned long currentTimeMicros = micros(); // Get the current time
    float elapsedTime = (currentTimeMicros - lastTime) / 1920000.0; // Convert to seconds, adjusted to match frequency

    // Update the phase based on the actual elapsed time
    anglePU += waveFrequency * elapsedTime;

    if (anglePU >= 1.0) {
        anglePU -= 1.0; // Ensure continuity of the wave
    }

    // Calculate armComponent based on waveMagnitude and armLength
    float armComponent = atan((waveMagnitude / armLength) /
                              sqrt(1 + (waveMagnitude * waveMagnitude / (armLength * armLength))));

    // Calculate angular speed
    angSpeed = armComponent * cos(TWOPI * anglePU) * waveMagnitude * waveFrequency;

    // Calculate the DAC value
    dacval = (constrain(abs((int)(angSpeed * 27669)), 0, 4095)); // Scaled to avoid clipping

    // Update the direction based on angSpeed
    if (angSpeed > 0) {
        digitalWrite(Fwd, LOW);
        digitalWrite(Rev, HIGH);
    } else {
        digitalWrite(Rev, LOW);
        digitalWrite(Fwd, HIGH);
    }

    // Update the current time in milliseconds
    time += timerPeriodMicroseconds / 1000;

    // Signal that data is ready to be processed
    dataReady = true;

    // Record the current time for the next ISR
    lastTime = currentTimeMicros;
}


