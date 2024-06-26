#include "simd_utils.h"
#include "arm_neon.h"
#include <vector>

#define DEFAULT_BUF_VAL 1 // 1 should never be a needle? 

std::optional<size_t> simd_next_pos(const std::string& search_str, const std::string& needles){
  std::vector<uint8x16_t> masks;
  for (char c: needles) {
    masks.push_back(vmovq_n_u8(c));
  }
  uint8x16_t bit_mask = 
    {16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1};

  char search_buf[16]; memset(search_buf, DEFAULT_BUF_VAL, 16);
  const char* search_c_str = search_str.c_str();
  memccpy(search_buf, search_c_str, 0, search_str.size());
  
  uint16x8_t search_data = vld1q_u8(reinterpret_cast<const uint8_t *>(search_buf));
  for(auto& mask: masks)
    mask = vceqq_u8(search_data, mask);
  uint8x16_t res_mask = vdupq_n_u8(0);
  for(auto& mask: masks)
    res_mask = vorrq_u8(res_mask, mask);
  uint8x16_t matches = vandq_u8(bit_mask, res_mask);
  int m = vmaxvq_u8(matches);
  if(m != 0) {
    return (16 - m);
  }
  return std::nullopt;
}