#include "glm/common.hpp"
#include "raylib.h"
#include "raymath.h"
#include <cstdio>
#include <vector>

#define RLIGHTS_IMPLEMENTATION
#include "rlights.h"

#include "stb_perlin.h"

#include "glm/glm.hpp"

const Matrix identity_matrix = MatrixMultiply(
                                MatrixScale(1, 1, 1),
                                MatrixTranslate(0, 0, 0));
const auto down_dir = glm::vec3(0.0f, -1.0f, 0.0f);
const auto max_vel = glm::vec3(10.0f, 1000.0f, 10.0f);
constexpr glm::vec3 gravity(0.0f, -10.0f, 0.0f);

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
    CreateLight(LIGHT_DIRECTIONAL, Vector3(5.0f, 5.0f, 5.0f), Vector3Zero(), WHITE, shader);

    return shader;
}

constexpr float terrain_size = 100.0f;
constexpr float cell_size = 1.0f;
constexpr int terrain_cells =
    static_cast<int>((2.0f * terrain_size) / cell_size) + 1;

Mesh generate_terrain() {
    std::vector<float> vertices;
    std::vector<float> normals;
    std::vector<unsigned short> indices;
    int triangle_count = 0;

    constexpr float frequency = 0.2f;
    constexpr float amplitude = 1.0f;

    const int vertex_count = terrain_cells * terrain_cells;

    vertices.reserve(vertex_count * 3);
    normals.resize(vertex_count * 3, 0.0f);
    indices.reserve((terrain_cells - 1) * (terrain_cells - 1) * 6);

    // Vertices
    for (int z = 0; z < terrain_cells; ++z) {
        for (int x = 0; x < terrain_cells; ++x) {
            const float px = -terrain_size + x * cell_size;
            const float pz = -terrain_size + z * cell_size;

            const float height = amplitude * stb_perlin_noise3(
                px * frequency,
                pz * frequency,
                0.0f,
                0, 0, 0
            );

            vertices.insert(
                vertices.end(),
                {px, height, pz}
            );
        }
    }

    // Indices
    for (int z = 0; z < terrain_cells - 1; ++z) {
        for (int x = 0; x < terrain_cells - 1; ++x) {
            const auto top_left = static_cast<unsigned short>(
                z * terrain_cells + x
            );
            const auto top_right =
                static_cast<unsigned short>(top_left + 1);
            const auto bottom_left = static_cast<unsigned short>(
                top_left + terrain_cells
            );
            const auto bottom_right =
                static_cast<unsigned short>(bottom_left + 1);

            indices.insert(
                indices.end(),
                {
                    top_left, bottom_left, top_right,
                    top_right, bottom_left, bottom_right
                }
            );
        }
    }

    // Normals
    for (size_t i = 0; i < indices.size(); i += 3) {
        const unsigned int ia = indices[i];
        const unsigned int ib = indices[i + 1];
        const unsigned int ic = indices[i + 2];

        const Vector3 a = {
            vertices[ia * 3],
            vertices[ia * 3 + 1],
            vertices[ia * 3 + 2]
        };

        const Vector3 b = {
            vertices[ib * 3],
            vertices[ib * 3 + 1],
            vertices[ib * 3 + 2]
        };

        const Vector3 c = {
            vertices[ic * 3],
            vertices[ic * 3 + 1],
            vertices[ic * 3 + 2]
        };

        const Vector3 edge1 = Vector3Subtract(b, a);
        const Vector3 edge2 = Vector3Subtract(c, a);

        // Cross product produces an area-weighted normal.
        const Vector3 normal = Vector3Normalize(Vector3CrossProduct(edge1, edge2));

        for (unsigned int index : {ia, ib, ic}) {
            normals[index * 3]     += normal.x;
            normals[index * 3 + 1] += normal.y;
            normals[index * 3 + 2] += normal.z;
        }
    }

    Mesh mesh = {0};

    mesh.vertexCount = static_cast<int>(vertices.size() / 3);
    mesh.triangleCount = static_cast<int>(indices.size() / 3);

    mesh.vertices = vertices.data();
    mesh.normals = normals.data();
    mesh.indices = indices.data();

    UploadMesh(&mesh, false);

    return mesh;
}

