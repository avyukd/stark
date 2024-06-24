#pragma once
#include <string>

//TODO: the templates here suck!!

struct attribute {
  std::string name;
  std::string value;
};

// use enable if to make this work
template <typename T1, typename T2>
attribute construct_attribute(T1 name, T2 value) {
  constexpr bool is_t1_char = (std::is_same_v<T1, char>);
  constexpr bool is_t2_char = (std::is_same_v<T2, char>);
  if constexpr (is_t1_char && is_t2_char) {
    return attribute{std::string(1, name), std::string(1, value)};
  } else if constexpr (is_t1_char) {
    return attribute{std::string(1, name), value};
  } else if constexpr (is_t2_char) {
    return attribute{name, std::string(1, value)};
  } else {
    return attribute{name, value};
  }
}

// template <>
// attribute construct_attribute<const std::string&, const std::string&>(const std::string& name, const std::string& value) {
//     return attribute{name, value};
// }

// template <>
// attribute construct_attribute<const std::string&, char>(const std::string& name, char value) {
//     return attribute{name, std::string(1, value)};
// }

// template <>
// attribute construct_attribute<char, const std::string&>(char name, const std::string& value) {
//     return attribute{std::string(1, name), value};
// }

// template <>
// attribute construct_attribute<char, char>(char name, char value) {
//     return attribute{std::string(1, name), std::string(1, value)};
// }