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



#include "pico/stdlib.h"


#include "Worker.h"
#include "defs.h"
#include "Endpoint.h"
#include "Dispatcher.h"
#include "Supervisor.h"


extern "C" uint32_t *__prefetch_switch(uint32_t *stack);


namespace Postman {

  enum WorkerState : uint16_t {
    READY      = 0x0,              //                                         00000000
    RUNNING    = 0x1,              // Worker is running                       00000001
    SLEEPING   = 0x2,              // Worker has a timeout running            00000010
    ZOMBIE     = 0x4,              // Worker has ended and will be halt()ed   00000100
    BLOCKED    = 0x8,              // Worker blocked                          00001000
    BLOCKED_TIMEOUT = 0x10,        // Worker blocked with timeout             00010000
    SUSPENDED  = 0x20,             // Worker suspended indefinitely           00100000
  };
  // NB. A blocked signal can also be sleeping with a timeout

  INTERNAL_NS

      /**
     * @brief Initialize user stack for execution 
     * 
     * @param stack pointer to the END of the stack (array).
     * @param handler the function to execute.
     * @param arg unsigned integer argument for function
     * 
     * @returns "Current" stack pointer for the Worker.
     * We setup the initial stack frame with:
     *  - "software saved" LR set to RETURN_THREAD_PSP so that exception return
     * works correctly. 
     *  - PC Set to the task starting point (function entry)
     *  - Exception frame LR set to `destructor` in case the task exits or returns
     * 
     * RETURN_THREAD_PSP means: 
     *  - Return to Thread mode.
     *  - Exception return gets state from the process stack.
     *  - Execution uses PSP after return.
     * See also:
     * http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0552a/Babefdjc.html
     * 
     * \note The starting argument is placed in R0, but this is only used internally for the totally fake idle runnable.
     */
    uint32_t *initStackFrame(uint32_t *stack, void (*handler)(void), uint32_t arg, void (*destructor)(void)) {

      /* 
        This stack frame needs to mimic would be saved by hardware and by the software (in isr_svcall)
      
        Exception frame saved by the hardware onto task_stack:
        +------+
        | xPSR | stack[16] 0x01000000 i.e. PSR Thumb bit
        |  PC  | stack[15] pointer to handler function
        |  LR  | stack[14] destructor
        |  R12 | stack[13]
        |  R3  | stack[12]
        |  R2  | stack[11]
        |  R1  | stack[10]
        |  R0  | stack[9] handler argument
        +------+
        Registers saved by the software (isr_svcall):
        +------+
        |  LR  | stack[8]	(THREAD_PSP i.e. 0xFFFFFFFD)
        |  R7  | stack[7]
        |  R6  | stack[6]
        |  R5  | stack[5]
        |  R4  | stack[4]
        |  R11 | stack[3]
        |  R10 | stack[2]
        |  R9  | stack[1]
        |  R8  | stack[0]
        +------+
      */
      stack = (uint32_t*) (((uint32_t) stack)&~((uint32_t) 7));    // 8 byte align initial stack frame...
      stack +=  - 17; /* End of task_stack, minus what we are about to push */
      stack[8] = (unsigned int) RETURN_THREAD_PSP;
      stack[15] = (unsigned int) handler;
      stack[14] = (unsigned int) (uint32_t) destructor; // in case the handler returns!
      stack[16] = (unsigned int) 0x01000000; /* PSR Thumb bit */
      stack[9] = arg;    // in R0 (for idle, really...)

      return stack;
    }

    
  END_INTERNAL
  
  
  Worker::Worker(){
    sem_init(&this->_binding, 1, 1);
  }

  // Worker::yield() definined inline in header

  void Worker::oncomplete(){
    Postman::Worker* worker = Supervisor::self();
    worker->halt();
  }

  void Worker::assign(Shared<Endpoint> endpoint, const Endpoint::Handler &handler, const uint32_t args){
    this->stack_ptr = NS::initStackFrame(this->stack + WORKER_STACK_SIZE, handler, args, &Worker::oncomplete);
    this->endpoint = endpoint;
    this->state = WorkerState::READY;
  }

  bool Worker::bind(bool blocking){
    if(blocking){
      sem_acquire_blocking(&this->_binding);
    }
    else if(!sem_try_acquire(&this->_binding)){
      return false;
    }
    this->_core = get_core_num();
    return true;
  }

