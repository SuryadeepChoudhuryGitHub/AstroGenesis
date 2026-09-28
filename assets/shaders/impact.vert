#version 330 core
layout(location = 0) in vec3 aPos;

uniform mat4 uVP;
uniform vec3 uImpactCenter;
uniform vec3 uImpactNormal;
uniform float uFlashRadius;

void main() {
    vec3 pos = uImpactCenter + aPos * uFlashRadius;
    gl_Position = uVP * vec4(pos, 1.0);
}
