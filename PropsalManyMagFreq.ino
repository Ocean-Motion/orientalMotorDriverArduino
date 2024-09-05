/** Most_Updated_OMTwaveArduinoSim
 *
 * Code will send the BLE2D200-C Motor driver the analog
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
float waveMagnitude = 0;
float waveFrequency = 0; // Desired frequency in Hz
float armLength = 1.0;

// Interval variables
int numIntervals = 1; // Number of intervals
unsigned long intervalDuration = 1000; // Duration for each interval in milliseconds
float magnitudes[4] = {1.0, 1.0, 1.0, 1.0}; // Default magnitudes
float frequencies[4] = {0.24, 0.24, 0.24, 0.24}; // Default frequencies

// Volatile variables for ISR access
volatile float anglePU = 0;
volatile uint16_t dacval = 0;
volatile float angSpeed = 0;
volatile float armComponent = 0; // Add this to hold armComponent value
volatile bool dataReady = false;
volatile unsigned long lastTime = 0;
volatile unsigned long time = 0; // Time tracker in milliseconds
volatile bool recordingComplete = false;
unsigned long recordingDuration = 10000; // Default recording duration in milliseconds

// Timer1 configuration
const int timerPeriodMicroseconds = 1000; // Timer period set to 1 ms (1000 us)

File dataFile;

// Declare CSVTime as a global variable
unsigned long CSVTime = 0;
int previousInterval = -1; // Track the previous interval

void setup() {
    Serial.begin(9600);
    while (!Serial) { ; } // Wait for serial port to connect

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
        dataFile.println("Time,AngularSpeed,DACValue,AnglePU,ArmComponent,Interval,Magnitude,Frequency"); // Write header with new columns
        dataFile.close(); // Close the file after writing
        Serial.println("New data.csv created.");
    } else {
        Serial.println("Error creating new data.csv.");
    }

    // Prompt user for number of intervals
    Serial.println("Enter the number of intervals (1-4):");
    while (Serial.available() == 0) {
        // Wait for user input
    }
    numIntervals = Serial.parseInt();
    Serial.read(); // Clear the newline character from the buffer

    // Ensure the number of intervals is within the allowed range
    if (numIntervals < 1) numIntervals = 1;
    if (numIntervals > 4) numIntervals = 4;

    // Prompt user for total recording duration
    Serial.println("Enter the total duration for the recording (in milliseconds):");
    while (Serial.available() == 0) {
        // Wait for user input
    }
    recordingDuration = Serial.parseInt();
    Serial.read(); // Clear the newline character from the buffer

    // Calculate the duration for each interval
    intervalDuration = recordingDuration / numIntervals;

    // Prompt user for magnitudes and frequencies for each interval
    for (int i = 0; i < numIntervals; i++) {
        Serial.print("Enter magnitude for interval ");
        Serial.print(i + 1);
        Serial.println(":");
        while (Serial.available() == 0) {
            // Wait for user input
        }
        magnitudes[i] = Serial.parseFloat();
        Serial.read(); // Clear the newline character from the buffer

        Serial.print("Enter frequency (in Hz) for interval ");
        Serial.print(i + 1);
        Serial.println(":");
        while (Serial.available() == 0) {
            // Wait for user input
        }
        frequencies[i] = Serial.parseFloat();
        Serial.read(); // Clear the newline character from the buffer
    }

    Serial.print("Interval duration set to ");
    Serial.print(intervalDuration);
    Serial.println(" milliseconds.");

    Serial.print("Total recording duration set to ");
    Serial.print(recordingDuration);
    Serial.println(" milliseconds.");

    // Initialize the timer
    Timer1.initialize(timerPeriodMicroseconds);

    // Initial reset of values at the start of the program
    resetValues();

    lastTime = micros(); // Record the start time
    Timer1.attachInterrupt(timerIsr); // Attach the timer ISR
}

void loop() {
    // Update DAC and SD card only if data is ready
    if (dataReady) {
        dac.setVoltage(dacval, false); // Update DAC output

        // Disable interrupts temporarily for safe SD card access
        Timer1.detachInterrupt();

        dataFile = SD.open("data.csv", FILE_WRITE); // Use FILE_WRITE
        if (dataFile) {
            int currentInterval = getCurrentInterval();
            dataFile.print(CSVTime);
            dataFile.print(",");
            dataFile.print(angSpeed);
            dataFile.print(",");
            dataFile.print(dacval);
            dataFile.print(",");
            dataFile.print(anglePU);
            dataFile.print(",");
            dataFile.print(armComponent); // Print armComponent
            dataFile.print(",");
            dataFile.print(currentInterval + 1); // Interval number (1-based)
            dataFile.print(",");
            dataFile.print(magnitudes[currentInterval]); // Magnitude for the current interval
            dataFile.print(",");
            dataFile.println(frequencies[currentInterval]); // Frequency for the current interval
            dataFile.close(); // Close the file after writing
        } else {
            Serial.println("Error opening data.csv in loop!");
        }

        Serial.print("Time: ");
        Serial.print(CSVTime); // Print the CSVTime
        Serial.print(", Angular Speed: ");
        Serial.print(angSpeed);
        Serial.print(", DAC Value: ");
        Serial.print(dacval);
        Serial.print(", Angle PU: ");
        Serial.print(anglePU);
        Serial.print(", Arm Component: ");
        Serial.print(armComponent); // Print armComponent
        Serial.print(", Interval: ");
        Serial.print(getCurrentInterval() + 1);
        Serial.print(", Magnitude: ");
        Serial.print(magnitudes[getCurrentInterval()]);
        Serial.print(", Frequency: ");
        Serial.println(frequencies[getCurrentInterval()]);

        // Reattach interrupt after SD card operations
        Timer1.attachInterrupt(timerIsr);

        dataReady = false; // Reset data ready flag

        // Check if time has reached the user-specified duration
        if (time >= recordingDuration) {
            recordingComplete = true;
            Timer1.stop(); // Stop the timer
            Serial.println("Recording complete. Halting execution.");

            // Set DAC to 0 to stop the motor
            dac.setVoltage(0, false);
            digitalWrite(Fwd, LOW);
            digitalWrite(Rev, LOW);
            
            while (1); // Halt the program to stop further execution
        }
    }
}

void timerIsr() {
    unsigned long currentTimeMicros = micros();
    float elapsedTime = (currentTimeMicros - lastTime) / 1000000.0; // Corrected elapsed time calculation
    lastTime = currentTimeMicros;

    // Manage interval switching
    int currentInterval = getCurrentInterval();
    if (currentInterval != previousInterval) {
        Timer1.detachInterrupt(); // Stop the timer during transition
        delay(1000); // Delay for stability

        // Update waveMagnitude and waveFrequency for the new interval
        waveMagnitude = magnitudes[currentInterval];
        waveFrequency = frequencies[currentInterval];

        // Update armComponent based on the new interval
        armComponent = atan((waveMagnitude / armLength) /
                            sqrt(1 + (waveMagnitude * waveMagnitude / (armLength * armLength))));

        resetValues(); // Reset necessary values
        previousInterval = currentInterval;
        Timer1.attachInterrupt(timerIsr); // Resume the timer
    }

    // Increment and wrap anglePU
    anglePU += waveFrequency * elapsedTime;
    if (anglePU >= 1.0) {
        anglePU -= 1.0; // Ensure continuity of the wave
    }

    // Calculate angular speed using a smooth phase
    angSpeed = waveMagnitude * waveFrequency * cos(TWOPI * anglePU) * armComponent;

    // Calculate DAC value
    dacval = constrain(abs(static_cast<int>(angSpeed * 27669)), 0, 4095);

    // Direction control
    if (angSpeed > 0) {
        digitalWrite(Fwd, LOW);
        digitalWrite(Rev, HIGH);
    } else {
        digitalWrite(Rev, LOW);
        digitalWrite(Fwd, HIGH);
    }

    // Update time and interval-specific CSV time
    time += timerPeriodMicroseconds / 1000;
    CSVTime = time - (intervalDuration * getCurrentInterval());

    dataReady = true;
}

// Function to determine the current interval based on elapsed time
int getCurrentInterval() {
    int intervalIndex = time / intervalDuration;
    if (intervalIndex >= numIntervals) {
        intervalIndex = numIntervals - 1; // Ensure it does not exceed the number of intervals
    }
    return intervalIndex;
}

// Function to reset anglePU, dacval, and angSpeed to 0
void resetValues() {
    anglePU = 0;
    dacval = 0;
    angSpeed = 0;
    // Add any other variable resets here if necessary
}
