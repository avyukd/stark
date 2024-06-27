#include "utils.h"
#include <algorithm>
#include <cctype>
#include <string>

bool is_tab_lf_ff_space(char c){
  return c == '\t' || c == '\n' || c == '\f' || isspace(c);
}

bool is_upper_hex(char c){
  return (c >= 'A' && c <= 'F');
}

bool is_lower_hex(char c){
  return (c >= 'a' && c <= 'f');
}

bool is_hex(char c){
  return isdigit(c) || is_upper_hex(c) || is_lower_hex(c);
}

bool is_non_character(int character_reference_code){
  // get last 16 bits
  int bit_mask = 0xFFFF;
  int trailing_16_bits = character_reference_code & bit_mask;

  if(trailing_16_bits == 0xFFFE || trailing_16_bits == 0xFFFF){
    return true;
  } // TODO: not sure if this is accurate -- what if U+20FFFF?

  return (
    character_reference_code >= 0xFDD0 && character_reference_code <= 0xFDEF
  );
}

bool is_non_whitespace_control(char c){
  if(isspace(c)){
    return false;
  }

  return (c >= 0x7F && c <= 0x9F) || (c >= 0x0000 && c <= 0x001F);
}

int get_code_point_from_character_reference(int character_reference_code){
  const int codepoints[] = {
    0x20AC, 0x0000, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021,
    0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0x0000, 0x017D, 0x0000,
    0x0000, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
    0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0x0000, 0x017E, 0x0178
  };
  
  int idx = character_reference_code - 0x80;
  if(idx >= 0 && idx < sizeof(codepoints) / sizeof(codepoints[0])){
    return codepoints[idx];
  }else{
    return 0x0000;
  }

}

std::string upper_case(const std::string& s){
  std::string tmp = s;
  std::transform(tmp.begin(), tmp.end(), tmp.begin(), ::toupper);
  return tmp;
}

void trim(std::string& s){
  auto func = [](unsigned char ch) {
    return !is_tab_lf_ff_space(ch);
  };
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), func));
  s.erase(std::find_if(s.rbegin(), s.rend(), func).base(), s.end());
}

std::wstring codepoint_to_wstring(uint32_t codepoint) {
  if (codepoint <= 0xFFFF) {
      return std::wstring(1, static_cast<wchar_t>(codepoint));
  } else {
      // Calculate surrogate pair
      uint32_t adjusted = codepoint - 0x10000;
      wchar_t high_surrogate = static_cast<wchar_t>((adjusted >> 10) + 0xD800);
      wchar_t low_surrogate = static_cast<wchar_t>((adjusted & 0x3FF) + 0xDC00);
      return std::wstring{high_surrogate, low_surrogate};
  }
}

std::string wstring_to_utf8(const std::wstring& wstr) {
    std::string utf8;
    utf8.reserve(wstr.length() * 4); // Reserve space for worst case scenario

    for (wchar_t wc : wstr) {
        if (wc <= 0x7F) {
            utf8.push_back(static_cast<char>(wc));
        } else if (wc <= 0x7FF) {
            utf8.push_back(static_cast<char>(0xC0 | ((wc >> 6) & 0x1F)));
            utf8.push_back(static_cast<char>(0x80 | (wc & 0x3F)));
        } else if (wc <= 0xFFFF) {
            utf8.push_back(static_cast<char>(0xE0 | ((wc >> 12) & 0x0F)));
            utf8.push_back(static_cast<char>(0x80 | ((wc >> 6) & 0x3F)));
            utf8.push_back(static_cast<char>(0x80 | (wc & 0x3F)));
        } else if (wc <= 0x10FFFF) {
            utf8.push_back(static_cast<char>(0xF0 | ((wc >> 18) & 0x07)));
            utf8.push_back(static_cast<char>(0x80 | ((wc >> 12) & 0x3F)));
            utf8.push_back(static_cast<char>(0x80 | ((wc >> 6) & 0x3F)));
            utf8.push_back(static_cast<char>(0x80 | (wc & 0x3F)));
        } else {
            // Invalid Unicode code point
            utf8.push_back('?');
        }
    }

    return utf8;
}