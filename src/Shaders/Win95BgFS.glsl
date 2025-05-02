#version 330 core
layout(location = 0) out vec4 FragColor;
in vec2 TexCoord;

uniform vec2 pixelBounds;
uniform float opacity;

void main()
{
    vec2 uv = TexCoord;
    float scale = pixelBounds.x / 400.f;
    float aspect = pixelBounds.y / pixelBounds.x;
    uv = uv * 2.f - 1.f;
    uv.x *= -1.f;
    uv += 1.f;
    uv /= 2.f;
    vec3 bgCol = vec3(0.764);
    vec3 edgeCol = vec3(1.f);
    
    float edgeDist = min(uv.x, min(uv.y * aspect, min(1.f - uv.x, (1.f - uv.y) * aspect))) * scale;
    float edgeDistSmooth = smoothstep(0.99, 1.0, 1.f - edgeDist);
    if (uv.x + uv.y < 1.f) {
        edgeCol = vec3(0.2f);
    }
    vec3 outCol;
    outCol = mix(bgCol, edgeCol, edgeDistSmooth);
    // Output to screen
    FragColor = vec4(outCol, opacity);
}