#pragma once

#include <string>
#include <Uri.h>

#include "Endpoint.h"
#include "Message.h"
#include "defs.h"
#include "pointers.h"

namespace Postman {
  /**
   * Start Postman with root Endpoint URI and handler
  */
  void start(const std::string &appUri, const Endpoint::Handler &handler);

  /**
   * Open new Endpoint URI with handler. Returns success
   * Handler only
  */
  bool open(const std::string &uri, const Endpoint::Handler &handler);

  /**
   * Close the current Endpoint and free the underlying Worker
   * Handler only
  */
  void close();

  /**
   * Yield the current handler until the next cycle
   * Handler only
  */
  void yield();

  /**
   * Yield the current handler for the duration in ms
   * Handler only.
  */
  void sleep(const uint32_t duration_ms);

  /**
   * Post Message to target Endpoint.
   * Handler only. Will block until success or timeout
  */
  bool post(SharedConst<Message> message, const std::string target, const uint32_t duration_ms);

  /**
   * Read Message post()ed by a source Endpoint
   * Handler only. Will block until success or timeout
  */
  SharedConst<Message> read(uint32_t timeout_ms);
  
  /**
   * Publish a shared Message against the current Endpoint. Can be fetch()ed by another Endpoint
   * Handler only. Will not block
  */
  void publish(Shared<Message> message);

  /**
   * Fetch public Message by target Endpoint newer than since with timeout
   * Handler only. Will block until success or timeout
  */
  SharedConst<Message> fetch(const std::string target, uint32_t since = 0, uint32_t timeout_ms = 0);

  /**
   * Get property last published by target with timeout
   * Handler only. Will block until success or timeout
  */
  template<typename T>
  const T get(const std::string target, const std::string resource, const std::string query = "", uint32_t timeout_ms = 0) {
    SharedConst<Message> message = Postman::fetch(target, 0, timeout_ms);
    if(message && message->hasProperty<T>(resource)){
      return message->getProperty<T>(resource);
    }
    return T();
  }

  /**
   * Expecting URI format: "/endpoint/path/resource?query=xxx"
  */
  template<typename T>
  const T get(const std::string uriString, uint32_t timeout_ms = 0) {
    uriparser::Uri uri(uriString);

    std::string resource = uri.resource();
    uint8_t len = uri.path().size() - resource.size();
    std::string target = uri.path().substr(0, len -1);  // -1 for trailing '/'

    return get<T>(target, resource, uri.query(), timeout_ms);
  }

  /**
   * Peek for new data post()ed by target Endpoint. Since last value
   * Handler only. Will not block
  */
  bool peek(const std::string target, uint32_t since = 0);

  /**
   * Notify target Endpoint. Endpoints wait()ing will be resumed
   * Handler only. Will block until success or timeout
  */
  bool notify(const std::string target, uint32_t timeout_ms = 0);

  /**
   * Wait until current Endpoint recieves at least 1 signal, return the total num signals and reset
   * Handler only. Will block until at least 1 signal or timeout
  */
  uint8_t wait(const uint32_t timeout = 0);

  /**
   * Compose new shared Message with current Endpoint as the origin.  Allocates Message from MessageBank
   * and returns to the bank when it goes out of scope in all consumers.
   * Will not block
  */
  Shared<Message> compose();
  
}
