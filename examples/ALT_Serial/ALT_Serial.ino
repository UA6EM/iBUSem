/*
 * This example will receive iBUS packets, and then re-transmit them
 * but with their values at 50%. For all channels but 3 (throttle)
 * this is still centered on 1500
 */

//#define ALT_SERIAL

#include "iBUSem.h"
#include <AltSoftSerial.h>
#include <CustomSoftwareSerial.h>

// *** ВАЖНО!!! *** ЧИТАТЬ ВНИМАТЕЛЬНО!!! ***

// Так как библиоткека iBUSTelemetry.h использует прерывание PCINT0 во избежание конфликта
// в библиотке CustomSoftwareSerial.h требуется замаркировать блок обработки прерывания PCINT0
// строки 313 - 320, то-есть для порта можно использовать только пины  A0 .. A5 или D0 .. D7
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
  while(!Serial) // On USB CDC serial ports, wait for the serial connection.
  {;}

  // Set timeout between received packets to 20 ms before considering it a lost connection
  ibus.set_alive_timeout(20);

  // Set minimum time between transmitting packets, to 5ms
  ibus.set_tx_period(5);

  pinMode(LED_BUILTIN, OUTPUT);

  Serial.println("iBus test");
}

void loop() 
{
  ibus.handle(); // Run this often
  
  // Reduce all channels by 50%
  for(int i=0; i<channels_per_packet; i++)
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
}


uint32_t last_print = 0;
const int PERIOD = 50;
const int channels = 6;
void print_channels()
{
  // Only print every PERIOD ms
  if(millis() - last_print > PERIOD)
  {
    last_print = millis();
    for(int i=0; i<channels; i++)
    {
      Serial.print("ch "); Serial.print(i+1); Serial.print(": "); Serial.print(ibus.get_channel(i));
      if(ibus.is_alive())
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
