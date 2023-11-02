#pragma once

#include "pico/sem.h"

#include "defs.h"
#include "Node.h"
#include "Endpoint.h"

namespace Postman {

  const uint8_t CORE_NONE = 2;


  class Worker : public Node {                                            // Pointer initialised stack

    public:
      typedef Postman::Result (*BlockingCallback)(Shared<Endpoint> &source, Weak<Endpoint> target);

      Shared<Endpoint> endpoint;

      /**
       * Absolute timeout timestamp
      */
      absolute_time_t timeout = 0;

      Worker();

      __force_inline static void yield(void){
        __asm volatile ("nop" );
        __asm volatile ("svc 0");
        __asm volatile ("nop" );
        return;
      }

      void assign(Shared<Endpoint> endpoint, const Endpoint::Handler &handler, const uint32_t args = 0);

      bool bind(bool blocking = false); // Bind this worker to the current core
      bool release();

      bool isRunning();
      bool isBlocking();
      bool isSleeping();
      bool isSuspended();
      bool isZombie();

      void sleep(const uint32_t duration_ms, bool blocking = false);
      Postman::Result block(const BlockingCallback &condition, Weak<Endpoint> target, const uint32_t timeout_ms = 0);
      
      void suspend();   // Suspend this Worker until resume()d
      void resume();

      void run();
      void halt();
      
    private:
      volatile uint8_t _core = CORE_NONE;
      
      volatile uint16_t state = 0;
      uint32_t __attribute__((aligned(8))) stack[WORKER_STACK_SIZE];        // Worker stack
      uint32_t* stack_ptr = 0; 

      semaphore_t _binding;
      
      Weak<Endpoint> _blockingTarget;
      BlockingCallback _blockingCallback;
      Postman::Result _blockingResult;

      static void oncomplete();

      bool hasState(uint16_t state);
      void setState(uint16_t state);
      void clearState(uint16_t state);

      void clearTimeout(); 

  };
}