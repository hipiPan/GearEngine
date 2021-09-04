#include "RenderPipeline.h"
#include "Resource/GpuBuffer.h"
#include "Resource/Texture.h"
#include "Resource/Material.h"
#include "Resource/RenderTarget.h"
#include "GearEngine.h"
#include "Renderer/Renderer.h"

namespace gear {

    static glm::vec4 g_debug_color[SHADOW_CASCADE_COUNT];
    static glm::vec4 g_current_debug_color;

    struct SegmentIndex {
        uint8_t v0, v1;
    };

    struct QuadIndex {
        uint8_t v0, v1, v2, v3;
    };

    static SegmentIndex g_box_segments[12] = {
            { 0, 1 }, { 1, 3 }, { 3, 2 }, { 2, 0 },
            { 4, 5 }, { 5, 7 }, { 7, 6 }, { 6, 4 },
            { 0, 4 }, { 1, 5 }, { 3, 7 }, { 2, 6 },
    };

    static QuadIndex g_box_quads[6] = {
            { 2, 0, 1, 3 },  // far
            { 6, 4, 5, 7 },  // near
            { 2, 0, 4, 6 },  // left
            { 3, 1, 5, 7 },  // right
            { 0, 4, 5, 1 },  // bottom
            { 2, 6, 7, 3 },  // top
    };

    // 计算不同坐标系中的near和far
    glm::vec2 ComputeNearFar(const glm::mat4& view, const glm::vec3* ws_vertices, uint32_t count) {
        glm::vec2 near_far = { std::numeric_limits<float>::lowest(), std::numeric_limits<float>::max() };

        for (uint32_t i = 0; i < count; i++) {
            // 右手坐标系，Z轴朝外，Z值越大越靠近近平面
            float c = TransformPoint(ws_vertices[i], view).z;
            near_far.x = std::max(near_far.x, c);  // near
            near_far.y = std::min(near_far.y, c);  // far
        }
        return near_far;
    }

    glm::vec2 ComputeNearFar(const glm::mat4& view, BBox& ws_shadow_casters_volume) {
        glm::vec3* ws_vertices = ws_shadow_casters_volume.GetCorners().vertices;
        return ComputeNearFar(view, ws_vertices, 8);
    }


    void RenderPipeline::ComputeCascadeParams(CascadeParameters& cascade_params) {
        // 方向光取任何位置都没有问题，这里默认设置为相机当前位置
        cascade_params.ws_light_position = _main_camera_info.position;

        // 计算灯光矩阵
        const glm::mat4 light_model_matrix = glm::lookAt(cascade_params.ws_light_position, cascade_params.ws_light_position + _light_info.sun_direction, glm::vec3(0, 1, 0));
        const glm::mat4 light_view_matrix = glm::inverse(light_model_matrix);
        const glm::mat4 camera_view_matrix = _main_camera_info.view;

        // 初始化参数
        cascade_params.ls_near_far = { std::numeric_limits<float>::lowest(), std::numeric_limits<float>::max() };
        cascade_params.vs_near_far = { std::numeric_limits<float>::lowest(), std::numeric_limits<float>::max() };
        cascade_params.ws_shadow_casters_volume = BBox();
        cascade_params.ws_shadow_receivers_volume = BBox();

        // 遍历所有renderable
        for (uint32_t i = 0; i < _num_common_renderables; ++i) {
            Renderable* rb = &_renderables[_common_renderables[i]];
            for (int j = 0; j < rb->primitives.size(); ++j) {
                RenderPrimitive* prim = &rb->primitives[j];
                if (prim->cast_shadow) {
                    cascade_params.ws_shadow_casters_volume.bb_min = glm::min(cascade_params.ws_shadow_casters_volume.bb_min, prim->bbox.bb_min);
                    cascade_params.ws_shadow_casters_volume.bb_max = glm::max(cascade_params.ws_shadow_casters_volume.bb_max, prim->bbox.bb_max);
                    glm::vec2 nf = ComputeNearFar(light_view_matrix, prim->bbox);
                    cascade_params.ls_near_far.x = std::max(cascade_params.ls_near_far.x, nf.x);
                    cascade_params.ls_near_far.y = std::min(cascade_params.ls_near_far.y, nf.y);
                }

                if (prim->receive_shadow) {
                    cascade_params.ws_shadow_receivers_volume.bb_min = min(cascade_params.ws_shadow_receivers_volume.bb_min, prim->bbox.bb_min);
                    cascade_params.ws_shadow_receivers_volume.bb_max = max(cascade_params.ws_shadow_receivers_volume.bb_max, prim->bbox.bb_max);
                    glm::vec2 nf = ComputeNearFar(camera_view_matrix, prim->bbox);
                    cascade_params.vs_near_far.x = std::max(cascade_params.vs_near_far.x, nf.x);
                    cascade_params.vs_near_far.y = std::min(cascade_params.vs_near_far.y, nf.y);
                }
            }
        }

    }

