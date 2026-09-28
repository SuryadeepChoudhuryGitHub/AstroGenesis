#version 330 core
out vec4 FragColor;
in vec4 LineColor;

void main() {
    FragColor = LineColor;
}
