#version 330 core
out vec4 FragColor;
in vec2 TexCoord;

uniform sampler2D uImage;
uniform bool uHorizontal;

// 9-tap Gaussian weights (normalized)
const float weight[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main() {
    vec2 tex_offset = 1.0 / vec2(textureSize(uImage, 0));
    vec3 result = texture(uImage, TexCoord).rgb * weight[0];
    if (uHorizontal) {
        for (int i = 1; i < 5; ++i) {
            result += texture(uImage, TexCoord + vec2(tex_offset.x * float(i), 0.0)).rgb * weight[i];
            result += texture(uImage, TexCoord - vec2(tex_offset.x * float(i), 0.0)).rgb * weight[i];
        }
    } else {
        for (int i = 1; i < 5; ++i) {
            result += texture(uImage, TexCoord + vec2(0.0, tex_offset.y * float(i))).rgb * weight[i];
            result += texture(uImage, TexCoord - vec2(0.0, tex_offset.y * float(i))).rgb * weight[i];
        }
    }
    FragColor = vec4(result, 1.0);
}
