#include "stark.h"
#include <vector>
#include <thread>

Stark::Stark(
  const std::string& html_doc,
  bool multithreaded
){
  if(!multithreaded){
    Tokenizer tokenizer{html_doc};
    auto tokens = tokenizer.run();
    BasicDomParser dom_parser;
    for(const auto& token : tokens){
      dom_parser.consume_token(token);
    }
    m_root = dom_parser.get_root();
  }else{
    // one thread for parser, one for tokenizer
    Tokenizer tokenizer{html_doc};
    BasicDomParser dom_parser;

    std::thread token_thread(&Stark::consume_tokens, this, std::ref(dom_parser));

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

void Stark::consume_tokens(BasicDomParser& dom_parser) {
  while(true){
    std::vector<Token> tokens = m_token_queue.pop();

    // eof token always single entry {EOF_TOKEN} -- TODO: make this cleaner?
    if(!tokens.empty() && is_eof_token(tokens[0])){
      break;
    }

    for(const Token& token : tokens){
      dom_parser.consume_token(token);
    }
  }
  m_root = dom_parser.get_root();
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
