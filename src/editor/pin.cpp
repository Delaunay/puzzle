#include "pin.h"
#include "node.h"
#include "application/logger.h"


Pin::Pin(char type, bool is_input, int side, int index, int count, Node* parent):
    ID(next_id<std::size_t>()), belt_type(type), is_input(is_input),
    side(side), index(index), count(count), parent(parent)
{}

Pin::Pin(Pin const&& obj) noexcept:
    ID(obj.ID), belt_type(obj.belt_type), side(obj.side), index(obj.index),
    count(obj.count), parent(obj.parent), child(obj.child)
{}


ImVec2 Pin::position() const {
    return parent->slot_position(side, float(index), float(count));
}

std::ostream& operator<<(std::ostream& out, Pin const& pin){
    return out << fmt::format(
               "Pin(ID={}, type={}, input={}, side={}, index={}, count={}, parent={})",
               pin.ID, pin.belt_type, pin.is_input, pin.side, pin.index, pin.count, pin.parent->ID);
}
