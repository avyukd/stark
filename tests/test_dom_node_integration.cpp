#include <fstream>
#include <iostream>
#include <string>
#include <sstream>

#include "tokenizer.h"
#include "basic_dom_parser.h"

/*
Input: HTML File
Assumes working tokenizer and dom parser.
Tests DOM Node interface for end user.
*/

const std::string input_root_prefix = "../tests/input/";
const std::string html_suffix = ".html";

int main (int argc, char** argv) {
  for(size_t i = 1; i < argc; i++){
    std::cout << "Test " << i << " of " << (argc - 1) << ": " << argv[i] << std::endl;

    std::string file_name = argv[i]; file_name += html_suffix;
    std::ifstream file(input_root_prefix + file_name);

    std::stringstream ss;
    ss << file.rdbuf();
    std::string html_doc = ss.str();

    Tokenizer tokenizer{html_doc};
    tokenizer.run();
    std::vector<Token> tokens = tokenizer.get_tokens();
    BasicDomParser dom_parser;
    for(const auto& token : tokens){
      dom_parser.consume_token(token);
    }

    std::unique_ptr<DomNode> root = dom_parser.get_root();

    // find all "a" tag
    auto a_tags = root->find_all_by_tag("a");
    for(auto a_tag : a_tags){
      std::cout << a_tag->contents() << std::endl;
    }

  }

}