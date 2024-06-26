#pragma once
#include <vector>
#include <memory>
#include "attribute.h"
#include "token.h"

class DomNode {
  friend class BasicDomParser;
  friend class Stark;

  // TODO: make the below functions part of class definition
  friend std::string to_string(const DomNode& node);

  public:

  DomNode() = default;
  DomNode(const Token& token, DomNode* parent);
  DomNode(const std::string& data, DomNode* parent);

  std::vector<const DomNode*> find_all_by_tag(const std::string&) const;
  std::vector<const DomNode*> search(
    std::function<bool(const DomNode&)> predicate
  ) const;
  
  // Node + children as HTML string
  std::string contents() const;

  // the Text contents of the node (only text nodes)
  std::string text() const;

  private:
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

  std::string get_tag_str(bool, bool = false) const;
  std::string get_attr_str() const;
  void init_tag_node(const Token& token);

};


void init_tag_node(DomNode& node, const Token& token);
std::string to_string(const DomNode& node);