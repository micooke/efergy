#include <Arduino.h>

#define _DEBUG 1 // define this before the efergy include if you want debug messages
#define EFERGY_SAMPLE_DATA
#include "efergy_tx.h"
//
//+------------+---+--------+--------+--------+--------+--------+
//| Chip       |   | Timer0 | Timer1 | Timer2 | Timer3 | Timer4 |
//+------------+---+--------+--------+--------+--------+--------+
//+------------+---+--------+--------+--------+--------+--------+
//|            |   | 8b PS  | 16b PS | 8b PS  |   --   |   --   |
//|            +---+--------+--------+--------+--------+--------+
//| ATmega328p | A |   D6#  |   D9   |  D12*  |   --   |   --   |
//|            | B |   D5   |  D10   |   D3   |   --   |   --   |
//+------------+---+--------+--------+--------+--------+--------+
// 8b/16b : 8 bit or 16 bit timer
// PS/ePS : Regular prescalar, Extended prescalar selection
//  PS = [0,1,8,64,256,1024]
// ePS = [0,1,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192,16384]
// # toggled output. The frequency is half the set frequency, the duty cycle is fixed at 50%
// * same as #, but software PWM. It is implemented through the respective TIMERx_OVF_vect ISR
//

#include <PWM.h>

uint32_t t0;

void setup()
{
#if (_DEBUG > 0)
  Serial.begin(9600);
#endif
  
  efergy_tx::init();
  
  t0 = millis();
}

void loop()
{
  if (efergy_tx::status == efergy::state::STOP)
  {
    efergy_tx::status = efergy::state::IDLE;
    t0 = millis();
  }
  else if (efergy_tx::status == efergy::state::IDLE)
  {
    if (millis() - t0 > 3000) // every 3 seconds
    {
      Serial.print(F("send: ")); efergy::S0.print();
      efergy_tx::send(efergy::S0.raw());
    }
  }
  
  yield();
}
