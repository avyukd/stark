#include <fstream>
#include <iostream>
#include <string>
#include <sstream>

#include "basic_dom_parser.h"

const std::string html_root_prefix = "../tests/input/";
const std::string tokens_suffix = ".tokens";

void strip_comma(std::string& s){
  s.pop_back();
}

std::vector<attribute> parse_attributes_from_string(std::string& attribute_str){
  // todo implement
}

std::vector<Token> get_tokens_from_test_input(std::ifstream& fs){
  std::vector<Token> tokens;
  
  std::string line;
  while(std::getline(fs, line)){
    std::stringstream ss(line);
    std::string token_type;
    ss >> token_type; strip_comma(token_type);

    if(token_type == "CHARACTER"){
      std::string _; ss >> _; // junk
      char value; ss >> value;
      tokens.push_back(construct_character_token(value));
    }

    // start tag or end tag
    std::string _; ss >> _; // skip over tag name
    std::string tag_name; ss >> tag_name; strip_comma(tag_name);
    
    ss >> _; // skip over self closing flag
    std::string self_closing_str; ss >> self_closing_str; strip_comma(self_closing_str);
    bool self_closing = self_closing_str == "0" ? false : true;

    ss >> _; // skip over attributes
    std::string attribute_str; std::getline(ss, attribute_str);
    std::vector<attribute> attributes = parse_attributes_from_string(attribute_str);
  }
}

int main (int argc, char** argv){

  for(size_t i = 1; i < argc; i++){
    std::cout << "Test " << i << " of " << (argc - 1) << ": " << argv[i] << std::endl; 

    std::string file_name = argv[i]; file_name += tokens_suffix;
    std::ifstream file(html_root_prefix + file_name);

    }
  }


}
