#include "glm/ext/quaternion_geometric.hpp"
#include "glm/ext/vector_float3.hpp"
#include "raylib.h"
#include "raymath.h"
#include <cstdio>
#include <vector>
#include "collision_detection.hpp"

#define RLIGHTS_IMPLEMENTATION
#include "rlights.h"

#include "rlgl.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/component_wise.hpp>

#include "gui.hpp"

float GetRandomFloat(){
    return (GetRandomValue(0, 1000000) / 1000000.0);
}

float random_float_in_box(float box_size){
    return GetRandomFloat() * box_size;
}

glm::vec3 gen_random_color(){
    auto random_color = glm::vec3(
                GetRandomFloat(),
                GetRandomFloat(),
                GetRandomFloat()
            );
    if (glm::length(random_color) > 1.23f) {
        float minValue = glm::compMin(random_color);
        if (random_color.x == minValue) 
            random_color.x = 0;
        if (random_color.y == minValue)
            random_color.y = 0;
        else
            random_color.z = 0;

        // random_color = glm::normalize(random_color)
    }
    return random_color;
}

Object random_object(float box_size, float radius){
    return Object(
        glm::vec3(
            random_float_in_box(box_size),
            random_float_in_box(box_size),
            random_float_in_box(box_size)
        ),
        glm::vec3(
            GetRandomFloat() * 2 - 1,
            GetRandomFloat() * 2 - 1,
            GetRandomFloat() * 2 - 1
        ),
        radius,
        gen_random_color()
    );
}

Shader load_shaders(){
    // Load lighting shader
    Shader shader = LoadShader("resources/shaders/broad/lighting_instancing.vs",
                               "resources/shaders/broad/lighting.fs");
    // Get shader locations
    shader.locs[SHADER_LOC_MATRIX_MVP] = GetShaderLocation(shader, "mvp");
    shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(shader, "viewPos");
    shader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocationAttrib(shader, "instanceTransform");
    // Set shader value: ambient light level
    int ambientLoc = GetShaderLocation(shader, "ambient");
    SetShaderValue(shader, ambientLoc, (float[4]){ 0.2f, 0.2f, 0.2f, 1.0f }, SHADER_UNIFORM_VEC4);

    // Create one light
    CreateLight(LIGHT_DIRECTIONAL, (Vector3){ 15.0f, 15.0f, 15.0f }, Vector3Zero(), WHITE, shader);

    return shader;
}

unsigned int init_color_buffer(
    const Shader& shader,
    const std::vector<Vector4>& colors,
    unsigned int mesh_vao_id)
{
    int color_loc = GetShaderLocationAttrib(shader, "instanceColor");

    if (color_loc < 0) {
        TraceLog(LOG_ERROR, "instanceColor attribute not found");
        return 0;
    }

    unsigned int color_vbo = rlLoadVertexBuffer(
        colors.data(),
        static_cast<int>(colors.size() * sizeof(Vector4)),
        true
    );

    rlEnableVertexArray(mesh_vao_id);
    rlEnableVertexBuffer(color_vbo);

    rlEnableVertexAttribute(color_loc);
    rlSetVertexAttribute(
        color_loc,
        4,
        RL_FLOAT,
        false,
        sizeof(Vector4),
        0
    );
    rlSetVertexAttributeDivisor(color_loc, 1);

    rlDisableVertexBuffer();
    rlDisableVertexArray();

    return color_vbo;
}

void load_n_spheres(std::vector<Object>& objects, std::vector<Vector4>& colors, std::vector<Matrix>& transforms, int no_spheres, float box_size, float radius){
    objects.clear();
    colors.clear();
    transforms.clear();
    objects.resize(no_spheres);
    colors.resize(no_spheres);
    transforms.resize(no_spheres);
    for (size_t i = 0; i < no_spheres ; i++) {
        objects[i] = random_object(box_size, radius);
        colors[i] = Vector4(objects[i].color.x, objects[i].color.y, objects[i].color.z, 1.0f);
    }
}

int main(void)
{
    SetRandomSeed(2);
    InitWindow(800, 450, "broad phase example");
    SetMousePosition(GetScreenWidth() / 2, GetScreenHeight() / 2);
    bool showMessageBox = false;

    Camera3D camera;
    camera.position = (Vector3){ 5.0f, 3.0f, 5.0f }; 
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };          
    camera.fovy = 45.0f;       
    camera.projection = CAMERA_PERSPECTIVE;

    float box_size = 3.0f;
    float radius = 0.3f;
    int no_spheres = 10;
    float max_vel = 0.1f;
    bool use_grid = false;
    bool render = true;
    bool render_ui = true;

    Vector3 box(box_size, box_size, box_size);

    DisableCursor();

    std::vector<Object> objects(no_spheres);

    std::vector<Matrix> transforms(objects.size());
    std::vector<Vector4> colors(objects.size());
    auto sphere_mesh = GenMeshSphere(1.0f, 10, 10);
    auto material = LoadMaterialDefault();

    Shader shader = load_shaders();
    material.shader = shader;

    load_n_spheres(objects, colors, transforms, no_spheres, box_size, radius);

    int color_vbo = init_color_buffer(shader, colors, sphere_mesh.vaoId);

    bool free_cam = true;
    
    while (!WindowShouldClose())
    {
        if (box_size != box.x) 
            box = Vector3(box_size, box_size, box_size);

        if (no_spheres != objects.size()) {
            load_n_spheres(objects, colors, transforms, no_spheres, box_size, radius);
            rlUnloadVertexBuffer(color_vbo);
            color_vbo = init_color_buffer(shader, colors, sphere_mesh.vaoId);
        }

        UpdateCamera(&camera, free_cam ? CAMERA_FREE : 0);
        float cameraPos[3] = { camera.position.x, camera.position.y, camera.position.z };
        SetShaderValue(shader, shader.locs[SHADER_LOC_VECTOR_VIEW], cameraPos, SHADER_UNIFORM_VEC3);
        
        float frame_time = GetFrameTime(); 
        float delta = frame_time * 1000.0f; 

        step_simulation(glm::vec3(box_size), objects, max_vel, delta, use_grid);

        if (IsKeyPressed(KEY_Z) ||
            IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            free_cam ? EnableCursor() : DisableCursor();
            free_cam = !free_cam;
        }

        if (IsKeyPressed(KEY_H)){
            render_ui = !render_ui;
        }


        BeginDrawing();
            ClearBackground(BLACK);

            if(render){
                BeginMode3D(camera);
                    DrawCubeWiresV({box.x / 2.0f, box.y / 2.0f , box.z / 2.0f}, box, GRAY);
                    for (size_t i = 0; i < objects.size(); ++i) {
                        const Object& s = objects[i];
                        transforms[i] = MatrixMultiply(
                            MatrixScale(radius, radius, radius),
                            MatrixTranslate(
                                s.pos.x,
                                s.pos.y,
                                s.pos.z
                            )
                        );
                        colors[i] = Vector4(s.color.x, s.color.y, s.color.z, 1.0f);
                    }
                    rlUpdateVertexBuffer(
                        color_vbo,
                        colors.data(),
                        (int)(colors.size() * sizeof(Vector4)),
                        0
                    );

                    DrawMeshInstanced(sphere_mesh, material, transforms.data(), objects.size());
                EndMode3D();
            }

            DrawFPS(10, 10);
            if(render_ui)
                draw_gui(box_size, no_spheres, use_grid, render);
        EndDrawing();
    }

    rlUnloadVertexBuffer(color_vbo);
    UnloadShader(shader);
    UnloadMesh(sphere_mesh);

    CloseWindow();

    return 0;
}