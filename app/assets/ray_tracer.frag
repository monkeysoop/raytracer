#version 430

layout(std430, binding = 0) buffer VerteciesBuffer {
    readonly vec4 vertecies[];
};

layout(std430, binding = 1) buffer NormalsBuffer {
    readonly vec4 normals[];
};

layout(std430, binding = 2) buffer IndeciesBuffer {
    readonly uvec4 indecies[];
};

layout(std430, binding = 3) buffer NodesBuffer {
    readonly uint nodes[];
};

in vec2 vs_out_ndc_coord;

out vec4 fs_out_col;

uniform samplerCube skyboxTexture;

uniform mat4 inv_view_proj_mat;
uniform vec3 camera_position;
uniform float width;
uniform float height;

uniform vec3 octree_min_bounds;
uniform vec3 octree_max_bounds;

uniform uint max_recursion_limit;
uniform float time;
uniform float blur_amount;

uniform vec3 portal_position_1;
uniform vec3 portal_direction_1;
uniform vec3 portal_position_2;
uniform vec3 portal_direction_2;

uniform float portal_width;
uniform float portal_height;

uniform mat4 portal_1_to_2;
uniform mat4 portal_2_to_1;

uniform float z_near;
uniform float z_far;


struct Ray {
    vec3 position;
    vec3 direction;
    vec3 inverse_direction;
};

struct AABB {
    vec3 min_bounds;
    vec3 max_bounds;
};

struct HitInfo {
    bool has_hit;
    vec3 position;
    vec3 normal;
    uint material_id;
    uint portal_id;
};

struct Material {
    uint type;
    vec3 color;
    float roughness;
    float refractive_index;
};

struct Sphere {
    vec3 position;
    float radius;
};

struct Box {
    vec3 position;
    vec3 sizes;
};

struct Portal {
    vec3 position;
    vec3 normal;
};

const float INFINITY = 1.0 / 0.0;
const float PI = 3.1415926535897932384626433832795;

const uint LAMBERTIAN = 0;
const uint METAL = 1;
const uint DIELECTRIC = 2;

const uint NUM_OF_MATERIALS = 6;

const uint NUM_OF_SPHERES = 84;

const Material materials[NUM_OF_MATERIALS] = Material[NUM_OF_MATERIALS](
    Material(METAL, vec3(0.3, 0.5, 0.4), 0.1, 1.5),
    Material(LAMBERTIAN, vec3(0.0, 1.0, 0.0), 0.3, 1.5),
    Material(METAL, vec3(1.0, 1.0, 0.0), 0.9, 1.5),
    Material(METAL, vec3(1.0, 0.0, 0.0), 0.01, 1.5),
    Material(DIELECTRIC, vec3(0.0, 1.0, 1.0), 0.3, 1.8),
    Material(DIELECTRIC, vec3(0.0, 1.0, 1.0), 0.0, 1.5)
);

const uint NO_PORTAL = 0;
const uint PORTAL_1 = 1;
const uint PORTAL_2 = 2;


