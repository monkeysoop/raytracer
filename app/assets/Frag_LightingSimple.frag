#version 430

// Per fragment variables coming from the pipeline
in vec3 vs_out_pos;
in vec3 vs_out_norm;
in vec2 vs_out_tex;

// Outgoing values - fragment color
out vec4 fs_out_col;

// External parameters of the shader

// Directional light source 
uniform vec3 light_dir = vec3(-1,-2,-0.5);

// Light attributes: ambient, diffuse
uniform vec3 La = vec3(0.4, 0.4, 0.4);
uniform vec3 Ld = vec3(0.6, 0.6, 0.6);

layout(binding = 0) uniform sampler2D texImage;

/*
	    - normalize:	http://www.opengl.org/sdk/docs/manglsl/xhtml/normalize.xml
	    - dot:			http://www.opengl.org/sdk/docs/manglsl/xhtml/dot.xml
	    - clamp:		http://www.opengl.org/sdk/docs/manglsl/xhtml/clamp.xml
*/

void main()
{
	// Ambient
	vec3 ambient = La;
	
	// Diffuse
	vec3 normal = normalize( vs_out_norm );
	vec3 to_light = normalize( -light_dir );

	float cosa = clamp(dot(normal, to_light), 0, 1);

	vec3 diffuse = cosa*Ld;

	fs_out_col = vec4(ambient + diffuse, 1) * texture(texImage, vs_out_tex);
}