/**
 * @copyright Copyright (C) 2023 Neil Stansbury
 * All rights reserved.
 */


#include "pico/multicore.h"

#include "Endpoint.h"
#include "defs.h"


#include <map>

namespace Postman {

  INTERNAL_NS
    std::map<std::string, Shared<Endpoint>> endpoints;
    critical_section_t crit_sec;

    const uint8_t MAX_SIGNALS = 255;
  END_INTERNAL

  const Weak<Endpoint> Endpoint::Empty = Weak<Endpoint>();

  // BEGIN STATIC
  void Endpoint::init(){
    critical_section_init(&NS::crit_sec);
  }

  Weak<Endpoint> Endpoint::create(const std::string &uri, Weak<Endpoint> owner) {
    auto endpoints = NS::endpoints.find(uri);
    if (endpoints != NS::endpoints.end()) {
      return Endpoint::Empty;
    }
    else {
      Shared<Endpoint> Ep = Shared<Endpoint>(new Endpoint(uri, owner));
      NS::endpoints.emplace(uri, Ep);
      Weak<Endpoint> endpoint = Ep;
      return endpoint;
    }
  }

  void Endpoint::release(Weak<Endpoint> target){
    Shared<Endpoint> endpoint = target.lock();
    if(endpoint){
      NS::endpoints.erase(endpoint->uri);
    }
  }

  Shared<Endpoint> Endpoint::get(const std::string &uri) {
    auto endpoints = NS::endpoints.find(uri);
    if (endpoints != NS::endpoints.end()) {
      return endpoints->second;
    }
    return nullptr;
  }

  bool Endpoint::isEmpty(std::weak_ptr<Endpoint> const &endpoint) {
    return !endpoint.owner_before(Endpoint::Empty) && !Endpoint::Empty.owner_before(endpoint);
  }

  // END STATIC

  Endpoint::Endpoint(const std::string &uri, const Weak<Endpoint> owner) : uri(uri), owner(owner){
    sem_init(&this->_signals, NS::MAX_SIGNALS, NS::MAX_SIGNALS);
  }

  const char* Endpoint::toString(){
    return this->uri.c_str();
  }

  bool Endpoint::hasSignals(){
    uint8_t signals = NS::MAX_SIGNALS - sem_available(&this->_signals);
    if(signals > 0){
      return true;
    }
    return false;
  }

  uint8_t Endpoint::getSignals(){
    uint8_t signals = NS::MAX_SIGNALS - sem_available(&this->_signals);
    if(signals > 0){
      sem_reset(&this->_signals, NS::MAX_SIGNALS);
    }
    return signals;
  }

  bool Endpoint::signal(){
    if(sem_try_acquire(&this->_signals)){
      return true;
    }
    return false;
  }

  void Endpoint::publish(Shared<Message> message){
    critical_section_enter_blocking(&NS::crit_sec);
    this->_public = message;
    critical_section_exit(&NS::crit_sec);
  }

  bool Endpoint::peek(uint32_t postid){
    if(this->_public && this->_public->id > postid){
      return true;
    }
    return false;
  }

  SharedConst<Message> Endpoint::pull(){
    return this->_public;
  }

}

