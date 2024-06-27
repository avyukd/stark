#pragma once
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>

#define PRINT_TEST_NUM std::cout << "Test " << i << " of " << (argc - 1) << ": " << argv[i] << std::endl 

const std::string INPUT_PREFIX = "../tests/input/";
const std::string TOKENS_SUFFIX = ".tokens";
const std::string HTML_SUFFIX = ".html";

std::string get_filename_from_arg(
  std::string arg, std::string suffix, std::string prefix = INPUT_PREFIX
) {
  if(arg.substr(
    arg.find_last_of(".") + 1, arg.length()
  ) == suffix) return arg; // assume full path

  return prefix + arg + suffix;
}

std::string get_html_doc_from_fs(std::ifstream& fs){
  std::stringstream ss;
  ss << fs.rdbuf();
  return ss.str();
}