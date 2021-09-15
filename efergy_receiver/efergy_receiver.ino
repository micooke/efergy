#include <Arduino.h>

#define _DEBUG 1 // define this before the efergy include if you want debug messages
#include "efergy_rx.h"
/*
 * pin|  GPIO  | at boot | notes
 * ---+--------+---------+-------
 * D0 | GPIO16 |    HIGH | WAKE no interrupts
 * D1 | GPIO5  |         | SCL
 * D2 | GPIO4  |         | SDA
 * D3 | GPIO0  |    HIGH | pulled HIGH, flash if LOW at boot 
 * D4 | GPIO2  |    HIGH | pulled HIGH, connected to LED, boot fails if LOW
 * D5 | GPIO14 |         | SCLK
 * D6 | GPIO12 |         | MISO
 * D7 | GPIO13 |         | MOSI
 * D8 | GPIO15 |     LOW | pulled to GND
 * RX | GPIO3  |    HIGH | 
 * TX | GPIO1  |    HIGH | boot fails if LOW
 * A0 | ADC0   |         | Analog in; no output
 */

efergy::buffer64b message;

void setup()
{
#if (_DEBUG > 0)
  Serial.begin(9600);
#endif

  efergy_rx::init(5);

  debug_println(__FILE__);
  debug_println(__TIME__);
}

void loop()
{
  if ( efergy_rx::status == efergy::state::IDLE)
  {
      if (efergy_rx::new_data)
      {
        message = (const uint64_t&)efergy_rx::buffer;
        
        message.print();

        efergy_rx::new_data = false;
      }
  }
  
  yield();
}
