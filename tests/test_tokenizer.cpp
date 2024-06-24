#include <fstream>
#include <iostream>
#include <string>
#include <sstream>

#include "tokenizer.h"

const std::string html_root_prefix = "../html/";
const std::string html_suffix = ".html";

int main (int argc, char** argv){

  for(size_t i = 1; i < argc; i++){
    std::cout << "Test " << i << "of " << (argc - 1) << ": " << argv[i] << std::endl; 

    std::string file_name = argv[i]; file_name += html_suffix;
    std::ifstream file(html_root_prefix + file_name);

    std::ifstream t("file.txt");
    std::stringstream buffer;
    buffer << t.rdbuf();
    std::string contents = buffer.str();

    Tokenizer tokenizer{contents};
    tokenizer.run();
  }


}