#pragma once

#include <unity.h>

#include <string>
#include <array>


#include <Properties.h>


Unique<PropertySet> properties;

struct testsuite_properties {

  static void test_setProperty_int(void) {
    auto propertyName = std::string("IntProp");
    auto propertyValue = int(12345);

    properties->setProperty(propertyName, propertyValue);
    auto actualValue = properties->getProperty<int>(propertyName);

    TEST_ASSERT_EQUAL(propertyValue, actualValue);
    TEST_ASSERT_EQUAL(1, properties->size());
  }

  static void test_setProperty_string(void) {
    auto propertyName = std::string("StrProp");
    auto propertyValue = std::string("A string property");

    properties->setProperty(propertyName, propertyValue);
    auto actualValue = properties->getProperty<std::string>(propertyName);

    TEST_ASSERT_EQUAL_STRING(propertyValue.c_str(), actualValue.c_str());
    TEST_ASSERT_EQUAL(2, properties->size());
  }

  static void test_getProperty_string_consecutive(void) {
    auto propertyName = std::string("StrProp");
    auto propertyValue = std::string("A string property");

    auto actualValue = properties->getProperty<std::string>(propertyName);

    TEST_ASSERT_TRUE(propertyValue == actualValue);
    TEST_ASSERT_EQUAL(2, properties->size());
  }

  static void test_setProperty_string_replaced(void) {
    auto propertyName = std::string("StrProp");
    auto propertyValue = std::string("A string property replaced");

    properties->setProperty(propertyName, propertyValue);
    auto actualValue = properties->getProperty<std::string>(propertyName);

    TEST_ASSERT_TRUE(propertyValue == actualValue);
    TEST_ASSERT_EQUAL(2, properties->size());
  }

  static void test_setProperty_array(void) {
    auto propertyName = std::string("ArrProp");
    auto propertyValue = std::array<int, 5>({1,2,3,4,5});

    properties->setProperty(propertyName, propertyValue);
    auto actualValue = properties->getProperty<std::array<int, 5>>(propertyName);

    TEST_ASSERT_TRUE(propertyValue == actualValue);
    TEST_ASSERT_EQUAL(3, properties->size());
  }

  static void test_hasProperty_type_int(void) {
    auto propertyName = std::string("IntProp");

    auto actualValue = properties->hasProperty<int>(propertyName);

    TEST_ASSERT_TRUE(actualValue);
  }

  static void test_hasProperty_type_int_false(void) {
    auto propertyName = std::string("StrProp");

    auto actualValue = properties->hasProperty<int>(propertyName);

    TEST_ASSERT_FALSE(actualValue);
  }

  static void test_hasProperty_type_string(void) {
    auto propertyName = std::string("StrProp");

    auto actualValue = properties->hasProperty<std::string>(propertyName);

    TEST_ASSERT_TRUE(actualValue);
  }

  static void test_hasProperty_type_array(void) {
    auto propertyName = std::string("ArrProp");

    auto actualValue = properties->hasProperty<std::array<int, 5>>(propertyName);

    TEST_ASSERT_TRUE(actualValue);
  }

  static void test_hasProperty_name(void) {
    auto propertyName = std::string("StrProp");

    auto actualValue = properties->hasProperty(propertyName);

    TEST_ASSERT_TRUE(actualValue);
  }

  static void setup(){
    properties = Unique<PropertySet>(new PropertySet());
    UNITY_BEGIN();
  }

  static void finish(){
    UNITY_END();
    properties.release();
  }

  static void run(){
    setup();
  
    RUN_TEST(test_setProperty_int);
    RUN_TEST(test_setProperty_string);
    RUN_TEST(test_getProperty_string_consecutive);
    RUN_TEST(test_setProperty_array);

    RUN_TEST(test_hasProperty_type_int);
    RUN_TEST(test_hasProperty_type_int_false);
    RUN_TEST(test_hasProperty_type_string);
    RUN_TEST(test_hasProperty_type_array);
    RUN_TEST(test_hasProperty_name);

    finish();
  }
};
