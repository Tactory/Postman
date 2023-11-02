/**
 * @copyright Copyright (C) 2023 Neil Stansbury
 * All rights reserved.
 */

#include "pointers.h"

#include <string>
#include <map>


template<class T>
class PropertyDescriptor;

class PropertyType {

  protected:
    std::type_info* type;

  public:
    
    template<typename T>
    bool typeOf(){
      const std::type_info& typeInfo = *this->type;
      return typeid(T) == typeInfo;
    }

    virtual ~PropertyType() = 0;
};
inline PropertyType::~PropertyType() {}


template<class T>
class PropertyDescriptor : public PropertyType {

  private:
    T _value;

  public:

    explicit PropertyDescriptor(const T value){
      this->type = const_cast<std::type_info*>(&typeid(value));
      this->_value = value;
    }

    PropertyDescriptor(){}

    T value(){
      return this->_value;
    }
};


class PropertySet {

  protected:
    std::map<std::string, Unique<PropertyType>> _properties;

  public:

    template<typename T>
    void setProperty(std::string name, T value){
      this->_properties[name].reset(new PropertyDescriptor<T>(value));
    }

    template<typename T>
    const T getProperty(const std::string name) const {
      auto iter = this->_properties.find(name);
      if(iter != this->_properties.end()){
        if(iter->second->typeOf<T>()){
          PropertyDescriptor<T> propertyDesc = dynamic_cast<PropertyDescriptor<T>&>(*iter->second);
          return propertyDesc.value();
        }
      }
      return T();
    }

    bool hasProperty(const std::string name) const {
      auto iter = this->_properties.find(name);
      if(iter != this->_properties.end()){
        return true;
      }
      return false;
    }

    template<typename T>
    bool hasProperty(const std::string name) const {
      auto iter = this->_properties.find(name);
      if(iter != this->_properties.end()){
        if(iter->second->typeOf<T>()){
          return true;
        }
      }
      return false;
    }

    int size() const {
      return this->_properties.size();
    }

    void clear(){
      this->_properties.clear();
    }
};