const Sphere spheres[NUM_OF_SPHERES] = Sphere[NUM_OF_SPHERES](
    Sphere(vec3( 0.000000, -1003.000000, 0.000000), 1000.000000),
    Sphere(vec3( -7.995381, 0.200000, -7.478668), 0.200000),
    Sphere(vec3( -7.696819, 0.200000, -5.468978), 0.200000),
    Sphere(vec3( -7.824804, 0.200000, -3.120637), 0.200000),
    Sphere(vec3( -7.132909, 0.200000, -1.701323), 0.200000),
    Sphere(vec3( -7.569523, 0.200000, 0.494554), 0.200000),
    Sphere(vec3( -7.730332, 0.200000, 2.358976), 0.200000),
    Sphere(vec3( -7.892865, 0.200000, 4.753728), 0.200000),
    Sphere(vec3( -7.656691, 0.200000, 6.888913), 0.200000),
    Sphere(vec3( -7.217835, 0.200000, 8.203466), 0.200000),
    Sphere(vec3( -5.115232, 0.200000, -7.980404), 0.200000),
    Sphere(vec3( -5.323222, 0.200000, -5.113037), 0.200000),
    Sphere(vec3( -5.410681, 0.200000, -3.527741), 0.200000),
    Sphere(vec3( -5.460670, 0.200000, -1.166543), 0.200000),
    Sphere(vec3( -5.457659, 0.200000, 0.363870), 0.200000),
    Sphere(vec3( -5.798715, 0.200000, 2.161684), 0.200000),
    Sphere(vec3( -5.116586, 0.200000, 4.470188), 0.200000),
    Sphere(vec3( -5.273591, 0.200000, 6.795187), 0.200000),
    Sphere(vec3( -5.120286, 0.200000, 8.731398), 0.200000),
    Sphere(vec3( -3.601565, 0.200000, -7.895600), 0.200000),
    Sphere(vec3( -3.735860, 0.200000, -5.163056), 0.200000),
    Sphere(vec3( -3.481116, 0.200000, -3.794556), 0.200000),
    Sphere(vec3( -3.866858, 0.200000, -1.465965), 0.200000),
    Sphere(vec3( -3.168870, 0.200000, 0.553099), 0.200000),
    Sphere(vec3( -3.428552, 0.200000, 2.627547), 0.200000),
    Sphere(vec3( -3.771736, 0.200000, 4.324785), 0.200000),
    Sphere(vec3( -3.768522, 0.200000, 6.384588), 0.200000),
    Sphere(vec3( -3.286992, 0.200000, 8.441148), 0.200000),
    Sphere(vec3( -1.552127, 0.200000, -7.728200), 0.200000),
    Sphere(vec3( -1.360796, 0.200000, -5.346098), 0.200000),
    Sphere(vec3( -1.287209, 0.200000, -3.735321), 0.200000),
    Sphere(vec3( -1.344859, 0.200000, -1.726654), 0.200000),
    Sphere(vec3( -1.974774, 0.200000, 0.183260), 0.200000),
    Sphere(vec3( -1.542872, 0.200000, 2.067868), 0.200000),
    Sphere(vec3( -1.743856, 0.200000, 4.752810), 0.200000),
    Sphere(vec3( -1.955621, 0.200000, 6.493702), 0.200000),
    Sphere(vec3( -1.350449, 0.200000, 8.068503), 0.200000),
    Sphere(vec3( 0.706123, 0.200000, -7.116040), 0.200000),
    Sphere(vec3( 0.897766, 0.200000, -5.938681), 0.200000),
    Sphere(vec3( 0.744113, 0.200000, -3.402960), 0.200000),
    Sphere(vec3( 0.867750, 0.200000, -1.311908), 0.200000),
    Sphere(vec3( 0.082480, 0.200000, 0.838206), 0.200000),
    Sphere(vec3( 0.649692, 0.200000, 2.525103), 0.200000),
    Sphere(vec3( 0.378574, 0.200000, 4.055579), 0.200000),
    Sphere(vec3( 0.425844, 0.200000, 6.098526), 0.200000),
    Sphere(vec3( 0.261365, 0.200000, 8.661150), 0.200000),
    Sphere(vec3( 2.814218, 0.200000, -7.751227), 0.200000),
    Sphere(vec3( 2.050073, 0.200000, -5.731364), 0.200000),
    Sphere(vec3( 2.020130, 0.200000, -3.472627), 0.200000),
    Sphere(vec3( 2.884277, 0.200000, -1.232662), 0.200000),
    Sphere(vec3( 2.644454, 0.200000, 0.596324), 0.200000),
    Sphere(vec3( 2.194283, 0.200000, 2.880603), 0.200000),
    Sphere(vec3( 2.281000, 0.200000, 4.094307), 0.200000),
    Sphere(vec3( 2.080841, 0.200000, 6.716384), 0.200000),
    Sphere(vec3( 2.287131, 0.200000, 8.583242), 0.200000),
    Sphere(vec3( 4.329136, 0.200000, -7.497218), 0.200000),
    Sphere(vec3( 4.502115, 0.200000, -5.941060), 0.200000),
    Sphere(vec3( 4.750631, 0.200000, -3.836759), 0.200000),
    Sphere(vec3( 4.082084, 0.200000, -1.180746), 0.200000),
    Sphere(vec3( 4.429173, 0.200000, 2.069721), 0.200000),
    Sphere(vec3( 4.277152, 0.200000, 4.297482), 0.200000),
    Sphere(vec3( 4.012743, 0.200000, 6.225072), 0.200000),
    Sphere(vec3( 4.047066, 0.200000, 8.419360), 0.200000),
    Sphere(vec3( 6.441846, 0.200000, -7.700798), 0.200000),
    Sphere(vec3( 6.047810, 0.200000, -5.519369), 0.200000),
    Sphere(vec3( 6.779211, 0.200000, -3.740542), 0.200000),
    Sphere(vec3( 6.430776, 0.200000, -1.332107), 0.200000),
    Sphere(vec3( 6.476387, 0.200000, 0.329973), 0.200000),
    Sphere(vec3( 6.568686, 0.200000, 2.116949), 0.200000),
    Sphere(vec3( 6.371189, 0.200000, 4.609841), 0.200000),
    Sphere(vec3( 6.011877, 0.200000, 6.569579), 0.200000),
    Sphere(vec3( 6.096087, 0.200000, 8.892333), 0.200000),
    Sphere(vec3( 8.185763, 0.200000, -7.191109), 0.200000),
    Sphere(vec3( 8.411960, 0.200000, -5.285309), 0.200000),
    Sphere(vec3( 8.047109, 0.200000, -3.427552), 0.200000),
    Sphere(vec3( 8.119639, 0.200000, -1.652587), 0.200000),
    Sphere(vec3( 8.818120, 0.200000, 0.401292), 0.200000),
    Sphere(vec3( 8.754155, 0.200000, 2.152549), 0.200000),
    Sphere(vec3( 8.595298, 0.200000, 4.802001), 0.200000),
    Sphere(vec3( 8.036216, 0.200000, 6.739752), 0.200000),
    Sphere(vec3( 8.256561, 0.200000, 8.129115), 0.200000),
    Sphere(vec3( 0.000000, 2.000000, 0.000000), 1.000000),
    Sphere(vec3( -4.000000, 2.000000, 0.000000), 1.000000),
    Sphere(vec3( 4.000000, 2.000000, 0.000000), 1.000000)
);

