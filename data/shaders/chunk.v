
#version 330 core

layout (location = 0) in uvec4 v0;
layout (location = 1) in uvec4 v1;

uniform vec4 ao_curve;
uniform float units_per_voxel;

uniform mat4 mvp;
uniform mat4 m;

const uint x_mask   = 0xff000000u;
const uint z_mask   = 0x00ff0000u;
const uint u_mask   = 0x0000ff00u;
const uint v_mask   = 0x000000ffu;

const uint y_mask   = 0xfff00000u;
const uint t_mask   = 0x000fff00u;
const uint ao0_mask = 0x000000c0u;
const uint ao1_mask = 0x00000030u;
const uint ao2_mask = 0x0000000cu;
const uint ao3_mask = 0x00000003u;

flat out uint f_t;
flat out vec4 f_ao;
out vec2 f_uv;
out vec3 f_n;
out float f_ah, f_d;

struct vert {
	vec3 pos;
	vec2 uv;
	vec4 ao;
	uint t;
};

vec3 unpack_pos(uvec2 i) {
	return vec3((i.x & x_mask) >> 24, (i.y & y_mask) >> 20, (i.x & z_mask) >> 16) / units_per_voxel;
}

vert unpack(uvec2 i) {

	vert o;
	
	o.pos   = vec3((i.x & x_mask) >> 24, (i.y & y_mask) >> 20, (i.x & z_mask) >> 16) / units_per_voxel;
	o.uv    = vec2((i.x & u_mask) >> 8, i.x & v_mask) / units_per_voxel;
	o.t     = (i.y & t_mask) >> 8;
	o.ao[0] = ao_curve[(i.y & ao0_mask) >> 6];
	o.ao[1] = ao_curve[(i.y & ao1_mask) >> 4];
	o.ao[2] = ao_curve[(i.y & ao2_mask) >> 2];
	o.ao[3] = ao_curve[(i.y & ao3_mask)];

	return o;
}

void main() {

	uvec2 verts[4] = uvec2[](v0.xy, v0.zw, v1.xy, v1.zw);

	int idx = gl_VertexID;

	vert v = unpack(verts[idx]);

	vec3 v1 = unpack_pos(verts[0]);
	vec3 v2 = unpack_pos(verts[1]);
	vec3 v3 = unpack_pos(verts[2]);

	gl_Position = mvp * vec4(v.pos, 1.0);
	
	vec3 m_pos = (m * vec4(v.pos, 1.0)).xyz;

	f_n = cross(v2 - v1, v3 - v1);
	f_uv = v.uv;
	f_ao = v.ao;
	f_t = v.t;
	
	f_ah = 0.5f * (m_pos.y / length(m_pos)) + 0.5f;
	f_d = length(m_pos.xz);
}
