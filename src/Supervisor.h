#pragma once

#include "Endpoint.h"


namespace Postman { 
  
  // Forward declare
  class Worker;
  class Dispatcher;

  namespace Supervisor {

    void start(const std::string &appUri, const Endpoint::Handler &appHandler);

    bool exec(Weak<Endpoint> endpoint, const Endpoint::Handler &handler);
    void halt(Worker* worker);

    Worker* self();
    Dispatcher* dispatcher();

    Worker* next();

}};
