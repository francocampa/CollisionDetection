#ifndef COLLISION_DETECTION
#define COLLISION_DETECTION

#include "glm/glm.hpp"

struct Object{
    glm::vec3 pos;
    glm::vec3 vel;
    float size;
    glm::vec3 color, original_color = color;
    
    float clock = 0;
    void hit(){
        color = glm::vec3(1.0f);
        clock = 100.0f;
    }
    void tick(float delta_ms){
        if (clock > 0) {
            clock -= delta_ms;
        }
        if (clock <= 0) {
            color = original_color;
        }
    }
};

void step_simulation(glm::vec3 box, std::vector<Object>& objects, float max_vel, float delta, bool use_grid);
#endif