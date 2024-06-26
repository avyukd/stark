#include <cassert>
#include "tokenizer.h"
#include "utils.h"
#include "simd_utils.h"

/*
TODO:
- did not recognize difference between emit and create
- will get eof errors (multiple places assume there is a next character)
*/

#define REPLACEMENT_STR "\xEF\xBF\xBD"

#define EMIT_CHAR_TOKENS(str) for(auto& token : construct_character_tokens(str)){ m_tokens.push_back(std::move(token)); }
#define CHAR_TOKEN(x) construct_character_token(x)
#define EMIT_CHAR_TOKEN(x) m_tokens.push_back(CHAR_TOKEN(x))
#define COMMENT_TOKEN(x) construct_comment_token(x)
#define START_TAG_TOKEN(x) construct_start_tag_token(x)
#define END_TAG_TOKEN(x) construct_end_tag_token(x)
#define REPLACEMENT_CHAR_TOKEN construct_character_token(REPLACEMENT_STR)
#define ON(ch) (cp == ch)
#define EMIT_CURRENT_TOKEN m_tokens.push_back(std::move(m_tmp_token)); reset_token(m_tmp_token)
#define FLUSH_CODE_POINTS if(is_attribute_return_state()){ \
                            append_to_tag_token_attribute_value(m_tmp_token, m_tmp_buffer); \
                          }else{ \
                            EMIT_CHAR_TOKENS(m_tmp_buffer); \
                          } 

Tokenizer::Tokenizer(const std::string& contents, TokenizerOptions options) : 
  m_contents{contents}, m_cursor{0}, 
  m_state{TokenizerState::DataState},
  m_return_state{m_state},
  m_tmp_buffer{},
  m_named_char_ref{},
  m_options{options}
{
  m_tokens.reserve(contents.size());
  reset_token(m_tmp_token);
}

