#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

out vec2 TexCoord;
out vec3 Normal;
out vec3 LocalPos;

uniform mat4 uMVP;

void main() {
    LocalPos = aPos;
    TexCoord = aTexCoord;
    Normal = aNormal;
    gl_Position = uMVP * vec4(aPos, 1.0);
}