    // 计算视锥体的顶点
    void ComputeFrustumCorners(glm::vec3* out, const glm::mat4& projection_view_inverse, const glm::vec2 cs_near_far) {
        // ndc -> camera -> world
        float near = cs_near_far.x;
        float far = cs_near_far.y;
        glm::vec3 csViewFrustumCorners[8] = {
                { -1, -1,  far },
                {  1, -1,  far },
                { -1,  1,  far },
                {  1,  1,  far },
                { -1, -1,  near },
                {  1, -1,  near },
                { -1,  1,  near },
                {  1,  1,  near },
        };
        for (glm::vec3 c : csViewFrustumCorners) {
            *out++ = TransformPoint(c, projection_view_inverse);
        }
    }

    // 线段与三角形相交
    bool IntersectSegmentWithTriangle(glm::vec3& p, glm::vec3 s0, glm::vec3 s1, glm::vec3 t0, glm::vec3 t1, glm::vec3 t2) {
        constexpr const float EPSILON = 1.0f / 65536.0f;  // ~1e-5
        const auto e1 = t1 - t0;
        const auto e2 = t2 - t0;
        const auto d = s1 - s0;
        const auto q = glm::cross(d, e2);
        const auto a = glm::dot(e1, q);
        if (std::abs(a) < EPSILON) {
            // 无法构成三角形
            return false;
        }
        const auto s = s0 - t0;
        const auto u = dot(s, q) * glm::sign(a);
        const auto r = cross(s, e1);
        const auto v = dot(d, r) * glm::sign(a);
        if (u < 0 || v < 0 || u + v > std::abs(a)) {
            // 射线没有和三角形相交
            return false;
        }
        const auto t = dot(e2, r) * glm::sign(a);
        if (t < 0 || t > std::abs(a)) {
            // 射线相交但是不在线段内
            return false;
        }

        // 计算相交点位置
        p = s0 + d * (t / std::abs(a));
        return true;
    }

    // 线段与平面相交
    bool IntersectSegmentWithPlanarQuad(glm::vec3& p, glm::vec3 s0, glm::vec3 s1, glm::vec3 t0, glm::vec3 t1, glm::vec3 t2, glm::vec3 t3) {
        bool hit = IntersectSegmentWithTriangle(p, s0, s1, t0, t1, t2) || IntersectSegmentWithTriangle(p, s0, s1, t0, t2, t3);
        return hit;
    }

    // 计算与视锥体相交的点
    void IntersectFrustum(glm::vec3* out_vertices, uint32_t& out_vertex_count, glm::vec3 const* segments_vertices, glm::vec3 const* quads_vertices) {
        for (const SegmentIndex segment : g_box_segments) {
            const glm::vec3 s0{ segments_vertices[segment.v0] };
            const glm::vec3 s1{ segments_vertices[segment.v1] };
            // 每条线段最多与两个quad相交
            uint32_t max_vertex_count = out_vertex_count + 2;
            for (uint32_t j = 0; j < 6 && out_vertex_count < max_vertex_count; ++j) {
                const QuadIndex quad = g_box_quads[j];
                const glm::vec3 t0{ quads_vertices[quad.v0] };
                const glm::vec3 t1{ quads_vertices[quad.v1] };
                const glm::vec3 t2{ quads_vertices[quad.v2] };
                const glm::vec3 t3{ quads_vertices[quad.v3] };
                if (IntersectSegmentWithPlanarQuad(out_vertices[out_vertex_count], s0, s1, t0, t1, t2, t3)) {
                    out_vertex_count++;
                }
            }
        }
    }

