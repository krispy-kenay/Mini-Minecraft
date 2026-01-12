#version 330 core

in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D u_FramebufferTexture;
uniform vec3 u_TintColor;

void main() {
    vec4 sceneColor = texture(u_FramebufferTexture, TexCoords);
    vec3 tintedColor = mix(sceneColor.rgb, u_TintColor, 0.5);
    FragColor = vec4(tintedColor, sceneColor.a);
}
