# Postman ::  Pico OS Task Manager

**Postman** is a multi-core, pre-emptive multi-tasking microkernel for the dual core Arm Cortex-M0+ Raspberry Pi Pico microcontroller.

It is based on and inspired by, the fascinating work of [Keith Standiford](https://github.com/KStandiford) and [Piccolo OS Plus](https://github.com/KStandiford/Piccolo_OS_Plus) which in turn was based on work by [Garry Sims](https://github.com/garyexplains) and his [Piccolo OS v1.1](https://github.com/garyexplains/piccolo_os_v1.1).

The primary goal of **Postman** is to facilitate fast and efficient message passing between cores, and as such is a complete re-write of the original "Piccolo OS" C code into a C++ implementation with a new architecture, some significant new features, and performance improvements.

## Core Architecture

**Postman** does not implement the idea of processing "*tasks*", but instead splits the concept of "*work-to-do*" into two separate layers: "***Endpoints***" and "***Workers***".

A **Postman::Endpoint** is a *source* and/or *target* of messages in the system, every ***Endpoint*** is referenced by a unique URI.  An ***Endpoint*** may send or receive messages to/from other endpoints by using their respective unique URIs.

A **Postman::Worker** is dynamically assigned an ***Endpoint*** from a pre-allocated "*Worker pool*" to act on behalf of, and process the ***Endpoint*** requests.  ***Workers*** are managed and scheduled by the **Postman::Supervisor**.

The ***Supervisor*** also allocates a dedicated **Postman::Dispatcher** to each core.  When its core is available, one of the ***Dispatcher***s requests a scheduled ***Worker*** from the ***Supervisor*** and attempts to dispatch it on the core it controls.  The dispatched ***Worker*** then executes on behalf of the ***Endpoint*** it has been assigned.

An ***Endpoint*** can be notified by another endpoint to create a simple signalling system, and an ***Endpoint*** may sleep until it receives one.

Additionally an ***Endpoint*** may publish a shared **Postman::Message**.  A **Message** is retrieved from a pre-allocated "message bank", composed by the source endpoint and then published under its URI.  The published **Message** may contain an arbitrary number of property fields and values.  The messages are thread-safe and can be read by multiple endpoints simultaneously.  When all locks on the shared message are released it is automatically returned to the message bank for re-use.

The **Postman::** API is designed so that a consumer need not care about the above implementation details, they need only open a new ***Endpoint*** and pass in an **Endpoint::Handler** as a callback for a **Worker** to ultimately execute.


## Getting started


**Postman** core is allocated a root "app" ***Endpoint*** to start, this is just like any other endpoint, except it becomes the owner of all other application endpoints in the system.

    #include  <Postman.h>
    
    // Root app handler
    void  app(){
      while(1){
        // App Handler code
       }
    }
    
    int  main() {
      stdio_init_all();
      Postman::start("/myapp", app);
    }
The root app endpoint can then create any other endpoints as required, who in turn can create more endpoints themselves.  When an ***Endpoint*** handler returns, the underlying ***Endpoint*** is closed and its Worker returned to the pool for re-use.
##
### Opening an Endpoint
Additional endpoints are opened similarly by calling **Postman::open( ... )** and passing in it's unique URI and an ***Endpoint::Handler*** as a callback
```
    #include  <Postman.h>
    
    const std::string URI_ENDPOINT_A = "/endpoint/a";
    
    // Endpoint A handler
    void handler_A(){
      while(1){
        // Endpoint A Handler code
       }
    }
    
    // Root app handler
    void  app(){
      Postman::open(URI_ENDPOINT_A, handler_A);
      
      while(1){
        // App Handler code
       }
    }
    
    int  main() {
      stdio_init_all();
      Postman::start("/myapp", app);
    }
```
An endpoint can be closed explicitly with **Postman::close()** or by allowing the handler to return.

An ***Endpoint::Handler*** can also be passed as a non-capturing Lambda function:
```
    Postman::open(URI_ENDPOINT_A, [](){
      while(1){
        // Endpoint Handler code
      }
    });
```

##
### Postman::yield() & Postman::sleep( ... )
An endpoint can either yield or sleep on a timeout.  The underlying ***Worker*** will release the core to its ***Dispatcher*** and the ***Supervisor*** will reschedule the ***Worker*** for a future cycle.
```
const std::string URI_ENDPOINT_A = "/endpoint/a";

// Endpoint A handler
void handler_A(){
  while(1){
    Postman::yield();
    // The handler will resume here on the next cycle
   }
}
```
```
const std::string URI_ENDPOINT_A = "/endpoint/a";

// Endpoint A handler
void handler_A(){
  while(1){
    Postman::sleep(timeout_ms);
    // The handler will resume here after timeout expires
   }
}
```
##
### Postman::notify( ... ) & Postman::wait( ... )
An endpoint can be notified by another endpoint.  An endpoint may sleep until it receives one or more notifications, optionally with a timeout:
```
const std::string URI_ENDPOINT_A = "/endpoint/a";

// Endpoint A handler
void handler_A(){
  while(1){
    uint8_t signals = Postman::wait(timeout_ms);
    // The handler will resume here when a signal is received or the timeout expires
   }
}
```
A "target" endpoint can be notified by passing in the URI of the target it wishes to notify, optionally with a timeout. 
```
const std::string URI_ENDPOINT_A = "/endpoint/a";

// Endpoint A handler
void handler_A(){
  while(1){
    bool success = Postman::notify("/endpoint/b", timeout_ms);
    // The handler will resume here when the target is notified or the timeout expires
   }
}
```
It is not possible to determine which source ***Endpoint*** notified the target, only how many signals the target received.  For that, you need messages:

##
### Postman::compose( ... ) & Postman::publish( ... ) & Postman::fetch( ... )
An ***Endpoint*** can publish a shared message that is available for any other endpoint/s to read:
```
const std::string URI_ENDPOINT_A = "/endpoint/a";

// Endpoint A handler
void handler_A(){
  while(1){
    std::shared_ptr<Postman::Message> message = Postman::compose();
    std::string some_value = "Some Value";
    message->setProperty("some_data_key", some_value);
    Postman::publish(message);
    
    // Now sleep for 3s
    Postman::sleep(3000);
   }
}
```
Other endpoints can block until the target endpoint publishes a new message, optionally with a timeout:
```
const std::string URI_ENDPOINT_A = "/endpoint/a";

// Endpoint A handler
void handler_A(){
  while(1){
    // Wait here until target endpoint publishes a message
    shared_ptr<const Postman::Message> message = Postman::fetch("/endpoint/b");
    if(message && message.hasProperty<std::string>("some_data_key")){
      std::string target_value = message->getProperty<std::string>("some_data_key");
    }
   }
}
```
An arbitrary number of properties with arbitrary types can be set on a ***Message***.  The underlying message is automatically released and returned to the message pool when no endpoint holds it. An endpoint is free to publish a new message whilst other endpoints might be reading and holding locks on the current one. However, the time spent holding a lock on a shared message should be minimised to allow it to be rapidly reused.

### Postman::post( ... ) & Postman::read( ... )
Not yet implemented

See `Postman.h` for further interface options.
