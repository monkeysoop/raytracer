#version 430

layout(std430, binding = 0) buffer VerteciesBuffer {
    readonly vec4 vertecies[];
};

layout(std430, binding = 1) buffer NormalsBuffer {
    readonly vec4 normals[];
};

layout(std430, binding = 2) buffer IndeciesBuffer {
    readonly uint indecies[];
};

layout(std430, binding = 3) buffer NodesBuffer {
    readonly uint nodes[];
};

in vec2 vs_out_ndc_coord;

out vec4 fs_out_col;

uniform samplerCube skyboxTexture;

uniform mat4 inv_view_proj_mat;
uniform vec3 position;
uniform float width;
uniform float height;

uniform vec3 octree_min_bounds;
uniform vec3 octree_max_bounds;

struct Ray {
    vec3 position;
    vec3 direction;
    vec3 inverse_direction;
};

struct AABB {
    vec3 min_bounds;
    vec3 max_bounds;
};

float min3(vec3 a) {
    return min(min(a.x, a.y), a.z);
}

float max3(vec3 a) {
    return max(max(a.x, a.y), a.z);
}

const float INFINITY = 1.0 / 0.0;


// from https://gamedev.stackexchange.com/questions/23743/whats-the-most-efficient-way-to-find-barycentric-coordinates
vec3 Barycentric(vec3 p, vec3 a, vec3 b, vec3 c) {
    vec3 v0 = b - a;
    vec3 v1 = c - a;
    vec3 v2 = p - a;
    float d00 = dot(v0, v0);
    float d01 = dot(v0, v1);
    float d11 = dot(v1, v1);
    float d20 = dot(v2, v0);
    float d21 = dot(v2, v1);
    float denom = d00 * d11 - d01 * d01;
    float v = (d11 * d20 - d01 * d21) / denom;
    float w = (d00 * d21 - d01 * d20) / denom;
    float u = 1.0f - v - w;

    return vec3(u, v, w);
}

// from https://github.com/btmxh/glsl-intersect/blob/master/3d/intersection/rayTriangle.glsl
float RayTriangle(vec3 rayOrigin, vec3 rayDir, vec3 v1, vec3 v2, vec3 v3, float epsilon) {
    vec3 e1 = v2 - v1;
    vec3 e2 = v3 - v1;
    vec3 pvec = cross(rayDir, e2);
    float det = dot(e1, pvec);

    if (abs(det) < epsilon) {
        return -1.0;
    }

    float invDet = 1.0 / det;
    vec3 tvec = rayOrigin - v1;

    float u = invDet * dot(tvec, pvec);
    if (u < 0.0 || u > 1.0) {
        return -1.0;
    }

    vec3 qvec = cross(tvec, e1);
    float v = invDet * dot(rayDir, qvec);

    if (v < 0.0 || u + v > 1.0) {
        return -1.0;
    }

    return dot(e2, qvec) * invDet;
}

bool RayAABB(const Ray ray, const AABB aabb) {
    vec3 t1 = (aabb.min_bounds - ray.position) * ray.inverse_direction;
    vec3 t2 = (aabb.max_bounds - ray.position) * ray.inverse_direction;

    float tmin = max3(min(t1, t2));
    float tmax = min3(max(t1, t2));

    return tmax > 0.0 && tmin < tmax;
}

