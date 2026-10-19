#include "glm/ext/quaternion_geometric.hpp"
#include "glm/ext/vector_float3.hpp"
#include "raylib.h"
#include "raymath.h"
#include <vector>
#include "collision_detection.hpp"

#define RLIGHTS_IMPLEMENTATION
#include "rlights.h"

#include "rlgl.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/component_wise.hpp>

#include "gui.hpp"

Color gen_random_color(){
    return Color(GetRandomValue(0, 255), GetRandomValue(0, 255), GetRandomValue(0, 255), 1);
}

Shader load_shaders(){
    // Load lighting shader
    Shader shader = LoadShader("resources/shaders/mid_narrow/lighting.vs",
                               "resources/shaders/mid_narrow/lighting.fs");
    // Get shader locations
    shader.locs[SHADER_LOC_MATRIX_MVP] = GetShaderLocation(shader, "mvp");
    shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(shader, "viewPos");
    shader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocationAttrib(shader, "instanceTransform");
    // Set shader value: ambient light level
    int ambientLoc = GetShaderLocation(shader, "ambient");
    SetShaderValue(shader, ambientLoc, (float[4]){ 0.2f, 0.2f, 0.2f, 1.0f }, SHADER_UNIFORM_VEC4);

    // Create one light
    CreateLight(LIGHT_DIRECTIONAL, (Vector3){ 5.0f, 5.0f, 5.0f }, Vector3Zero(), WHITE, shader);

    return shader;
}

BoundingBox bbox_to_BBox(const BBox& bbox){
    Vector3 min(bbox.min.values[0],
                bbox.min.values[1],
                bbox.min.values[2]);
    Vector3 max(bbox.max.values[0],
                bbox.max.values[1],
                bbox.max.values[2]);
    return BoundingBox(min,max);
}

void draw_bvh(const Bvh& bvh){
    for (const Node& node: bvh.nodes) {
        if (!node.is_leaf()) 
            continue;

        DrawBoundingBox(bbox_to_BBox(node.get_bbox()), RED);
    }
}

struct Dragging{
    Object* obj = nullptr;
    RayCollision collision { false, 1000.0f };
    glm::vec3 front, up, right;
};

void draw_metrics(StepMetrics metrics){
    DrawText(TextFormat("Objetos intersecando   : %d", metrics.object_collisions), 10, GetScreenHeight() - 90, 24, WHITE);
    DrawText(TextFormat("Primitivas testeadas: %d", metrics.primitive_tests), 10, GetScreenHeight() - 60, 24, WHITE);
    DrawText(TextFormat("Primitivas intersecando: %d", metrics.primitive_collisions), 10, GetScreenHeight() - 30, 24, WHITE);
}

void init_objects(auto& objects, const Shader& shader){
    Mesh sphere = GenMeshSphere(1.0f, 10, 10);
    Mesh sphere2 = GenMeshSphere(1.0f, 10, 10);

    Model model1 = LoadModelFromMesh(sphere);
    model1.materials[0].shader = shader;
    Model model2 = LoadModelFromMesh(sphere2);
    model2.materials[0].shader = shader;

    objects.push_back(Object(model1, glm::vec3(0,0,0), BLUE));
    objects[0].build_bvh();
    objects.push_back(Object(model2, glm::vec3(1.0f,0,0), GREEN));
    objects[1].build_bvh();
}

void handle_event(GUI_EVENT event, std::vector<Object>& objects, Shader shader){
    switch (event) {
        case ADD_SPHERE:{
            Model model = LoadModelFromMesh(GenMeshSphere(1, 10, 10));
            model.materials[0].shader = shader;

            objects.push_back(Object(model, glm::vec3(0), gen_random_color()));
            objects[objects.size()-1].build_bvh();
        }
        break;
        case ADD_SUZANNE:{
            Model suzanne = LoadModel("resources/obj/suzanne.obj");
            suzanne.materials[0].shader = shader;

            objects.push_back(Object(suzanne, glm::vec3(0), gen_random_color()));
            objects[objects.size()-1].build_bvh();
        }
        break;
        case ADD_DRAGON:{
            Model dragon = LoadModel("resources/obj/stanford_dragon_low.obj");
            dragon.materials[0].shader = shader;

            objects.push_back(Object(dragon, glm::vec3(0), gen_random_color()));
            objects[objects.size()-1].build_bvh();
        }
        break;
        case NONE:
            return;
    }
}