float min3(vec3 a) {
    return min(min(a.x, a.y), a.z);
}

float max3(vec3 a) {
    return max(max(a.x, a.y), a.z);
}




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

// from https://www.shadertoy.com/view/MtycDD
float RaySphere(Ray ray, Sphere sphere, float closest_distance) {
	vec3 oc = ray.position - sphere.position;
    float b = dot(oc, ray.direction);
    float c = dot(oc, oc) - sphere.radius * sphere.radius;
    float discriminant = b * b - c;
    if (discriminant < 0.0) {
        return INFINITY;
    }

	float s = sqrt(discriminant);
	float t1 = -b - s;
	float t2 = -b + s;
	
	float t = t1 < 0.0 ? t2 : t1;
    if (t < closest_distance && t > 0.0) {
	    return t;
    } else {
        return INFINITY;
    }
}

// from https://github.com/btmxh/glsl-intersect/blob/master/3d/intersection/rayTriangle.glsl
float RayTriangle(Ray ray, vec3 v1, vec3 v2, vec3 v3, float epsilon) {
    vec3 e1 = v2 - v1;
    vec3 e2 = v3 - v1;
    vec3 pvec = cross(ray.direction, e2);
    float det = dot(e1, pvec);

    if (abs(det) < epsilon) {
        return -1.0;
    }

    float invDet = 1.0 / det;
    vec3 tvec = ray.position - v1;

    float u = invDet * dot(tvec, pvec);
    if (u < 0.0 || u > 1.0) {
        return -1.0;
    }

    vec3 qvec = cross(tvec, e1);
    float v = invDet * dot(ray.direction, qvec);

    if (v < 0.0 || u + v > 1.0) {
        return -1.0;
    }

    return dot(e2, qvec) * invDet;
}

