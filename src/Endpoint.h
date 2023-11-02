#pragma once

#include "pico/sem.h"

#include <string>
#include "Message.h"

namespace Postman {
  
  class Endpoint {
    
    public:
      typedef void (*Handler)();
      const static Weak<Endpoint> Empty;

      static void init();
      
      static Weak<Endpoint> create(const std::string &uri, Weak<Endpoint> owner);
      static void release(Weak<Endpoint> endpoint);

      static Shared<Endpoint> get(const std::string &uri);
      static bool isEmpty(std::weak_ptr<Endpoint> const &endpoint);

      const std::string uri;
      const Weak<Endpoint> owner;

      /**
       * Arbitrary data used by Endpoint during callbacks
      */
      void* data;

      bool signal();
      bool hasSignals();
      uint8_t getSignals();

      void publish(Shared<Message> message);
      bool peek(uint32_t since = 0);
      SharedConst<Message> pull();

      // absolute_time_t deadlock;

      const char* toString();

    protected:
      Endpoint(const std::string &uri, const Weak<Endpoint> owner);

    private:
      semaphore_t _signals;
      Shared<Message> _public;

  };

};
