#pragma once

#include "efergy_structs.h"

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

#include <PWM.h> // yes its a self-licking icecream, but you need github.com/micooke/PWM library

namespace efergy_tx{
volatile static int8_t bit_index;
volatile static uint8_t pulse_width_index;
volatile efergy::state status;

static efergy::buffer64b message;

// this is a bit lazy (all these variables should be 16b) - but the math 
// to calculate the pulse width needs to be done in 32b, otherwise it
// will overflow.
uint32_t pulse_width[2]={0};
uint16_t data_period=0;
uint32_t tail_register=0;
uint16_t tail_period=0;
uint32_t marker_register=0; 
uint16_t marker_period=0;
const uint8_t setup_bits = 1;
const uint8_t marker_bits = 1;
const uint8_t tail_bits = 1;
const uint8_t data_bits = 64;

void stop()
{
  pwm.disableInterrupt(1, 'a');
  pwm.stop(1);
}

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
static void compare_ISR()
{
  if (efergy_tx::status == efergy::state::RUN)
  {
    // note: all comparisons are to bit_index + 1,
    // which gets incremented in the overflow ISR and refers to the next state
    
    // 1 marker bits, 64 data bits, 1 tail bit, -1 as the data is zero-indexed
    if ((efergy_tx::bit_index + 1) >= 0 )
    {
      if ((efergy_tx::bit_index + 1) == data_bits )//(marker_bits + data_bits)) // 1 tail bit
      {
        OCR1A = efergy_tx::tail_register;
      }
      else if ( efergy_tx::bit_index + 1 < 64 ) // 64 data bits
      {
        efergy_tx::pulse_width_index = efergy_tx::message[efergy_tx::bit_index + 1]; // - marker_bits];
        OCR1A = efergy_tx::pulse_width[efergy_tx::pulse_width_index];
      }
    }
  }
}

static void overflow_ISR()
{
  if (efergy_tx::status == efergy::state::RUN)
  {
    ++efergy_tx::bit_index;
    if (efergy_tx::bit_index == 0)
    {
      TCCR1A |=  _BV(COM1A0); // set invert bit
      ICR1 = efergy_tx::data_period;
    }
    else if (efergy_tx::bit_index == -1)
    {
      //TCCR1A &=  ~_BV(COM1A0); // clear invert bit
      TCCR1A |=  _BV(COM1A0); // set invert bit
    }
    else if (efergy_tx::bit_index > 0)
    {
      // the overflow ISR occurs at the end of a pulse, so in the ISR we 
      // set up the next pulse in the pulse train
  
      // 1 marker bits, 64 data bits, 1 tail bit, -1 as the data is zero-indexed
      if (efergy_tx::bit_index == (data_bits + tail_bits))//(marker_bits + data_bits + tail_bits - 1))
      {
        efergy_tx::status = efergy::state::STOP;
        stop();
      }
      else if (efergy_tx::bit_index == data_bits)//(marker_bits + data_bits)) // 1 tail bit
      {
        ICR1 = efergy_tx::tail_period;
      }
    }
  }
}

void init()
{
  // divisor = 2 -> 50% duty cycle
  // this doesn't matter, we set it to the correct pulse width prior to pwm.start();
  const uint8_t divisor = 2;

  pwm.stop(1);
  
  pwm.set(1, 'a', 9174, divisor, true);
  efergy_tx::tail_period = ICR1;

  //pwm.set(1, 'a', 1879, divisor, false);
  pwm.set(1, 'a', 1953, divisor, false);
  efergy_tx::marker_period = ICR1; // assumes that the prescalars are the same for marker_period and data_period
  //efergy_tx::marker_register = ICR1;
  efergy_tx::marker_register = (efergy_tx::marker_period * 512)/ 532;
  
  pwm.set(1, 'a', 5050, divisor, true);
  efergy_tx::data_period = ICR1;

  efergy_tx::pulse_width[0] = (efergy_tx::data_period*130)/198; // 130 = midpoint of [125:135]
  efergy_tx::pulse_width[1] = (efergy_tx::data_period*55)/198; // 55 = midpoint of [52:58]

  efergy_tx::tail_register = (efergy_tx::tail_period * 89) / 109;
  
  debug_print("header_period: "); debug_println(efergy_tx::marker_period);
  debug_print("header_reg   : "); debug_println(efergy_tx::marker_register);

  debug_print("data_period  : "); debug_println(efergy_tx::data_period);
  debug_print("low_pw       : "); debug_println(efergy_tx::pulse_width[0]);
  debug_print("high_pw      : "); debug_println(efergy_tx::pulse_width[1]);
  
  debug_print("tail_period  : "); debug_println(efergy_tx::tail_period);
  debug_print("tail_register: "); debug_println(efergy_tx::tail_register);
  
  pwm.attachInterrupt(1, 'a', efergy_tx::compare_ISR);
  pwm.attachInterrupt(1, 'o', efergy_tx::overflow_ISR);
  
  efergy_tx::status = efergy::state::IDLE;
}

void send(const uint64_t &message)
{
  stop();

  efergy_tx::message = message; // allocate the new message
  
  TCCR1A &= ~_BV(COM1A0); // clear invert bit
  //TCCR1A |=  _BV(COM1A0); // set invert bit
  
  // this is a hack, but it works
  // i think the ISRs take too long, so pulse widths/period times
  // dont propogate properly for the marker pulse
  efergy_tx::bit_index = -2; // -1 for setup: to propogate ICR1, OCR1A settings -1 for the marker
  
  TCNT1 = 0; // clear the Timer1 counter
  OCR1A = efergy_tx::marker_register;
  ICR1 = efergy_tx::marker_period;
  
  pwm.enableInterrupt(1, 'a');
  efergy_tx::status = efergy::state::RUN;
  pwm.start(1);
}

};