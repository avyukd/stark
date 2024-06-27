#include "test_utils.h"
#include "tokenizer.h"


int main (int argc, char** argv){

  for(size_t i = 1; i < argc; i++){
    PRINT_TEST_NUM;

    std::string file_name = get_filename_from_arg(argv[i], HTML_SUFFIX);
    std::ifstream file(file_name);

    std::string contents = get_html_doc_from_fs(file);

    Tokenizer tokenizer{contents, TokenizerOptions{
      .use_simd = true
    }};
    auto tokens = tokenizer.run();

    for(const Token& token : tokens){
      std::cout << to_string(token) << std::endl;
    }
  }


}
