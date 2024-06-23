#include <cassert>
#include "tokenizer.h"
#include "utils.h"

/*
TODO:
- did not recognize difference between emit and create
*/

#define REPLACEMENT_STR "\xEF\xBF\xBD"

#define CHAR_TOKEN(x) construct_character_token(x)
#define COMMENT_TOKEN(x) construct_comment_token(x)
#define START_TAG_TOKEN(x) construct_start_tag_token(x)
#define END_TAG_TOKEN(x) construct_end_tag_token(x)
#define REPLACEMENT_CHAR_TOKEN construct_character_token(REPLACEMENT_STR)
#define ON(ch) (cp == ch)

Tokenizer::Tokenizer(const std::string& contents) : 
  m_contents(contents), m_cursor(0), 
  m_state(TokenizerState::DataState),
  m_return_state(m_state),
  m_tmp_buffer("")
{
  m_tokens.reserve(contents.size());
  reset_token(m_tmp_token);
}

void Tokenizer::run() {
  while(true){
  
    if(in_simd_state()){
      std::optional<size_t> opt_state_change_char_pos = simd_next_pos();
      if(!opt_state_change_char_pos.has_value()){
        // add chunk to m_tokens
        // set m_cursor
        continue;
      }else{
        size_t state_change_char_pos = opt_state_change_char_pos.value();
        // add chunk to m_tokens
        m_cursor = state_change_char_pos;
      }
    }else{
      advance();
    }

    if(m_cursor >= m_contents.size()){
      break;
    }

    char cp = m_contents[m_cursor];
    switch(m_state) {
      case TokenizerState::DataState:
        if(ON('&')){
          set_return_state(TokenizerState::DataState);
          m_state = TokenizerState::CharacterReferenceState;
        }else if(ON('<')){
          m_state = TokenizerState::TagOpenState;
        }else{
          m_tokens.push_back(CHAR_TOKEN(cp));
        }
        break;
      case TokenizerState::RcDataState:
        if(ON('&')){
          set_return_state(TokenizerState::RcDataState);
          m_state = TokenizerState::CharacterReferenceState;
        }else if(ON('<')){
          m_state = TokenizerState::RcDataLessThanSignState;
        }else if(cp == '\0'){
          m_tokens.push_back(REPLACEMENT_CHAR_TOKEN);
        }else{
          m_tokens.push_back(CHAR_TOKEN(cp));
        }
      case TokenizerState::RawTextState:
        if(ON('<')){
          m_state = TokenizerState::RawTextLessThanSignState;
        }else if(cp == '\0'){
          m_tokens.push_back(REPLACEMENT_CHAR_TOKEN);
        }else{
          m_tokens.push_back(CHAR_TOKEN(cp));
        }
        break;
      case TokenizerState::ScriptDataState:
        if(ON('<')){
          m_state = TokenizerState::ScriptDataLessThanSignState;
        }else if(cp == '\0'){
          m_tokens.push_back(REPLACEMENT_CHAR_TOKEN);
        }else{
          m_tokens.push_back(CHAR_TOKEN(cp));
        }
        break;
      case TokenizerState::PlainTextState:
        if(cp == '\0'){
          m_tokens.push_back(REPLACEMENT_CHAR_TOKEN);
        }else{
          m_tokens.push_back(CHAR_TOKEN(cp));
        }
        break;
      case TokenizerState::TagOpenState:
        if(ON('!')){
          m_state = TokenizerState::MarkupDeclarationOpenState;
        }else if(ON('/')){
          m_state = TokenizerState::EndTagOpenState;
        }else if(ON('?')){
          m_tmp_token = COMMENT_TOKEN("");
          m_state = TokenizerState::BogusCommentState;
          m_cursor--;
        }else if(isalpha(cp)){
          m_tmp_token = START_TAG_TOKEN("");
          m_state = TokenizerState::TagNameState;
          m_cursor--;
        }else{
          m_tokens.push_back(CHAR_TOKEN('<'));
          m_state = TokenizerState::DataState;
          m_cursor--;
        }
        break;
      case TokenizerState::EndTagOpenState:
        if(isalpha(cp)){
          m_tmp_token = END_TAG_TOKEN("");
          m_state = TokenizerState::TagNameState;
          m_cursor--;
        }else if(ON('>')){
          m_state = TokenizerState::DataState;
        }else{
          m_tmp_token = COMMENT_TOKEN("");
          m_state = m_return_state;
          m_cursor--;
        }
        break;
      case TokenizerState::TagNameState:
        if(is_tab_lf_ff_space(cp)){
          m_state = TokenizerState::BeforeAttributeNameState;
        }else if(ON('/')){
          m_state = TokenizerState::SelfClosingStartTagState;
        }else if(ON('>')){
          m_tokens.push_back(std::move(m_tmp_token)); reset_token(m_tmp_token);
          m_state = TokenizerState::DataState;
        }else if(cp == '\0'){
          m_tmp_token.m_start_or_end_tag.tag_name += REPLACEMENT_STR;
        }else if(isalpha(cp) && isupper(cp)){
          m_tmp_token.m_start_or_end_tag.tag_name.push_back(tolower(cp));
        }else{
          m_tmp_token.m_start_or_end_tag.tag_name.push_back(cp);
        }
        break;
      case TokenizerState::RcDataLessThanSignState:
        if(ON('/')){
          m_tmp_buffer = "";
          m_state = TokenizerState::RcDataEndTagOpenState;
        }else{
          m_tokens.push_back(CHAR_TOKEN('<'));
          m_state = TokenizerState::RcDataState;
          m_cursor--;
        }
        break;
      case TokenizerState::RcDataEndTagOpenState:
        if(isalpha(cp)){
          m_tmp_token = END_TAG_TOKEN("");
          m_state = TokenizerState::RcDataEndTagNameState;
          m_cursor--;
        }else{
          m_tokens.push_back(CHAR_TOKEN("<"));
          m_tokens.push_back(CHAR_TOKEN("/"));
          m_state = TokenizerState::RcDataState;
          m_cursor--;
        }
        break;
      case TokenizerState::RcDataEndTagNameState:
        // TODO: appropriate end tag token
        if( 
          is_tab_lf_ff_space(cp) &&
          is_appropriate_end_tag_token()
        ){
          m_state = TokenizerState::BeforeAttributeNameState;
        }else if(ON('/') && is_appropriate_end_tag_token()){
          m_state = TokenizerState::SelfClosingStartTagState;
        }else if(ON('>') && is_appropriate_end_tag_token()){
          m_tokens.push_back(std::move(m_tmp_token)); reset_token(m_tmp_token);
          m_state = TokenizerState::DataState;
        }else if(isalpha(cp)){
          m_tmp_token.m_start_or_end_tag.tag_name.push_back(isupper(cp) ? tolower(cp) : cp);
          m_tmp_buffer.push_back(cp);
        }else{
          m_tokens.push_back(CHAR_TOKEN('<'));
          m_tokens.push_back(CHAR_TOKEN('/'));
          for(char c : m_tmp_buffer){
            m_tokens.push_back(CHAR_TOKEN(c));
          }
          m_state = TokenizerState::RcDataState;
          m_cursor--;
        }
        break;
      case TokenizerState::RawTextLessThanSignState:
        if(ON('/')){
          m_tmp_buffer = "";
          m_state = TokenizerState::RawTextEndTagOpenState;
        }else{
          m_tokens.push_back(CHAR_TOKEN('<'));
          m_state = TokenizerState::RawTextState;
          m_cursor--;
        }
        break;
      case TokenizerState::RawTextEndTagOpenState:
        if(isalpha(cp)){
          m_tmp_token = END_TAG_TOKEN("");
          m_state = TokenizerState::RawTextEndTagNameState;
          m_cursor--;
        }else{
          m_tokens.push_back(CHAR_TOKEN("<"));
          m_tokens.push_back(CHAR_TOKEN("/"));
          m_state = TokenizerState::RawTextState;
          m_cursor--;
        }
        break;
      case TokenizerState::RawTextEndTagNameState:
        if( 
          is_tab_lf_ff_space(cp) &&
          is_appropriate_end_tag_token()
        ){
          m_state = TokenizerState::BeforeAttributeNameState;
        }else if(ON('/') && is_appropriate_end_tag_token()){
          m_state = TokenizerState::SelfClosingStartTagState;
        }else if(ON('>') && is_appropriate_end_tag_token()){
          m_state = TokenizerState::DataState;
        }else if(isalpha(cp)){
          m_tokens.back().m_start_or_end_tag.tag_name.push_back(isupper(cp) ? tolower(cp) : cp);
          m_tmp_buffer.push_back(cp);
        }else{
          m_tokens.push_back(CHAR_TOKEN('<'));
          m_tokens.push_back(CHAR_TOKEN('/'));
          for(char c : m_tmp_buffer){
            m_tokens.push_back(CHAR_TOKEN(c));
          }
          m_state = TokenizerState::RawTextState;
          m_cursor--;
        }
      case TokenizerState::ScriptDataLessThanSignState:
        if(ON('/')){
          m_tmp_buffer = "";
          m_state = TokenizerState::ScriptDataEndTagOpenState;
        }else if(ON('!')){
          m_tokens.push_back(CHAR_TOKEN('<'));
          m_tokens.push_back(CHAR_TOKEN('!'));
          m_state = TokenizerState::ScriptDataEscapeStartState;
        }else{
          m_tokens.push_back(CHAR_TOKEN('<'));
          m_state = TokenizerState::ScriptDataState;
          m_cursor--;
        }
        break;
      case TokenizerState::ScriptDataEndTagOpenState:
        if(isalpha(cp)){
          m_tmp_token = END_TAG_TOKEN("");
          m_state = TokenizerState::ScriptDataEndTagNameState;
          m_cursor--;
        }else{
          m_tokens.push_back(CHAR_TOKEN("<"));
          m_tokens.push_back(CHAR_TOKEN("/"));
          m_state = TokenizerState::ScriptDataState;
          m_cursor--;
        }
        break;
      case TokenizerState::ScriptDataEndTagNameState:
        if( 
          is_tab_lf_ff_space(cp) &&
          is_appropriate_end_tag_token()
        ){
          m_state = TokenizerState::BeforeAttributeNameState;
        }else if(ON('/') && is_appropriate_end_tag_token()){
          m_state = TokenizerState::SelfClosingStartTagState;
        }else if(ON('>') && is_appropriate_end_tag_token()){
          m_tokens.push_back(std::move(m_tmp_token)); reset_token(m_tmp_token);
          m_state = TokenizerState::DataState;
        }else if(isalpha(cp)){
          m_tmp_token.m_start_or_end_tag.tag_name.push_back(isupper(cp) ? tolower(cp) : cp);
          m_tmp_buffer.push_back(cp);
        }else{
          m_tokens.push_back(CHAR_TOKEN('<'));
          m_tokens.push_back(CHAR_TOKEN('/'));
          for(char c : m_tmp_buffer){
            m_tokens.push_back(CHAR_TOKEN(c));
          }
          m_state = TokenizerState::ScriptDataState;
          m_cursor--;
        }
        break;
      case TokenizerState::ScriptDataEscapeStartState:
        if(ON('-')){
          m_tokens.push_back(CHAR_TOKEN('-'));
          m_state = TokenizerState::ScriptDataEscapeStartDashState;
        }else{
          m_state = TokenizerState::ScriptDataState;
          m_cursor--;
        }
        break;
      case TokenizerState::ScriptDataEscapeStartDashState:
        if(ON('-')){
          m_tokens.push_back(CHAR_TOKEN('-'));
          m_state = TokenizerState::ScriptDataEscapedDashDashState;
        }else{
          m_state = TokenizerState::ScriptDataState;
          m_cursor--;
        }
        break;
      case TokenizerState::ScriptDataEscapedState:
        if(ON('-')){
          m_tokens.push_back(CHAR_TOKEN('-'));
          m_state = TokenizerState::ScriptDataEscapedDashState;
        }else if(ON('<')){
          m_state = TokenizerState::ScriptDataEscapedLessThanSignState;
        }else if(ON('\0')){
          m_tokens.push_back(REPLACEMENT_CHAR_TOKEN);
        }else{
          m_tokens.push_back(CHAR_TOKEN(cp));
        }
        break;
      case TokenizerState::ScriptDataEscapedDashState:
        if(ON('-')){
          m_tokens.push_back(CHAR_TOKEN('-'));
          m_state = TokenizerState::ScriptDataEscapedDashDashState;
        }else if(ON('<')){
          m_state = TokenizerState::ScriptDataEscapedLessThanSignState;
        }else if(ON('\0')){
          m_tokens.push_back(REPLACEMENT_CHAR_TOKEN);
          m_state = TokenizerState::ScriptDataEscapedState;
        }else{
          m_tokens.push_back(CHAR_TOKEN(cp));
          m_state = TokenizerState::ScriptDataEscapedState;
        }
        break;
      case TokenizerState::ScriptDataEscapedDashDashState:
        if(ON('-')){
          m_tokens.push_back(CHAR_TOKEN('-'));
        }else if(ON('<')){
          m_state = TokenizerState::ScriptDataEscapedLessThanSignState;
        }else if(ON('>')){
          m_tokens.push_back(CHAR_TOKEN('>'));
          m_state = TokenizerState::ScriptDataState;
        }else if(ON('\0')){
          m_tokens.push_back(REPLACEMENT_CHAR_TOKEN);
          m_state = TokenizerState::ScriptDataEscapedState;
        }else{
          m_tokens.push_back(CHAR_TOKEN(cp));
          m_state = TokenizerState::ScriptDataEscapedState;
        }
        break;
      case TokenizerState::ScriptDataEscapedLessThanSignState:
        if(ON('/')){
          m_tmp_buffer = "";
          m_state = TokenizerState::ScriptDataEscapedEndTagOpenState;
        }else if(isalpha(cp)){
          m_tmp_buffer = "";
          m_tokens.push_back(CHAR_TOKEN('<'));
          m_state = TokenizerState::ScriptDataDoubleEscapeStartState;
          m_cursor--;
        }else{
          m_tokens.push_back(CHAR_TOKEN('<'));
          m_state = TokenizerState::ScriptDataEscapedState;
        }
        break;
      case TokenizerState::ScriptDataEscapedEndTagOpenState:
        if(isalpha(cp)){
          m_tmp_token = END_TAG_TOKEN("");
          m_state = TokenizerState::ScriptDataEscapedEndTagNameState;
          m_cursor--;
        }else{
          m_tokens.push_back(CHAR_TOKEN('<'));
          m_tokens.push_back(CHAR_TOKEN('/'));
          m_state = TokenizerState::ScriptDataEscapedState;
          m_cursor--;
        }
        break;
      case TokenizerState::ScriptDataEscapedEndTagNameState:
        if( 
          is_tab_lf_ff_space(cp) &&
          is_appropriate_end_tag_token()
        ){
          m_state = TokenizerState::BeforeAttributeNameState;
        }else if(ON('/') && is_appropriate_end_tag_token()){
          m_state = TokenizerState::SelfClosingStartTagState;
        }else if(ON('>') && is_appropriate_end_tag_token()){
          m_tokens.push_back(std::move(m_tmp_token)); reset_token(m_tmp_token);
          m_state = TokenizerState::DataState;
        }else if(isalpha(cp)){
          m_tmp_token.m_start_or_end_tag.tag_name.push_back(isupper(cp) ? tolower(cp) : cp);
          m_tmp_buffer.push_back(cp);
        }else{
          m_tokens.push_back(CHAR_TOKEN('<'));
          m_tokens.push_back(CHAR_TOKEN('/'));
          for(char c : m_tmp_buffer){
            m_tokens.push_back(CHAR_TOKEN(c));
          }
          m_state = TokenizerState::ScriptDataEscapedState;
          m_cursor--;
        }
        break;
      case TokenizerState::ScriptDataDoubleEscapeStartState:
        if(is_tab_lf_ff_space(cp) || ON('/') || ON('>')){
          if(m_tmp_buffer == "script"){
            m_state = TokenizerState::ScriptDataDoubleEscapedState;
          }else{
            m_state = TokenizerState::ScriptDataEscapedState;
          }
          m_tokens.push_back(CHAR_TOKEN(cp));
        }else if(isalpha(cp)){
          m_tmp_buffer.push_back(isupper(cp) ? tolower(cp) : cp);
          m_tokens.push_back(CHAR_TOKEN(cp));
        }else{
          m_state = TokenizerState::ScriptDataEscapedState;
          m_cursor--;
        }
        break;
      case TokenizerState::ScriptDataDoubleEscapedState:
        if(ON('-')){
          m_tokens.push_back(CHAR_TOKEN('-'));
          m_state = TokenizerState::ScriptDataDoubleEscapedDashState;
        }else if(ON('<')){
          m_tokens.push_back(CHAR_TOKEN('<'));
          m_state = TokenizerState::ScriptDataDoubleEscapedLessThanSignState;
        }else if(ON('\0')){
          m_tokens.push_back(REPLACEMENT_CHAR_TOKEN);
        }else{
          m_tokens.push_back(CHAR_TOKEN(cp));
        }
        break;
      case TokenizerState::ScriptDataDoubleEscapedDashState:
        if(ON('-')){
          m_tokens.push_back(CHAR_TOKEN('-'));
          m_state = TokenizerState::ScriptDataDoubleEscapedDashDashState;
        }else if(ON('<')){
          m_tokens.push_back(CHAR_TOKEN('<'));
          m_state = TokenizerState::ScriptDataDoubleEscapedLessThanSignState;
        }else if(ON('\0')){
          m_tokens.push_back(REPLACEMENT_CHAR_TOKEN);
          m_state = TokenizerState::ScriptDataDoubleEscapedState;
        }else{
          m_tokens.push_back(CHAR_TOKEN(cp));
          m_state = TokenizerState::ScriptDataDoubleEscapedState;
        }
        break;
      case TokenizerState::ScriptDataDoubleEscapedDashDashState:
        if(ON('-')){
          m_tokens.push_back(CHAR_TOKEN('-'));
        }else if(ON('<')){
          m_tokens.push_back(CHAR_TOKEN('<'));
          m_state = TokenizerState::ScriptDataDoubleEscapedLessThanSignState;
        }else if(ON('>')){
          m_tokens.push_back(CHAR_TOKEN('>'));
          m_state = TokenizerState::ScriptDataState;
        }else if(ON('\0')){
          m_tokens.push_back(REPLACEMENT_CHAR_TOKEN);
          m_state = TokenizerState::ScriptDataDoubleEscapedState;
        }else{
          m_tokens.push_back(CHAR_TOKEN(cp));
          m_state = TokenizerState::ScriptDataDoubleEscapedState;
        }
        break;
      case TokenizerState::ScriptDataDoubleEscapedLessThanSignState:
        if(ON('/')){
          m_tmp_buffer = "";
          m_tokens.push_back(CHAR_TOKEN('/'));
          m_state = TokenizerState::ScriptDataDoubleEscapeEndState;
        }else{
          m_state = TokenizerState::ScriptDataDoubleEscapedState;
          m_cursor--;
        }
        break;
      case TokenizerState::ScriptDataDoubleEscapeEndState:
        if(is_tab_lf_ff_space(cp) || ON('/') || ON('>')){
          if(m_tmp_buffer == "script"){
            m_state = TokenizerState::ScriptDataEscapedState;
          }else{
            m_state = TokenizerState::ScriptDataDoubleEscapedState;
          }
          m_tokens.push_back(CHAR_TOKEN(cp));
        }else if(isalpha(cp)){
          m_tmp_buffer.push_back(isupper(cp) ? tolower(cp) : cp);
          m_tokens.push_back(CHAR_TOKEN(cp));
        }else{
          m_state = TokenizerState::ScriptDataDoubleEscapedState;
          m_cursor--;
        }
        break;
      case TokenizerState::BeforeAttributeNameState:
        if(is_tab_lf_ff_space(cp)){
          ; // pass
        }else if(ON('/') || ON('>')){
          m_state = TokenizerState::AfterAttributeNameState;
          m_cursor--;
        }else if(ON('=')){
          m_tokens.back().m_start_or_end_tag.attributes.push_back(
            construct_attribute(std::string(1, cp), "")
          );
          m_state = TokenizerState::AttributeNameState;
        }else{
          m_tokens.back().m_start_or_end_tag.attributes.push_back(
            construct_attribute(std::string(1, cp), "")
          );
          m_state = TokenizerState::AttributeNameState;
        }
        break;
      // TODO: START HERE
      case TokenizerState::AttributeNameState:
        if(isspace(cp)){
          m_state = TokenizerState::AfterAttributeNameState;
        }else if(ON('/')){
          m_state = TokenizerState::SelfClosingStartTagState;
        }else if(ON('=')){
          m_state = TokenizerState::BeforeAttributeValueState;
        }else if(ON('>')){
          m_state = TokenizerState::DataState;
          m_tokens.push_back(std::move(construct_token()));
        }else if(isalpha(cp)){
          m_tokens.back().m_start_or_end_tag.attributes.back().name.push_back(cp);
        }else{
          m_tokens.back().m_start_or_end_tag.attributes.back().name.push_back(cp);
        }
        break;
      case TokenizerState::AfterAttributeNameState:
        if(isspace(cp)){
          continue;
        }else if(ON('/')){
          m_state = TokenizerState::SelfClosingStartTagState;
        }else if(ON('=')){
          m_state = TokenizerState::BeforeAttributeValueState;
        }else if(ON('>')){
          m_state = TokenizerState::DataState;
          m_tokens.push_back(std::move(construct_token()));
        }else if(isalpha(cp)){
          m_tokens.back().m_start_or_end_tag.attributes.push_back(construct_attribute());
          m_state = TokenizerState::AttributeNameState;
          m_cursor--;
        }else{
          m_tokens.back().m_start_or_end_tag.attributes.push_back(construct_attribute());
          m_cursor--;
          m_state = TokenizerState::AttributeNameState;
        }
        break;
      case TokenizerState::BeforeAttributeValueState:
        if(isspace(cp)){
          continue;
        }else if(ON('"')){
          m_state = TokenizerState::AttributeValueDoubleQuotedState;
        }else if(ON('\')'){
          m_state = TokenizerState::AttributeValueSingleQuotedState;
        }else if(ON('>')){
          m_state = TokenizerState::DataState;
          m_tokens.push_back(std::move(construct_token()));
        }else{
          m_state = TokenizerState::AttributeValueUnquotedState;
          m_cursor--;
        }
        break;
      case TokenizerState::AttributeValueDoubleQuotedState:
        if(ON('"')){
          m_state = TokenizerState::AfterAttributeValueQuotedState;
        }else if(ON('&')){
          set_return_state(TokenizerState::AttributeValueDoubleQuotedState);
          m_state = TokenizerState::CharacterReferenceState;
        }else if(cp == '\0'){
          m_tokens.back().m_start_or_end_tag.attributes.back().value.push_back('\xEF\xBF\xBD');
        }else{
          m_tokens.back().m_start_or_end_tag.attributes.back().value.push_back(cp);
        }
        break;
      case TokenizerState::AttributeValueSingleQuotedState:
        if(ON('\')'){
          m_state = TokenizerState::AfterAttributeValueQuotedState;
        }else if(ON('&')){
          set_return_state(TokenizerState::AttributeValueSingleQuotedState);
          m_state = TokenizerState::CharacterReferenceState;
        }else if(cp == '\0'){
          m_tokens.back().m_start_or_end_tag.attributes.back().value.push_back('\xEF\xBF\xBD');
        }else{
          m_tokens.back().m_start_or_end_tag.attributes.back().value.push_back(cp);
        }
        break;
      case TokenizerState::AttributeValueUnquotedState:
        if(isspace(cp)){
          m_state = TokenizerState::BeforeAttributeNameState;
        }else if(ON('&')){
          set_return_state(TokenizerState::AttributeValueUnquotedState);
          m_state = TokenizerState::CharacterReferenceState;
        }else if(ON('>')){
          m_state = TokenizerState::DataState;
          m_tokens.push_back(std::move(construct_token()));
        }else if(cp == '\0'){
          m_tokens.back().m_start_or_end_tag.attributes.back().value.push_back('\xEF\xBF\xBD');
        }else{
          m_tokens.back().m_start_or_end_tag.attributes.back().value.push_back(cp);
        }
        break;
      case TokenizerState::AfterAttributeValueQuotedState:
        if(isspace(cp)){
          m_state = TokenizerState::BeforeAttributeNameState;
        }else if(ON('/')){
          m_state = TokenizerState::SelfClosingStartTagState;
        }else if(ON('>')){
          m_state = TokenizerState::DataState;
          m_tokens.push_back(std::move(construct_token()));
        }else{
          m_state = TokenizerState::BeforeAttributeNameState;
          m_cursor--;
        }
        break;
      case TokenizerState::SelfClosingStartTagState:
        if(ON('>')){
          m_tokens.back().m_start_or_end_tag.self_closing = true;
          m_state = TokenizerState::DataState;
          m_tokens.push_back(std::move(construct_token()));
        }else{
          m_state = TokenizerState::BeforeAttributeNameState;
          m_cursor--;
        }
        break;
      case TokenizerState::CharacterReferenceState:
        if(ON('x')){
          m_state = TokenizerState::CharacterReferenceHexState;
        }else{
          m_state = TokenizerState::CharacterReferenceDecimalState;
          m_cursor--;
        }
        break;
      case TokenizerState::CharacterReferenceDecimalState:
        if(isdigit(cp)){
          m_state = TokenizerState::CharacterReferenceDecimalState;
          m_cursor--;
        }else if(ON(';')){
          m_state = m_return_state;
        }else{
          m_state = m_return_state;
          m_cursor--;
        }
        break;
    }

  }
}

/*
if current state has transitions that are a subset of SIMD states, return true.
*/
bool Tokenizer::in_simd_state() {
  return m_state == TokenizerState::DataState ||
         m_state == TokenizerState::RcDataState ||
         m_state == TokenizerState::RawTextState ||
         m_state == TokenizerState::ScriptDataState ||
         m_state == TokenizerState::PlainTextState;
}

void Tokenizer::set_return_state(TokenizerState state) {
  m_return_state = state;
}

bool Tokenizer::is_appropriate_end_tag_token() {
  assert(m_tmp_token.token_type == Token::TokenType::END_TAG);
  for(int i = m_tokens.size() - 1; i >= 0; i--){
    if(m_tokens[i].token_type == Token::TokenType::START_TAG){
      return m_tmp_token.m_start_or_end_tag.tag_name == m_tokens[i].m_start_or_end_tag.tag_name;
    }
  }
  return false;
}