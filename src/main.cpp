#include <stdio.h>
#include <inttypes.h>

#include "pico/stdlib.h"

#include <Postman.h>

const std::string ENDPOINT_A = "/endpoint/a";
const std::string ENDPOINT_B = "/endpoint/b";
const std::string ENDPOINT_C = "/endpoint/c";
const std::string ENDPOINT_D = "/endpoint/d";
const std::string ENDPOINT_E = "/endpoint/e";
const std::string ENDPOINT_F = "/endpoint/f";
const std::string ENDPOINT_G = "/endpoint/g";


void handler_A(){
  while (1) {
    absolute_time_t start = get_absolute_time();
    Postman::sleep(1000);
    uint32_t diff = us_to_ms(absolute_time_diff_us(start, get_absolute_time()));

    printf("Core: %i :: Endpoint B - Timeout: %lum/s\n", get_core_num(), diff);
  }
}

void handler_B(){
  while (1) {
    uint8_t signals = Postman::wait();
    printf("Core: %i :: Endpoint C - Signals: %i \n", get_core_num(), signals);
  }
}

void handler_C(){
  while (1) {
    absolute_time_t start = get_absolute_time();
    Postman::sleep(2000);
    uint32_t diff = us_to_ms(absolute_time_diff_us(start, get_absolute_time()));

    printf("Core: %i :: Endpoint D - Timeout: %lum/s\n", get_core_num(), diff);
  }
}

int is_prime(unsigned int n){
	unsigned int p;
	if (!(n & 1) || n < 2 ) return n == 2;
 
	// comparing p*p <= n can overflow 
	for (p = 3; p <= n/p; p += 2)
		if (!(n % p)) return 0;
	return 1;
}

void handler_D(){
  uint32_t primes[2];
  int p;

  while (1) {
    for (p = 5; p; p += 2) if(is_prime(p) == 1) {
      primes[get_core_num()]++;
      // Every 4096 prime numbers, notify Endpoint B
      if(!(primes[get_core_num()] & 0xFFF)) {
        printf("Core: %i :: Endpoint D - Notify Endpoint B\n", get_core_num());
        Postman::notify(ENDPOINT_B);
        Postman::yield();
      }
    }
  }
}

void handler_E(){
  uint32_t messageid = 0;

  while (1) {
    SharedConst<Postman::Message> message = Postman::fetch(ENDPOINT_F, messageid);
    if(message){
      messageid = message->id;
      uint32_t time_ms = message->getProperty<uint32_t>("time");
      std::string data = message->getProperty<std::string>("data");
      printf("Core: %i :: Endpoint E - Fetch Message time: %lu Data: %s\n", get_core_num(), time_ms, data.c_str());
    }
  }
}

void handler_F(){
  int p;

  while (1) {
    Postman::sleep(3000);

    Shared<Postman::Message> message = Postman::compose();
    uint32_t time_ms = to_ms_since_boot(get_absolute_time());
    message->setProperty("time", time_ms);
    std::string data = "Endpoint F";
    message->setProperty("data", data);

    Postman::publish(message);
    printf("Core: %i :: Endpoint F - Publish Message ID: %lu at: %lu\n", get_core_num(), message->id, time_ms);
  }
}



void app(){
  Postman::open(ENDPOINT_A, handler_A);
  Postman::open(ENDPOINT_B, handler_B);
  Postman::open(ENDPOINT_C, handler_C);
  Postman::open(ENDPOINT_D, handler_D);
  Postman::open(ENDPOINT_E, handler_E);
  Postman::open(ENDPOINT_F, handler_F);

  // Endpoint handler as lambda
  Postman::open(ENDPOINT_G, [](){
    while (1) {
      absolute_time_t start = get_absolute_time();
      Postman::sleep(5100);
      uint32_t diff = us_to_ms(absolute_time_diff_us(start, get_absolute_time()));

      printf("Core: %i :: Endpoint G - Timeout: %lum/s\n", get_core_num(), diff);
    }
  });
}


int main() {
  stdio_init_all();

  sleep_ms(10000);

  Postman::start("/app", app);
}
