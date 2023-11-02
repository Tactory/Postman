/**
 * @copyright Copyright (C) 2023 Neil Stansbury
 * All rights reserved.
 */


#include "Message.h"
#include "Endpoint.h"
#include "Queue.h"
#include "defs.h"

namespace Postman {
  INTERNAL_NS
    uint32_t messageId = 0;
    Postman::Queue messages;

    void release(Message* message){
      message->clear();
      NS::messages.push(message);
    }
  END_INTERNAL

  void Message::init(){
    NS::messages = Queue();

    Message* MessageBank = new Message[MESSAGE_BANK_SIZE];
    for (int i = 0; i < MESSAGE_BANK_SIZE; i++) {
      NS::messages.push(&MessageBank[i]);
    }
  }

  Shared<Message> Message::create(Shared<Endpoint> origin){
    Message* free = (Message*) NS::messages.pop();
    Shared<Message> message;
    if(free){
      message = Shared<Message>(free, NS::release);
    }
    else {  // Fall back and allow shared_ptr to clean up
      message = Shared<Message>(new Message());
    }
    message->origin = origin;
    message->id = ++NS::messageId;
    return message;
  }

}