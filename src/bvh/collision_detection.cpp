#include "collision_detection.hpp"
#include "raylib.h"
#include <algorithm>
#include <cstddef>
#include <stdbool.h>

namespace {
    static bool s_draw_bvh = false; 

    BoundingBox bbox_to_BBox(const BBox& bbox){
        Vector3 min(bbox.min.values[0],
                    bbox.min.values[1],
                    bbox.min.values[2]);
        Vector3 max(bbox.max.values[0],
                    bbox.max.values[1],
                    bbox.max.values[2]);
        return BoundingBox(min,max);
    }

    bool triangle_triangle_intersection(
        const Triangle& a,
        glm::vec3 a_pos,
        const Triangle& b,
        glm::vec3 b_pos)
    {
        using V = glm::vec3;

        const std::array<V, 3> A = {a.a + a_pos, a.b + a_pos, a.c + a_pos};
        const std::array<V, 3> B = {b.a + b_pos, b.b + b_pos, b.c + b_pos};

        constexpr float EPS = 1e-6f;

        const std::array<V, 3> EA = {
            A[1] - A[0],
            A[2] - A[1],
            A[0] - A[2]
        };

        const std::array<V, 3> EB = {
            B[1] - B[0],
            B[2] - B[1],
            B[0] - B[2]
        };

        const V normalA = glm::cross(EA[0], EA[1]);
        const V normalB = glm::cross(EB[0], EB[1]);

        constexpr float DEGENERATE_RATIO = 1e-4f;
        const auto degenerate = [&](const V& n, const std::array<V, 3>& E) -> bool {
            const float max_e2 = std::max({glm::dot(E[0], E[0]),
                                           glm::dot(E[1], E[1]),
                                           glm::dot(E[2], E[2])});
            const float limit = DEGENERATE_RATIO * max_e2;
            return glm::dot(n, n) <= limit * limit;
        };
        if (degenerate(normalA, EA) || degenerate(normalB, EB))
            return false;

        const auto separated = [&](const V& axis) -> bool {
            const float len2 = glm::dot(axis, axis);

            if (len2 < EPS * EPS)
                return false;

            float minA = glm::dot(A[0], axis);
            float maxA = minA;
            float minB = glm::dot(B[0], axis);
            float maxB = minB;

            for (int i = 1; i < 3; ++i) {
                const float pa = glm::dot(A[i], axis);
                const float pb = glm::dot(B[i], axis);

                minA = std::min(minA, pa);
                maxA = std::max(maxA, pa);
                minB = std::min(minB, pb);
                maxB = std::max(maxB, pb);
            }

            const float tolerance = EPS * std::sqrt(len2);

            return maxA < minB - tolerance ||
                maxB < minA - tolerance;
        };

        if (separated(normalA) || separated(normalB))
            return false;

        for (const V& ea : EA) {
            for (const V& eb : EB) {
                if (separated(glm::cross(ea, eb)))
                    return false;
            }
        }

        return true;
    }

    void draw_triangle(Triangle t, glm::vec3 pos){
        t.a += pos;
        t.b += pos;
        t.c += pos;
        Vector3 a = Vector3(t.a.x, t.a.y, t.a.z);
        Vector3 b = Vector3(t.b.x, t.b.y, t.b.z);
        Vector3 c = Vector3(t.c.x, t.c.y, t.c.z);
        DrawLine3D(a, b, WHITE);
        DrawLine3D(b, c, WHITE);
        DrawLine3D(c, a, WHITE);
    }

    StepMetrics naive_mid_narrow_phase(const Object& a, const Object& b){
        size_t primitive_collisions = 0;
        size_t primitive_tests = 0;
        bool object_collision = false;
        const Triangle* a_triangles = a.get_triangles();
        const Triangle* b_triangles = b.get_triangles();
        
        for (size_t r = 0; r < a.get_triangle_count() ; r++) {
            const Triangle& a_t = a_triangles[r];
            for (size_t t = 0; t < b.get_triangle_count() ; t++) {
                const Triangle& b_t = b_triangles[t];
                primitive_tests++;
                if (triangle_triangle_intersection(a_t, a.pos, b_t, b.pos)) {
                    primitive_collisions++;
                    draw_triangle(a_t, a.pos);
                    draw_triangle(b_t, b.pos);
                    if (!object_collision){
                        object_collision = true;
                    }                    
                }
            }
        }

        return {primitive_collisions, object_collision, primitive_tests};
    }