    // 计算视锥体与包围盒相交的顶点
    void IntersectFrustumWithBBox(const glm::vec3* ws_frustum_corners, const BBox& ws_bbox, glm::vec3* out_vertices, uint32_t& out_vertex_count) {
        /*
         * 1.计算视锥体处于包围盒的顶点
         * 2.计算包围盒处于视锥体的顶点
         * 3.计算包围盒的边和视锥体平面之间的相交
         * 4.计算视锥体的边和包围盒平面之间的相交
         */
        uint32_t vertex_count = 0;

        for (uint32_t i = 0; i < 8; i++) {
            glm::vec3 p = ws_frustum_corners[i];
            out_vertices[vertex_count] = p;
            if ((p.x >= ws_bbox.bb_min.x && p.x <= ws_bbox.bb_max.x) &&
                (p.y >= ws_bbox.bb_min.y && p.y <= ws_bbox.bb_max.y) &&
                (p.z >= ws_bbox.bb_min.z && p.z <= ws_bbox.bb_max.z)) {
                vertex_count++;
            }
        }
        const bool some_frustum_vertices_are_in_bbox = vertex_count > 0;
        constexpr const float EPSILON = 1.0f / 8192.0f; // ~0.012 mm

        // 如果顶点数量等于8说明视锥体被完全覆盖，不需要再进行剩余步骤
        if (vertex_count < 8) {
            Frustum frustum(ws_frustum_corners);
            glm::vec4* ws_frustum_planes = frustum.planes;

            // 包围盒的8个顶点
            const glm::vec3* ws_scene_receivers_corners = ws_bbox.GetCorners().vertices;

            for (uint32_t i = 0; i < 8; ++i) {
                glm::vec3 p = ws_scene_receivers_corners[i];
                out_vertices[vertex_count] = p;
                // l/b/r/t/f/n分别表示到视锥体对应平面的距离
                float l = glm::dot(glm::vec3(ws_frustum_planes[0]), p) + ws_frustum_planes[0].w;
                float b = glm::dot(glm::vec3(ws_frustum_planes[1]), p) + ws_frustum_planes[1].w;
                float r = glm::dot(glm::vec3(ws_frustum_planes[2]), p) + ws_frustum_planes[2].w;
                float t = glm::dot(glm::vec3(ws_frustum_planes[3]), p) + ws_frustum_planes[3].w;
                float f = glm::dot(glm::vec3(ws_frustum_planes[4]), p) + ws_frustum_planes[4].w;
                float n = glm::dot(glm::vec3(ws_frustum_planes[5]), p) + ws_frustum_planes[5].w;
                if ((l <= EPSILON) && (b <= EPSILON) &&
                    (r <= EPSILON) && (t <= EPSILON) &&
                    (f <= EPSILON) && (n <= EPSILON)) {
                    ++vertex_count;
                }
            }

            // 如果视锥体没有被包围盒完全包围，或者没有包围整个包围盒，则说明两者边界存在交点
            if (some_frustum_vertices_are_in_bbox || vertex_count < 8) {
                IntersectFrustum(out_vertices, out_vertex_count, ws_scene_receivers_corners, ws_frustum_corners);

                IntersectFrustum(out_vertices, out_vertex_count, ws_frustum_corners, ws_scene_receivers_corners);
            }
        }
    }

