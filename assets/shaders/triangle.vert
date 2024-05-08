#version 460 core


layout(location = 0) in vec2 postion;
layout(location = 1) in vec4 color;

layout(location = 0) out vec4 ocolor;

void main()
{
    ocolor = color;
    gl_Position = vec4(postion, 0, 1.0);
}