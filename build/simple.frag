#version 330 core

//uniform vec3 objectColor;

in vec4 color_from_vshader;
out vec4 outColor;

void main()
{
    outColor = color_from_vshader;//vec4(objectColor,1);
}
