#pragma once
#include <string>

#define ASSERT_NOT_REACHED assert(false) // for unimplemented states

bool is_tab_lf_ff_space(char c);
bool is_upper_hex(char c);
bool is_lower_hex(char c);
bool is_hex(char c);
bool is_non_character(int character_reference_code); // unused
bool is_non_whitespace_control(char c); // unused
int get_code_point_from_character_reference(int character_reference_code);
std::string upper_case(const std::string& s);
void trim(std::string& s);
std::wstring codepoint_to_wstring(uint32_t codepoint);
std::string wstring_to_utf8(const std::wstring& wstr);