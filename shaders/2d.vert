#version 450
layout(push_constant) uniform PushConstants {
    mat4 proj; // projection matrix
} pc;

layout(set = 0, binding = 0) uniform MyUniform {
    mat4 view;       // view matrix
    mat4 model;      // model matrix
} ubo;

layout(location = 0) in vec2 inPos;          // unit quad
layout(location = 1) in vec2 instancePos;    // per-instance
layout(location = 2) in vec2 instanceSize;
layout(location = 3) in vec4 instanceColor;

layout(location = 0) out vec4 fragColor;

void main() {
    vec2 pos = inPos * instanceSize + instancePos;
    gl_Position = pc.proj * vec4(pos, 0.0, 1.0);
    fragColor = instanceColor;
}
