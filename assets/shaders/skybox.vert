#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 2) in vec2 aTexCoord;
out vec2 TexCoord;
uniform mat4 uVP;
void main() {
    TexCoord = aTexCoord;
    gl_Position = uVP * vec4(aPos, 1.0);
}
