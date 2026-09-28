#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out vec3 LocalPos;

uniform mat4 uMVP;
uniform mat4 uModel;

void main() {
    LocalPos = aPos;
    FragPos = vec3(uModel * vec4(aPos, 1.0));
    Normal = normalize(mat3(uModel) * aNormal);
    TexCoord = aTexCoord;
    gl_Position = uMVP * vec4(aPos, 1.0);
}
