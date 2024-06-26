#pragma once
#define SIMD_SEARCH_SIZE 16

#include <string>
#include <optional>

std::optional<size_t> simd_next_pos(const std::string&, const std::string&);  