    void RenderPipeline::UpdateShadowMapInfo(const CascadeParameters& cascade_params, ShadowMapInfo& shadow_map_info) {
        // projection[2][2] = f / (n - f)
        // projection[3][2] = -(f * n) / (f - n)

        // 开始计算灯光投影矩阵
        if (cascade_params.ws_shadow_casters_volume.IsEmpty() || cascade_params.ws_shadow_receivers_volume.IsEmpty()) {
            shadow_map_info.has_visible_shadows = false;
            return;
        }

        // 1.计算灯光矩阵，使用lookat函数
        const glm::vec3 light_position = cascade_params.ws_light_position;
        const glm::mat4 light_model_matrix = glm::lookAt(light_position, light_position + _light_info.sun_direction, glm::vec3{ 0, 1, 0 });
        const glm::mat4 light_view_matrix = glm::inverse(light_model_matrix);

        // 2.计算相机视锥体在世界坐标的顶点
        glm::vec3 ws_view_frustum_vertices[8];
        ComputeFrustumCorners(ws_view_frustum_vertices, _main_camera_info.model * glm::inverse(_main_camera_info.projection), cascade_params.cs_near_far);

        // 3.计算接收阴影包围盒与相机视锥体的相交的顶点
        glm::vec3 ws_clipped_shadow_receiver_volume[64];
        uint32_t vertex_count = 0;
        IntersectFrustumWithBBox(ws_view_frustum_vertices, cascade_params.ws_shadow_receivers_volume, ws_clipped_shadow_receiver_volume, vertex_count);
        // 相交顶点小于等于2时，说明当前没有一个renderable受阴影影响，所以说不需要阴影贴图
        shadow_map_info.has_visible_shadows = vertex_count >= 2;
        if (!shadow_map_info.has_visible_shadows) {
            return;
        }
        // bebug包围盒
        BBox ws_light_frustum_bounds;
        for (uint32_t i = 0; i < vertex_count; ++i) {
            ws_light_frustum_bounds.Grow(ws_clipped_shadow_receiver_volume[i]);
        }
        DrawDebugBox(ws_light_frustum_bounds, g_current_debug_color);

        // 4.计算灯光空间的包围盒
        BBox ls_light_frustum_bounds;
        for (uint32_t i = 0; i < vertex_count; ++i) {
            // 利用相机视锥体与物体相交信息求出灯光包围盒
            glm::vec3 v = TransformPoint(ws_clipped_shadow_receiver_volume[i], light_view_matrix);
            ls_light_frustum_bounds.bb_min.z = std::min(ls_light_frustum_bounds.bb_min.z, v.z);
            ls_light_frustum_bounds.bb_max.z = std::max(ls_light_frustum_bounds.bb_max.z, v.z);
        }

        // 5.由灯光空间的包围盒计算出灯光视锥体的远近平面
        const float znear = -ls_light_frustum_bounds.bb_max.z;
        const float zfar = -ls_light_frustum_bounds.bb_min.z;
        if (znear >= zfar) {
            shadow_map_info.has_visible_shadows = false;
            return;
        }

        // 6.远近平面计算出灯光的投影矩阵，方向光使用正交投影
        shadow_map_info.light_projection_matrix = glm::mat4(1.0f);
        shadow_map_info.light_projection_matrix[2][2] = zfar / (znear - zfar);
        shadow_map_info.light_projection_matrix[3][2] = -(zfar * znear) / (zfar - znear);
        // 翻转y轴
        shadow_map_info.light_projection_matrix[1][1] *= -1;

        // 7.设置相机参数
        shadow_map_info.camera_position = GetTranslate(light_model_matrix);
        shadow_map_info.camera_direction = GetAxisZ(light_model_matrix);

        // 后续优化，通过扭曲灯光投影矩阵的方法去改善阴影贴图
    }

    void RenderPipeline::ExecShadowStage() {
        Renderer* renderer = gEngine.GetRenderer();

        g_debug_color[0] = glm::vec4(0.0, 1.0, 0.0, 1.0);
        g_debug_color[1] = glm::vec4(0.0, 0.0, 1.0, 1.0);
        g_debug_color[2] = glm::vec4(1.0, 0.0, 0.0, 1.0);

        CascadeParameters cascade_params;
        ComputeCascadeParams(cascade_params);

        // 划分
        float vs_near = cascade_params.vs_near_far.x;
        float vs_far = cascade_params.vs_near_far.y;

        float split_percentages[SHADOW_CASCADE_COUNT + 1];
        for (uint32_t i = 0; i < SHADOW_CASCADE_COUNT + 1; ++i) {
            split_percentages[i] = i * 0.3f;
        }
        split_percentages[SHADOW_CASCADE_COUNT] = 1.0f;

        float splits_ws[SHADOW_CASCADE_COUNT + 1];
        float splits_cs[SHADOW_CASCADE_COUNT + 1];
        for (uint32_t i = 0; i < SHADOW_CASCADE_COUNT + 1; i++) {
            splits_ws[i] = vs_near + (vs_far - vs_near) * split_percentages[i];
            splits_cs[i] = TransformPoint(glm::vec3(0.0f, 0.0f, splits_ws[i]), _main_camera_info.projection).z;
        }

        // 更新shadow map的投影矩阵
        ShadowMapInfo shadow_map_infos[SHADOW_CASCADE_COUNT];
        for (uint32_t i = 0; i < SHADOW_CASCADE_COUNT; i++) {
            g_current_debug_color = g_debug_color[i];
            cascade_params.cs_near_far = { splits_cs[i], splits_cs[i + 1] };
            UpdateShadowMapInfo(cascade_params, shadow_map_infos[i]);
        }
    }
}