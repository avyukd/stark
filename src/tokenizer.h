#pragma once
#include <string>
#include <optional>
#include "tokenizer_state.h"
#include "token.h"
#include "named_character_ref.h"

class Tokenizer {
  public:
  Tokenizer(const std::string&);
  void run();

  private:
  using TokenizerState = TokenizerNamespace::TokenizerState; 

  bool advance();
  std::optional<size_t> simd_next_pos();
  bool in_simd_state();
  void set_return_state(TokenizerState);
  bool is_appropriate_end_tag_token();
  bool is_attribute_return_state();

  std::string m_contents;
  size_t m_cursor;
  std::vector<Token> m_tokens;
  std::string m_tmp_buffer;
  Token m_tmp_token;

  TokenizerState m_state;
  TokenizerState m_return_state;
  NamedCharacterRef m_named_char_ref;
};