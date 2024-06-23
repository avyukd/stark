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