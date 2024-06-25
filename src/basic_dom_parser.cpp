#include "basic_dom_parser.h"
#include <queue>
#include <ostream>

BasicDomParser::BasicDomParser() : 
  m_root{std::make_unique<DomNode>()},
  m_current_node{m_root.get()},
  m_data_str{""},
  m_seen_html_tag{false}
{}

void BasicDomParser::push_data_str_to_current_node(){
  if(m_data_str != ""){
    m_current_node->m_children.push_back(construct_text_node(m_data_str, m_current_node));
    m_data_str = "";
  }
}

void BasicDomParser::consume_token(const Token& token){
  switch(token.token_type){
    case Token::TokenType::DOCTYPE:
      break;
    case Token::TokenType::CHARACTER:
      m_data_str += token.m_comment_or_character.data;
      break;
    case Token::TokenType::COMMENT:
      break;
    case Token::TokenType::START_TAG:
      if(token.m_start_or_end_tag.tag_name == "html" && !m_seen_html_tag){
        m_seen_html_tag = true;
        init_tag_node(*m_current_node, token);
      } else {
        if(!m_seen_html_tag) [[unlikely]] {
          m_seen_html_tag = true;
          init_tag_node(*m_current_node, construct_start_tag_token("html"));
        }else{
          push_data_str_to_current_node();
        }

        auto tag_node = construct_tag_node(token, m_current_node); auto tmp = tag_node.get();
        m_current_node->m_children.push_back(std::move(tag_node));
        m_current_node = tmp;
      }
      break;
    case Token::TokenType::END_TAG:
      if(token.m_start_or_end_tag.tag_name == "html"){
        if(!m_seen_html_tag) [[unlikely]] {
          throw std::runtime_error("Unexpected html end tag.");
        }else{
          push_data_str_to_current_node();
          m_current_node = m_root.get();
          return;
        }
      }

      push_data_str_to_current_node();

      // todo: more informative error msg
      if(m_current_node->m_element_node.tag_name != token.m_start_or_end_tag.tag_name)
        throw std::runtime_error("Mismatched start and end tags: "
          + m_current_node->m_element_node.tag_name + " " 
          + token.m_start_or_end_tag.tag_name);
      
      m_current_node = m_current_node->m_parent;
      break;
  };
}

void BasicDomParser::print_tree(std::ostream& os) const {
  // bfs with depth
  std::queue<std::pair<DomNode*, size_t>> q;
  q.push({m_root.get(), 0});

  while(!q.empty()){
    auto [node, depth] = q.front(); q.pop();
    for(size_t i = 0; i < depth; i++) os << '\t';
    os << to_string(*node) << '\n';
    for(auto& child : node->m_children){
      q.push({child.get(), depth + 1});
    }
  }
}