// from https://www.shadertoy.com/view/tl23Rm
float RayBox(Ray ray, Box box, float closest_distance, out vec3 normal) {
    vec3 m = sign(ray.direction) / max(abs(ray.direction), 1e-8);
    vec3 n = m * (ray.position - box.position);
    vec3 k = abs(m) * box.sizes;
	
    vec3 t1 = -n - k;
    vec3 t2 = -n + k;

	float tN = max(max(t1.x, t1.y), t1.z);
	float tF = min(min(t2.x, t2.y), t2.z);
	
    if (tN > tF || tF <= 0.0) {
        return INFINITY;
    } else {
        if (tN >= 0.0 && tN <= closest_distance) {
        	normal = -sign(ray.direction) * step(t1.yzx, t1.xyz) * step(t1.zxy, t1.xyz);
            return tN;
        } else if (tF >= 0.0 && tF <= closest_distance) { 
        	normal = -sign(ray.direction) * step(t1.yzx, t1.xyz) * step(t1.zxy, t1.xyz);
            return tF;
        } else {
            return INFINITY;
        }
    }
}
float RayCylinder( in vec3 ro, in vec3 rd, in vec2 distBound, inout vec3 normal, in vec3 pa, in vec3 pb, float ra ) {
    vec3 ca = pb-pa;
    vec3 oc = ro-pa;

    float caca = dot(ca,ca);
    float card = dot(ca,rd);
    float caoc = dot(ca,oc);
    
    float a = caca - card*card;
    float b = caca*dot( oc, rd) - caoc*card;
    float c = caca*dot( oc, oc) - caoc*caoc - ra*ra*caca;
    float h = b*b - a*c;
    
    if (h < 0.) return INFINITY;
    
    h = sqrt(h);
    float d = (-b-h)/a;

    float y = caoc + d*card;
    if (y > 0. && y < caca && d >= distBound.x && d <= distBound.y) {
        normal = (oc+d*rd-ca*y/caca)/ra;
        return d;
    }

    d = ((y < 0. ? 0. : caca) - caoc)/card;
    
    if( abs(b+a*d) < h && d >= distBound.x && d <= distBound.y) {
        normal = normalize(ca*sign(y)/caca);
        return d;
    } else {
        return INFINITY;
    }
}
float RayPortal(Ray ray, Portal portal, float closest_distance) {
    float d = dot(portal.normal, ray.direction);
    
    if (abs(d) <= 0.0001) {
        return INFINITY;
    }

    float t = dot((portal.position - ray.position), portal.normal) / d;
    
    if (t < 0.0 || t > closest_distance) {
        return INFINITY;
    }

    vec3 intersect_point = ray.position + t * ray.direction;

    vec3 plane_right = cross(portal.normal, vec3(0.0, 1.0, 0.0));
    if (length(plane_right) <= 0.0001) {
        return INFINITY;
    }
    
    plane_right = normalize(plane_right);
    vec3 plane_up = normalize(cross(plane_right, portal.normal));

    vec3 c = intersect_point - portal.position;

    if (abs(dot(plane_right, c)) < 0.5 * portal_width && abs(dot(plane_up, c)) < 0.5 * portal_height) {
        return t;
    } else {
        return INFINITY;
    }
}

float ComputeNonLinearDepth(float linear_depth) {
    return (z_near * z_far - linear_depth * z_far) / (linear_depth * (z_near - z_far));
}



// from https://www.shadertoy.com/view/Xt3cDn
uint baseHash(uvec2 p) {
    p = 1103515245U*((p >> 1U)^(p.yx));
    uint h32 = 1103515245U*((p.x)^(p.y>>3U));
    return h32^(h32 >> 16);
}
float hash1(inout float seed) {
    uint n = baseHash(floatBitsToUint(vec2(seed+=.1,seed+=.1)));
    return float(n)/float(0xffffffffU);
}
vec2 hash2(inout float seed) {
    uint n = baseHash(floatBitsToUint(vec2(seed+=.1,seed+=.1)));
    uvec2 rz = uvec2(n, n*48271U);
    return vec2(rz.xy & uvec2(0x7fffffffU))/float(0x7fffffff);
}
vec3 hash3(inout float seed) {
    uint n = baseHash(floatBitsToUint(vec2(seed+=.1,seed+=.1)));
    uvec3 rz = uvec3(n, n*16807U, n*48271U);
    return vec3(rz & uvec3(0x7fffffffU))/float(0x7fffffff);
}

