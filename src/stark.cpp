#include "stark.h"
#include <vector>

Stark::Stark(const std::string& html_doc){
  Tokenizer tokenizer{html_doc};
  tokenizer.run();
  BasicDomParser dom_parser;
  for(const auto& token : tokenizer.get_tokens()){
    dom_parser.consume_token(token);
  }
  m_root = dom_parser.get_root();
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
