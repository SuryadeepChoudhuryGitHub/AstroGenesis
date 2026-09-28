#version 330 core
layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 BrightColor;

uniform vec3 uImpactColor;
uniform float uIntensity;
uniform float uAge;

void main() {
    float alpha = uIntensity * (1.0 - uAge * 0.33);
    vec3 col = uImpactColor * 2.0;
    FragColor = vec4(col, clamp(alpha, 0.0, 1.0));
    BrightColor = vec4(col * alpha * 1.5, 1.0);
}
