#include "test_utils.h"
#include "basic_dom_parser.h"

/*
TODO: 
- meta and link tags are self-closing -- should this be marked by tokenizer or is it an error on dom side? either way parser doesn't handle
- think its probably not self closing (tokenizer is fine), and parser should handle by knowing link and meta don't require a close tag in html 5...
- link and meta don't have content so that is one workaround
*/
void strip_comma(std::string& s){
  s.pop_back();
}

std::vector<attribute> parse_attributes_from_string(std::string& attribute_str){
  std::vector<attribute> attributes;
  for(size_t i = 0; i < attribute_str.size(); i++){
    if(attribute_str[i] == '{'){
      i += sizeof("name: ");
      size_t j = i; while(attribute_str[j] != ','){ j++; }
      std::string name = attribute_str.substr(i, j - i);
      i = j + 1;

      i += sizeof("value: ");
      j = i; while(attribute_str[j] != '}'){ j++; }
      std::string value = attribute_str.substr(i, j - i);
      i = j;

      attributes.push_back({name, value});
    }
  }
  return attributes;
}

std::vector<Token> get_tokens_from_test_input(std::ifstream& fs){
  std::vector<Token> tokens;
  
  std::string line;
  while(std::getline(fs, line)){
    if(line == "") continue; // skip empty lines

    std::stringstream ss(line);
    std::string token_type;
    ss >> token_type; strip_comma(token_type);

    if(token_type == "CHARACTER"){
      std::string _; ss >> _; // junk
      char value; ss >> value;
      if(ss.eof()) value = ' '; // TODO: not clean, hack to handle space character
      tokens.push_back(construct_character_token(value));
      continue;
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

    Token t = ((token_type == "START_TAG") ? construct_start_tag_token(tag_name) : construct_end_tag_token(tag_name));
    if(self_closing) set_tag_self_closing_flag(t);
    t.m_start_or_end_tag.attributes = attributes;
    tokens.push_back(std::move(t));
  }
  return tokens;
}

int main (int argc, char** argv){

  for(size_t i = 1; i < argc; i++){
    PRINT_TEST_NUM;
    std::string fn = get_filename_from_arg(argv[i], TOKENS_SUFFIX);
    std::ifstream file(fn);

    std::vector<Token> tokens = get_tokens_from_test_input(file);
    std::cout << "Num tokens: " << tokens.size() << std::endl;
    BasicDomParser parser;
    for(auto& token : tokens){
      parser.consume_token(token);
    }

    parser.print_tree(std::cout);

  }
}


