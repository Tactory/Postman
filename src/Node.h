#pragma once

#include <stdint.h>

namespace Postman {

  struct Node {
    uint8_t tag;
    Node* next = 0;
    Node* prev = 0;
  };

};