#include "solve_utils.hpp"

#include <cstdio>
#include <exception>
#include <iostream>

unsigned int char_to_uint(char input) {
  if(input >= '0' && input <= '9')
    return input - '0';
  if(input >= 'A' && input <= 'F')
    return input - 'A' + 10;
  if(input >= 'a' && input <= 'f')
    return input - 'a' + 10;
  throw std::exception();
}

void hex_to_bytes(const char* src, unsigned char* target) {
  while(*src && src[1]) {
    *(target++) = char_to_uint(*src) * 16 + char_to_uint(src[1]);
    src += 2;
  }
}

void print_bytes_hex(const unsigned char* bytes, size_t len) {
  char target[len * 2 + 1];
  for(size_t i = 0 ; i < len ; i++) {
    snprintf(target + i * 2, 3, "%02x", bytes[i]);
  }

  std::cout << target;
}
