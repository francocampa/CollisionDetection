cmake_minimum_required(VERSION 4.4.3)

project(CollisionDetection)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(lib/raylib)
add_subdirectory(lib/glm)
add_subdirectory(lib/bvh)

set(BROAD_PHASE_PROJECT broad_phase)
set(BROAD_PHASE_SOURCE_FILES 
    src/broad_phase/main.cpp
    src/broad_phase/collision_detection.cpp            
    src/broad_phase/hash_grid.cpp            
)

add_executable(${BROAD_PHASE_PROJECT} ${BROAD_PHASE_SOURCE_FILES})

target_include_directories(${BROAD_PHASE_PROJECT} PRIVATE 
    include/broad_phase
    lib/raylib/src
    lib/raygui/src
    lib/rlights
)

target_link_libraries(${BROAD_PHASE_PROJECT} PRIVATE 
    raylib 
    glm::glm
)

set(MID_NARROW_PHASE_PROJECT mid_narrow_phase)
set(MID_NARROW_PHASE_SOURCE_FILES 
    src/bvh/main.cpp
    src/bvh/collision_detection.cpp            
)

add_executable(${MID_NARROW_PHASE_PROJECT} ${MID_NARROW_PHASE_SOURCE_FILES})

target_include_directories(${MID_NARROW_PHASE_PROJECT} PRIVATE 
    include/bvh
    lib/raylib/src
    lib/raygui/src
    lib/rlights
)

target_link_libraries(${MID_NARROW_PHASE_PROJECT} PRIVATE 
    raylib 
    glm::glm
    bvh
)