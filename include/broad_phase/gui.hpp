#ifndef GUI_HPP
#define GUI_HPP

#include "glm/ext/vector_common.hpp"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include <string>

enum SELECTED_FIELD{
    NO_SPHERE,
    BOX_SIZE
};

inline const Rectangle rec_no_sphere = Rectangle(10,30,100,20);
inline const Rectangle rec_no_sphere_label = Rectangle(115,30,100,20);
inline const Rectangle rec_box_size = Rectangle(10,60,100,20);
inline const Rectangle rec_box_size_label = Rectangle(115,60,100,20);
inline const Rectangle rec_use_grid = Rectangle(10,90,20,20);
inline const Rectangle rec_render = Rectangle(10,120,20,20);

inline void draw_gui(float& box_size, int& no_spheres, bool& use_grid, bool& render){
    static SELECTED_FIELD editing;
    static std::string str_box_size = "";
    static float gui_box_size = box_size;
    static int gui_no_spheres = no_spheres;

    GuiLabel(rec_box_size_label, "Box size");
    if(GuiValueBoxFloat(rec_box_size, "", str_box_size.data(), &gui_box_size, editing == BOX_SIZE))
        editing = BOX_SIZE;
    GuiLabel(rec_no_sphere_label, "Number of spheres");
    if(GuiValueBox(rec_no_sphere, "", &gui_no_spheres, 0, 100000, editing == NO_SPHERE))
        editing = NO_SPHERE;

    GuiCheckBox(rec_use_grid, "Use grid", &use_grid);
    GuiCheckBox(rec_render, "Render", &render);

    if(gui_box_size != box_size)
        box_size = glm::clamp(gui_box_size, 1.0f, 1000.0f);
    if(gui_no_spheres != no_spheres)
        no_spheres = glm::clamp(gui_no_spheres, 2, 100000);

}

#endif