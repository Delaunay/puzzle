#include "node-editor.h"

void draw_bezier(ImDrawList* draw_list, ImVec2 p1, ImVec2 p2, ImU32 color){
    if (p1.x > p2.x) {
        std::swap(p1, p2);
    }

    auto offset = ImVec2((p2.x - p1.x + 1) / 2.f , 0);
    auto y_offset = sgn(p2.y - p1.y) * (p2.y - p1.y) / 2.f;

    if (p2.y - p1.y > p2.x - p1.x){
        offset = ImVec2(0, y_offset);
    }

    draw_list->AddBezierCurve(
        p1,
        p1 + offset,
        p2 - offset,
        p2,
        color,
        // Conveyor belt are w=2
        Node::scaling * 2.f);
}
