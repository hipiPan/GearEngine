#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 vertex;

void main() {
    gl_Position = vec4(vertex, 0.0, 1.0);
    gl_Position.y = -gl_Position.y;
}