float FindIntersection(Ray ray, AABB bounding_box) {
    AABB bounding_box_stack[100];
    uint node_start_stack[100];
    uint stack_size = 0;

    if (RayAABB(ray, bounding_box)) {
        bounding_box_stack[0] = bounding_box;
        node_start_stack[0] = 0;
        stack_size = 1;
    }


    float closest_distance = INFINITY;
    uint closest_triangle_start;

    while (stack_size != 0) {
        stack_size--;
        AABB current_bounding_box = bounding_box_stack[stack_size];
        uint current_node_start = node_start_stack[stack_size];

        uint node_info = nodes[current_node_start];
        uint triangle_start = nodes[current_node_start + 1];

        uint child_count = node_info & uint(0x0000000F);
        uint triangle_count = (node_info >> 16);

        for (uint i = 0; i < triangle_count; i++) {
            uint ind_1 = indecies[triangle_start + 3 * i + 0];
            uint ind_2 = indecies[triangle_start + 3 * i + 1];
            uint ind_3 = indecies[triangle_start + 3 * i + 2];

            vec3 v1 = vertecies[ind_1].xyz;
            vec3 v2 = vertecies[ind_2].xyz;
            vec3 v3 = vertecies[ind_3].xyz;

            float t = RayTriangle(ray.position, ray.direction, v1, v2, v3, 0.000001);
            if (t >= 0.0 && t < closest_distance) {
                closest_distance = t;
                closest_triangle_start = triangle_start + 3 * i;
            }
        }

        uint child_pointers[8];
        for (uint child_index = 0; child_index < child_count; child_index++) {
            child_pointers[child_index] = nodes[current_node_start + 2 + child_index];
        }

        uint child_index = 0;
        vec3 mid_point = (current_bounding_box.max_bounds + current_bounding_box.min_bounds) / 2.0;

        for (uint i = 0; i < 8; i++) {
            if (bool(node_info & (uint(0x00000100) << i))) {
                vec3 child_min_bounds = vec3(
                    bool(i & 1) ? mid_point.x : current_bounding_box.min_bounds.x,
                    bool(i & 2) ? mid_point.y : current_bounding_box.min_bounds.y,
                    bool(i & 4) ? mid_point.z : current_bounding_box.min_bounds.z
                );
                vec3 child_max_bounds = vec3(
                    bool(i & 1) ? current_bounding_box.max_bounds.x : mid_point.x,
                    bool(i & 2) ? current_bounding_box.max_bounds.y : mid_point.y,
                    bool(i & 4) ? current_bounding_box.max_bounds.z : mid_point.z
                );

                AABB child_bounding_box = AABB(child_min_bounds, child_max_bounds);

                if (RayAABB(ray, child_bounding_box)) {
                    //fs_out_col = vec4(
                    //    bool(i & uint(1)) ? 1.0 : 0.2,
                    //    bool(i & uint(2)) ? 1.0 : 0.2,
                    //    bool(i & uint(4)) ? 1.0 : 0.2,
                    //    0.0
                    //);
                    uint child_start = child_pointers[child_index];

                    bounding_box_stack[stack_size] = child_bounding_box;
                    node_start_stack[stack_size] = child_start;
                    stack_size++;
                }
                child_index++;
            }
        }
    }


    if (isinf(closest_distance)) {
        fs_out_col = texture(skyboxTexture, ray.direction);
    } else {
        uint ind_1 = indecies[closest_triangle_start + 0];
        uint ind_2 = indecies[closest_triangle_start + 1];
        uint ind_3 = indecies[closest_triangle_start + 2];

        vec3 v1 = vertecies[ind_1].xyz;
        vec3 v2 = vertecies[ind_2].xyz;
        vec3 v3 = vertecies[ind_3].xyz;

        vec3 n1 = normals[ind_1].xyz;
        vec3 n2 = normals[ind_2].xyz;
        vec3 n3 = normals[ind_3].xyz;
        
        vec3 p = ray.position + closest_distance * ray.direction;
        vec3 uvw = Barycentric(p, v1, v2, v3);
        vec3 normal = uvw.x * n1 + uvw.y * n2 + uvw.z * n3;

        fs_out_col = vec4(vec3(normal), 1.0);
        //fs_out_col = texture(skyboxTexture, reflect(ray.direction, normal));
    }

    return 0.0;
}

void main() {
    vec2 ndc_coord = vs_out_ndc_coord;
    ndc_coord.x *= (width / height);
    vec4 projected_position = inv_view_proj_mat * vec4(ndc_coord, -1.0, 1.0);
    projected_position /= projected_position.w;
    vec3 ray_dir = normalize(projected_position.xyz - position);

    Ray ray = Ray(position, ray_dir, (1.0 / ray_dir));


    FindIntersection(ray, AABB(octree_min_bounds, octree_max_bounds));
    return;


    float closest_distance = INFINITY;

    vec3 normal;
    for (int i = 0; i < 968; i++) {
        //uvec3 ind = indecies[i].xyz;

        uint ind_1 = indecies[3 * i + 0];
        uint ind_2 = indecies[3 * i + 1];
        uint ind_3 = indecies[3 * i + 2];

        vec3 v1 = vertecies[ind_1].xyz;
        vec3 v2 = vertecies[ind_2].xyz;
        vec3 v3 = vertecies[ind_3].xyz;

        //uint ind_1 = indecies[3 * i + 0];
        //uint ind_2 = indecies[3 * i + 1];
        //uint ind_3 = indecies[3 * i + 2];

        //vec3 v1 = vec3(vertecies[3 * ind_1], vertecies[3 * ind_1 + 1], vertecies[3 * ind_1 + 2]);
        //vec3 v2 = vec3(vertecies[3 * ind_2], vertecies[3 * ind_2 + 1], vertecies[3 * ind_2 + 2]);
        //vec3 v3 = vec3(vertecies[3 * ind_3], vertecies[3 * ind_3 + 1], vertecies[3 * ind_3 + 2]);

        float t = RayTriangle(ray.position, ray.direction, v1, v2, v3, 0.00000001);
        if (t >= 0.0 && t < closest_distance) {
            closest_distance = t;

            vec3 p = ray.position + t * ray.direction;
            vec3 uvw = Barycentric(p, v1, v2, v3);

            vec3 n1 = normals[ind_1].xyz;
            vec3 n2 = normals[ind_2].xyz;
            vec3 n3 = normals[ind_3].xyz;

            normal = uvw.x * n1 + uvw.y * n2 + uvw.z * n3;
        }
    }

    if (isinf(closest_distance)) {
        fs_out_col = texture(skyboxTexture, ray.direction);
    } else {
        fs_out_col = texture(skyboxTexture, reflect(ray.direction, normal));
    }
}