void Tokenizer::run() {
  while(true){
    if(m_cursor >= m_contents.size()) break;

    // todo check bounds here?
    if(m_options.use_simd){
      if(std::string needles = in_simd_emit_char_state(); needles != ""){
        auto state_change_pos_opt = simd_state_change(needles);
        size_t safe_adv_amount = std::min((int) (m_contents.size() - m_cursor), SIMD_SEARCH_SIZE);
        if(state_change_pos_opt.has_value()){
          safe_adv_amount = state_change_pos_opt.value();
        }
        for(size_t i = 0 ; i < safe_adv_amount; i++){
          EMIT_CHAR_TOKEN(m_contents[m_cursor++]);
        }
        if(!state_change_pos_opt.has_value()){
          continue;
        }
        // else: pass and continue into state change
      } 
    }

    char cp = m_contents[m_cursor++];
    switch(m_state) {
      case TokenizerState::DataState:
        if(ON('&')){
          set_return_state(TokenizerState::DataState);
          m_state = TokenizerState::CharacterReferenceState;
        }else if(ON('<')){
          m_state = TokenizerState::TagOpenState;
        }else{
          EMIT_CHAR_TOKEN(cp);
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
          EMIT_CHAR_TOKEN(cp);
        }
        break;
      case TokenizerState::RawTextState:
        if(ON('<')){
          m_state = TokenizerState::RawTextLessThanSignState;
        }else if(cp == '\0'){
          m_tokens.push_back(REPLACEMENT_CHAR_TOKEN);
        }else{
          EMIT_CHAR_TOKEN(cp);
        }
        break;
      case TokenizerState::ScriptDataState:
        if(ON('<')){
          m_state = TokenizerState::ScriptDataLessThanSignState;
        }else if(cp == '\0'){
          m_tokens.push_back(REPLACEMENT_CHAR_TOKEN);
        }else{
          EMIT_CHAR_TOKEN(cp);
        }
        break;
      case TokenizerState::PlainTextState:
        if(cp == '\0'){
          m_tokens.push_back(REPLACEMENT_CHAR_TOKEN);
        }else{
          EMIT_CHAR_TOKEN(cp);
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
          EMIT_CHAR_TOKEN('<');
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
          EMIT_CURRENT_TOKEN;
          m_state = TokenizerState::DataState;
        }else if(cp == '\0'){
          append_to_tag_token_tag_name(m_tmp_token, REPLACEMENT_STR);
        }else{
          append_to_tag_token_tag_name(m_tmp_token, isupper(cp) ? tolower(cp) : cp);
        }
        break;
      case TokenizerState::RcDataLessThanSignState:
        if(ON('/')){
          m_tmp_buffer = "";
          m_state = TokenizerState::RcDataEndTagOpenState;
        }else{
          EMIT_CHAR_TOKEN('<');
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
          EMIT_CHAR_TOKENS("</");
          m_state = TokenizerState::RcDataState;
          m_cursor--;
        }
        break;
      case TokenizerState::RcDataEndTagNameState:
        if( 
          is_tab_lf_ff_space(cp) &&
          is_appropriate_end_tag_token()
        ){
          m_state = TokenizerState::BeforeAttributeNameState;
        }else if(ON('/') && is_appropriate_end_tag_token()){
          m_state = TokenizerState::SelfClosingStartTagState;
        }else if(ON('>') && is_appropriate_end_tag_token()){
          EMIT_CURRENT_TOKEN;
          m_state = TokenizerState::DataState;
        }else if(isalpha(cp)){
          append_to_tag_token_tag_name(m_tmp_token, isupper(cp) ? tolower(cp) : cp);
          m_tmp_buffer.push_back(cp);
        }else{
          EMIT_CHAR_TOKENS("</");
          EMIT_CHAR_TOKENS(m_tmp_buffer);
          m_state = TokenizerState::RcDataState;
          m_cursor--;
        }
        break;
      case TokenizerState::RawTextLessThanSignState:
        if(ON('/')){
          m_tmp_buffer = "";
          m_state = TokenizerState::RawTextEndTagOpenState;
        }else{
          EMIT_CHAR_TOKEN('<');
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
          EMIT_CHAR_TOKENS("</")
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
          append_to_tag_token_tag_name(m_tmp_token, isupper(cp) ? tolower(cp) : cp);
          m_tmp_buffer.push_back(cp);
        }else{
          EMIT_CHAR_TOKENS("</")
          EMIT_CHAR_TOKENS(m_tmp_buffer);
          m_state = TokenizerState::RawTextState;
          m_cursor--;
        }
        break;
      case TokenizerState::ScriptDataLessThanSignState:
        if(ON('/')){
          m_tmp_buffer = "";
          m_state = TokenizerState::ScriptDataEndTagOpenState;
        }else if(ON('!')){
          EMIT_CHAR_TOKENS("<!");
          m_state = TokenizerState::ScriptDataEscapeStartState;
        }else{
          EMIT_CHAR_TOKEN('<');
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
          EMIT_CHAR_TOKENS("</");
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
          EMIT_CURRENT_TOKEN;
          m_state = TokenizerState::DataState;
        }else if(isalpha(cp)){
          append_to_tag_token_tag_name(m_tmp_token, isupper(cp) ? tolower(cp) : cp);
          m_tmp_buffer.push_back(cp);
        }else{
          EMIT_CHAR_TOKENS("</");
          EMIT_CHAR_TOKENS(m_tmp_buffer);
          m_state = TokenizerState::ScriptDataState;
          m_cursor--;
        }
        break;
      case TokenizerState::ScriptDataEscapeStartState:
        if(ON('-')){
          EMIT_CHAR_TOKEN('-');
          m_state = TokenizerState::ScriptDataEscapeStartDashState;
        }else{
          m_state = TokenizerState::ScriptDataState;
          m_cursor--;
        }
        break;
      case TokenizerState::ScriptDataEscapeStartDashState:
        if(ON('-')){
          EMIT_CHAR_TOKEN('-');
          m_state = TokenizerState::ScriptDataEscapedDashDashState;
        }else{
          m_state = TokenizerState::ScriptDataState;
          m_cursor--;
        }
        break;
      case TokenizerState::ScriptDataEscapedState:
        if(ON('-')){
          EMIT_CHAR_TOKEN('-');
          m_state = TokenizerState::ScriptDataEscapedDashState;
        }else if(ON('<')){
          m_state = TokenizerState::ScriptDataEscapedLessThanSignState;
        }else if(ON('\0')){
          m_tokens.push_back(REPLACEMENT_CHAR_TOKEN);
        }else{
          EMIT_CHAR_TOKEN(cp);
        }
        break;
      case TokenizerState::ScriptDataEscapedDashState:
        if(ON('-')){
          EMIT_CHAR_TOKEN('-');
          m_state = TokenizerState::ScriptDataEscapedDashDashState;
        }else if(ON('<')){
          m_state = TokenizerState::ScriptDataEscapedLessThanSignState;
        }else if(ON('\0')){
          m_tokens.push_back(REPLACEMENT_CHAR_TOKEN);
          m_state = TokenizerState::ScriptDataEscapedState;
        }else{
          EMIT_CHAR_TOKEN(cp);
          m_state = TokenizerState::ScriptDataEscapedState;
        }
        break;
      case TokenizerState::ScriptDataEscapedDashDashState:
        if(ON('-')){
          EMIT_CHAR_TOKEN('-');
        }else if(ON('<')){
          m_state = TokenizerState::ScriptDataEscapedLessThanSignState;
        }else if(ON('>')){
          EMIT_CHAR_TOKEN('>');
          m_state = TokenizerState::ScriptDataState;
        }else if(ON('\0')){
          m_tokens.push_back(REPLACEMENT_CHAR_TOKEN);
          m_state = TokenizerState::ScriptDataEscapedState;
        }else{
          EMIT_CHAR_TOKEN(cp);
          m_state = TokenizerState::ScriptDataEscapedState;
        }
        break;
      case TokenizerState::ScriptDataEscapedLessThanSignState:
        if(ON('/')){
          m_tmp_buffer = "";
          m_state = TokenizerState::ScriptDataEscapedEndTagOpenState;
        }else if(isalpha(cp)){
          m_tmp_buffer = "";
          EMIT_CHAR_TOKEN('<');
          m_state = TokenizerState::ScriptDataDoubleEscapeStartState;
          m_cursor--;
        }else{
          EMIT_CHAR_TOKEN('<');
          m_state = TokenizerState::ScriptDataEscapedState;
        }
        break;
      case TokenizerState::ScriptDataEscapedEndTagOpenState:
        if(isalpha(cp)){
          m_tmp_token = END_TAG_TOKEN("");
          m_state = TokenizerState::ScriptDataEscapedEndTagNameState;
          m_cursor--;
        }else{
          EMIT_CHAR_TOKENS("</");
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
          EMIT_CURRENT_TOKEN;
          m_state = TokenizerState::DataState;
        }else if(isalpha(cp)){
          append_to_tag_token_tag_name(m_tmp_token, isupper(cp) ? tolower(cp) : cp);
          m_tmp_buffer.push_back(cp);
        }else{
          EMIT_CHAR_TOKENS("</");
          EMIT_CHAR_TOKENS(m_tmp_buffer);
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
          EMIT_CHAR_TOKEN(cp);
        }else if(isalpha(cp)){
          m_tmp_buffer.push_back(isupper(cp) ? tolower(cp) : cp);
          EMIT_CHAR_TOKEN(cp);
        }else{
          m_state = TokenizerState::ScriptDataEscapedState;
          m_cursor--;
        }
        break;
      case TokenizerState::ScriptDataDoubleEscapedState:
        if(ON('-')){
          EMIT_CHAR_TOKEN('-');
          m_state = TokenizerState::ScriptDataDoubleEscapedDashState;
        }else if(ON('<')){
          EMIT_CHAR_TOKEN('<');
          m_state = TokenizerState::ScriptDataDoubleEscapedLessThanSignState;
        }else if(ON('\0')){
          m_tokens.push_back(REPLACEMENT_CHAR_TOKEN);
        }else{
          EMIT_CHAR_TOKEN(cp);
        }
        break;
      case TokenizerState::ScriptDataDoubleEscapedDashState:
        if(ON('-')){
          EMIT_CHAR_TOKEN('-');
          m_state = TokenizerState::ScriptDataDoubleEscapedDashDashState;
        }else if(ON('<')){
          EMIT_CHAR_TOKEN('<');
          m_state = TokenizerState::ScriptDataDoubleEscapedLessThanSignState;
        }else if(ON('\0')){
          m_tokens.push_back(REPLACEMENT_CHAR_TOKEN);
          m_state = TokenizerState::ScriptDataDoubleEscapedState;
        }else{
          EMIT_CHAR_TOKEN(cp);
          m_state = TokenizerState::ScriptDataDoubleEscapedState;
        }
        break;
      case TokenizerState::ScriptDataDoubleEscapedDashDashState:
        if(ON('-')){
          EMIT_CHAR_TOKEN('-');
        }else if(ON('<')){
          EMIT_CHAR_TOKEN('<');
          m_state = TokenizerState::ScriptDataDoubleEscapedLessThanSignState;
        }else if(ON('>')){
          EMIT_CHAR_TOKEN('>');
          m_state = TokenizerState::ScriptDataState;
        }else if(ON('\0')){
          m_tokens.push_back(REPLACEMENT_CHAR_TOKEN);
          m_state = TokenizerState::ScriptDataDoubleEscapedState;
        }else{
          EMIT_CHAR_TOKEN(cp);
          m_state = TokenizerState::ScriptDataDoubleEscapedState;
        }
        break;
      case TokenizerState::ScriptDataDoubleEscapedLessThanSignState:
        if(ON('/')){
          m_tmp_buffer = "";
          EMIT_CHAR_TOKEN('/');
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
          EMIT_CHAR_TOKEN(cp);
        }else if(isalpha(cp)){
          m_tmp_buffer.push_back(isupper(cp) ? tolower(cp) : cp);
          EMIT_CHAR_TOKEN(cp);
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
          append_attribute_to_tag_token(m_tmp_token, cp, "");
          m_state = TokenizerState::AttributeNameState;
        }else{
          append_attribute_to_tag_token(m_tmp_token, cp, "");
          m_state = TokenizerState::AttributeNameState;
        }
        break;
      case TokenizerState::AttributeNameState:
        //TODO: this allows for duplicate attribute names
        if(is_tab_lf_ff_space(cp) || ON('/') || ON('>')){
          m_state = TokenizerState::AfterAttributeNameState;
          m_cursor--;
        }else if(ON('=')){
          m_state = TokenizerState::BeforeAttributeValueState;
        }else if(ON('\0')){
          append_to_tag_token_attribute_name(m_tmp_token, REPLACEMENT_STR);
        }else{
          append_to_tag_token_attribute_name(m_tmp_token, cp);
        }
        break;
      case TokenizerState::AfterAttributeNameState:
        if(is_tab_lf_ff_space(cp)){
          ; // pass
        }else if(ON('/')){
          m_state = TokenizerState::SelfClosingStartTagState;
        }else if(ON('=')){
          m_state = TokenizerState::BeforeAttributeValueState;
        }else if(ON('>')){
          EMIT_CURRENT_TOKEN;
          m_state = TokenizerState::DataState;
        }else{
          append_attribute_to_tag_token(m_tmp_token, "", "");
          m_state = TokenizerState::AttributeNameState;
          m_cursor--;
        }
        break;
      case TokenizerState::BeforeAttributeValueState:
        if(is_tab_lf_ff_space(cp)){
          ; // pass
        }else if(ON('"')){
          m_state = TokenizerState::AttributeValueDoubleQuotedState;
        }else if(ON('\'')){
          m_state = TokenizerState::AttributeValueSingleQuotedState;
        }else if(ON('>')){
          EMIT_CURRENT_TOKEN;
          m_state = TokenizerState::DataState;
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
        }else if(ON('\0')){
          append_to_tag_token_attribute_value(m_tmp_token, REPLACEMENT_STR);
        }else{
          append_to_tag_token_attribute_value(m_tmp_token, cp);
        }
        break;
      case TokenizerState::AttributeValueSingleQuotedState:
        if(ON('\'')){
          m_state = TokenizerState::AfterAttributeValueQuotedState;
        }else if(ON('&')){
          set_return_state(TokenizerState::AttributeValueSingleQuotedState);
          m_state = TokenizerState::CharacterReferenceState;
        }else if(ON('\0')){
          append_to_tag_token_attribute_value(m_tmp_token, REPLACEMENT_STR);
        }else{
          append_to_tag_token_attribute_value(m_tmp_token, cp);
        }
        break;
      case TokenizerState::AttributeValueUnquotedState:
        if(is_tab_lf_ff_space(cp)){
          m_state = TokenizerState::BeforeAttributeNameState;
        }else if(ON('&')){
          set_return_state(TokenizerState::AttributeValueUnquotedState);
          m_state = TokenizerState::CharacterReferenceState;
        }else if(ON('>')){
          EMIT_CURRENT_TOKEN;
          m_state = TokenizerState::DataState;
        }else if(ON('\0')){
          append_to_tag_token_attribute_value(m_tmp_token, REPLACEMENT_STR);
        }else{
          append_to_tag_token_attribute_value(m_tmp_token, cp);
        }
        break;
      case TokenizerState::AfterAttributeValueQuotedState:
        if(is_tab_lf_ff_space(cp)){
          m_state = TokenizerState::BeforeAttributeNameState;
        }else if(ON('/')){
          m_state = TokenizerState::SelfClosingStartTagState;
        }else if(ON('>')){
          EMIT_CURRENT_TOKEN;
          m_state = TokenizerState::DataState;
        }else{
          m_state = TokenizerState::BeforeAttributeNameState;
          m_cursor--;
        }
        break;
      case TokenizerState::SelfClosingStartTagState:
        if(ON('>')){
          set_tag_self_closing_flag(m_tmp_token);
          EMIT_CURRENT_TOKEN;
          m_state = TokenizerState::DataState;
        }else{
          m_state = TokenizerState::BeforeAttributeNameState;
          m_cursor--;
        }
        break;
      case TokenizerState::BogusCommentState:
        if(ON('>')){
          EMIT_CURRENT_TOKEN;
          m_state = TokenizerState::DataState;
        }else if(ON('\0')){
          append_to_comment_or_character_data(m_tmp_token, REPLACEMENT_STR);
        }else{
          append_to_comment_or_character_data(m_tmp_token, cp);
        }
        break;
      case TokenizerState::MarkupDeclarationOpenState:
        if(m_contents.substr(m_cursor, 2) == "--"){
          m_tmp_token = COMMENT_TOKEN("");
          m_state = TokenizerState::CommentStartState;
          m_cursor++;
        }else if(m_contents.substr(m_cursor, 7) == "DOCTYPE"){
          m_state = TokenizerState::DoctypeState;
          m_cursor += 6;
        }else if(m_contents.substr(m_cursor, 7) == "[CDATA["){
          // TODO: Does not consider adjusted current node branch!
          m_tmp_token = COMMENT_TOKEN("");
          m_state = TokenizerState::BogusCommentState;
          m_cursor += 6;
        }else{
          m_tmp_token = COMMENT_TOKEN("");
          m_state = TokenizerState::BogusCommentState;
        }
        break;
      case TokenizerState::CommentStartState:
        if(ON('-')){
          m_state = TokenizerState::CommentStartDashState;
        }else if(ON('>')){
          EMIT_CURRENT_TOKEN;
          m_state = TokenizerState::DataState;
        }else{
          m_state = TokenizerState::CommentState;
          m_cursor--;
        }
        break;
      case TokenizerState::CommentStartDashState:
        if(ON('-')){
          m_state = TokenizerState::CommentEndState;
        }else if(ON('>')){
          EMIT_CURRENT_TOKEN;
          m_state = TokenizerState::DataState;
        }else{
          append_to_comment_or_character_data(m_tmp_token, '-');
          m_state = TokenizerState::CommentState;
          m_cursor--;
        }
        break;
      case TokenizerState::CommentState:
        if(ON('<')){
          append_to_comment_or_character_data(m_tmp_token, '<');
          m_state = TokenizerState::CommentLessThanSignState;
        }else if(ON('-')){
          m_state = TokenizerState::CommentEndDashState;
        }else if(ON('\0')){
          append_to_comment_or_character_data(m_tmp_token, REPLACEMENT_STR);
        }else{
          append_to_comment_or_character_data(m_tmp_token, cp);
        }
        break;
      case TokenizerState::CommentLessThanSignState:
        if(ON('!')){
          append_to_comment_or_character_data(m_tmp_token, cp);
          m_state = TokenizerState::CommentLessThanSignBangState;
        }else if(ON('<')){
          append_to_comment_or_character_data(m_tmp_token, '<');
        }else{
          m_state = TokenizerState::CommentState;
          m_cursor--;
        }
        break;
      case TokenizerState::CommentLessThanSignBangState:
        if(ON('-')){
          m_state = TokenizerState::CommentLessThanSignBangDashState;
        }else{
          m_state = TokenizerState::CommentState;
          m_cursor--;
        }
        break;
      case TokenizerState::CommentLessThanSignBangDashState:
        if(ON('-')){
          m_state = TokenizerState::CommentLessThanSignBangDashDashState;
        }else{
          m_state = TokenizerState::CommentEndDashState;
          m_cursor--;
        }
        break;
      case TokenizerState::CommentLessThanSignBangDashDashState:
        m_state = TokenizerState::CommentEndState;
        m_cursor--;
        break;
      case TokenizerState::CommentEndDashState:
        if(ON('-')){
          m_state = TokenizerState::CommentEndState;
        }else{
          append_to_comment_or_character_data(m_tmp_token, '-');
          m_state = TokenizerState::CommentState;
          m_cursor--;
        }
        break;
      case TokenizerState::CommentEndState:
        if(ON('>')){
          EMIT_CURRENT_TOKEN;
          m_state = TokenizerState::DataState;
        }else if(ON('!')){
          m_state = TokenizerState::CommentEndBangState;
        }else if(ON('-')){
          append_to_comment_or_character_data(m_tmp_token, '-');
        }else{
          append_to_comment_or_character_data(m_tmp_token, "--");
          m_state = TokenizerState::CommentState;
          m_cursor--;
        }
        break;
      case TokenizerState::CommentEndBangState:
        if(ON('-')){
          append_to_comment_or_character_data(m_tmp_token, "--!");
          m_state = TokenizerState::CommentEndDashState;
        }else if(ON('>')){
          EMIT_CURRENT_TOKEN;
          m_state = TokenizerState::DataState;
        }else{
          append_to_comment_or_character_data(m_tmp_token, "--!");
          m_state = TokenizerState::CommentState;
          m_cursor--;
        }
        break;
      case TokenizerState::DoctypeNameState:
        if(is_tab_lf_ff_space(cp)){
          m_state = TokenizerState::AfterDoctypeNameState;
        }else if(ON('>')){
          EMIT_CURRENT_TOKEN;
          m_state = TokenizerState::DataState;
        }else if(ON('\0')){
          append_to_doctype_token_name(m_tmp_token, REPLACEMENT_STR);
        }else{
          append_to_doctype_token_name(m_tmp_token, isupper(cp) ? (char) tolower(cp) : cp);
        }
        break;
      case TokenizerState::AfterDoctypeNameState:
        if(is_tab_lf_ff_space(cp)){
          ; // pass
        }else if(ON('>')){
          EMIT_CURRENT_TOKEN;
          m_state = TokenizerState::DataState;
        }else{
          const std::string& sub = m_contents.substr(m_cursor, 6);
          if(sub == "PUBLIC"){
            m_state = TokenizerState::AfterDoctypePublicKeywordState;
            m_cursor += 5;
          }else if(sub == "SYSTEM"){
            m_state = TokenizerState::AfterDoctypeSystemKeywordState;
            m_cursor += 5;
          }else{
            set_force_quirks(m_tmp_token);
            m_state = TokenizerState::BogusDoctypeState;
          }
        }
        break;
      case TokenizerState::AfterDoctypePublicKeywordState:
        if(is_tab_lf_ff_space(cp)){
          m_state = TokenizerState::BeforeDoctypePublicIdentifierState;
        }else if(ON('"')){
          // TODO: does not distinguish between empty string and missing
          m_state = TokenizerState::DoctypePublicIdentifierDoubleQuotedState;
        }else if(ON('\'')){
          // TODO: does not distinguish between empty string and missing
          m_state = TokenizerState::DoctypePublicIdentifierSingleQuotedState;
        }else if(ON('>')){
          set_force_quirks(m_tmp_token);
          EMIT_CURRENT_TOKEN;
          m_state = TokenizerState::DataState;
        }else{
          set_force_quirks(m_tmp_token);
          m_state = TokenizerState::BogusDoctypeState;
          m_cursor--;
        }
        break;
      case TokenizerState::BeforeDoctypePublicIdentifierState:
        if(is_tab_lf_ff_space(cp)){
          ; // pass
        }else if(ON('"')){
          m_state = TokenizerState::DoctypePublicIdentifierDoubleQuotedState;
        }else if(ON('\'')){
          m_state = TokenizerState::DoctypePublicIdentifierSingleQuotedState;
        }else if(ON('>')){
          set_force_quirks(m_tmp_token);
          EMIT_CURRENT_TOKEN;
          m_state = TokenizerState::DataState;
        }else{
          set_force_quirks(m_tmp_token);
          m_state = TokenizerState::BogusDoctypeState;
          m_cursor--;
        }
        break;
      case TokenizerState::DoctypePublicIdentifierDoubleQuotedState:
        if(ON('"')){
          m_state = TokenizerState::AfterDoctypePublicIdentifierState;
        }else if(ON('>')){
          set_force_quirks(m_tmp_token);
          EMIT_CURRENT_TOKEN;
          m_state = TokenizerState::DataState;
        }else if(ON('\0')){
          append_to_doctype_token_public_identifier(m_tmp_token, REPLACEMENT_STR);
        }else{
          append_to_doctype_token_public_identifier(m_tmp_token, cp);
        }
        break;
      case TokenizerState::DoctypePublicIdentifierSingleQuotedState:
        if(ON('\'')){
          m_state = TokenizerState::AfterDoctypePublicIdentifierState;
        }else if(ON('>')){
          set_force_quirks(m_tmp_token);
          EMIT_CURRENT_TOKEN;
          m_state = TokenizerState::DataState;
        }else if(ON('\0')){
          append_to_doctype_token_public_identifier(m_tmp_token, REPLACEMENT_STR);
        }else{
          append_to_doctype_token_public_identifier(m_tmp_token, cp);
        }
        break;
      case TokenizerState::AfterDoctypePublicIdentifierState:
        if(is_tab_lf_ff_space(cp)){
          m_state = TokenizerState::BetweenDoctypePublicAndSystemIdentifiersState;
        }else if(ON('>')){
          EMIT_CURRENT_TOKEN;
          m_state = TokenizerState::DataState;
        }else if(ON('"')){
          //TODO: no distinguish between empty string and missing
          m_state = TokenizerState::DoctypeSystemIdentifierDoubleQuotedState;
        }else if(ON('\'')){
          m_state = TokenizerState::DoctypeSystemIdentifierSingleQuotedState;
        }else{
          set_force_quirks(m_tmp_token);
          m_state = TokenizerState::BogusDoctypeState;
          m_cursor--;
        }
        break;
      case TokenizerState::BetweenDoctypePublicAndSystemIdentifiersState:
        if(is_tab_lf_ff_space(cp)){
          ; // pass
        }else if(ON('>')){
          EMIT_CURRENT_TOKEN;
          m_state = TokenizerState::DataState;
        }else if(ON('"')){
          //TODO: no distinguish between empty string and missing
          m_state = TokenizerState::DoctypeSystemIdentifierDoubleQuotedState;
        }else if(ON('\'')){
          m_state = TokenizerState::DoctypeSystemIdentifierSingleQuotedState;
        }else{
          set_force_quirks(m_tmp_token);
          m_state = TokenizerState::BogusDoctypeState;
          m_cursor--;
        }
        break;
      case TokenizerState::AfterDoctypeSystemKeywordState:
        if(is_tab_lf_ff_space(cp)){
          m_state = TokenizerState::BeforeDoctypeSystemIdentifierState;
        }else if(ON('"')){
          //TODO: no distinguish between empty string and missing
          m_state = TokenizerState::DoctypeSystemIdentifierDoubleQuotedState;
        }else if(ON('\'')){
          m_state = TokenizerState::DoctypeSystemIdentifierSingleQuotedState;
        }else if(ON('>')){
          set_force_quirks(m_tmp_token);
          EMIT_CURRENT_TOKEN;
          m_state = TokenizerState::DataState;
        }else{
          set_force_quirks(m_tmp_token);
          m_state = TokenizerState::BogusDoctypeState;
          m_cursor--;
        }
        break;
      case TokenizerState::BeforeDoctypeSystemIdentifierState:
        if(is_tab_lf_ff_space(cp)){
          ; // pass
        }else if(ON('"')){
          m_state = TokenizerState::DoctypeSystemIdentifierDoubleQuotedState;
        }else if(ON('\'')){
          m_state = TokenizerState::DoctypeSystemIdentifierSingleQuotedState;
        }else if(ON('>')){
          set_force_quirks(m_tmp_token);
          EMIT_CURRENT_TOKEN;
          m_state = TokenizerState::DataState;
        }else{
          set_force_quirks(m_tmp_token);
          m_state = TokenizerState::BogusDoctypeState;
          m_cursor--;
        }
        break;
      case TokenizerState::DoctypeSystemIdentifierDoubleQuotedState:
        if(ON('"')){
          m_state = TokenizerState::AfterDoctypeSystemIdentifierState;
        }else if(ON('>')){
          set_force_quirks(m_tmp_token);
          EMIT_CURRENT_TOKEN;
          m_state = TokenizerState::DataState;
        }else if(ON('\0')){
          append_to_doctype_token_system_identifier(m_tmp_token, REPLACEMENT_STR);
        }else{
          append_to_doctype_token_system_identifier(m_tmp_token, cp);
        }
        break;
      case TokenizerState::DoctypeSystemIdentifierSingleQuotedState:
        if(ON('\'')){
          m_state = TokenizerState::AfterDoctypeSystemIdentifierState;
        }else if(ON('>')){
          set_force_quirks(m_tmp_token);
          EMIT_CURRENT_TOKEN;
          m_state = TokenizerState::DataState;
        }else if(ON('\0')){
          append_to_doctype_token_system_identifier(m_tmp_token, REPLACEMENT_STR);
        }else{
          append_to_doctype_token_system_identifier(m_tmp_token, cp);
        }
        break;
      case TokenizerState::AfterDoctypeSystemIdentifierState:
        if(is_tab_lf_ff_space(cp)){
          ; // pass
        }else if(ON('>')){
          EMIT_CURRENT_TOKEN;
          m_state = TokenizerState::DataState;
        }else{
          set_force_quirks(m_tmp_token);
          m_state = TokenizerState::BogusDoctypeState;
          m_cursor--;
        }
        break;
      case TokenizerState::BogusDoctypeState:
        if(ON('>')){
          EMIT_CURRENT_TOKEN;
          m_state = TokenizerState::DataState;
        }else{
          ; // pass
        }
        break;
      case TokenizerState::CdataSectionState:
        if(ON(']')){
          m_state = TokenizerState::CdataSectionBracketState;
        }else{
          EMIT_CHAR_TOKEN(cp);
        }
        break;
      case TokenizerState::CdataSectionBracketState:
        if(ON(']')){
          m_state = TokenizerState::CdataSectionEndState;
        }else{
          EMIT_CHAR_TOKEN(']');
          m_state = TokenizerState::CdataSectionState;
          m_cursor--;
        }
        break;
      case TokenizerState::CdataSectionEndState:
        if(ON(']')){
          EMIT_CHAR_TOKEN(']');
        }else if(ON('>')){
          m_state = TokenizerState::DataState;
        }else{
          EMIT_CHAR_TOKENS("]]");
          m_state = TokenizerState::CdataSectionState;
          m_cursor--;
        }
        break;
      case TokenizerState::CharacterReferenceState:
        m_tmp_buffer = "&";
        if(isalpha(cp) || isnumber(cp)){
          m_state = TokenizerState::NamedCharacterReferenceState;
          m_cursor--;
        }else if(ON('#')){
          m_tmp_buffer.push_back(cp);
          m_state = TokenizerState::NumericCharacterReferenceState;
        }else{
          FLUSH_CODE_POINTS;
          m_state = m_return_state;
          m_cursor--;
        }
        break;
      // todo: issues with initialization of char_replacement_str?
      case TokenizerState::NamedCharacterReferenceState: {
        // TODO: deviates from spec, disallows missing semicolon
        size_t i = 0;
        while(m_contents[m_cursor + i] != ';'){
          m_tmp_buffer.push_back(m_contents[m_cursor + i]);
          i++;
        }
        std::optional<std::string> char_replacement_str = m_named_char_ref.find(m_tmp_buffer);
        if(!char_replacement_str.has_value()){
          FLUSH_CODE_POINTS;
          m_state = TokenizerState::AmbiguousAmpersandState;
        }else{
          m_tmp_buffer = char_replacement_str.value();
          FLUSH_CODE_POINTS;
          m_tmp_buffer = "";
          m_state = m_return_state;
        }
        break;
      }
      case TokenizerState::AmbiguousAmpersandState:
        if(isalpha(cp) || isnumber(cp)){
          if(is_attribute_return_state()){
            append_to_tag_token_attribute_value(m_tmp_token, cp);
          }else{
            EMIT_CHAR_TOKEN(cp);
          }
        }else{
          m_state = m_return_state;
          m_cursor--;
        }
        break;
      // TODO: need to implement
      case TokenizerState::NumericCharacterReferenceState:
      case TokenizerState::HexadecimalCharacterReferenceStartState:
      case TokenizerState::DecimalCharacterReferenceStartState:
      case TokenizerState::HexadecimalCharacterReferenceState:
      case TokenizerState::DecimalCharacterReferenceState:
      case TokenizerState::NumericCharacterReferenceEndState:
        ASSERT_NOT_REACHED;
      default:
        ASSERT_NOT_REACHED;
    }

  }
}

