#pragma once
/**
 * @copyright Copyright (C) 2023 Neil Stansbury
 * All rights reserved.
 */

#include "pico/multicore.h"
#include "Node.h"
#include "defs.h"

namespace Postman {

  INTERNAL_NS
    critical_section_t crit_sec;
  END_INTERNAL

  class Queue {
    private:
      Node* _head = 0;
      Node* _tail = 0;
      volatile Node* _current = 0;
      volatile uint32_t _length = 0;

      uint8_t _tag = 0; // cycle id

      void lock(){
        critical_section_enter_blocking(&NS::crit_sec);
      }

      void unlock() {
        critical_section_exit(&NS::crit_sec);
      }

    public:

      static void init(){
        critical_section_init(&NS::crit_sec);
      };

      Node* next(){
        Node* current = 0;

        this->lock();

        Node* next;
        if(this->_current && this->_current->next){
          next = this->_current->next;
        }
        else {
          next = this->_head;
        }

        if(next && next->tag != this->_tag){
          next->tag = this->_tag;
          this->_current = current = next;
        }
        else {
          /**
           * Next node tag matches, increment tag and start new cycle
           * Wrapping uint8_t to 0 at end
           * Set current = next to so next cycle starts after last node
          */
          this->_current = next;
          this->_tag = (this->_tag +1) & 0xff;
        }

        this->unlock();
        return current;

      };

      void push(Node* node){
        if(!node) return;

        node->next = 0;
        node->prev = 0;
        node->tag = 0;

        this->lock();

        if(this->_head){
          this->_tail->next = node;
          node->prev = this->_tail;
          this->_tail = node;
        }
        else {  // First node
          this->_head = node;
          this->_tail = node;
        }
        this->_length += 1;

        this->unlock();
      };

      void remove(Node* node){
        if(!node->next && !node->prev){
          return;
        }

        this->lock();

        if(node->prev){
          node->prev->next = node->next;
        }
        else {
          this->_head = node->next;
        }

        if(node->next){
          node->next->prev = node->prev;
        }
        else {
          this->_tail = node->prev;
        }

        if(this->_current == node){   // Try and roll current back to previous node
          if(node->prev){
            this->_current = node->prev;
          }
          else {
            this->_current = this->_tail;
          }
        }

        node->next = 0;
        node->prev = 0;
        this->_length -= 1;

        this->unlock();
      };

      Node* pop(){
        Node* node = 0;

        this->lock();

        if((node = this->_head)){
          this->_head = node->next;
          if(node->next){
            node->next->prev = 0;
          }
          else {  // No next, Node must be tail
            this->_tail = this->_head;
          }
          if(this->_current == node){
            this->_current = this->_tail; // Roll back
          }

          node->next = 0;
          node->prev = 0;
          this->_length -= 1;
        }

        this->unlock();
        return node;
      }

      void insert(Node* node, Node* before){
        node->next = 0;
        node->prev = 0;
        node->tag = 0;

        this->lock();

        node->next = before;
        if(before->prev){
          before->prev->next = node;
          node->prev = before->prev;
          before->prev = node;
        }
        else {
          before->prev = node;
          this->_head = node;
        }
        this->_length += 1;

        this->unlock();
      };

      int length() const {
        return this->_length;
      };
  };

};