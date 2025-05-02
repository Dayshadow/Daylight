#version 330

in vec2 TexCoord;
uniform float iTime;
layout(location = 0) out vec4 FragColor;

vec2 rotateUV(vec2 uv, float rotation)
{
    float mid = 0.5;
    return vec2(
        cos(rotation) * (uv.x - mid) + sin(rotation) * (uv.y - mid) + mid,
        cos(rotation) * (uv.y - mid) - sin(rotation) * (uv.x - mid) + mid
    );
}

void main()
{
    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv = TexCoord;
    
    vec2 uvd = rotateUV(uv, 1.0) + (sin(uv.x*17.0 + iTime/1.0) / (8.0 + (uv.x * iTime)/80.0));

    
    float waveStrength = abs(cos(uvd.x * 20.0 + iTime)) + fract(sin((uvd.x + uv.x) * 10.0));
    
    float highlight = 1.0 - smoothstep(0.0, 0.5, waveStrength);
    
    vec3 highlightCol = vec3(uvd, uv.x);

    vec3 col = vec3(0.011, 0.098, 0.866) * waveStrength + highlightCol * highlight;

    // Output to screen
    FragColor = vec4(col,1.0);
}

