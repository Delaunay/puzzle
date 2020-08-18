#include "node.h"


Node::Node(int building, const ImVec2& pos, int recipe_idx, int rotation):
    ID(next_id<std::size_t>()), building(building), recipe_idx(recipe_idx), rotation(rotation)
{
    assertf(building >= 0, "Node shoudl have a building");

    descriptor = &Resources::instance().buildings[std::size_t(building)];

    for(auto& side: descriptor->layout){
        int pin_side = get_side(side.first);
        std::vector<std::string>& pin_str = side.second;

        std::vector<Pin>& side_pins = pins[std::size_t(get_side(side.first))];
        side_pins.reserve(side.second.size());

        for(int i = 0, n = pin_str.size(); i < n; ++i) {
            auto& p = pin_str[std::size_t(i)];
            assertf(p.size() == 2,
                    "Pin descriptor is of size 2");

            assertf(p[0] == 'C' || p[0] == 'P' || p[0] == 'N',
                    "Pin type should be defined");

            assertf(p[1] == 'I' || p[1] == 'O',
                    "Pin type should be defined");

            side_pins.emplace_back(
                p[0],           // Belt Type
                p[1] == 'I',    // Input
                pin_side,       // Pin Side
                i,              // Pin Index
                n,              // Pin Count on that side
                this);          // Parent
        }
    }
    Pos = pos;
}


void Node::update_size(ImVec2){
    ImVec2 base = {scaling * descriptor->l, scaling *descriptor->w};
    _size = base;
}

ImVec2 Node::size() const {
    switch (Direction(rotation)){
    case LeftToRight:
    case RightToLeft:
        return _size;

    case TopToBottom:
    case BottomToTop:
        return ImVec2(_size.y, _size.x);
    }

    __builtin_unreachable();
}

ImVec2 Node::left_slots(float num, float count) const {
    return ImVec2(
        Pos.x,
        Pos.y + size().y * (float(num + 1)) / (float(count + 1)));
}

ImVec2 Node::right_slots(float num, float count) const {
    return ImVec2(
        Pos.x + size().x,
        Pos.y + size().y * (float(num + 1)) / (float(count + 1)));
}

ImVec2 Node::top_slots(float num, float count) const {
    return ImVec2(
        Pos.x + size().x * (float(num + 1)) / (float(count + 1)),
        Pos.y);
}

ImVec2 Node::bottom_slots(float num, float count) const {
    return ImVec2(
        Pos.x + size().x * (float(num + 1)) / (float(count + 1)),
        Pos.y + size().y);
}

ImVec2 Node::slot_position(int side, float num, float count) const {
    side = (side + rotation) % 4;

    switch (Direction(side)){
    case LeftToRight:
        return left_slots(num, count);

    case RightToLeft:
        return right_slots(num, count);

    case BottomToTop:
        return bottom_slots(num, count);

    case TopToBottom:
        return top_slots(num, count);
    }

    __builtin_unreachable();
}

Recipe* Node::recipe() const {
    if (descriptor == nullptr){
        return nullptr;
    }

    if (recipe_idx < 0){
        return nullptr;
    }

    if (recipe_idx < int(descriptor->recipes.size())){
        return &descriptor->recipes[std::size_t(recipe_idx)];
    }

    return nullptr;
}
