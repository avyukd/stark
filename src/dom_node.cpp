#include "dom_node.h"
#include "utils.h"
#include <iostream>
#include <functional>

// DomNode public interface
// TODO: performance issues with having a lambda passed to search std::function? 

void DomNode::init_tag_node(const Token& token){
  m_node_type = DomNode::DomNodeType::ELEMENT_NODE;
  m_element_node.tag_name = token.m_start_or_end_tag.tag_name;

  const auto& token_attrs = token.m_start_or_end_tag.attributes;
  m_element_node.attributes.reserve(token_attrs.size());
  for(const attribute& attribute : token_attrs){
    m_element_node.attributes.push_back(attribute);
  }
  m_element_node.self_closing = token.m_start_or_end_tag.self_closing;
}

DomNode::DomNode(const Token& token, DomNode* parent){
  // should always be start tag node
  init_tag_node(token);
  m_parent = parent;
}

DomNode::DomNode(const std::string& data, DomNode* parent){
  m_node_type = DomNode::DomNodeType::TEXT_NODE;
  m_text_or_comment_node.data = data;
  m_parent = parent;
}

std::vector<const DomNode*> DomNode::find_all_by_tag(const std::string& tag_name) const{
  return search([&tag_name](const DomNode& node){
    return node.m_element_node.tag_name == tag_name;
  });
}

std::vector<const DomNode*> DomNode::search(
  std::function<bool(const DomNode&)> predicate
) const {
  // dfs to find all nodes with tag_name
  std::vector<const DomNode*> nodes;

  std::vector<const DomNode*> s;
  s.push_back(this);
  while(!s.empty()){
    auto node = s.back(); s.pop_back();
    if(predicate(*node))
      nodes.push_back(node);
    for(const auto& child : node->m_children){
      if(child.get() == node){
        std::cout << "cycle detected!" << std::endl;
        continue;
      }
      s.push_back(child.get());
    }
  }
  return nodes;
}

std::string DomNode::contents() const {
  if(m_node_type == DomNodeType::TEXT_NODE){
    return m_text_or_comment_node.data;
  }else if(m_node_type == DomNodeType::ELEMENT_NODE 
          && m_element_node.self_closing)
  {
    return get_tag_str(true, true);
  }

  std::string s = "";
  for(const auto& child : m_children){
    s += child->contents();
  }

  return get_tag_str(true) + s + get_tag_str(false);
}

// DomNode private
std::string DomNode::get_attr_str() const {
  std::string s = "";
  for(const attribute& attr : m_element_node.attributes){
    s += " " + attr.name + "=\"" + attr.value + "\"";
  }
  return s;
}

std::string DomNode::get_tag_str(
  bool is_start, bool is_self_closing
) const {
  if(is_start){
    if(is_self_closing){
      return "<" + m_element_node.tag_name + get_attr_str() + "/>";
    }else{
      return "<" + m_element_node.tag_name + get_attr_str() + ">";
    }
  }else{
    return "</" + m_element_node.tag_name + ">";
  }
}

std::string DomNode::to_string() const {
  const size_t max_len = 50;

  std::string s = "";
  switch(m_node_type){
    case DomNode::DomNodeType::ELEMENT_NODE:
      s += upper_case(m_element_node.tag_name) + ": ";
      for(const attribute& attr : m_element_node.attributes){
        if(s.size() + attr.name.size() + attr.value.size() + 5 > max_len){
          s += " ...";
          break;
        }
        s += " " + attr.name + "=\"" + attr.value + "\",";
      }
      break;
    case DomNode::DomNodeType::TEXT_NODE:
      s += m_text_or_comment_node.data.substr(0, max_len);
      break;
    case DomNode::DomNodeType::COMMENT_NODE:
      s += "<!--" + m_text_or_comment_node.data.substr(0, max_len - 7) + "-->";
      break;
  }
  return s;
}