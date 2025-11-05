/*
 * Project Ledger Temperature Logger
 * Author: Eric Pietrowicz
 * Date: November 3rd, 2025
 * For comprehensive documentation and examples, please visit:
 * https://docs.particle.io/firmware/best-practices/firmware-template/
 */

// Include Particle Device OS APIs
#include "Particle.h"
#include <Wire.h>

#define TMP112A_ADDR 0x48

#ifndef SYSTEM_VERSION_v620
SYSTEM_THREAD(ENABLED); // System thread defaults to on in 6.2.0 and later and this line is not required
#endif

// Let Device OS manage the connection to the Particle Cloud
SYSTEM_MODE(AUTOMATIC);

typedef struct
{
  float degreesF;
  float degreesC;
} TemperatureReading;

// Show system, cloud connectivity, and application logs over USB
// View logs with CLI using 'particle serial monitor --follow'
SerialLogHandler logHandler(LOG_LEVEL_INFO);
TemperatureReading tempReading = {0.0, 0.0};

const std::chrono::milliseconds publishPeriod = 60s;
unsigned long lastPublish;

Ledger temperatureLedger;

void initializeTemperature()
{
  Wire.begin();
  Wire.beginTransmission(TMP112A_ADDR);

  // Select configuration register
  Wire.write(0x01);
  // Continuous conversion, comparator mode, 12-bit resolution
  Wire.write(0x60);
  Wire.write(0xA0);
  // Stop I2C Transmission
  Wire.endTransmission();
  delay(300);
}

void readTemperature(TemperatureReading *reading)
{
  unsigned data[2] = {0, 0};

  // Start I2C Transmission
  Wire.beginTransmission(TMP112A_ADDR);
  // Select data register
  Wire.write(0x00);
  // Stop I2C Transmission
  Wire.endTransmission();
  delay(300);

  // Request 2 bytes of data
  Wire.requestFrom(TMP112A_ADDR, 2);

  // Read 2 bytes of data
  // temp msb, temp lsb
  if (Wire.available() == 2)
  {
    data[0] = Wire.read();
    data[1] = Wire.read();
  }

  // Convert the data to 12-bits
  int temp = ((data[0] * 256) + data[1]) / 16;
  if (temp > 2048)
  {
    temp -= 4096;
  }
  float cTemp = temp * 0.0625;
  float fTemp = cTemp * 1.8 + 32;

  reading->degreesC = cTemp;
  reading->degreesF = fTemp;
}

// setup() runs once, when the device is first turned on
void setup()
{
  // Put initialization like pinMode and begin functions here
  temperatureLedger = Particle.ledger("temperature");
}

// loop() runs over and over again, as quickly as it can execute.
void loop()
{
  if (Particle.connected())
  {
    if ((lastPublish == 0) || (millis() - lastPublish >= publishPeriod.count()))
    {
      readTemperature(&tempReading);
      Log.info("Temperature: %.2f C / %.2f F", tempReading.degreesC, tempReading.degreesF);

      Variant data;
      data.set("tempF", tempReading.degreesF);
      data.set("tempC", tempReading.degreesC);
      if (Time.isValid())
      {
        data.set("time", Time.format(TIME_FORMAT_ISO8601_FULL)); // Time.format returns a String
      }
      temperatureLedger.set(data);
      lastPublish = millis();
    }
  }
}