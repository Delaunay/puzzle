#ifndef PUZZLE_EDITOR_UTILS_HEADER
#define PUZZLE_EDITOR_UTILS_HEADER

#include <list>
#include <string>
#include <cmath> // fmodf

#include <SDL2/SDL_keycode.h>

#include <imgui.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>


#ifdef IM_COL32
#undef IM_COL32
#endif

#define IM_COL32(R,G,B,A)\
    (ImU32(ImU32(A)<<IM_COL32_A_SHIFT) | ImU32(ImU32(B)<<IM_COL32_B_SHIFT) | ImU32(ImU32(G)<<IM_COL32_G_SHIFT) | ImU32(ImU32(R)<<IM_COL32_R_SHIFT))

int get_side(std::string const& name);

template <typename T>
float sgn(T val) {
    return (T(0) < val) - (val < T(0));
}

enum Direction {
    LeftToRight,
    TopToBottom,
    RightToLeft,
    BottomToTop
};

template<typename T>
struct Iterator{
    T s;
    T e;

    Iterator(T s, T e):
        s(s), e(e)
    {}

    T begin() { return s; }
    T end  () { return e; }

    T begin() const { return s; }
    T end  () const { return e; }
};

#endif
