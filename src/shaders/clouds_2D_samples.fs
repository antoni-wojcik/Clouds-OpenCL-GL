#version 410 core
in vec2 fragPos;
out vec4 fragColor;

uniform sampler3D sam;
uniform float time;

void main() {
    fragColor = vec4(texture(sam, vec3(fragPos, time*0.05f)).r, 0, 0, 1);
}
