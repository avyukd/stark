#include "test_utils.h"
#include "stark.h"

/*
input: HTML file
- tests multithreaded tokenizer + parser
*/

int main(int argc, char** argv) {
  for(size_t i = 1; i < argc; i++){
    PRINT_TEST_NUM;

    std::string file_name = get_filename_from_arg(argv[i], HTML_SUFFIX);
    std::ifstream file(file_name);

    std::string html_doc = get_html_doc_from_fs(file);

    Stark stark{html_doc, true};
    const DomNode* root = stark.get_root();

    // find all "a" tag
    auto a_tags = root->find_all_by_tag("a");
    for(auto a_tag : a_tags){
      std::cout << a_tag->contents() << std::endl;
    }
  }
}
