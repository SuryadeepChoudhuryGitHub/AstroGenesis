#version 330 core
layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 BrightColor;
in vec2 TexCoord;
uniform sampler2D uTexture;
uniform bool uHasTexture;
uniform bool uCinematicMode;

void main() {
    if (uHasTexture) {
        vec3 rawCol = texture(uTexture, TexCoord).rgb;
        if (uCinematicMode) {
            // Linearize from sRGB to radiometric space
            vec3 linearSky = pow(rawCol, vec3(2.2));
            float lum = dot(linearSky, vec3(0.2126, 0.7152, 0.0722));
            
            // Deep space void drops to black, while subtle Milky Way and pinpoint stars remain
            vec3 skyCol = pow(linearSky, vec3(1.25)) * 0.25;
            if (lum > 0.40) {
                skyCol += linearSky * (lum - 0.40) * 0.75; // Crisp pinpoint stars
            }
            FragColor = vec4(skyCol, 1.0);
        } else {
            FragColor = vec4(rawCol, 1.0);
        }
    } else {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
    BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}
