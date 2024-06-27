#include "token.h"

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

Token construct_empty_doctype_token(){
  Token token;
  token.token_type = Token::TokenType::DOCTYPE;
  return token;
}

Token construct_eof_token(){
  Token token;
  token.token_type = Token::TokenType::END_OF_FILE;
  return token;
}

Token construct_doctype_token(char c) {
  Token token = construct_empty_doctype_token();
  token.m_doctype.name = "" + c;
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

bool is_eof_token(const Token& token){
  return token.token_type == Token::TokenType::END_OF_FILE;
}

void set_force_quirks(Token& token){
  token.m_doctype.force_quirks = true;
}

void set_tag_self_closing_flag(Token& token){
  token.m_start_or_end_tag.self_closing = true;
}

std::string get_token_type_string(const Token::TokenType tt){
  switch (tt)
  {
    case Token::TokenType::NOT_SET:
      return "NOT_SET";
    case Token::TokenType::DOCTYPE:
      return "DOCTYPE";
    case Token::TokenType::START_TAG:
      return "START_TAG";
    case Token::TokenType::END_TAG:
      return "END_TAG";
    case Token::TokenType::COMMENT:
      return "COMMENT";
    case Token::TokenType::CHARACTER:
      return "CHARACTER";
    case Token::TokenType::END_OF_FILE:
      return "END_OF_FILE";
  }
}

std::string to_string(const Token& token){
  std::string ret;
  ret += get_token_type_string(token.token_type) + ", ";
  switch (token.token_type)
  {
    case Token::TokenType::NOT_SET:
      ret += "NULL";
      break;
    case Token::TokenType::DOCTYPE:
      ret += "name: " + token.m_doctype.name + ", ";
      ret += "public_identifier: " + token.m_doctype.public_identifier + ", ";
      ret += "system_identifier: " + token.m_doctype.system_identifier + ", ";
      ret += "force_quirks: " + std::to_string(token.m_doctype.force_quirks);
      break;
    case Token::TokenType::START_TAG:
    case Token::TokenType::END_TAG:
      ret += "tag_name: " + token.m_start_or_end_tag.tag_name + ", ";
      ret += "self_closing: " + std::to_string(token.m_start_or_end_tag.self_closing) + ", ";
      ret += "attributes: [";
      for (const attribute& attr : token.m_start_or_end_tag.attributes) {
        ret += "{name: " + attr.name + ", value: " + attr.value + "}, ";
      }
      ret += "]";
      break;
    case Token::TokenType::COMMENT:
    case Token::TokenType::CHARACTER:
      ret += "data: " + token.m_comment_or_character.data;
      break;
    case Token::TokenType::END_OF_FILE:
      ret += "NULL";
      break;
  }
  return ret;
}