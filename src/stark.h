#pragma once
#include "tokenizer.h"
#include "basic_dom_parser.h"
#include "ts_queue.h"

#include <string>

class Stark {
  public:
    Stark(const std::string&, bool = false);
    const DomNode* get_root() const;
  
  private:
    std::unique_ptr<DomNode> m_root; 
    ThreadSafeQueue<std::vector<Token>> m_token_queue;

    void consume_tokens(BasicDomParser& dom_parser);
};