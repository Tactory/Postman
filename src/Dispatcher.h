#pragma once

#include "pico/stdlib.h"

namespace Postman { 

  // Forward declare
  class Worker;

  class Dispatcher {

    public:
      Dispatcher();
      
      static void init();

      const uint8_t core;
      Worker* worker();
      void begin();

    private:
      __force_inline void dispatch(Worker* worker);
      Worker* _worker;
      
  };

}
