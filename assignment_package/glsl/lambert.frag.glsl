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
    // Material base color (before shading)
    // vec4 diffuseColor = fs_Col;
    // vec4 diffuseColor = vec4(texture(u_Texture, fs_UV).rgb, 1.);

    // // Add black lines between blocks (REMOVE WHEN YOU APPLY TEXTURES)
    // bool xBound = fract(fs_Pos.x) < 0.0125 || fract(fs_Pos.x) > 0.9875;
    // bool yBound = fract(fs_Pos.y) < 0.0125 || fract(fs_Pos.y) > 0.9875;
    // bool zBound = fract(fs_Pos.z) < 0.0125 || fract(fs_Pos.z) > 0.9875;
    // if((xBound && yBound) || (xBound && zBound) || (yBound && zBound)) {
    //     diffuseColor.rgb = vec3(0,0,0);
    // }

    vec4 diffuseColor;
    if (fs_Animated == 1.f) {
        vec2 timeUv = vec2(fs_UV.x + cos(u_Time * 2.f) * 0.01f, fs_UV.y);
        diffuseColor = vec4(texture(u_Texture, timeUv));
    } else {
        diffuseColor = vec4(texture(u_Texture, fs_UV));
    }

    // Calculate the diffuse term for Lambert shading
    float diffuseTerm = dot(normalize(fs_Nor), normalize(fs_LightVec));
    // Avoid negative lighting values
    diffuseTerm = clamp(diffuseTerm, 0, 1);

    float ambientTerm = 0.2;
    float lightIntensity = diffuseTerm + ambientTerm;   //Add a small float value to the color multiplier
    //to simulate ambient lighting. This ensures that faces that are not
    //lit by our point light are not completely black.

    // Compute final shaded color
    out_Col = vec4(diffuseColor.rgb * lightIntensity, diffuseColor.a);
}
