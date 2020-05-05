#version 410 core
layout (location = 0) in vec3 aPos;

out vec2 fragPos;

void main() {
    fragPos = vec2((aPos.x+1.0f)*0.5f, 1.0f-(aPos.y+1.0f)*0.5f);
    gl_Position = vec4(aPos.x, aPos.y, 0.0f, 1.0f);
}
