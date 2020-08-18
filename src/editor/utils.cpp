#include "utils.h"

int get_side(std::string const& name){
    if (name == "left")   return 0;
    if (name == "top")    return 1;
    if (name == "right")  return 2;
    if (name == "bottom") return 3;
    return 0;
}
