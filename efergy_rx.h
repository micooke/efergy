#pragma once

#include "efergy_structs.h"

/* Regarding the input pin
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

namespace efergy_rx {
  // note: cant use WAKE pin (D0/GPIO16) for interrupts
  // as it is already allocated to WAKE from deep sleep
  uint8_t inputPin = 5;
  
  volatile static uint32_t tR;
  volatile static uint32_t tF;
  volatile static int8_t bit_index;
  volatile efergy::state status;
  volatile bool new_data;

  volatile static uint64_t buffer;

//           _________________________            pw = 512us | freq = 1953Hz
// [ MARKER:/          512us          \ 20us] period = 532us | freq = 1879Hz
//           _________________________
// [   62us_/  136us | 198us  | 178us \_20us : as multiples of 198us
//                                _________
// [ LOW   :\____125-145us_______/ 65-72us \] period = 198us | freq = 5050Hz
//                         ________________
// [ HIGH  :\___52-58us___/    138-144us   \] period = 198us | freq = 5050Hz
//                                   ______
// [ TAIL  :\__________89us_________/ 20us \] period = 109us | freq = 9174Hz

  // on the esp8266, if you get the User exception "ISR not in IRAM!"
  // it is because you need to prepend ICACHE_RAM_ATTR to the ISR function
  // note: according to the esp8266 core documentation, this should be IRAM_ATTR  
  IRAM_ATTR void pinchange_ISR()
  {
    if (digitalRead(efergy_rx::inputPin)) // pin is HIGH == RISING edge
    {
      efergy_rx::tR = micros();  
    }
    else // pin is LOW == RISING edge
    {
      efergy_rx::tF = micros();
  
      if (efergy_rx::tR < efergy_rx::tF)
      {
        uint16_t pulse_width = efergy_rx::tF - efergy_rx::tR;
  
        if (pulse_width > 400) // marker
        {
          bit_index = 0;
          efergy_rx::status = efergy::state::RUN;
        }
        else if ((pulse_width < 40) && (bit_index > 63)) // tail and we have filled the buffer
        {
          efergy_rx::new_data = true;
          efergy_rx::status = efergy::state::IDLE;
        }
        else
        {
          uint8_t isHIGH = (pulse_width > 100) & 0x1;
          efergy_rx::buffer <<= 1;
          efergy_rx::buffer |= isHIGH;
          bit_index++;
        }
      }
    }
  }

  void init(const uint8_t receive_pin = 5)
  {
    efergy_rx::inputPin = receive_pin;

    efergy_rx::status = efergy::state::IDLE;
    efergy_rx::tR=0xFFFFFFFF;
    efergy_rx::tF=0xFFFFFFFF;
    efergy_rx::new_data = false;
    
    pinMode(efergy_rx::inputPin, INPUT);
    
    attachInterrupt(digitalPinToInterrupt(efergy_rx::inputPin), efergy_rx::pinchange_ISR, CHANGE); 
  }
};
