#pragma once
#include <string>

#define ASSERT_NOT_REACHED assert(false) // for unimplemented states

bool is_tab_lf_ff_space(char c);
std::string upper_case(const std::string& s);
void trim(std::string& s);