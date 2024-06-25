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
    bool self_closing;
  } m_element_node;

  struct {
    std::string data;
  } m_text_or_comment_node;
};


void init_tag_node(DomNode& node, const Token& token);
std::unique_ptr<DomNode> construct_tag_node(const Token& token, DomNode* parent);
std::unique_ptr<DomNode> construct_text_node(const std::string& data, DomNode* parent);
std::string to_string(const DomNode& node);