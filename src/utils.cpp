#include "utils.h"
#include <algorithm>
#include <cctype>
#include <string>

bool is_tab_lf_ff_space(char c){
  return c == '\t' || c == '\n' || c == '\f' || isspace(c);
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