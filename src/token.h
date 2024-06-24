#pragma once
#include <string>
#include <vector>
#include "attribute.h"

struct Token {
  enum class TokenType {
    NOT_SET,
    DOCTYPE,
    START_TAG,
    END_TAG,
    COMMENT,
    CHARACTER,
    END_OF_FILE
  };

  TokenType token_type;

  struct {
    std::string name;
    std::string public_identifier;
    std::string system_identifier;
    bool force_quirks{false};
  } m_doctype;

  struct {
    std::string tag_name;
    bool self_closing{false};
    std::vector<attribute> attributes;
  } m_start_or_end_tag;

  struct {
    std::string data;
  } m_comment_or_character;
};

void reset_token(Token& token);
Token construct_character_token(char);
Token construct_character_token(const std::string&);
std::vector<Token> construct_character_tokens(const std::string&);
Token construct_comment_token(const std::string&);
Token construct_start_tag_token(const std::string&);
Token construct_end_tag_token(const std::string&);
template <typename name_t, typename value_t>
void append_attribute_to_tag_token(Token&, const name_t&, const value_t&);
template <typename string_like_t>
void append_to_tag_token_attribute_name(Token&, const string_like_t&);
template <typename string_like_t>
void append_to_tag_token_attribute_value(Token&, const string_like_t&);
template <typename string_like_t>
void append_to_tag_token_tag_name(Token&, const string_like_t&);
template <typename string_like_t>
void append_to_comment_or_character_data(Token&, const string_like_t&);
void set_tag_self_closing_flag(Token&);
template <typename string_like_t>
void append_to_doctype_token_name(Token&, string_like_t);
template <typename string_like_t>
void append_to_doctype_token_public_identifier(Token&, const string_like_t&);
template <typename string_like_t>
void append_to_doctype_token_system_identifier(Token&, const string_like_t&);
void set_force_quirks(Token&);