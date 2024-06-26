#pragma once
#include "tokenizer.h"
#include "basic_dom_parser.h"

#include <string>

class Stark {
  public:
    Stark(const std::string&);
    // std::vector<const DomNode* const> find_all(const std::string&) const;
  
  private:
    std::unique_ptr<DomNode> m_root;  
};