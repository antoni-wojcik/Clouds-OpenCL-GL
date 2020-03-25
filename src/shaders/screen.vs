#version 410 core
layout (location = 0) in vec3 aPos;

out vec2 fragPos;

void main() {
    fragPos = vec2((aPos.x+1.0)*0.5, 1.0-(aPos.y+1.0)*0.5); // instead of using UV for now
    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
}