struct Player{
    glm::vec3 pos, vel, acc;
};

float raycast_to_floor(glm::vec3 pos, glm::vec3 dir, const Mesh& terrain){
    constexpr float offset = 10;

    glm::vec3 ray_origin = pos + dir * -offset;

    Ray ray(Vector3(ray_origin.x, ray_origin.y, ray_origin.z), Vector3(dir.x, dir.y, dir.z));
    
    RayCollision ray_collision = GetRayCollisionMesh(ray, terrain, identity_matrix);


    return ray_collision.hit ? ray_collision.distance - offset : 1.0f;
}


void step_player_physics(Player& player, const Mesh& terrain, float delta){
    glm::vec3 move_vel(0);
    float horizontal_vel = 40.0f;
    float jump_vel = 10.0f;
    if(IsKeyDown(KEY_LEFT)){
        move_vel.x -= horizontal_vel;
    }
    if(IsKeyDown(KEY_RIGHT)){
        move_vel.x += horizontal_vel;
    }
    if(IsKeyDown(KEY_UP)){
        move_vel.z -= horizontal_vel;
    }
    if(IsKeyDown(KEY_DOWN)){
        move_vel.z += horizontal_vel;
    }

    if(IsKeyPressed(KEY_Z)){
        move_vel.y = jump_vel;
    }

    player.vel = glm::vec3(move_vel.x, player.vel.y + move_vel.y, move_vel.z);
    
    float floor_t = raycast_to_floor(player.pos, down_dir, terrain);
    if(floor_t > EPSILON){
        player.vel += gravity * delta;
        printf("Floor_t: %f, vel y: %f\n",floor_t, player.vel.y);
    }else if (floor_t < EPSILON) {
        player.pos.y = player.pos.y + (down_dir * floor_t).y;
    }
    player.vel = glm::clamp(player.vel, -max_vel, max_vel);

    player.pos += player.vel * delta;
}

const float player_radius = 1.0f;

void render_player(const Player& player){
    Vector3 sphere_pos = Vector3(
        player.pos.x,
        player.pos.y + player_radius,
        player.pos.z
    );;
    DrawSphere(sphere_pos, player_radius, RED);
    DrawSphere(Vector3Add(sphere_pos, Vector3(0.5f ,1.0f, 0.0f)) , 0.3, BLUE);
}

int main(void)
{
    SetRandomSeed(2);
    InitWindow(800, 450, "Ray casting example");
    SetMousePosition(GetScreenWidth() / 2, GetScreenHeight() / 2);

    Camera3D camera;
    camera.position = (Vector3){ 5.0f, 3.0f, 5.0f }; 
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };          
    camera.fovy = 45.0f;       
    camera.projection = CAMERA_PERSPECTIVE;

    DisableCursor();
    Shader shader = load_shaders();

    Mesh terrain_mesh = generate_terrain();
    Material terrain_material = LoadMaterialDefault();
    terrain_material.shader = shader;
    terrain_material.maps[MATERIAL_MAP_DIFFUSE].color = GREEN;

    int color_loc = GetShaderLocationAttrib(shader, "instanceColor");
    int transform_loc = GetShaderLocationAttrib(shader, "instanceTransform");

    bool free_cam = true;

    Player player(glm::vec3(0.0f, 5.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f));

    while (!WindowShouldClose())
    {
        UpdateCamera(&camera, free_cam ? CAMERA_FREE : 0);
        float cameraPos[3] = { camera.position.x, camera.position.y, camera.position.z };
        SetShaderValue(shader, shader.locs[SHADER_LOC_VECTOR_VIEW], cameraPos, SHADER_UNIFORM_VEC3);
        
        float frame_time = GetFrameTime(); 

        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            free_cam ? EnableCursor() : DisableCursor();
            free_cam = !free_cam;
        }

        step_player_physics(player, terrain_mesh, frame_time);

        BeginDrawing();
            ClearBackground(BLACK);
            BeginMode3D(camera);
                DrawMesh(terrain_mesh, terrain_material, identity_matrix);
                render_player(player);
            EndMode3D();
            DrawFPS(10, 10);
            
        EndDrawing();
    }

    UnloadShader(shader);
    UnloadMesh(terrain_mesh);

    CloseWindow();

    return 0;
}