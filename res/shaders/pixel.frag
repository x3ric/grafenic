#version 330 core

uniform vec2 iMouse;
uniform float iTime;
uniform vec2 iResolution;
uniform sampler2D screenTexture;

in vec2 texCoord;
out vec4 fragColor;

float motion(in float intensity,in float time) {
   return (sin(iTime * time) + intensity) / 2.0;
}

vec3 rainbow(in float time) {
    float red = sin(time) * 0.5 + 0.5;
    float green = sin(time + 2.0/3.0 * 3.1415) * 0.5 + 0.5;
    float blue = sin(time + 4.0/3.0 * 3.1415) * 0.5 + 0.5;
    return vec3(red, green, blue);
}

vec4 waves( in vec3 Color, in vec2 fragCoord) {
    vec2 uv =  texCoord + (2.0 * fragCoord - iResolution.xy) / min(iResolution.x, iResolution.y);
    for(float i = 1.0; i < 10.0; i++){
        uv.x += 0.6 / i * cos(i * 2.5* uv.y + iTime);
        uv.y += 0.6 / i * cos(i * 1.5 * uv.x + iTime);
    }
    return vec4(Color/abs(sin(iTime-uv.y-uv.x)),1.0);   
}

vec4 blur(in sampler2D textureSampler, in vec2 texCoord, float intensity) {
    float blurSize = intensity / iResolution.x;
    float weights[9] = float[](0.05, 0.09, 0.12, 0.15, 0.16, 0.15, 0.12, 0.09, 0.05);
    vec4 col = vec4(0.0);
    float total = 0.0;
    for (int x = -4; x <= 4; x++) {
        for (int y = -4; y <= 4; y++) {
            vec2 samplePos = texCoord + vec2(x, y) * blurSize;
            vec4 sampleCol = texture(textureSampler, samplePos);
            float weight = weights[abs(x)] * weights[abs(y)];
            col += sampleCol * weight;
            total += weight;
        }
    }
    return vec4(col / total);
}

vec4 aberration(in sampler2D textureSampler, in vec2 texCoord, float intensity) {
    vec2 offset = vec2(intensity, -vec2(intensity, 0.0) / iResolution.xy) / iResolution.xy;
    float r = texture(textureSampler, texCoord + offset).r;
    float g = texture(textureSampler, texCoord).g;
    float b = texture(textureSampler, texCoord - offset).b;
    float a = texture(textureSampler, texCoord).a;
    return vec4(r, g, b, a);
}

vec4 glow(in sampler2D textureSampler, in vec2 texCoord, float intensity) {
    float blurSize = intensity / iResolution.x; 
    vec4 color = texture(textureSampler, texCoord);
    vec4 blurredColor = vec4(0.0);
    float total = 0.0;
    for (int x = -4; x <= 4; x++) {
        for (int y = -4; y <= 4; y++) {
            vec2 samplePos = texCoord + vec2(x, y) * blurSize;
            blurredColor += texture(textureSampler, samplePos);
            total += 1.0;
        }
    }
    blurredColor /= total;
    vec3 glowColor = mix(color.rgb, blurredColor.rgb, intensity);
    glowColor = clamp(glowColor, 0.0, 1.0);
    return vec4(glowColor, color.a); // Preserve original alpha
}

void mainImage(in vec2 texCoord, in vec2 fragCoord, out vec4 fragColor) {
    fragColor = texture(screenTexture, texCoord)
    //+ (aberration(screenTexture,texCoord,sin(iTime/2)*2.0) - texture(screenTexture, texCoord))
    //+ (blur(screenTexture,texCoord,sin(iTime)*1.0) - texture(screenTexture, texCoord)) 
    //+ (glow(screenTexture,texCoord,sin(iTime/2)*2.0) - texture(screenTexture, texCoord))
    //+ (waves(rainbow(iTime),fragCoord))
    ;
}

void main() {
    mainImage(texCoord, gl_FragCoord.xy, fragColor);
}
