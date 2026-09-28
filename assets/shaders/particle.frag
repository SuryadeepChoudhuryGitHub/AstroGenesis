#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec4 InstanceColor;

uniform vec3 uLightPos; // Camera-relative Sol position

void main() {
    vec3 lightDir = normalize(uLightPos - FragPos);
    vec3 norm = normalize(Normal);

    // Directional point diffuse from Sol (no artificial ambient floor in space)
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 color = InstanceColor.rgb * diff;

    FragColor = vec4(color, InstanceColor.a);
}
