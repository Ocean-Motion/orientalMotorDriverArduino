#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <SD.h>  // Include the SD library

// Create an instance of the Adafruit_ADS1115
Adafruit_ADS1115 ads;  // Use this for the 16-bit version

// Sensor specifications
const float fullScaleOutputV = 2.0; // mV/V (sensor output at full scale)
const float excitationVoltage = 5.0; // V (excitation voltage applied to the sensor)
const float fullScaleTorque = 200.0; // in-lb (torque corresponding to the full-scale output)

// Conversion factor from inch-pounds to newton-meters
const float inchToNmConversionFactor = 0.113;

// Calculate Full Scale Voltage from sensor specifications
const float fullScaleVoltage = fullScaleOutputV * excitationVoltage * 1e-3; // Convert to Volts from mV

// SD card configuration
const int chipSelect = 53; // Pin 53 connected to the CS pin of the SD card module
File dataFile; // File object for writing data

void setup(void)
{
  Serial.begin(9600);
  Serial.println("Getting differential reading from AIN0 (P) and AIN1 (N)");
  Serial.println("ADC Range: +/- 6.144V (1 bit = 0.1875mV/ADS1115)");

  // Initialize SD card
  if (!SD.begin(chipSelect)) {
    Serial.println("Failed to initialize SD card. Please check the wiring and card.");
    while (1);  // Stop execution if the SD card doesn't initialize
  } else {
    Serial.println("SD card initialized successfully.");

    // Remove the existing file if it exists
    if (SD.exists("NmSec.csv")) {
      if (SD.remove("NmSec.csv")) {
        Serial.println("Old NmSec.csv file deleted.");
      } else {
        Serial.println("Failed to delete old NmSec.csv file.");
      }
    }
  }

  // Create a new CSV file
  dataFile = SD.open("NmSec.csv", FILE_WRITE);
  if (dataFile) {
    // Write the header line to the CSV file
    dataFile.println("Time (m),Time (s),Differential Reading (raw),Voltage (mV),Measured Voltage (V),Torque (in-lb),Torque (N·m)");
    dataFile.close(); // Close the file
  } else {
    Serial.println("Failed to create NmSec.csv file.");
  }

  // Verify the I2C connection and initialize the ADS1115
  if (!ads.begin()) {
    Serial.println("Failed to initialize ADS. Please check wiring and I2C address.");
    while (1);  // Stop execution if the ADS1115 doesn't initialize
  } else {
    Serial.println("ADS1115 initialized successfully.");
  }

  // Set the gain to ±6.144V (default)
  ads.setGain(GAIN_TWOTHIRDS);
}

void loop(void)
{
  // Read a single differential value from AIN0 and AIN1
  int16_t result = ads.readADC_Differential_0_1();

  // Calculate the voltage based on the result
  float multiplier = 0.1875F; // For ADS1115 @ ±6.144V gain (16-bit results)
  float voltage = result * multiplier;  // Result in millivolts

  // Convert voltage from millivolts to volts
  float measuredVoltage = voltage * 1e-3; // Convert to volts

  // Calculate the Torque
  float torqueInInLb = (measuredVoltage / fullScaleVoltage) * fullScaleTorque;
  
  // Convert torque from inch-pounds to newton-meters
  float torqueInNm = torqueInInLb * inchToNmConversionFactor;

  // Get the current time (in milliseconds) since the board started
  unsigned long currentTime = millis();
  
  // Print time in a readable format (convert millis to seconds)
  unsigned long seconds = currentTime / 1000;
  unsigned long minutes = seconds / 60;
  seconds = seconds % 60; // Remaining seconds

  // Print all information in Serial Monitor
  Serial.print("Time: ");
  Serial.print(minutes);
  Serial.print("m ");
  Serial.print(seconds);
  Serial.print("s - Differential Reading: ");
  Serial.print(result);
  Serial.print(" (");
  Serial.print(voltage);
  Serial.print(" mV), Measured Voltage: ");
  Serial.print(measuredVoltage, 6); // Print voltage in volts with six decimal places
  Serial.print(" V, Calculated Torque: ");
  Serial.print(torqueInInLb);
  Serial.print(" in-lb, Calculated Torque: ");
  Serial.print(torqueInNm);
  Serial.println(" N·m");

  // Write all information to SD card
  dataFile = SD.open("NmSec.csv", FILE_WRITE);
  if (dataFile) {
    dataFile.print(minutes);
    dataFile.print(",");
    dataFile.print(seconds);
    dataFile.print(",");
    dataFile.print(result);
    dataFile.print(",");
    dataFile.print(voltage);
    dataFile.print(",");
    dataFile.print(measuredVoltage, 6); // Print voltage in volts with six decimal places
    dataFile.print(",");
    dataFile.print(torqueInInLb);
    dataFile.print(",");
    dataFile.println(torqueInNm);
    dataFile.close(); // Close the file
  } else {
    Serial.println("Failed to write to NmSec.csv file.");
  }

  delay(1000); // Delay before the next loop iteration
}