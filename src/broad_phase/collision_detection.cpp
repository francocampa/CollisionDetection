#include "collision_detection.hpp"
#include "hash_grid.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/geometric.hpp"
#include "raylib.h"
#include <vector>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/component_wise.hpp>

namespace{
    struct GridWrapper{
        HashGrid grid;

        void sync(const std::vector<Object>& objects){
            if (objects.empty())
                return;

            if (grid.needs_update(objects[0].size, objects.size())) 
                grid = HashGrid(objects[0].size, objects.size());
            
            grid.fill(objects);
        }
    };

    static GridWrapper grid(HashGrid(0,0));

    void collision_response(Object& a, Object& b, float dis, float max_vel){
        // Rebote

        glm::vec3 normal = glm::normalize(a.pos - b.pos);
        glm::vec3 relative_vel = a.vel - b.vel;
        float normal_vel = glm::dot(relative_vel, normal);

        if (normal_vel < 0.0f) {
            a.vel = glm::clamp(a.vel - normal_vel * normal, -max_vel, max_vel);
            b.vel = glm::clamp(b.vel + normal_vel * normal, -max_vel, max_vel);
        }

        // Resolver solapamiento

        float penetration = a.size + b.size - dis;

        a.pos += 0.5f * penetration * normal;
        b.pos -= 0.5f * penetration * normal;

        // Marcar colisión

        a.hit();
        b.hit();
    }

    void naive_broad_phase(std::vector<Object>& objects, Object& obj, float max_vel){
        for (Object& other_obj: objects) {
            if (&obj == &other_obj)
                continue;

            float dis = glm::distance(obj.pos, other_obj.pos);
            if(dis < obj.size + other_obj.size)
                collision_response(obj, other_obj, dis, max_vel);
        }
    }

    void grid_broad_phase(std::vector<Object>& objects, Object& obj, float max_vel){
        const QueryResult& res = grid.grid.query(obj.pos, obj.size * 2.0f);

        for (int i = 0; i < res.size; i++) {
            auto& other_obj = objects[res.indices[i]];
            if (&obj == &other_obj)
                continue;

            float dis = glm::distance(obj.pos, other_obj.pos);
            if(dis < obj.size + other_obj.size)
                collision_response(obj, other_obj, dis, max_vel);
        }
    }

    glm::vec3 box_collision_normal(glm::vec3 pos, float radius, glm::vec3 box){
        glm::vec3 res(0);

        if (pos.x < radius) 
            res += glm::vec3(1,0,0);
        if (pos.y < radius) 
            res += glm::vec3(0,1,0);
        if (pos.z < radius) 
            res += glm::vec3(0,0,1);
        if (pos.x > box.x - radius) 
            res += -glm::vec3(1,0,0);
        if (pos.y > box.y - radius) 
            res += -glm::vec3(0,1,0);
        if (pos.z > box.z - radius) 
            res += -glm::vec3(0,0,1);
        
        return glm::compMax(res) != 0 ? glm::normalize(res) : res;
    }

}


void step_simulation(glm::vec3 box, std::vector<Object>& objects, float max_vel, float delta, bool use_grid){
    grid.sync(objects);

    use_grid &= objects.size() > 50;

    for (Object& obj: objects) {
        obj.tick(delta);

        // Static objects
        glm::vec3 wallNormal = box_collision_normal(obj.pos, obj.size, {box.x,box.y,box.z});
        if (glm::dot(wallNormal, wallNormal) > 0.0f &&
            glm::dot(obj.vel, wallNormal) < 0.0f) 
            obj.vel = glm::reflect(obj.vel, wallNormal);
        
        // Dynamic objects
        if(use_grid)
            grid_broad_phase(objects, obj, max_vel);
        else 
            naive_broad_phase(objects, obj, max_vel);

        

        obj.pos += delta * obj.vel / 50.0f; 
    }
}