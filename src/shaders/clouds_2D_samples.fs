#version 410 core
in vec2 fragPos;
out vec4 FragColor;

uniform sampler3D sam;
uniform float time;

void main() {
    FragColor = vec4(texture(sam, vec3(fragPos, time*0.05f)).r, 0, 0, 1);
}
