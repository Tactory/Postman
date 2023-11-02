
#include "./tests/testsuite_properties.cpp"


int run_testsuites(void) {
  testsuite_properties::run();
  
  return 0;
}

void setUp(void) {}
void tearDown(void) {}


int main(int argc, char **argv) {
  run_testsuites();
  return 0;
}