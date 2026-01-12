#version 150
// ^ Change this to version 130 if you have compatibility issues

// This is a fragment shader. If you've opened this file first, please
// open and read lambert.vert.glsl before reading on.
// Unlike the vertex shader, the fragment shader actually does compute
// the shading of geometry. For every pixel in your program's output
// screen, the fragment shader is run for every bit of geometry that
// particular pixel overlaps. By implicitly interpolating the position
// data passed into the fragment shader by the vertex shader, the fragment shader
// can compute what color to apply to its pixel based on things like vertex
// position, light position, and vertex color.

uniform vec4 u_Color; // The color with which to render this instance of geometry.
uniform vec3 u_CamLook; // The camera's forward vector
uniform vec3 u_CamPos; // The camera's position

// These are the interpolated values out of the rasterizer, so you can't know
// their specific values without knowing the vertices that contributed to them
in vec4 fs_Pos;
in vec4 fs_Nor;
in vec4 fs_LightVec;
in vec4 fs_Col;

uniform sampler2D u_Texture;
in vec2 fs_UV;

// A constantly-increasing value updated in MyGL::tick()
uniform float u_Time;

in float fs_Animated;

out vec4 out_Col; // This is the final output color that you will see on your
// screen for the pixel that is currently being processed.

void main()
{
    vec4 baseColor;
    if (fs_Animated == 1.f) {
        vec2 timeUv = vec2(fs_UV.x + cos(u_Time * 2.f) * 0.01f, fs_UV.y);
        baseColor = vec4(texture(u_Texture, timeUv));
    } else {
        baseColor = vec4(texture(u_Texture, fs_UV));
    }

    vec3 color = baseColor.rgb;

    vec3 V = normalize(u_CamPos - fs_Pos.xyz); // view vector
    vec3 L = normalize(-u_CamLook); // light vector
    vec3 N = normalize(fs_Nor.xyz); // normalized surface normal
    vec3 H = normalize((V + L) / 2);
    vec3 lightColor = vec3(1, 1, 1);

    // Specular
    float exp = 100;
    float specularIntensity = max(pow(dot(H, N), exp), 0);
    vec3 specular = specularIntensity * lightColor;

    // Ambient
    vec3 ambient = color;

    // Diffuse lighting
    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = diff * lightColor;

    float ka = 0.2;
    float ks = 1.0;
    float kd = 1.0;

    // out_Color = color * (ka * ambient + ks * specular + kd * diffuse);
    out_Col = vec4(ka * ambient + kd * diffuse * color + specularIntensity, baseColor.w);
}
