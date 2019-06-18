
#version 330 core

in vec2 f_uv;

out vec4 color;

uniform sampler2D col_tex;
uniform sampler2D pos_tex;
uniform sampler2D norm_tex;

uniform int debug_show;

void main() {

	vec3 c = texture(col_tex, f_uv).rgb;
	vec3 p = texture(pos_tex, f_uv).rgb;
	vec3 n = texture(norm_tex, f_uv).rgb;

	if(debug_show == 0) {	
		color = vec4(c, 1.0f);
	} else if(debug_show == 1) {
		color = vec4(p, 1.0f);
	} else {
		color = vec4(abs(n), 1.0f);
	} 
}
