#pragma once
#include <string>

struct attribute {
  std::string name;
  std::string value;
};

template <typename T1, typename T2>
attribute construct_attribute(T1 name, T2 value) {
    return attribute{std::string(1, name), std::string(1, value)};
}

template <>
attribute construct_attribute<const std::string&, const std::string&>(const std::string& name, const std::string& value) {
    return attribute{name, value};
}

template <>
attribute construct_attribute<const std::string&, char>(const std::string& name, char value) {
    return attribute{name, std::string(1, value)};
}

template <>
attribute construct_attribute<char, const std::string&>(char name, const std::string& value) {
    return attribute{std::string(1, name), value};
}