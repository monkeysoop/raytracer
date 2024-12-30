#version 430

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

void main() {
    vec2 ndc_coord = vs_out_ndc_coord;
    ndc_coord.x *= (width / height);
    vec4 projected_position = inv_view_proj_mat * vec4(ndc_coord, -1.0, 1.0);
    projected_position /= projected_position.w;
    vec3 ray_dir = normalize(projected_position.xyz - position);

    Ray ray = Ray(position, ray_dir);

    fs_out_col = texture(skyboxTexture, ray.direction);
}