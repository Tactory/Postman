/**
 * @copyright Copyright (C) 2023 Neil Stansbury
 * All rights reserved.
 */


#include "pico/multicore.h"

#include "Postman.h"
#include "Supervisor.h"
#include "Dispatcher.h"
#include "Queue.h"
#include "Worker.h"
#include "defs.h"


namespace Postman { 
namespace Supervisor {

  INTERNAL_NS

    Dispatcher* dispatcher[2];

    Postman::Queue free;
    Postman::Queue ready;
    Postman::Queue zombies;
    Shared<Endpoint> gc_endpoint;

    /**
     * This Worker is created during the Supervisor start-up on core 0.
     * It deletes any dead Workers on the zombies list. 
    */
    void garbage_collector() {
      Postman::Worker* zombie;
      uint8_t signals = 0;

      while (1) {
        signals = Postman::wait();
        printf("Core: %i :: GC Signals: %i  Zombies: %i\n", get_core_num(), signals, NS::zombies.length());
        while ((zombie = (Worker*) NS::zombies.next())) {
          NS::zombies.remove(zombie);
          zombie->endpoint = nullptr;
          Endpoint::release(zombie->endpoint);
          NS::free.push(zombie);
        }
      }
    }

  END_INTERNAL

  bool exec(Weak<Endpoint> endpoint, const Endpoint::Handler &handler){
    /**
     * Safe to call from handler...
     *  Queue::push(worker) and Queue::pop() on a queue aquires
     *  a blocking spinlock which disables interrupts
     * However... the other dispatcher may busywait on the lock
    */
    Shared<Endpoint> target = endpoint.lock();
    Postman::Worker* worker = (Worker*) NS::free.pop();
    if(target && worker){
      worker->assign(target, handler);
      NS::ready.push(worker);
      return true;
    }
    return false;
  }

  void halt(Worker* worker){
    NS::ready.remove(worker);
    NS::zombies.push(worker);
    /*
      We could suspend() the GC on creation
      then Worker->resume() rather than waiting on a signal
    */
    NS::gc_endpoint->signal();
  }

  Worker* self(){
    /**
     * Safe to call from handler...
     *  Ensure we're not preempted and switched to a different core during lookup
    */
    int interrupts = save_and_disable_interrupts();

    __compiler_memory_barrier();
    Dispatcher* dispatcher = NS::dispatcher[get_core_num()];

    __compiler_memory_barrier();
    Postman::Worker* worker = dispatcher->worker();

    __compiler_memory_barrier();
    restore_interrupts(interrupts);

    return worker;
  }

  Dispatcher* dispatcher(){
    return NS::dispatcher[get_core_num()];
  }

  Postman::Worker* next(){
    /**
     * Each queue "cycles"
     * On each call to next() the next node is tagged with the current cycle
     * When the queue reaches a node matching the current tag it restarts the cycle and returns null
     * The dispatcher that ends the cycle may idle
     * This allows nodes to be added or removed without affecting the current processing loop
    */
    return (Worker*) NS::ready.next();

    /**
     * Todo ...
     * Remove free/ready/zombie queues, link all Workers up front, keeping locality
     *  Don't remove, so no node->prev
     *  NS:current is always last Worker returned to either dispatcher
     
      lock()
      if(NS::current && NS::current->next){
        next = NS::current->next;
      }
      else {
        next = NS::head;
      }

      if(next && next != NS::start){
        NS::current = current = next;
      }
      else {
        //  Next node matches start, start new cycle
        //  Set current = next to so next cycle starts after last node
        NS::current = NS::start = next;
      }
      // Update Worker/Cycle run-time stats
      unlock()
      return current;
      
    */
  }

  void launch() {
    NS::dispatcher[get_core_num()]->begin();
  }

  void start(const std::string &appUri, const Endpoint::Handler &appHandler){
    if(get_core_num() != 0){
      return;
    }

    Queue::init();
    NS::free = Queue();
    NS::ready = Queue();
    NS::zombies = Queue();

    Worker* WorkerPool = new Worker[WORKER_POOL_SIZE];
    for (int i = 0; i < WORKER_POOL_SIZE; i++) {
      NS::free.push(&WorkerPool[i]);
    }

    Endpoint::init();
    Message::init();

    // Create & add the GC endpoint
    Weak<Endpoint> gc = Endpoint::create("/postman/gc", Endpoint::Empty);
    NS::gc_endpoint = gc.lock();
    Supervisor::exec(gc, NS::garbage_collector);

    // Create & add the main app endpoint
    Weak<Endpoint> app = Endpoint::create(appUri, Endpoint::Empty);
    Supervisor::exec(app, appHandler);  

    Dispatcher::init();

    NS::dispatcher[0] = new Dispatcher();

    if(DISPATCHER_MULTICORE){
      NS::dispatcher[1] = new Dispatcher();
      multicore_launch_core1(Supervisor::launch);
    }
    Supervisor::launch();
  }

}}