#include "dom_node.h"
#include "utils.h"

void init_tag_node(DomNode& node, const Token& token){
  node.m_node_type = DomNode::DomNodeType::ELEMENT_NODE;
  node.m_element_node.tag_name = token.m_start_or_end_tag.tag_name;

  const auto& token_attrs = token.m_start_or_end_tag.attributes;
  node.m_element_node.attributes.reserve(token_attrs.size());
  for(const attribute& attribute : token_attrs){
    node.m_element_node.attributes.push_back(attribute);
  }
}


std::unique_ptr<DomNode> construct_tag_node(const Token& token, DomNode* parent){
  auto node = std::make_unique<DomNode>();
  init_tag_node(*node, token);
  node->m_parent = parent;
  return node;
}

std::unique_ptr<DomNode> construct_text_node(const std::string& data, DomNode* parent){
  auto node = std::make_unique<DomNode>();
  node->m_node_type = DomNode::DomNodeType::TEXT_NODE;
  node->m_text_or_comment_node.data = data;
  node->m_parent = parent;
  return node;
}

std::string to_string(const DomNode& node) {
  const size_t max_len = 50;

  std::string s;
  switch(node.m_node_type){
    case DomNode::DomNodeType::ELEMENT_NODE:
      s += upper_case(node.m_element_node.tag_name) + ": ";
      for(const attribute& attr : node.m_element_node.attributes){
        if(s.size() + attr.name.size() + attr.value.size() + 5 > max_len){
          s += " ...";
          break;
        }
        s += " " + attr.name + "=\"" + attr.value + "\",";
      }
      break;
    case DomNode::DomNodeType::TEXT_NODE:
      s += node.m_text_or_comment_node.data.substr(0, max_len);
      break;
    case DomNode::DomNodeType::COMMENT_NODE:
      s += "<!--" + node.m_text_or_comment_node.data.substr(0, max_len - 7) + "-->";
      break;
  }
  return s;
}