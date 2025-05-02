#version 330 core

layout(location = 0) out vec4 FragColor;

uniform vec3 col;
uniform float opacity;

in vec2 TexCoord;

void main()
{
     gl_FragData[0] = vec4(col, opacity);
     gl_FragData[1] = vec4(0.f); // workaround 
}