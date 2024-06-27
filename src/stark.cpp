#include "stark.h"
#include <vector>
#include <thread>
#include <optional>

Stark::Stark(
  const std::string& html_doc,
  const StarkOptions& options
) : 
  m_dom_parser{options.cache_by_tag, options.cache_by_class, options.cache_by_id}
{
  if(!options.multi_threaded){
    Tokenizer tokenizer{html_doc};
    auto tokens = tokenizer.run();
    for(const auto& token : tokens){
      m_dom_parser.consume_token(token);
    }
    m_root = m_dom_parser.get_root();
  }else{
    // one thread for parser, one for tokenizer
    Tokenizer tokenizer{html_doc};

    std::thread token_thread(&Stark::consume_tokens, this);

    while(true){
      auto tokens = tokenizer.produce();
      
      // if is_eof_token, will always be single entry vector with {EOF_TOKEN}
      bool is_eof = (!tokens.empty() && is_eof_token(tokens[0]));
      m_token_queue.push(std::move(tokens));

      if(is_eof){
        break;
      }
    }

    token_thread.join();
  }
}

void Stark::consume_tokens() {
  while(true){
    std::vector<Token> tokens = m_token_queue.pop();

    // eof token always single entry {EOF_TOKEN} -- TODO: make this cleaner?
    if(!tokens.empty() && is_eof_token(tokens[0])){
      break;
    }

    for(const Token& token : tokens){
      m_dom_parser.consume_token(token);
    }
  }
  m_root = m_dom_parser.get_root();
}

std::optional<std::vector<const DomNode*>> Stark::try_to_find_in_cache(
  const std::string& key, BasicDomParser::cache_t* cache
) const {
  if(cache){
    auto it = cache->find(key);
    if(it != cache->end()){
      return it->second;
    }else{
      return {};
    }
  }
  return std::nullopt;
}

std::vector<const DomNode*> Stark::find_all_by_class(const std::string& class_name) const {
  auto opt_res = try_to_find_in_cache(class_name, m_dom_parser.m_class_cache.get());
  return opt_res.value_or(m_root->find_all_by_class(class_name));
}

std::vector<const DomNode*> Stark::find_by_id(const std::string& id) const {
  auto opt_res = try_to_find_in_cache(id, m_dom_parser.m_id_cache.get());
  return opt_res.value_or(m_root->find_by_id(id));
}
std::vector<const DomNode*> Stark::find_all_by_tag(const std::string& tag_name) const {
  auto opt_res = try_to_find_in_cache(tag_name, m_dom_parser.m_tag_cache.get());
  return opt_res.value_or(m_root->find_all_by_tag(tag_name));
}


const DomNode* Stark::get_root() const {
  return m_root.get();
}

// std::vector<const DomNode* const> Stark::find_all(const std::string& tag_name) const {
//   // dfs to find all nodes with tag_name
//   std::vector<const DomNode* const> nodes;
//   nodes.push_back(m_root.get());
//   while(!nodes.empty()){
//     auto node = nodes.back(); nodes.pop_back();
//     if(node->m_element_node.tag_name == tag_name)
//       nodes.push_back(node);
//     for(const auto& child : node->m_children)
//       nodes.push_back(child.get());
//   }
//   return nodes;
// }
