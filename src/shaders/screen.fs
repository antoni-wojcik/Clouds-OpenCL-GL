#version 410 core

out vec4 fragColor;
in vec2 fragPos;

uniform sampler2D screenTexture;

void main() {
    fragColor = vec4(texture(screenTexture, fragPos).rgb, 1.0f); //= vec4(fragPos, 1.0f, 1.0f);//
}
