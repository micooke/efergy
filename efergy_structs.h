#pragma once

#include <stdint.h>

#ifndef _DEBUG
#define _DEBUG 0
#endif

#if (_DEBUG == 1)
#define debug_print(...) Serial.print(__VA_ARGS__)
#define debug_println(...) Serial.println(__VA_ARGS__)
#elif (_DEBUG == 2)
#include <iostream>
#define debug_print(...) std::cout << __VA_ARGS__
#define debug_println(...) debug_print(__VA_ARGS__);debug_print("\n")
#else
#define debug_print(...)
#define debug_println(...)
#endif

// this is for normal cpp debugging
#ifndef Arduino_h
#include <string>
#define String std::string
#endif

namespace efergy {
// replicating: https://github.com/hardtechnology/efergy_v2/blob/master/efergy.cpp#L92
// but for clarity I just replicate the std::pow function, not the modified one by @steven-geo
const uint32_t pow(const uint8_t base, const uint8_t exponent) {
  uint32_t pow_ = 1;
  for (uint8_t i = 0; i < exponent; ++i) {
    pow_ *= base;
  }
  return pow_;
}

enum state { RUN, STOP, IDLE};

// force 1 byte bit packing for the structs, otherwise the
// struct -> union alignment will fail
#pragma pack(push, 1)
struct efergy_e2{
  //b63
  uint8_t checksum; // B7
  uint8_t exponent; // B6
  uint16_t current_mA; // B4:B5
  uint8_t unused_:4; //bit0-3 of B3
  uint8_t battery:1; // bit6 of B3
  uint8_t interval:2; // bit5,4 of B3: 11=12s, 10=18s, 00=6s
  uint8_t pairing:1; // bit7 of B3
  uint16_t  ID; // B1:2
  uint8_t sync_bits; // B0
  //b0
};
#pragma pack(pop) // back to default bitpacking

union data_frame{
  efergy_e2 frame;
  uint64_t buffer;
  uint8_t bytes[8];
};

class buffer64b{
  private:
    data_frame data;
  public:
  const uint8_t operator[](const uint8_t bit_index) const {
    const uint8_t bit_value = (uint8_t)((data.buffer >> (63 - bit_index)) & 0x01);
    return bit_value;
  }
  //                          MSB..........................................................LSB
  buffer64b(const String &in="0000000000000000000000000000000000000000000000000000000000000000")
  {
    data.buffer = 0; // clear the buffer
    for (uint8_t b=0; b < in.length(); ++b)
    {
      push(in[b]=='1');
    }
  }
  const uint64_t& raw() const
  {
    return data.buffer;
  }
  buffer64b& operator=(const uint64_t &r)
  {
    data.buffer = r;
    return *this;
  }
  void set(const uint8_t i, const uint8_t r)
  {
    data.buffer |= (r << (63 - i));
  }
  uint8_t pop()
  {
    const uint8_t out = data.buffer & 0x1;
    data.buffer >>=1;
    return out;
  }
  void push(uint8_t r)
  {
    data.buffer <<=1;
    data.buffer |= (r & 0x1);
  }
    const uint32_t getCurrent_milliAmps() const
  {
    // note: the divisor is (2^15 / 2^(byte[6]) == 2^(15 - byte[6])) as per
    // * https://github.com/magellannh/RPI_Efergy/blob/master/EfergyRPI_log.c#L335
    // which is bundled in the power2 function here: https://github.com/hardtechnology/efergy_v2/blob/master/efergy.cpp#L92
    const uint32_t current_mA = (data.frame.current_mA * 1000) / efergy::pow(2, 15 - data.frame.exponent);
    return current_mA;
  }
  const uint32_t getPower_Watts(const uint32_t reference_voltage=240) const
  {
    const uint32_t current_mA = getCurrent_milliAmps();
    return (current_mA*reference_voltage)/1000;
  }
  const bool checksumIsGood() const
  {
    // read it out as bytes
    //note: this is inverted, as thats how I packed it
    uint32_t cs_ = data.bytes[1] + data.bytes[2] + 
      data.bytes[3] + data.bytes[4] + data.bytes[5] +
      data.bytes[6] + data.bytes[7];

    return ((cs_ & 0xFF) == data.frame.checksum);
  }
  const void print() const
  {
    char bit_value='0';
    debug_print("buffer = [");
    for(uint8_t i=0; i < 64; ++i)
    {
      bit_value = (uint8_t)((data.buffer >> (63 - i)) & 0x01)?'1':'0';
      debug_print(bit_value);
    }
    debug_println("]");
    
    String result_;
    debug_print("sync_bits  :"); debug_println((int)data.frame.sync_bits);
    debug_print("ID         :"); debug_println((int)data.frame.ID);
    debug_print("interval   :"); debug_println((int)data.frame.interval);
    debug_print("battery    :"); 
    result_ = (data.frame.battery)?"ok":"low"; debug_println(result_);
    debug_print("pairing    :");
    result_ = (data.frame.pairing)?"yes":"no"; debug_println(result_);
    uint32_t current_mA = getCurrent_milliAmps();
    uint32_t power_W = getPower_Watts();
    debug_print("current(mA):"); debug_println((int)current_mA);
    debug_print("power   (W):"); debug_println((int)power_W);
    debug_print("exponent   :"); debug_println((int)data.frame.exponent);
    debug_print("checksum   :"); debug_print((int)data.frame.checksum);
    bool csOK = checksumIsGood();
    result_ = csOK?"(pass)":"(fail)";
    debug_println(result_);
  }
};
};

#ifdef EFERGY_SAMPLE_DATA
namespace efergy {

// example data from @mr-sneezy
//                                [ byte7][ byte6][ byte5][ byte4][ byte3][ byte2][ byte1][ byte0]
//                                [  B0  ][  B1  ][  B2  ][  B3  ][  B4  ][  B5  ][  B6  ][  B7  ]
//                               b0       8      16      24      32      40      48      56     63
static const buffer64b S0(String("0000010100110100001100100110000001101010100011100000001011000101")); // 0.798kW
static const buffer64b S1(String("0000010100110100001100100110000001000001001010010000010000111001")); // 1.954kW
static const buffer64b S2(String("0000010100110100001100100110000001001010011010000000010110000010")); // 4.464kW
}
#endif
