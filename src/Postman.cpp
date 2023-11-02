/**
 * @copyright Copyright (C) 2023 Neil Stansbury
 * All rights reserved.
 */


#include "Postman.h"
#include "Supervisor.h"
#include "Worker.h"
#include "Endpoint.h"



namespace Postman {

  void start(const std::string &appUri, const Endpoint::Handler &appHandler) {
    Supervisor::start(appUri, appHandler);
    printf("Postman Started\n");
  }

  bool open(const std::string &uri, const Endpoint::Handler &handler) {
    Worker* self = Supervisor::self();
    Weak<Endpoint> endpoint = Endpoint::create(uri, self->endpoint);
    if(!Endpoint::isEmpty(endpoint)){
      return Supervisor::exec(endpoint, handler);
    }
    return false;
  }

  void close(){
    Worker* self = Supervisor::self();
    self->halt();
  }

  void yield(){
    Worker::yield();
  }

  void sleep(const uint32_t duration_ms){
    Worker* self = Supervisor::self();
    self->sleep(duration_ms);
  }

  uint8_t wait(const uint32_t timeout){
    Worker* self = Supervisor::self();

    auto callback = [](Shared<Endpoint> &source, Weak<Endpoint> target) -> Postman::Result {
      if(source->hasSignals()){
        return Postman::Result::SUCCESS;
      }
      return Postman::Result::CONTINUE;
    };
    Postman::Result result = self->block(callback, Endpoint::Empty, timeout);
    // Handler resumes here
    if(result == Postman::Result::SUCCESS){
      return self->endpoint->getSignals();
    }
    return 0;
  }

  void publish(Shared<Message> message){
    Worker* self = Supervisor::self();
    Shared<Endpoint> endpoint = self->endpoint;
    endpoint->publish(message);
  }

  bool notify(const std::string target, uint32_t timeout_ms){
    Worker* self = Supervisor::self();
    Shared<Endpoint> endpoint = Endpoint::get(target);
    if(!endpoint || endpoint == self->endpoint){ // Can't block on self
      return false;
    }
    
    Weak<Endpoint> weakTarget = endpoint;
    endpoint.reset(); // Release shared pointer lock

    auto callback = [](Shared<Endpoint> &source, Weak<Endpoint> target) -> Postman::Result {
      Shared<Endpoint> endpoint = target.lock();
      if(endpoint){
        if(endpoint->signal()){
          return Postman::Result::SUCCESS;
        }
        return Postman::Result::CONTINUE;
      }
      return Postman::Result::ENDPOINT_NOT_AVAILABLE;
    };

    Postman::Result result = self->block(callback, weakTarget, timeout_ms);
    if(result == Postman::Result::SUCCESS){
      return true;
    }
    return false;
  }

  bool peek(const std::string target, uint32_t since){
    Shared<Endpoint> endpoint = Endpoint::get(target);
    if(endpoint){
      return endpoint->peek(since);
    }
    return false;
  }

  SharedConst<Message> fetch(const std::string target, uint32_t since, uint32_t timeout_ms){
    Worker* self = Supervisor::self();
    Shared<Endpoint> endpoint = Endpoint::get(target);
    if(!endpoint || endpoint == self->endpoint){  // Can't block on self
      return nullptr;
    }

    self->endpoint->data = static_cast<void*>(&since);

    Weak<Endpoint> weakTarget = endpoint;
    endpoint.reset(); // Release shared pointer lock

    auto callback = [](Shared<Endpoint> &source, Weak<Endpoint> target) -> Postman::Result {
      Shared<Endpoint> endpoint = target.lock();
      if(endpoint){
        uint32_t since = *static_cast<uint32_t*>(source->data);
        if(endpoint->peek(since)){
          return Postman::Result::SUCCESS;
        }
        return Postman::Result::CONTINUE;
      }
      return Postman::Result::ENDPOINT_NOT_AVAILABLE;
    };

    Postman::Result result = self->block(callback, weakTarget, timeout_ms);
    // Handler resumes here
    if(result == Postman::Result::SUCCESS){
      endpoint = Endpoint::get(target);
      if(endpoint){
        SharedConst<Message> message = endpoint->pull();
        return message;
      }
    }
    return nullptr;
  }

  Shared<Message> compose(){
    Worker* self = Supervisor::self();
    return Message::create(self->endpoint);
  }

}