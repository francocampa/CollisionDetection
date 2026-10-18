#ifndef HASH_GRID_HPP
#define HASH_GRID_HPP

#include "collision_detection.hpp"
#include <cstddef>
#include <vector>
#include <glm/glm.hpp>

struct QueryResult {
    std::vector<size_t> indices;
    size_t size = 0;
};

class HashGrid {
    private:
        std::vector<int> cell_starts;
        std::vector<size_t> cell_entries;
        QueryResult query_res;
        
        float cell_size;
        unsigned int no_objects;
        unsigned int table_size;

        const static int K = 3;

        size_t hash_pos(glm::vec3 pos);
        size_t hash_indices(glm::ivec3 indices);

    public:
        HashGrid(float cell_size, int no_objects) : cell_size(cell_size), no_objects(no_objects)
        {
            table_size = K * no_objects;
            cell_starts.resize(table_size + 1);
            cell_entries.resize(no_objects);
            query_res.indices.resize(no_objects);
        }

        void fill(const std::vector<Object>& objects);

        const QueryResult& query(glm::vec3 pos, float dist);

        bool needs_update(float cell_size, int no_objects){
            return this->cell_size != cell_size || this->no_objects != no_objects;
        }
};

#endif