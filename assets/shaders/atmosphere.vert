#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;

out vec3 FragPos;
out vec3 Normal;

uniform mat4 uMVP;
uniform mat4 uModel;

void main() {
    FragPos = vec3(uModel * vec4(aPos, 1.0));
    Normal = normalize(mat3(uModel) * aNormal);
    gl_Position = uMVP * vec4(aPos, 1.0);
}
