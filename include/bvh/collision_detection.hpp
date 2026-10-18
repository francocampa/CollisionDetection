#ifndef COLLISION_DETECTION
#define COLLISION_DETECTION

#include "glm/glm.hpp"
#include "raylib.h"

#include <bvh/v2/bvh.h>
#include <bvh/v2/vec.h>
#include <bvh/v2/ray.h>
#include <bvh/v2/node.h>
#include <bvh/v2/default_builder.h>
#include <bvh/v2/thread_pool.h>
#include <bvh/v2/executor.h>
#include <bvh/v2/stack.h>
#include <bvh/v2/tri.h>
#include <cstddef>

using Vec3    = bvh::v2::Vec<float, 3>;
using BBox    = bvh::v2::BBox<float, 3>;
using Tri     = bvh::v2::Tri<float, 3>;
using Node    = bvh::v2::Node<float, 3>;
using Bvh     = bvh::v2::Bvh<Node>;
using BvhRay  = bvh::v2::Ray<float, 3>;

struct Triangle {
    glm::vec3 a, b, c;
};

struct Object{
    Model model;
    glm::vec3 pos;
    Color color;
    Bvh bvh;

    void build_bvh(){
        std::vector<Tri> tris;
        Vec3 *vertices = (Vec3 *)model.meshes[0].vertices;
        const auto& indices = model.meshes[0].indices;

        for(size_t i = 0; i < model.meshes[0].triangleCount; i++){
            int idx0, idx1, idx2;

            if (indices != NULL) {
                idx0 = indices[i * 3 + 0];
                idx1 = indices[i * 3 + 1];
                idx2 = indices[i * 3 + 2];
            } else {
                idx0 = i * 3 + 0;
                idx1 = i * 3 + 1;
                idx2 = i * 3 + 2;
            }

            Vec3 a = vertices[idx0] + Vec3(pos.x, pos.y, pos.z);
            Vec3 b = vertices[idx1] + Vec3(pos.x, pos.y, pos.z);
            Vec3 c = vertices[idx2] + Vec3(pos.x, pos.y, pos.z);

            tris.emplace_back(a,b,c);
        }

        bvh::v2::ThreadPool thread_pool;
        bvh::v2::ParallelExecutor executor(thread_pool);

        // Get triangle centers and bounding boxes (required for BVH builder)
        std::vector<BBox> bboxes(tris.size());
        std::vector<Vec3> centers(tris.size());
        executor.for_each(0, tris.size(), [&] (size_t begin, size_t end) {
            for (size_t i = begin; i < end; ++i) {
                bboxes[i]  = tris[i].get_bbox();
                centers[i] = tris[i].get_center();
            }
        });

        typename bvh::v2::DefaultBuilder<Node>::Config config;
        config.quality = bvh::v2::DefaultBuilder<Node>::Quality::High;
        bvh = bvh::v2::DefaultBuilder<Node>::build(thread_pool, bboxes, centers, config);
    }

    const Triangle* get_triangles() const {
        return (Triangle *)model.meshes[0].vertices;
    }

    size_t get_triangle_count() const {
        return model.meshes[0].triangleCount;
    }
};

struct StepMetrics {
    size_t primitive_collisions = 0, object_collisions = 0;

    void sum(StepMetrics metrics){
        this->primitive_collisions += metrics.primitive_collisions;
        this->object_collisions += metrics.object_collisions;
    }
};

StepMetrics step_simulation(const std::vector<Object>& objects, bool use_bvh);

#endif