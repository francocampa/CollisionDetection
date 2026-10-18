#include "hash_grid.hpp"
#include "glm/common.hpp"
#include <cstdio>
#include <cstdlib>

// Adapted from: https://github.com/matthias-research/pages/blob/master/tenMinutePhysics/11-hashing.html

inline glm::uvec3 magic_hash_vec = glm::uvec3(92837111, 689287499, 283923481);

size_t HashGrid::hash_pos(glm::vec3 pos){
    glm::uvec3 res = glm::floor(pos/cell_size);
    
    return hash_indices(res);
}

size_t HashGrid::hash_indices(glm::ivec3 indices){
    indices *= magic_hash_vec;
    size_t h = indices.x ^ indices.y ^ indices.z;

    return glm::abs(h) % table_size;
}


void HashGrid::fill(const std::vector<Object>& objects){
    no_objects = std::min(objects.size(), (size_t) this->no_objects);

    // Cell sizes

    std::fill(cell_starts.begin(), cell_starts.end(), 0); 
    std::fill(cell_entries.begin(), cell_entries.end(), 0);
    
    for(size_t i = 0; i < no_objects; i++)
        cell_starts[hash_pos(objects[i].pos)]++;

    // Cell starts

    size_t start = 0;

    for(size_t i = 0; i < table_size; i++){
        start += cell_starts[i];
        cell_starts[i] = start; 
    }
    
    cell_starts[table_size] = start;

    // Fill

    for(size_t i = 0; i < no_objects; i++){
        size_t h = hash_pos(objects[i].pos);
        cell_starts[h]--;
        cell_entries[cell_starts[h]] = i;
    }
}

const QueryResult& HashGrid::query(glm::vec3 pos, float dist){
    glm::ivec3 min = glm::ivec3(glm::floor((pos - glm::vec3(dist)) / cell_size));
    glm::ivec3 max = glm::ivec3(glm::floor((pos + glm::vec3(dist)) / cell_size));

    query_res.size = 0;

    for (size_t x = min.x; x < max.x; x++) {
        for (size_t y = min.y; y < max.y; y++) {
            for (size_t z = min.z; z < max.z; z++) {
                size_t h = hash_indices(glm::uvec3(x,y,z));
                size_t start = cell_starts[h];
                size_t end = cell_starts[h + 1];

                for (size_t i = start; i < end; i++) {
                    query_res.indices[query_res.size] = cell_entries[i];
                    query_res.size++;
                }
            }
        }
    }

    return query_res;
}