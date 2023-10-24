#version 330 core

uniform vec2 iResolution;
uniform sampler2D screenTexture;
uniform sampler2D textTexture;  // SDF texture for text
uniform vec4 textColor;         // RGBA color for the text

in vec2 texCoord;
out vec4 fragColor;

void mainImage(in vec2 texCoord, in vec2 fragCoord, out vec4 fragColor) {
    vec4 originalColor = texture(screenTexture, texCoord);
    vec4 sdfSample = texture(textTexture, texCoord);
    float edgeSmooth = 0.5 / iResolution.x;
    float alpha = smoothstep(0.5 - edgeSmooth, 0.5 + edgeSmooth, sdfSample.r);
    vec4 blendedColor = mix(originalColor, textColor, alpha * textColor.a);
    fragColor = blendedColor;
}

void main() {
    mainImage(texCoord, gl_FragCoord.xy, fragColor);
}