    bool aabb_overlap(const BBox& a, const BBox& b){
        for (int axis = 0; axis < 3; ++axis) {
            if (a.max.values[axis] < b.min.values[axis] ||
                b.max.values[axis] < a.min.values[axis]) {
                return false;
            }
        }

        return true;
    }
    float aabb_surface(const BBox& bbox){
        const auto extent = bbox.max - bbox.min;

        const float dx = extent.values[0];
        const float dy = extent.values[1];
        const float dz = extent.values[2];

        return 2.0f * (dx * dy + dy * dz + dz * dx);
    }

    StepMetrics narrow_phase_cd(const Object& a, const auto& a_node, const Object& b, const auto& b_node){
        size_t primitive_collisions = 0;
        size_t primitive_tests = 0;
        bool object_collision = false;
        
        const Triangle* a_triangles = a.get_triangles();
        const Triangle* b_triangles = b.get_triangles();

        const size_t a_begin = a_node.index.first_id();
        const size_t a_end = a_begin + a_node.index.prim_count();

        const size_t b_begin = b_node.index.first_id();
        const size_t b_end = b_begin + b_node.index.prim_count();

        for (size_t i = a_begin; i < a_end; i++) {
            const Triangle& a_t = a_triangles[a.bvh.prim_ids[i]];
            for (size_t j = b_begin; j < b_end; j++) {
                const Triangle& b_t = b_triangles[b.bvh.prim_ids[j]];
                primitive_tests++;
                if (triangle_triangle_intersection(a_t, a.pos, b_t, b.pos)) {
                    primitive_collisions++;
                    draw_triangle(a_t, a.pos);
                    draw_triangle(b_t, b.pos);
                    if (!object_collision){
                        object_collision = true;
                    }                    
                }
            }
        }

        return {primitive_collisions, object_collision, primitive_tests};
    }

    StepMetrics mid_phase_cd(const Object& a, size_t a_index, const Object& b, size_t b_index){
        const auto& a_node = a.bvh.nodes[a_index];
        const auto& b_node = b.bvh.nodes[b_index];

        bool a_leaf = a_node.is_leaf();
        bool b_leaf = b_node.is_leaf();
        const auto& a_bbox = a_node.get_bbox();
        const auto& b_bbox = b_node.get_bbox();
        
        if(!aabb_overlap(a_bbox, b_bbox))
            return StepMetrics();

        if(a_leaf && b_leaf){
            if(s_draw_bvh){
                DrawBoundingBox(bbox_to_BBox(a_bbox), GREEN);
                DrawBoundingBox(bbox_to_BBox(b_bbox), GREEN);
            }
            return narrow_phase_cd(a, a_node, b, b_node);
        }
        
        StepMetrics result{};
        
        bool split_a = !a_leaf && (b_leaf || aabb_surface(a_bbox) >= aabb_surface(b_bbox));

        if (split_a) {
            const size_t left = a_node.index.first_id();

            result.sum(mid_phase_cd(a, left,     b, b_index));
            result.sum(mid_phase_cd(a, left + 1, b, b_index));
        } else {
            const size_t left = b_node.index.first_id();

            result.sum(mid_phase_cd(a, a_index, b, left));
            result.sum(mid_phase_cd(a, a_index, b, left + 1));
        }
        
        return result;
    }

    StepMetrics bvh_mid_narrow_phase(const Object& a, const Object& b){
        StepMetrics m = mid_phase_cd(a, 0, b, 0);
        m.object_collisions = m.object_collisions ? 1 : 0;
        return m;
    }
}

StepMetrics step_simulation(const std::vector<Object>& objects, bool use_bvh, bool draw_bvh){
    s_draw_bvh = draw_bvh;
    StepMetrics metrics;
    bool object_collision = false;
    for (size_t i = 0; i < objects.size(); i++) {
        const Object& a = objects[i];
        for (size_t j = i + 1; j < objects.size(); j++) {
            object_collision = false;
            const Object& b = objects[j];
            
            // Broad phase
            if(!aabb_overlap(a.bvh.nodes[0].get_bbox(), b.bvh.nodes[0].get_bbox()))
                continue;

            if(use_bvh)
                metrics.sum(bvh_mid_narrow_phase(a, b));
            else
                metrics.sum(naive_mid_narrow_phase(a, b));
        }
    }

    return metrics;
}