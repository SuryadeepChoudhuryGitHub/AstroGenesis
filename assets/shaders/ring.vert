#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;

out vec3 FragPos;
out vec3 WorldNormal;
out vec2 TexCoord;
out float NormalizedRadius;

uniform mat4 uMVP;
uniform mat4 uModel;
uniform mat3 uNormalMat;
uniform float uInnerRadius;
uniform float uOuterRadius;

void main() {
    float r = mix(uInnerRadius, uOuterRadius, aTexCoord.x);
    vec3 pos = vec3(aPos.x * r, aPos.y, aPos.z * r);
    FragPos = vec3(uModel * vec4(pos, 1.0));
    WorldNormal = normalize(uNormalMat * vec3(0.0, 1.0, 0.0));
    TexCoord = aTexCoord;
    NormalizedRadius = aTexCoord.x;
    gl_Position = uMVP * vec4(pos, 1.0);
}
