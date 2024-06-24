#pragma once
#include <vector>
#include <memory>
#include "attribute.h"
#include "token.h"

struct DomNode {
  public:
  enum class DomNodeType {
    ELEMENT_NODE,
    TEXT_NODE,
    COMMENT_NODE
  };

  DomNodeType m_node_type;
  DomNode* m_parent;
  std::vector<std::unique_ptr<DomNode>> m_children;
  
  struct {
    std::string tag_name;
    std::vector<attribute> attributes;
  } m_element_node;

  struct {
    std::string data;
  } m_text_or_comment_node;
};

std::unique_ptr<DomNode> construct_tag_node(const Token& token, DomNode* parent){
  auto node = std::make_unique<DomNode>();
  init_tag_node(*node, token);
  node->m_parent = parent;
  return node;
}

void init_tag_node(DomNode& node, const Token& token){
  node.m_node_type = DomNode::DomNodeType::ELEMENT_NODE;
  node.m_element_node.tag_name = token.m_start_or_end_tag.tag_name;

  const auto& token_attrs = token.m_start_or_end_tag.attributes;
  node.m_element_node.attributes.reserve(token_attrs.size());
  for(const attribute& attribute : token_attrs){
    node.m_element_node.attributes.push_back(attribute);
  }
}

std::unique_ptr<DomNode> construct_text_node(const std::string& data, DomNode* parent){
  auto node = std::make_unique<DomNode>();
  node->m_node_type = DomNode::DomNodeType::TEXT_NODE;
  node->m_text_or_comment_node.data = data;
  node->m_parent = parent;
  return node;
}