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
#include <unordered_map>

class BasicDomParser {
  friend class Stark;

  public:
    BasicDomParser(
      std::vector<std::string> cache_by_tag,
      std::vector<std::string> cache_by_class,
      std::vector<std::string> cache_by_id
    );
    void consume_token(const Token&);
    void push_data_str_to_current_node();
    void print_tree(std::ostream& os) const;
    std::unique_ptr<DomNode> get_root();

  private:
    std::unique_ptr<DomNode> m_root;
    DomNode* m_current_node;
    std::string m_data_str;
    bool m_seen_html_tag;

    using cache_t = std::unordered_map<std::string, std::vector<const DomNode*>>;
    std::unique_ptr<cache_t> m_tag_cache;
    std::unique_ptr<cache_t> m_class_cache;
    std::unique_ptr<cache_t> m_id_cache; // todo: ideally should be different type, genericizing for now

    void init_cache(std::unique_ptr<cache_t>&, const std::vector<std::string>& keys);
    void check_and_add_to_cache(DomNode* node);
};