#ifndef GUI_HPP
#define GUI_HPP

#include "glm/ext/vector_common.hpp"
#include "raylib.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

enum SELECTED_FIELD{
    NO_SPHERE,
    BOX_SIZE
};

enum GUI_EVENT{
    ADD_SPHERE,
    ADD_SUZANNE,
    ADD_DRAGON,
    NONE
};

inline const Rectangle rec_add_sphere = Rectangle(10,30,200,20);
inline const Rectangle rec_add_suzanne = Rectangle(220,30,200,20);
inline const Rectangle rec_add_dragon = Rectangle(430,30,200,20);
inline const Rectangle rec_use_bvh = Rectangle(10,90,20,20);
inline const Rectangle rec_draw_bvh = Rectangle(10,120,20,20);
inline const Rectangle rec_tested_bvh = Rectangle(100,120,20,20);
inline const Rectangle rec_render = Rectangle(10,150,20,20);

inline GUI_EVENT draw_gui(bool& use_bvh, bool& draw_all_bvh, bool& draw_tested_bvh, bool& render){
    static SELECTED_FIELD editing;
    
    GuiCheckBox(rec_use_bvh, "Use bvh", &use_bvh);
    GuiCheckBox(rec_draw_bvh, "Draw bvh", &draw_all_bvh);
    GuiCheckBox(rec_tested_bvh, "Draw tested leafs", &draw_tested_bvh);
    GuiCheckBox(rec_render, "Render", &render);

    if (GuiButton(rec_add_sphere, "Add a sphere")) {
        return ADD_SPHERE;
    }

    if (GuiButton(rec_add_suzanne, "Add a suzanne")) {
        return ADD_SUZANNE;
    }

    if (GuiButton(rec_add_dragon, "Add a dragon")) {
        return ADD_DRAGON;
    }

    return NONE;
}

#endif