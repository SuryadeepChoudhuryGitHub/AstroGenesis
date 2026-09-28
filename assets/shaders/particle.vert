#version 330 core
layout(location = 0) in vec3 aPos;           // Mesh vertex position
layout(location = 1) in vec3 aNormal;        // Mesh vertex normal
layout(location = 2) in vec3 aInstancePos;   // Instance camera-relative position (AU)
layout(location = 3) in float aInstanceScale;// Instance scale
layout(location = 4) in vec4 aInstanceColor; // Instance color + opacity

out vec3 FragPos;
out vec3 Normal;
out vec4 InstanceColor;

uniform mat4 uVP;

void main() {
    vec3 worldPos = aInstancePos + aPos * aInstanceScale;
    FragPos = worldPos;
    Normal = aNormal;
    InstanceColor = aInstanceColor;
    gl_Position = uVP * vec4(worldPos, 1.0);
}
