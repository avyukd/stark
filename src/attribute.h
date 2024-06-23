#pragma once
#include <string>

struct attribute {
  std::string name;
  std::string value;
};

attribute construct_attribute(const std::string& name, const std::string& value){
  return attribute{name, value};
}