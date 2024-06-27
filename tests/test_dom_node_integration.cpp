#include "test_utils.h"
#include "tokenizer.h"
#include "basic_dom_parser.h"

/*
Input: HTML File
Assumes working tokenizer and dom parser.
Tests DOM Node interface for end user.
*/

int main (int argc, char** argv) {
  for(size_t i = 1; i < argc; i++){
    PRINT_TEST_NUM;

    std::string fn = get_filename_from_arg(argv[i], HTML_SUFFIX);
    std::ifstream file(fn);

    std::string html_doc = get_html_doc_from_fs(file);

    Tokenizer tokenizer{html_doc};
    auto tokens = tokenizer.run();
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