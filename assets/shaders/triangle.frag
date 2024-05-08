#version 460 core

layout(location = 0) out vec4 FragColor;

layout(location = 0) in vec4 ocolor;

void main()
{
    FragColor = vec4(0.6, 0.3, 0.4, 1.0);
    FragColor = ocolor;
}