std::optional<size_t> Tokenizer::simd_state_change(const std::string& needles){
  std::string search_str = m_contents.substr(m_cursor, SIMD_SEARCH_SIZE);
  return simd_next_pos(search_str, needles);
}

bool Tokenizer::advance(){
  m_cursor++; 
  return m_cursor < m_contents.size();
}

/*
if current state has transitions that are a subset of SIMD states, return true.
*/
std::string Tokenizer::in_simd_emit_char_state() const {
  switch(m_state) {
    case TokenizerState::DataState:
    case TokenizerState::RcDataState:
      return "<&\r\0"; //TODO: this may not allow checking '\0' properly!!! 
    case TokenizerState::RawTextState:
    case TokenizerState::ScriptDataState:
      return "<\0";
    case TokenizerState::PlainTextState:
      return "\0";
    case TokenizerState::ScriptDataEscapedState:
    case TokenizerState::ScriptDataDoubleEscapedState:
      return "-<\0";
    default:
      return "";
  }

}

void Tokenizer::set_return_state(TokenizerState state) {
  m_return_state = state;
}

bool Tokenizer::is_attribute_return_state(){
  return m_return_state == TokenizerState::AttributeValueDoubleQuotedState ||
         m_return_state == TokenizerState::AttributeValueSingleQuotedState ||
         m_return_state == TokenizerState::AttributeValueUnquotedState;
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

std::vector<Token> Tokenizer::get_tokens(){
  return m_tokens;
}