int main(void)
{
    SetRandomSeed(2);
    InitWindow(800, 450, "mid-narrow phase example");
    SetMousePosition(GetScreenWidth() / 2, GetScreenHeight() / 2);
    bool showMessageBox = false;

    Camera3D camera;
    camera.position = (Vector3){ 5.0f, 3.0f, 5.0f }; 
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };          
    camera.fovy = 45.0f;       
    camera.projection = CAMERA_PERSPECTIVE;

    bool render = true;
    bool render_ui = true;
    bool use_bvh = false;
    bool draw_all_bvh = false;
    bool draw_tested_bvh = false;

    DisableCursor();

    std::vector<Object> objects;

    auto material = LoadMaterialDefault();

    Shader shader = load_shaders();
    material.shader = shader;

    int color_loc = GetShaderLocationAttrib(shader, "instanceColor");
    int transform_loc = GetShaderLocationAttrib(shader, "instanceTransform");

    bool free_cam = true;

    // init_objects(objects, shader);

    Dragging drag;
    bool drag_mouse_down = false;
    
    while (!WindowShouldClose())
    {
        UpdateCamera(&camera, free_cam ? CAMERA_FREE : 0);
        float cameraPos[3] = { camera.position.x, camera.position.y, camera.position.z };
        SetShaderValue(shader, shader.locs[SHADER_LOC_VECTOR_VIEW], cameraPos, SHADER_UNIFORM_VEC3);
        
        float frame_time = GetFrameTime(); 
        float delta = frame_time * 1000.0f; 

        if (IsKeyPressed(KEY_Z) ||
            IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            free_cam ? EnableCursor() : DisableCursor();
            free_cam = !free_cam;
        }

        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && !drag_mouse_down) {
            drag_mouse_down = true;
            Ray mouse_ray = GetScreenToWorldRay(GetMousePosition(), camera);
            for (Object& obj : objects) {
                RayCollision hit = GetRayCollisionBox(mouse_ray, bbox_to_BBox(obj.bvh.nodes[0].get_bbox()));
                if (hit.hit && hit.distance < drag.collision.distance) {
                    drag.collision = hit;
                    drag.obj = &obj;
                }
            }
            if (drag.obj != nullptr) {
                drag.front = glm::vec3(mouse_ray.direction.x, mouse_ray.direction.y, mouse_ray.direction.z);
                drag.right = glm::normalize(glm::cross(drag.front, { 0,1,0 }));
				drag.up = glm::normalize(glm::cross(drag.right, drag.front));
            }
        }else if(IsMouseButtonDown(MOUSE_BUTTON_LEFT) && drag.obj){
            Vector2 mouse_move = GetMouseDelta();
            if (mouse_move.x != 0 || mouse_move.y != 0) {
                drag.obj->pos += (mouse_move.x * drag.right - mouse_move.y * drag.up) / 20.0f; // El 20 salió del corazón
                drag.obj->build_bvh();
            }
            float wheel = GetMouseWheelMove();
            if (wheel != 0.0f) {
                drag.obj->pos += wheel * drag.front / 5.0f; // El 5 salió del corazón
                drag.obj->build_bvh();
            }
        }else if(IsMouseButtonUp(MOUSE_BUTTON_LEFT)){
            drag_mouse_down = false;
            drag = Dragging();
        }

        if (IsKeyPressed(KEY_H)){
            render_ui = !render_ui;
        }

        BeginDrawing();
            ClearBackground(BLACK);
            BeginMode3D(camera);
                if(render){
                    DrawLine3D(Vector3(-100.0f, 0.0f, 0.0f), Vector3(100.0f, 0.0f, 0.0f), RED);
                    DrawLine3D(Vector3(0.0f, -100.0f, 0.0f), Vector3(0.0f, 100.0f, 0.0f), GREEN);
                    DrawLine3D(Vector3(0.0f, 0.0f, -100.0f), Vector3(0.0f, 0.0f, 100.0f), BLUE);

                    for (const Object& obj : objects) {
                        DrawModel(obj.model, Vector3(obj.pos.x, obj.pos.y, obj.pos.z), 1.0f, obj.color);
                        if (drag.obj == &obj) 
                            DrawBoundingBox(bbox_to_BBox(obj.bvh.nodes[0].get_bbox()), YELLOW);
                        
                        if(draw_all_bvh)
                            draw_bvh(obj.bvh);
                    }
                }
                StepMetrics metrics = step_simulation(objects, use_bvh, draw_tested_bvh);
            EndMode3D();
            DrawFPS(10, 10);
            if(render_ui){
                GUI_EVENT event = draw_gui(use_bvh, draw_all_bvh, draw_tested_bvh, render);
                handle_event(event, objects, shader);
            }
            draw_metrics(metrics);
        EndDrawing();

        if (GetFrameTime() > 1.0f / 3.0f) {
            use_bvh = true;
        }
    }

    UnloadShader(shader);

    CloseWindow();

    return 0;
}