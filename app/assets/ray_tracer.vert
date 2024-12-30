#version 430

out vec2 vs_out_ndc_coord;

const vec2 vertices[3] = vec2[3](
	vec2(-1, -1), 
	vec2( 3, -1), 
	vec2(-1,  3)
);

void main() {
    gl_Position = vec4(vertices[gl_VertexID], 0, 1);
	vs_out_ndc_coord = gl_Position.xy;
}