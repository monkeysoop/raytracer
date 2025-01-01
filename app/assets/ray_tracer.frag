#version 430

layout(std430, binding = 0) buffer VerteciesBuffer {
    readonly vec4 vertecies[];
    //readonly float vertecies[];
};

layout(std430, binding = 1) buffer NormalsBuffer {
    readonly vec4 normals[];
    //readonly float normals[];
};

layout(std430, binding = 2) buffer IndeciesBuffer {
    readonly uvec4 indecies[];
    //readonly uint indecies[];
};

in vec2 vs_out_ndc_coord;

out vec4 fs_out_col;

uniform samplerCube skyboxTexture;

uniform mat4 inv_view_proj_mat;
uniform vec3 position;
uniform float width;
uniform float height;

struct Ray {
    vec3 position;
    vec3 direction;
};

// from https://gamedev.stackexchange.com/questions/23743/whats-the-most-efficient-way-to-find-barycentric-coordinates
vec3 Barycentric(vec3 p, vec3 a, vec3 b, vec3 c)
{
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
float rayTriangle(vec3 rayOrigin, vec3 rayDir, vec3 v1, vec3 v2, vec3 v3, float epsilon) {
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

void main() {
    vec2 ndc_coord = vs_out_ndc_coord;
    ndc_coord.x *= (width / height);
    vec4 projected_position = inv_view_proj_mat * vec4(ndc_coord, -1.0, 1.0);
    projected_position /= projected_position.w;
    vec3 ray_dir = normalize(projected_position.xyz - position);

    Ray ray = Ray(position, ray_dir);

    float closest_distance = 10000.0;

    vec3 normal;
    for (int i = 0; i < 968; i++) {
        uvec3 ind = indecies[i].xyz;

        vec3 v1 = vertecies[ind.x].xyz;
        vec3 v2 = vertecies[ind.y].xyz;
        vec3 v3 = vertecies[ind.z].xyz;

        //uint ind_1 = indecies[3 * i + 0];
        //uint ind_2 = indecies[3 * i + 1];
        //uint ind_3 = indecies[3 * i + 2];

        //vec3 v1 = vec3(vertecies[3 * ind_1], vertecies[3 * ind_1 + 1], vertecies[3 * ind_1 + 2]);
        //vec3 v2 = vec3(vertecies[3 * ind_2], vertecies[3 * ind_2 + 1], vertecies[3 * ind_2 + 2]);
        //vec3 v3 = vec3(vertecies[3 * ind_3], vertecies[3 * ind_3 + 1], vertecies[3 * ind_3 + 2]);

        float t = rayTriangle(ray.position, ray.direction, v1, v2, v3, 0.00000001);
        if (t >= 0.0 && t < closest_distance) {
            closest_distance = t;

            vec3 p = ray.position + t * ray.direction;
            vec3 uvw = Barycentric(p, v1, v2, v3);

            vec3 n1 = normals[ind.x].xyz;
            vec3 n2 = normals[ind.y].xyz;
            vec3 n3 = normals[ind.z].xyz;

            normal = uvw.x * n1 + uvw.y * n2 + uvw.z * n3;
        }
    }

    if (closest_distance == 10000.0) {
        fs_out_col = texture(skyboxTexture, ray.direction);
    } else {
        fs_out_col = texture(skyboxTexture, reflect(ray.direction, normal));
    }
}