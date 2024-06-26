/*
This is a non-spec compliant DOM parser. 
It builds a tree from tags and text nodes.
Input is a stream of tokens.

TODO:
account for self closing tags
*/
#pragma once
#include "token.h"
#include "dom_node.h"

class BasicDomParser {
  public:
    BasicDomParser();
    void consume_token(const Token&);
    void push_data_str_to_current_node();
    void print_tree(std::ostream& os) const;
    std::unique_ptr<DomNode> get_root();

  private:
    std::unique_ptr<DomNode> m_root;
    DomNode* m_current_node;
    std::string m_data_str;
    bool m_seen_html_tag;
};