  bool Worker::release(){
    if(this->_core == get_core_num()){
      this->_core = CORE_NONE;
      sem_release(&this->_binding);
      return true;
    }
    return false;
  }

  bool Worker::hasState(uint16_t state){
    return (this->state & state) == state;
  }

  void Worker::setState(uint16_t state){
    this->state |= state;
  }

  void Worker::clearState(uint16_t state){
    this->state &= ~state;
  }

  void Worker::clearTimeout(){
    this->timeout = 0;
    clearState(WorkerState::SLEEPING);
  }

  bool Worker::isSleeping(){
    int64_t time_to_wait;

    if(hasState(WorkerState::SLEEPING)){
      time_to_wait = absolute_time_diff_us(get_absolute_time(), this->timeout);
      if(time_to_wait > 0) {
        this->timeout = make_timeout_time_us(time_to_wait);
        return true;
      }
      clearTimeout();
    }
    return false;
  }

  bool Worker::isBlocking(){
    if(!hasState(WorkerState::BLOCKED)){ // fast fail
      return false;
    }

    bool blocking = true;

    this->_blockingResult = this->_blockingCallback(this->endpoint, this->_blockingTarget);
    
    if(this->_blockingResult != Postman::Result::CONTINUE){
      blocking = false;
    }

    if(hasState(WorkerState::BLOCKED_TIMEOUT)){
      if(!blocking)   {               // No longer blocked so clear any timeout
        clearTimeout();
      }
      else if(!this->isSleeping()){   // If we're not still sleeping then clear
        clearState(WorkerState::BLOCKED_TIMEOUT);
        this->_blockingResult = Postman::Result::TIMEOUT;
        blocking = false;
      }
    }

    if(!blocking){
      this->_blockingCallback = nullptr;
      this->_blockingTarget = Endpoint::Empty;
      clearState(WorkerState::BLOCKED);
    }
    return blocking;
  }

  bool Worker::isSuspended(){
    return hasState(WorkerState::SUSPENDED);
  }

  bool Worker::isZombie(){
    return hasState(WorkerState::ZOMBIE);
  }

  bool Worker::isRunning(){
    return hasState(WorkerState::RUNNING);
  }

  void Worker::sleep(const uint32_t duration_ms, bool blocking){
    absolute_time_t timeout = make_timeout_time_ms(duration_ms);
    if(!time_reached(timeout)){
      int interrupts = save_and_disable_interrupts();
        this->timeout = timeout;
        setState(WorkerState::SLEEPING);
        if(blocking){
          setState(WorkerState::BLOCKED_TIMEOUT);
        }
        __compiler_memory_barrier();
      restore_interrupts(interrupts);
      Worker::yield();
      // Worker resumes here
    }
  }

  Postman::Result Worker::block(const BlockingCallback &condition, Weak<Endpoint> target, const uint32_t timeout_ms){

    Postman::Result result = condition(this->endpoint, target);
    if(result != Postman::Result::CONTINUE){
      return result;
    }

    // Need to check target Endpoint deadlock

    int interrupts = save_and_disable_interrupts();
      this->_blockingCallback = condition;
      this->_blockingTarget = target;
      setState(WorkerState::BLOCKED);
      __compiler_memory_barrier();
    restore_interrupts(interrupts);
    if(timeout_ms > 0){
      this->sleep(timeout_ms, true);
    }
    else {
      Worker::yield();
    }
    // Worker resumes here
    return this->_blockingResult;
    
  }

  void Worker::suspend(){
    if(this->_core == get_core_num()){
      this->setState(WorkerState::SUSPENDED);
      Worker::yield();
    }
  }

  void Worker::resume(){
    if(this->bind(true)){
      this->clearState(WorkerState::SUSPENDED);
      this->release();
    }
  }

  void Worker::run(){
    if(this->_core != get_core_num()){
      return;   // Can't run a worker not bound to current core
    }
    setState(WorkerState::RUNNING);
    this->stack_ptr = __prefetch_switch(this->stack_ptr);
    // Worker suspended or yields here
    clearState(WorkerState::RUNNING);
  }

  void Worker::halt(){
    setState(WorkerState::ZOMBIE);
    while(1) Worker::yield();
  }


}