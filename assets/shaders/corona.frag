#version 330 core
layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 BrightColor;

in vec2 TexCoord;
in vec3 Normal;
in vec3 LocalPos;

uniform vec3 uCoronaColor;
uniform float uCoronaIntensity;
uniform float uSimTime;

void main() {
    float distFromCenter = length(LocalPos.xy);
    if (distFromCenter > 1.0) discard;

    // Radial exponential falloff
    float radial = pow(1.0 - distFromCenter, 2.2);

    // Dynamic solar flare prominences
    float angle = atan(LocalPos.y, LocalPos.x);
    float flare = sin(angle * 9.0 + uSimTime * 2.0) * cos(angle * 14.0 - uSimTime * 1.5);
    float flareIntensity = 1.0 + 0.30 * flare;

    float alpha = clamp(radial * flareIntensity * uCoronaIntensity * 0.65, 0.0, 1.0);
    vec3 col = uCoronaColor * (1.3 + 0.4 * flare);

    FragColor = vec4(col, alpha);
    BrightColor = vec4(col * alpha * 2.2, 1.0);
}
