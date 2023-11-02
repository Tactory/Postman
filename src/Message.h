#pragma once

#include "Node.h"
#include "Properties.h"
#include "pointers.h"

namespace Postman {
  class Endpoint;
  
  class Message : public Node, public PropertySet {
    public:
      static void init();
      static Shared<Message> create(Shared<Endpoint> origin);

      Weak<Endpoint> origin;
      uint32_t id;
  };
}


