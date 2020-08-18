#ifndef PUZZLE_EDITOR_BRUSH_HEADER
#define PUZZLE_EDITOR_BRUSH_HEADER

#include "config.h"

#include <vector>

// Building brush
// Save the config of the building we want to build
struct Brush {
    int       building =  0;    // Building Descriptor Index
    int       recipe   = -1;    // Recipe Descriptor Index
    int       rotation =  0;    // Rotation

    std::vector<const char*> building_names; // Names of all available buildings
    std::vector<const char*> recipe_names;   // Names of all avaiable recipes for a given building

    Building* building_descriptor(){
        if (building < 0)
            return nullptr;

        return &Resources::instance().buildings[std::size_t(building)];
    }

    Recipe* recipe_descriptor(){
        if (recipe < 0)
            return nullptr;

        Building* building = building_descriptor();
        if (!building)
            return nullptr;

        return &building->recipes[std::size_t(recipe)];
    }

    void set(int b, int r, int rot = 0){
        building = b;
        rotation = rot;

        if (b >= 0){
            recipe_names = building_descriptor()->recipe_names();
        } else {
            recipe_names.clear();
        }

        recipe = r;
    }
};

#endif
