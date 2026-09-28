#version 330 core
in vec4 vColor;
layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 BrightColor;
void main() {
    FragColor = vColor;
    BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}
