#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec4 aColor;
layout(location = 3) in float aScalar;

out vec3 FragPos;
out vec3 Normal;
out vec4 VertexColor;
out float ScalarVal;

uniform mat4 uVP;

void main() {
    FragPos = aPos;
    Normal = aNormal;
    VertexColor = aColor;
    ScalarVal = aScalar;
    gl_Position = uVP * vec4(aPos, 1.0);
}
