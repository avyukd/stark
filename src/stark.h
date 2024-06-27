#pragma once
#include "tokenizer.h"
#include "basic_dom_parser.h"
#include "ts_queue.h"

#include <string>

struct StarkOptions {
  std::vector<std::string> cache_by_tag = {};
  std::vector<std::string> cache_by_class = {};
  std::vector<std::string> cache_by_id = {};
  bool multi_threaded = false;
};

class Stark {
  public:
    Stark(
      const std::string&, 
      const StarkOptions& = StarkOptions{}
    );
    const DomNode* get_root() const;
    
    std::vector<const DomNode*> find_all_by_class(const std::string&) const;
    std::vector<const DomNode*> find_by_id(const std::string&) const;
    std::vector<const DomNode*> find_all_by_tag(const std::string&) const;
  
  private:
    std::unique_ptr<DomNode> m_root; 
    ThreadSafeQueue<std::vector<Token>> m_token_queue;
    BasicDomParser m_dom_parser;

    void consume_tokens();

    std::optional<std::vector<const DomNode*>> try_to_find_in_cache(
      const std::string&, BasicDomParser::cache_t*
    ) const;
};