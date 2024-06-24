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