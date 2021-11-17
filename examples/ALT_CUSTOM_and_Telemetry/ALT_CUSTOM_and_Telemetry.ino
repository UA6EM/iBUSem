/*
   This example will receive iBUS packets, and then re-transmit them
   but with their values at 50%. For all channels but 3 (throttle)
   this is still centered on 1500
*/

//#define ALT_SERIAL
#define UPDATE_INTERVAL 500

#include "iBUSem.h"
#include <AltSoftSerial.h>
#include <CustomSoftwareSerial.h>
#include <iBUSTelemetry.h>
iBUSTelemetry telemetry(11); // I use only PCINT0 interrupt vector, so you can use D8 to D13 pins.

uint32_t prevMillis = 0; // Necessary to updateValues() method. Look below.
float i = 0;


// ЧИТАТЬ ВНИМАТЕЛЬНО!!!
// Так как библиоткека iBUSTelemetry.h использует прерывание PCINT0 во избежание конфликта
// в библиотке CustomSoftwareSerial.h требуется замаркировать блок обработки прерывания PCINT0
// строки 313 - 320, то-есть можно использовать только пины  A0 .. A5 или D0 .. D7
/*
    D8 .. D13 - генерируют запрос прерывания PCINT0
    A0 .. A5  - генерируют запрос прерывания PCINT1
    D0 .. D7  - генерируют запрос прерывания PCINT2
*/

#ifdef ALT_SERIAL
AltSoftSerial sw_ser(8, 9);
iBus ibus(sw_ser);
#else
CustomSoftwareSerial sw_ser(6, 7);
iBus ibus(sw_ser);
#endif

// If you are on a board with multiple hardware serial, you can use that instead
// iBus ibus(Serial2);

// Currently, recivers transmit 14 (10) channels over iBUS, with the inactive channels set to 1500
int channels_per_packet = 10;

void setup()
{
  Serial.begin(115200);
  while (!Serial) // On USB CDC serial ports, wait for the serial connection.
  {
    ;
  }

  // Set timeout between received packets to 20 ms before considering it a lost connection
  ibus.set_alive_timeout(20);

  // Set minimum time between transmitting packets, to 5ms
  ibus.set_tx_period(5);
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.println("iBus test");

  telemetry.begin(); // Let's start having fun!
  telemetry.addSensor(0x01);  // You can use sensors definitions from iBUSSensors.h instead of numbers.
  // Ex.: telemetry.addSensor(IBUS_MEAS_TYPE_TEM);
  telemetry.addSensor(IBUS_MEAS_TYPE_GPS_STATUS);
  telemetry.addSensor(IBUS_MEAS_TYPE_SPE);
  telemetry.addSensor(IBUS_MEAS_TYPE_GPS_LAT);
  telemetry.addSensor(IBUS_MEAS_TYPE_ARMED);
  telemetry.addSensor(IBUS_MEAS_TYPE_FLIGHT_MODE);
  telemetry.addSensor(IBUS_MEAS_TYPE_ALT);
}

void loop()
{
  ibus.handle(); // Run this often

  // Reduce all channels by 50%
  for (int i = 0; i < channels_per_packet; i++)
  {
    int old_value = ibus.get_channel(i);

    // map the old value (1000 to 2000) to a new value going from 1250 to 1750
    int new_value = map(old_value, 1000, 2000, 1250, 1750);

    // Set this new value to be sent on the next handle()
    ibus.set_channel(i, new_value);
  }

  // The above will mess with the throttle, so fix the throttle channel separately
  // Channels are zero-indexed.
  ibus.set_channel(2, map(ibus.get_channel(2), 1000, 2000, 1000, 1500));

  print_channels();

  updateValues(); // Very important! iBUS protocol is very sensitive to timings.
  // DO NOT USE ANY delay()! Look at updateValues() method.
  // It's an example of how to use intervals without delays.

  telemetry.run(); //It must be here. Period.
}


uint32_t last_print = 0;
const int PERIOD = 50;
const int channels = 6;
void print_channels()
{
  // Only print every PERIOD ms
  if (millis() - last_print > PERIOD)
  {
    last_print = millis();
    for (int i = 0; i < channels; i++)
    {
      Serial.print("ch "); Serial.print(i + 1); Serial.print(": "); Serial.print(ibus.get_channel(i));
      if (ibus.is_alive())
      {
        Serial.println();
        digitalWrite(LED_BUILTIN, LOW);
      }
      else
      {
        Serial.println(" !!LINK DEAD!!");
        digitalWrite(LED_BUILTIN, HIGH);
      }
    }
    Serial.print("Last valid packet was received "); Serial.print(ibus.time_since_last()); Serial.println(" ms ago");
  }
}

void updateValues()
{
  uint32_t currMillis = millis();

  if (currMillis - prevMillis >= UPDATE_INTERVAL) { // Code in the middle of these brackets will be performed every 500ms.
    prevMillis = currMillis;

    telemetry.setSensorValueFP(1, i); // Now, you have two ways to set sensors values. Using floating point variables
    // or directly in 32bit integer, but then you have to format variable by yourself.
    // Ex.: telemetry.setSensorValueFP(1, 24.45); is equivalent to telemetry.setSensorValue(1, 2445);
    // The values differ for different sensors.

    telemetry.setSensorValue(2, telemetry.gpsStateValues(3, 8)); // As GPS status consists of two values,
    // use gpsStateValues(firstVal, secondVal) to set it properly.

    telemetry.setSensorValue(3, 123 * 10);

    telemetry.setSensorValue(4, 179583647); // You can set LAT / LON using FP metohod, but due to fact floats have only 6 digits precision,
    // your values on tx may be corrupted in some cases. It's better to use setSensorValue().

    telemetry.setSensorValue(5, UNARMED); // ARMED / UNARMED or 1 / 0 could be used.

    telemetry.setSensorValue(6, LOITER); // Available flight modes:     STAB   0
    // ACRO   1
    // AHOLD  2
    // AUTO   3
    // GUIDED 4
    // LOITER 5
    // RTL    6
    // CIRCLE 7
    // PHOLD  8
    // LAND   9

    telemetry.setSensorValueFP(7, 54.87);

    i += 0.1;
    if (i > 50)
      i = 0;

    // These were the most difficult sensors to use. I hope that this library will be useful for you and will make your work easier. :)
  }
} /* updateValues */
