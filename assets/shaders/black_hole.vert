#version 330 core
layout(location = 0) in vec3 aPos;

out vec2 TexCoord;
out vec3 RayDir;

uniform mat4 uInvViewProj;
uniform vec3 uCameraPos;

void main() {
    TexCoord = aPos.xy * 0.5 + 0.5;
    // Unproject far plane position in camera-relative world coordinates
    vec4 farPos = uInvViewProj * vec4(aPos.x, aPos.y, 1.0, 1.0);
    vec3 worldFar = farPos.xyz / farPos.w;
    RayDir = normalize(worldFar - uCameraPos);
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
}
