#pragma once
#include "visual_map/visual_map_common.h"
#include "visual_map/visual_map.h"
namespace vm{
    void save_visual_map(VisualMap& map, std::string file_addr);
    void loader_visual_map(VisualMap& map, std::string file_addr);
}