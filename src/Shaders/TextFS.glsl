#version 330 core

layout(location = 0) out vec4 FragColor;

uniform sampler2D fontAtlas;
uniform vec3 textCol;

in vec2 TexCoord;

void main()
{
    vec4 col = texture(fontAtlas, TexCoord);

    vec3 newCol = textCol * col.r;
    FragColor = vec4(newCol, col.r);
}