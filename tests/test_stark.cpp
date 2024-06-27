#include <fstream>
#include <iostream>
#include <string>
#include <sstream>

#include "stark.h"

/*
input: HTML file
- tests multithreaded tokenizer + parser
*/

const std::string input_root_prefix = "../tests/input/";
const std::string html_suffix = ".html";

int main(int argc, char** argv) {
  for(size_t i = 1; i < argc; i++){
    std::cout << "Test " << i << " of " << (argc - 1) << ": " << argv[i] << std::endl;

    std::string file_name = argv[i]; file_name += html_suffix;
    std::ifstream file(input_root_prefix + file_name);

    std::stringstream ss;
    ss << file.rdbuf();
    std::string html_doc = ss.str();

    Stark stark{html_doc, true};
    const DomNode* root = stark.get_root();

    // find all "a" tag
    auto a_tags = root->find_all_by_tag("a");
    for(auto a_tag : a_tags){
      std::cout << a_tag->contents() << std::endl;
    }
  }
}
