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

void reset_token(Token& token) {
  memset(&token, 0, sizeof(Token));
}

Token construct_character_token(char data){
  Token token;
  token.token_type = Token::TokenType::CHARACTER;
  token.m_comment_or_character.data = std::string(1, data);
  return token;
}

Token construct_character_token(const std::string& data){
  Token token;
  token.token_type = Token::TokenType::CHARACTER;
  token.m_comment_or_character.data = data;
  return token;
}

std::vector<Token> construct_character_tokens(const std::string& data){
  std::vector<Token> tokens; tokens.reserve(data.size());
  for (char c : data) {
    tokens.push_back(construct_character_token(c));
  }
  return tokens;
}

Token construct_comment_token(const std::string& data){
  Token token;
  token.token_type = Token::TokenType::COMMENT;
  token.m_comment_or_character.data = data;
  return token;
}

Token construct_start_tag_token(const std::string& tag_name){
  Token token;
  token.token_type = Token::TokenType::START_TAG;
  token.m_start_or_end_tag.tag_name = tag_name;
  return token;
}

Token construct_end_tag_token(const std::string& tag_name){
  Token token;
  token.token_type = Token::TokenType::END_TAG;
  token.m_start_or_end_tag.tag_name = tag_name;
  return token;
}

template <typename name_t, typename value_t>
void append_attribute_to_tag_token(Token& token, const name_t& name, const value_t& value){
  token.m_start_or_end_tag.attributes.push_back(construct_attribute(name, value));
}

template <typename string_like_t>
void append_to_tag_token_attribute_name(Token& token, const string_like_t& data){
  token.m_start_or_end_tag.attributes.back().name += data;
}

template <typename string_like_t>
void append_to_tag_token_attribute_value(Token& token, const string_like_t& data){
  token.m_start_or_end_tag.attributes.back().value += data;
}

template <typename string_like_t>
void append_to_tag_token_tag_name(Token& token, const string_like_t& data){
  token.m_start_or_end_tag.tag_name += data;
}

template <typename string_like_t>
void append_to_comment_or_character_data(Token& token, const string_like_t& data){
  token.m_comment_or_character.data += data;
}

void set_tag_self_closing_flag(Token& token){
  token.m_start_or_end_tag.self_closing = true;
}

template <typename string_like_t>
void append_to_doctype_token_name(Token& token, const string_like_t& data){
  token.m_doctype.name += data;
}

template <typename string_like_t>
void append_to_doctype_token_public_identifier(Token& token, const string_like_t& data) {
  token.m_doctype.public_identifier += data;
}

template <typename string_like_t>
void append_to_doctype_token_system_identifier(Token& token, const string_like_t& data){
  token.m_doctype.system_identifier += data;
}

void set_force_quirks(Token& token){
  token.m_doctype.force_quirks = true;
}