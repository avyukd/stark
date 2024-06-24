#pragma once 
#include <unordered_map>
#include <fstream>
#include <string>
#include <optional>

#include "json.hpp"

using json = nlohmann::json;

static const std::string file_name = "named_characters.json";

class NamedCharacterRef {
  public:
  NamedCharacterRef(){
    std::ifstream file(file_name);
    json j; j << file;
    for (auto& [key, value] : j.items()) {
      m_named_char_to_unicode_chars[key] = value["characters"];
    }
  }

  std::optional<std::string> find(const std::string& key) {
    auto it = m_named_char_to_unicode_chars.find(key);
    if (it != m_named_char_to_unicode_chars.end()) {
      return it->second;
    }
    return std::nullopt;
  }

  private:
  std::unordered_map<std::string, std::string> m_named_char_to_unicode_chars;
};