#version 410 core
layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_tex_coords;

out vec3 normal;
out vec2 tex_coords;
out vec3 cam_rel_pos;

uniform mat4 M;
uniform mat4 PVM;

void main() {
    normal = a_normal;
    tex_coords = a_tex_coords;
    
    vec4 pos4 = vec4(a_pos, 1.0f);
    
    cam_rel_pos = (M * pos4).xyz;
    gl_Position = PVM * pos4;
}
