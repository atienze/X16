#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include "bits.h"

// Assert that the argument is a bit of either 0 or 1
#define assert_bit(a) if ((a) != 0 && (a) != 1) { assert(false); }

// Get the nth bit
uint16_t getbit(uint16_t number, int n) {
  unsigned short mask = 0x0001;
  if ((number & (mask << n)) != 0) {
    return 1;
  } else {
    return 0;
  }
}

// Get bits that are the given number of bits wide
uint16_t getbits(uint16_t number, int n, int wide) {
  uint16_t ret_val = 0x0000;
  int ret_val_ind = 0;
  for (int i = 0; i < wide; i++) {
    if (getbit(number, n+i) == 0){
      ret_val_ind++;
    } else{
      ret_val = setbit(ret_val, ret_val_ind);
      ret_val_ind++;
    }
  }
  return ret_val;
}

// Set the nth bit to the given bit value and return the result
uint16_t setbit(uint16_t number, int n) {
    unsigned short mask = 0x0001;
    mask = (mask << n);
    number = (number | mask);
    return number;
}

// Clear the nth bit
uint16_t clearbit(uint16_t number, int n) {
    unsigned short mask = 0x0001;
    mask = (mask << n);
    number = (number & ~(mask));
    return number;
}

// Sign extend a number of the given bits to 16 bits
uint16_t sign_extend(uint16_t x, int bit_count) {
    if (getbit(x, bit_count-1) != 0){
      for (int i = 0; i < (16-bit_count); i++) {
        x = setbit(x, bit_count+i);
      }
    }
    return x;
}

bool is_positive(uint16_t number) {
    return getbit(number, 15) == 0;
}

bool is_negative(uint16_t number) {
    return getbit(number, 15) == 1;
}