// from https://www.shadertoy.com/view/MtycDD
vec3 random_cos_weighted_hemisphere_direction(const vec3 n, inout float seed) {
  	vec2 r = hash2(seed);
	vec3  uu = normalize(cross(n, abs(n.y) > .5 ? vec3(1.,0.,0.) : vec3(0.,1.,0.)));
	vec3  vv = cross(uu, n);
	float ra = sqrt(r.y);
	float rx = ra*cos(6.28318530718*r.x); 
	float ry = ra*sin(6.28318530718*r.x);
	float rz = sqrt(1.-r.y);
	vec3  rr = vec3(rx*uu + ry*vv + rz*n);
    return normalize(rr);
}
vec3 random_in_unit_sphere(inout float seed) {
    vec3 h = hash3(seed) * vec3(2.,6.28318530718,1.)-vec3(1,0,0);
    float phi = h.y;
    float r = pow(h.z, 1./3.);
	return r * vec3(sqrt(1.-h.x*h.x)*vec2(sin(phi),cos(phi)),h.x);
}

// from from: https://learnopengl.com/PBR/Lighting
vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// from https://www.shadertoy.com/view/tl23Rm
float FresnelSchlickRoughness(float cosTheta, float F0, float roughness) {
    return F0 + (max((1. - roughness), F0) - F0) * pow(abs(1. - cosTheta), 5.0);
}


bool RayAABB(const Ray ray, const AABB aabb, float closest_distance) {
    vec3 t1 = (aabb.min_bounds - ray.position) * ray.inverse_direction;
    vec3 t2 = (aabb.max_bounds - ray.position) * ray.inverse_direction;

    float tmin = max3(min(t1, t2));
    float tmax = min3(max(t1, t2));

    return tmax > 0.0 && tmin < tmax && tmin < closest_distance;
}

