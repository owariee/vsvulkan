#version 450

layout(location = 0) in vec2 inPos;   // clip-space (-1..1)
layout(location = 1) in vec3 inColor; // RGB

layout(location = 0) out vec3 vColor;

void main() {
    vColor = inColor;
    gl_Position = vec4(inPos, 0.0, 1.0);
}
