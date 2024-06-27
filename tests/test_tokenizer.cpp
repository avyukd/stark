#include <fstream>
#include <iostream>
#include <string>
#include <sstream>

#include "tokenizer.h"

const std::string html_root_prefix = "../tests/input/";
const std::string html_suffix = ".html";

int main (int argc, char** argv){

  for(size_t i = 1; i < argc; i++){
    std::cout << "Test " << i << " of " << (argc - 1) << ": " << argv[i] << std::endl; 

    std::string file_name = argv[i]; file_name += html_suffix;
    std::ifstream file(html_root_prefix + file_name);

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string contents = buffer.str();

    Tokenizer tokenizer{contents, TokenizerOptions{
      .use_simd = true
    }};
    auto tokens = tokenizer.run();

    for(const Token& token : tokens){
      std::cout << to_string(token) << std::endl;
    }
  }


}
