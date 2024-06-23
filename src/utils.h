#pragma once

bool is_tab_lf_ff_space(char c){
  return c == '\t' || c == '\n' || c == '\f' || isspace(c);
}