HitInfo FindAABBIntersection(Ray ray) {
    AABB bounding_box_stack[100];
    uint node_start_stack[100];
    uint stack_size = 0;

    float closest_distance = INFINITY;
    uint closest_triangle_start;

    AABB bounding_box = AABB(octree_min_bounds, octree_max_bounds);

    if (RayAABB(ray, bounding_box, closest_distance)) {
        bounding_box_stack[0] = bounding_box;
        node_start_stack[0] = 0;
        stack_size = 1;
    }



    while (stack_size != 0) {
        stack_size--;
        AABB current_bounding_box = bounding_box_stack[stack_size];
        uint current_node_start = node_start_stack[stack_size];

        uint node_info = nodes[current_node_start];
        uint triangle_start = nodes[current_node_start + 1];

        uint child_count = node_info & uint(0x0000000F);
        uint triangle_count = (node_info >> 16);

        for (uint i = 0; i < triangle_count; i++) {
            uvec4 ind = indecies[triangle_start + i];
            //uint ind_2 = indecies[triangle_start + i];
            //uint ind_3 = indecies[triangle_start + i];

            vec3 v1 = vertecies[ind.x].xyz;
            vec3 v2 = vertecies[ind.y].xyz;
            vec3 v3 = vertecies[ind.z].xyz;

            float t = RayTriangle(ray, v1, v2, v3, 0.000000000000001);
            if (t >= 0.0 && t < closest_distance) {
                closest_distance = t;
                closest_triangle_start = triangle_start + i;
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
                    bool(i & uint(1)) ? mid_point.x : current_bounding_box.min_bounds.x,
                    bool(i & uint(2)) ? mid_point.y : current_bounding_box.min_bounds.y,
                    bool(i & uint(4)) ? mid_point.z : current_bounding_box.min_bounds.z
                );
                vec3 child_max_bounds = vec3(
                    bool(i & uint(1)) ? current_bounding_box.max_bounds.x : mid_point.x,
                    bool(i & uint(2)) ? current_bounding_box.max_bounds.y : mid_point.y,
                    bool(i & uint(4)) ? current_bounding_box.max_bounds.z : mid_point.z
                );

                AABB child_bounding_box = AABB(child_min_bounds, child_max_bounds);

                if (RayAABB(ray, child_bounding_box, closest_distance)) {
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
        return HitInfo(false, vec3(0.0), vec3(0.0), 0, NO_PORTAL);
        //fs_out_col = texture(skyboxTexture, ray.direction);
    } else {
        uvec4 ind = indecies[closest_triangle_start];
        //uint ind_2 = indecies[closest_triangle_start + 1];
        //uint ind_3 = indecies[closest_triangle_start + 2];

        vec3 v1 = vertecies[ind.x].xyz;
        vec3 v2 = vertecies[ind.y].xyz;
        vec3 v3 = vertecies[ind.z].xyz;

        vec3 n1 = normals[ind.x].xyz;
        vec3 n2 = normals[ind.y].xyz;
        vec3 n3 = normals[ind.z].xyz;
        
        vec3 position = ray.position + closest_distance * ray.direction;
        vec3 uvw = Barycentric(position, v1, v2, v3);
        vec3 normal = uvw.x * n1 + uvw.y * n2 + uvw.z * n3;

        return HitInfo(true, position, normal, ind.w, NO_PORTAL);
        //fs_out_col = vec4(ind.w * vec3(normal), 1.0);
        //fs_out_col = texture(skyboxTexture, reflect(ray.direction, normal));
    }

    //return 0.0;
}

HitInfo FindSphereIntersection(Ray ray) {
    float closest_distance = INFINITY;
    uint closest_i;

    AABB bounding_box_stack[100];
    uint node_start_stack[100];
    uint stack_size = 0;

    uint closest_triangle_start;
    bool triangle_intersect = false;

    AABB bounding_box = AABB(octree_min_bounds, octree_max_bounds);

    if (RayAABB(ray, bounding_box, closest_distance)) {
        bounding_box_stack[0] = bounding_box;
        node_start_stack[0] = 0;
        stack_size = 1;
    }

    while (stack_size != 0) {
        stack_size--;
        AABB current_bounding_box = bounding_box_stack[stack_size];
        uint current_node_start = node_start_stack[stack_size];

        uint node_info = nodes[current_node_start];
        uint triangle_start = nodes[current_node_start + 1];

        uint child_count = node_info & uint(0x0000000F);
        uint triangle_count = (node_info >> 16);

        for (uint i = 0; i < triangle_count; i++) {
            uvec4 ind = indecies[triangle_start + i];
            //uint ind_2 = indecies[triangle_start + i];
            //uint ind_3 = indecies[triangle_start + i];

            vec3 v1 = vertecies[ind.x].xyz;
            vec3 v2 = vertecies[ind.y].xyz;
            vec3 v3 = vertecies[ind.z].xyz;

            float t = RayTriangle(ray, v1, v2, v3, 0.000000000000001);
            if (t >= 0.0 && t < closest_distance) {
                closest_distance = t;
                closest_triangle_start = triangle_start + i;
                triangle_intersect = true;
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
                    bool(i & uint(1)) ? mid_point.x : current_bounding_box.min_bounds.x,
                    bool(i & uint(2)) ? mid_point.y : current_bounding_box.min_bounds.y,
                    bool(i & uint(4)) ? mid_point.z : current_bounding_box.min_bounds.z
                );
                vec3 child_max_bounds = vec3(
                    bool(i & uint(1)) ? current_bounding_box.max_bounds.x : mid_point.x,
                    bool(i & uint(2)) ? current_bounding_box.max_bounds.y : mid_point.y,
                    bool(i & uint(4)) ? current_bounding_box.max_bounds.z : mid_point.z
                );

                AABB child_bounding_box = AABB(child_min_bounds, child_max_bounds);

                if (RayAABB(ray, child_bounding_box, closest_distance)) {
                    uint child_start = child_pointers[child_index];

                    bounding_box_stack[stack_size] = child_bounding_box;
                    node_start_stack[stack_size] = child_start;
                    stack_size++;
                }
                child_index++;
            }
        }
    }

    for (uint i = 0; i < NUM_OF_SPHERES; i++) {
        float t = RaySphere(ray, spheres[i], closest_distance);
        if (!isinf(t)) {
            closest_distance = t;
            closest_i = i;
        }
    }

    float portal_1_t = RayPortal(ray, Portal(portal_position_1, portal_direction_1), closest_distance);
    float portal_2_t = RayPortal(ray, Portal(portal_position_2, portal_direction_2), closest_distance);

    if (!isinf(portal_1_t) && portal_1_t < portal_2_t) {
        vec3 position = ray.position + portal_1_t * ray.direction;
        return HitInfo(true, position, portal_direction_1, 0, PORTAL_1);
    } else if (!isinf(portal_2_t) && portal_2_t < portal_1_t) {
        vec3 position = ray.position + portal_2_t * ray.direction;
        return HitInfo(true, position, portal_direction_2, 0, PORTAL_2);
    }

    if (isinf(closest_distance)) {
        return HitInfo(false, vec3(0.0), vec3(0.0), 0, NO_PORTAL);
    } else {
        if (triangle_intersect) {
            uvec4 ind = indecies[closest_triangle_start];

            vec3 v1 = vertecies[ind.x].xyz;
            vec3 v2 = vertecies[ind.y].xyz;
            vec3 v3 = vertecies[ind.z].xyz;

            vec3 n1 = normals[ind.x].xyz;
            vec3 n2 = normals[ind.y].xyz;
            vec3 n3 = normals[ind.z].xyz;
        
            vec3 position = ray.position + closest_distance * ray.direction;
            vec3 uvw = Barycentric(position, v1, v2, v3);
            vec3 normal = uvw.x * n1 + uvw.y * n2 + uvw.z * n3;

            return HitInfo(true, position, normal, ind.w, NO_PORTAL);
        } else {
            vec3 position = ray.position + closest_distance * ray.direction;
            vec3 normal = normalize(position - spheres[closest_i].position);
            return HitInfo(true, position, normal, closest_i % NUM_OF_MATERIALS, NO_PORTAL);
        }
    }
}

//HitInfo FindSphereIntersection(Ray ray) {
//    float closest_distance = INFINITY;
//    uint closest_i;
//    vec3 normal;
//
//    for (uint i = 0; i < NUM_OF_SPHERES; i++) {
//        vec3 n;
//        float t = RayBox(ray, Box(spheres[i].position, vec3(spheres[i].radius)), closest_distance, n);
//        //float t = RayPortal(ray, Portal(spheres[i].position, normalize(spheres[i].position)), closest_distance);
//        if (!isinf(t)) {
//            closest_distance = t;
//            closest_i = i;
//            normal = n;
//        }
//    }
//
//    if (isinf(closest_distance)) {
//        return HitInfo(false, vec3(0.0), vec3(0.0), 0);
//    } else {
//        vec3 position = ray.position + closest_distance * ray.direction;
//        //vec3 normal = normalize(position - spheres[closest_i].position);
//        return HitInfo(true, position, normal, closest_i % NUM_OF_MATERIALS);
//    }
//}

float linearize_depth(float d,float zNear,float zFar)
{
    return zNear * zFar / (zFar + d * (zNear - zFar));
}

void RayTrace(Ray r, inout float seed) {
    vec3 color = vec3(1.0);
    float depth = 1.0;
    float linear_depth;
    Ray ray = r;
    for (uint i = 0; i < max_recursion_limit; i++) {
        //HitInfo hit_info = FindIntersection(ray);
        HitInfo hit_info = FindSphereIntersection(ray);
        if (i == 0) {
            if (hit_info.has_hit) {
                linear_depth = length(hit_info.position - ray.position);
                depth = clamp(ComputeNonLinearDepth(length(hit_info.position - ray.position)), 0.0, 1.0);
            } else {
                linear_depth = z_far;
                depth = clamp(ComputeNonLinearDepth(z_far), 0.0, 1.0);
            }
        }

        if (hit_info.has_hit) {
            if (hit_info.portal_id == PORTAL_1) {
                if (dot(ray.direction, portal_direction_1) < 0.0) {
                    color *= 0.5;
                } else {
                    color *= 0.05;
                }

                ray.position = (portal_1_to_2 * vec4((hit_info.position - portal_position_1), 1.0)).xyz + portal_position_2;
                ray.direction = normalize((portal_1_to_2 * vec4(ray.direction, 0.0)).xyz);
                ray.position += 0.001 * ray.direction;
            } else if (hit_info.portal_id == PORTAL_2) {
                if (dot(ray.direction, portal_direction_2) < 0.0) {
                    color *= 0.5;
                } else {
                    color *= 0.05;
                }

                ray.position = (portal_2_to_1 * vec4((hit_info.position - portal_position_2), 1.0)).xyz + portal_position_1;
                ray.direction = normalize((portal_2_to_1 * vec4(ray.direction, 0.0)).xyz);
                ray.position += 0.001 * ray.direction;
            } else if (hit_info.portal_id == NO_PORTAL) {
                Material material = materials[hit_info.material_id];

                if (material.type == LAMBERTIAN) {
                    float F = FresnelSchlickRoughness(max(-dot(ray.direction, hit_info.normal), 0.0), 0.04, material.roughness);

                    ray.position = hit_info.position + 0.001 * hit_info.normal;
                    if (hash1(seed) > F) {
                        color *= material.color;
                        ray.direction = random_cos_weighted_hemisphere_direction(hit_info.normal, seed);
                    } else {
                        ray.direction = normalize(reflect(ray.direction, hit_info.normal) + material.roughness * random_in_unit_sphere(seed));
                    }
                } else if (material.type == METAL) {
                    ray.position = hit_info.position + 0.001 * hit_info.normal;
                    ray.direction = normalize(reflect(ray.direction, hit_info.normal) + material.roughness * random_in_unit_sphere(seed));

                    color *= material.color;
                } else if (material.type == DIELECTRIC) {
                    float refractive_index;
                    float cosine;
                    vec3 outgoing_normal;

                    if (dot(ray.direction, hit_info.normal) > 0.0) {
                        refractive_index = material.refractive_index;
                        cosine = dot(ray.direction, hit_info.normal);
                        cosine = sqrt(1.0 - refractive_index * refractive_index * (1.0 - cosine * cosine));
                        outgoing_normal = -1.0 * hit_info.normal;
                    } else {
                        refractive_index = 1.0 / material.refractive_index;
                        cosine = -1.0 * dot(ray.direction, hit_info.normal);
                        outgoing_normal = hit_info.normal;
                    }

                    vec3 modified_direction = ray.direction + material.roughness * random_in_unit_sphere(seed);
                    //vec3 modified_direction = ray.direction;
                    vec3 refracted_direction = normalize(refract(modified_direction, outgoing_normal, refractive_index));

                    if (refracted_direction != vec3(0.0)) {
                        float r = (1.0 - refractive_index) / (1.0 + refractive_index);
                        float F = FresnelSchlickRoughness(cosine, r * r, material.roughness);
                        if (hash1(seed) > F) {
                            ray.position = hit_info.position - 0.001 * outgoing_normal;
                            ray.direction = refracted_direction;
                        } else {
                            ray.position = hit_info.position + 0.001 * outgoing_normal;
                            ray.direction = normalize(reflect(modified_direction, outgoing_normal));
                        }
                    } else {
                        // internal reflection
                        ray.position = hit_info.position - 0.001 * outgoing_normal;
                        ray.direction = normalize(reflect(modified_direction, outgoing_normal));
                    }



                }
            }

            ray.inverse_direction = 1.0 / ray.direction;


        } else {
            color *= texture(skyboxTexture, ray.direction).xyz;
            break;
        }
    }
    color = max(vec3(0.0), color - 0.004);
    color = (color * (6.2 * color + 0.5)) / (color * (6.2 * color + 1.7) + 0.06);

    fs_out_col = vec4(color, 0.0);
    gl_FragDepth = clamp(depth, 0.0, 1.0);
}

void main() {
    vec2 ndc_coord = vs_out_ndc_coord;
    vec4 projected_position = inv_view_proj_mat * vec4(ndc_coord, -1.0, 1.0);
    projected_position /= projected_position.w;

    float seed = float(baseHash(floatBitsToUint(projected_position.xy - time)))/float(0xffffffffU);
    seed = float(baseHash(floatBitsToUint(vec2(seed, seed))))/float(0xffffffffU);
    
    vec3 ray_dir = normalize(projected_position.xyz - (camera_position + blur_amount * random_in_unit_sphere(seed)));

    Ray ray = Ray(camera_position, ray_dir, (1.0 / ray_dir));

    RayTrace(ray, seed);
}