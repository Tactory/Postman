/**
 * @copyright Copyright (C) 2023 Neil Stansbury
 * All rights reserved.
 * 
 * Portions copyright (C) 2021 Keith Standiford
 * Portions copyright (C) 2021 Gary Sims
 * Portions copyright (C) 2017 Scott Nelson
 * Portions copyright (C) 2015-2018 National Cheng Kung University, Taiwan
 * Portions copyright (C) 2014-2017 Chris Stones
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Based on Piccolo OS, a simple cooperative multi-tasking OS originally written by 
 * [Gary Sims](https://github.com/garyexplains) as a teaching tool and later enhanced by
 * [Keith Standiford](https://github.com/KStandiford/Piccolo_OS_Plus) as Piccolo OS Plus
 */


#include "pico/multicore.h"
#include "hardware/exception.h"
#include "hardware/structs/systick.h"

#include "Dispatcher.h"
#include "Supervisor.h"
#include "defs.h"
#include "Worker.h"


extern "C" void __isr_SVCALL(void);
extern "C" void __init_worker_stack(uint32_t *stack);


namespace Postman { 

  INTERNAL_NS

    /*
    * Switch the Dispatcher to handler mode
    * After a reset, the processor is in thread mode. 
    * Switch to handler mode, so that the Dispatcher runs in handler mode,
    * and to ensure the correct return from an exception/interrupt later
    * when switching to a Worker. We make a dummy stack and arrange to route the
    * service call exception back to ourselves!
   */
    void initHandlerMode(void) {
      uint32_t dummy[48];
      __init_worker_stack((uint32_t *)((uint32_t)(dummy + 48) & ~((uint32_t) 7)));  // Create phony 8 byte aligned stack & SVC back to ourselves
    }

  END_INTERNAL

  Dispatcher::Dispatcher() : core(get_core_num()){}

  void Dispatcher::init(){
      // Install the exception handlers for Systick and SVC
      exception_set_exclusive_handler(SYSTICK_EXCEPTION, &__isr_SVCALL);
      exception_set_exclusive_handler(SVCALL_EXCEPTION, &__isr_SVCALL);
      // exception_set_exclusive_handler(HARDFAULT_EXCEPTION, fault_handler);
  }

  Postman::Worker* Dispatcher::worker(){
    return this->_worker;
  }

  void __time_critical_func(Dispatcher::begin)(){   // Never returns
    /*
    * set interrupt priority for SVC, PENDSV and Systick to 'all bits on'
    * for LOWEST interrupt priority. We do not want ANY of them to preempt
    * any other irq or each other. Remember that the vector table is the same 
    * for both cores, but interrupt enable and priorities ARE NOT!
    * Also, Systick is different for each core.
    */
    hw_set_bits((io_rw_32 *)(PPB_BASE + M0PLUS_SHPR2_OFFSET),M0PLUS_SHPR2_BITS);
    hw_set_bits((io_rw_32 *)(PPB_BASE + M0PLUS_SHPR3_OFFSET),M0PLUS_SHPR3_BITS);

    NS::initHandlerMode();

    Worker* worker;
    
    absolute_time_t idle_time;
    absolute_time_t worker_timeout;

    while(1){
      /*
      * We get to the top of the scheduler loop three ways...
      * 
      * 1) Initial entry. (Which can't have conflicts...)
      * 2) SVC call returned from previous Worker directly. (Handler yielded, we are in the main loop)
      * 3) Systick fired which "acts" like SVC. (Worker is preempted.)
      * 
      * SVC and Systick preemption are asynchronous and *could both* occur. We want to make sure that only
      * one happens, otherwise we could schedule twice and skip a Worker (at best). If they try to occur on top
      * of each other, SVC will alway eventually win because it had a lower exception number.
      * So it is sufficient to clear any pending systick pending flag. 
      */
      systick_hw->csr = 0;    //Disable timer and IRQ   
      __dsb();                // make sure systick is disabled
      __isb();                // and it is really off

      // clear the systick pending bit if it got set
      hw_set_bits((io_rw_32*)(PPB_BASE + M0PLUS_ICSR_OFFSET),M0PLUS_ICSR_PENDSTCLR_BITS);
      
      idle_time = DISPATCHER_MAX_IDLE_TIME;
      
      while((worker = Supervisor::next())){
        if(worker->bind()){      // Try to bind the worker to the current core
          if(!worker->isSleeping() && !worker->isBlocking()){
            this->dispatch(worker);
            // Worker suspended here
            if(worker->isZombie()){
              printf("Found zombie: %s\n", worker->endpoint->uri.c_str());
              Supervisor::halt(worker);
            }
          }

          if(worker->timeout > 0){
            worker_timeout = absolute_time_diff_us(get_absolute_time(), worker->timeout);
            if(worker_timeout < idle_time){
              idle_time = worker_timeout;
            }
          }

          worker->release();    // Release the worker from current dispatcher
        }
      }
      if(idle_time > 0){
        sleep_us(idle_time);
      }
    }

  }

  void Dispatcher::dispatch(Worker* worker){
    // NOTE: setting Time Slice to 0 will disable Systick and turn off preemptive scheduling!
    systick_hw->rvr = WORKER_TIME_SLICE; // set for interval
    systick_hw->cvr = 0;    // reset the current counter
    __dsb();                // make sure systick is set
    __isb();                // and it is really ready
    systick_hw->csr = 3;    // Enable systick timer and IRQ, select 1 usec clock

    this->_worker = worker;
    worker->run();
    this->_worker = 0;
  }

}