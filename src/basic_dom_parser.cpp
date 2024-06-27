#include "basic_dom_parser.h"
#include "tag_utils.h"
#include "utils.h"
#include "constants.h"

#include <queue>
#include <ostream>

BasicDomParser::BasicDomParser(
  std::vector<std::string> cache_by_tag,
  std::vector<std::string> cache_by_class,
  std::vector<std::string> cache_by_id
) : 
  m_root{std::make_unique<DomNode>()},
  m_current_node{m_root.get()},
  m_data_str{""},
  m_seen_html_tag{false},
  m_tag_cache{},
  m_class_cache{},
  m_id_cache{}
{
  if(!cache_by_tag.empty()) init_cache(m_tag_cache, cache_by_tag);
  if(!cache_by_class.empty()) init_cache(m_class_cache, cache_by_class);
  if(!cache_by_id.empty()) init_cache(m_id_cache, cache_by_id);
}

void BasicDomParser::init_cache(std::unique_ptr<cache_t>& cache, const std::vector<std::string>& keys){
  cache = std::make_unique<cache_t>();
  for(const auto& key : keys){
    cache->insert({key, {}});
  }
}

void BasicDomParser::push_data_str_to_current_node(){
  if(!std::all_of(m_data_str.begin(), m_data_str.end(), isspace)){
    trim(m_data_str);
    m_current_node->m_children.emplace_back(
      new DomNode(m_data_str, m_current_node) // text nodes don't need to be added to cache
    );
    m_data_str = "";
  }
}

void BasicDomParser::check_and_add_to_cache(DomNode* node){
  for(auto cache : {m_tag_cache.get(), m_class_cache.get(), m_id_cache.get()}){
    if(cache){
      auto it = cache->find(node->m_element_node.tag_name);
      if(it != cache->end()){
        it->second.push_back(node);
      }
    }
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
      if(token.m_start_or_end_tag.tag_name == HTML_TAG && !m_seen_html_tag){
        m_seen_html_tag = true;
        m_current_node->init_tag_node(token);
        m_data_str = ""; // ignore any data before html tag
      } else {
        if(!m_seen_html_tag) [[unlikely]] {
          m_seen_html_tag = true;
          m_current_node->init_tag_node(construct_start_tag_token(HTML_TAG));
        }else{
          push_data_str_to_current_node();
        }

        auto tag_node_uq = std::make_unique<DomNode>(token, m_current_node);
        auto tag_node_ptr = tag_node_uq.get();
        check_and_add_to_cache(tag_node_ptr);

        if(is_void_tag(tag_node_ptr->m_element_node.tag_name))
          tag_node_ptr->m_element_node.self_closing = true;

        m_current_node->m_children.push_back(std::move(tag_node_uq));

        if(!tag_node_ptr->m_element_node.self_closing)
          m_current_node = tag_node_ptr;
      }
      break;
    case Token::TokenType::END_TAG:
      if(token.m_start_or_end_tag.tag_name == HTML_TAG){
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
  // dfs with depth
  std::vector<std::pair<DomNode*, size_t>> s;
  s.push_back({m_root.get(), 0});

  while(!s.empty()){
    auto [node, depth] = s.back(); s.pop_back();
    for(size_t i = 0; i < depth; i++) os << '\t';
    os << node->to_string() << '\n';
    for(auto& child : node->m_children){
      s.push_back({child.get(), depth + 1});
    }
  }
}

std::unique_ptr<DomNode> BasicDomParser::get_root(){
  return std::move(m_root);
}