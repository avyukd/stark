#include "tag_utils.h"

bool is_void_tag(const std::string& tag_name) {
  return tag_name == AREA_TAG ||
          tag_name == BASE_TAG ||
          tag_name == BR_TAG ||
          tag_name == COL_TAG ||
          tag_name == COMMAND_TAG ||
          tag_name == EMBED_TAG ||
          tag_name == HR_TAG ||
          tag_name == IMG_TAG ||
          tag_name == INPUT_TAG ||
          tag_name == KEYGEN_TAG ||
          tag_name == LINK_TAG ||
          tag_name == META_TAG ||
          tag_name == PARAM_TAG ||
          tag_name == SOURCE_TAG ||
          tag_name == TRACK_TAG ||
          tag_name == WBR_TAG;
}