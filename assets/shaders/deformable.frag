#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec4 VertexColor;
in float ScalarVal;

uniform vec3 uLightPos;
uniform int uVisMode;
uniform vec3 uBaseColor;
uniform float uMetallic;
uniform float uRoughness;

vec3 colormapTurbo(float x) {
    x = clamp(x, 0.0, 1.0);
    return clamp(vec3(
        1.6 * x - 0.3,
        sin(x * 3.14159),
        1.1 - 1.6 * x
    ), 0.0, 1.0);
}

vec3 colormapTemperature(float t) {
    t = clamp(t, 0.0, 1.0);
    if (t < 0.33) {
        return mix(vec3(0.05, 0.05, 0.1), vec3(0.85, 0.15, 0.0), t * 3.0);
    } else if (t < 0.66) {
        return mix(vec3(0.85, 0.15, 0.0), vec3(1.0, 0.75, 0.1), (t - 0.33) * 3.0);
    } else {
        return mix(vec3(1.0, 0.75, 0.1), vec3(1.0, 1.0, 1.0), (t - 0.66) * 3.0);
    }
}

vec3 colormapDamage(float d) {
    d = clamp(d, 0.0, 1.0);
    return mix(vec3(0.18, 0.72, 0.28), vec3(0.95, 0.12, 0.08), d);
}

void main() {
    vec3 N = normalize(Normal);
    vec3 L = normalize(uLightPos - FragPos);
    float diff = max(dot(N, L), 0.22);

    vec3 finalColor = uBaseColor * diff;

    if (uVisMode == 1) { // Von Mises Stress
        finalColor = colormapTurbo(ScalarVal) * diff;
    } else if (uVisMode == 2) { // Mechanical Strain
        finalColor = mix(vec3(0.1, 0.4, 0.8), vec3(1.0, 0.2, 0.8), ScalarVal) * diff;
    } else if (uVisMode == 3) { // Temperature Heatmap
        vec3 heatCol = colormapTemperature(ScalarVal);
        finalColor = heatCol * diff + heatCol * max(0.0, ScalarVal - 0.35) * 1.8; // Blackbody incandescence glow
    } else if (uVisMode == 4) { // Damage & Fracture
        finalColor = colormapDamage(ScalarVal) * diff;
    } else if (uVisMode == 5) { // Plastic Strain
        finalColor = mix(vec3(0.35, 0.35, 0.40), vec3(0.95, 0.45, 0.85), ScalarVal) * diff;
    } else if (uVisMode == 6) { // Tidal Gravity Field
        finalColor = mix(vec3(0.15, 0.30, 0.85), vec3(1.0, 0.90, 0.15), ScalarVal) * diff;
    }

    FragColor = vec4(finalColor, 1.0);
}
