#version 150
// ^ Change this to version 130 if you have compatibility issues

//This is a vertex shader. While it is called a "shader" due to outdated conventions, this file
//is used to apply matrix transformations to the arrays of vertex data passed to it.
//Since this code is run on your GPU, each vertex is transformed simultaneously.
//If it were run on your CPU, each vertex would have to be processed in a FOR loop, one at a time.
//This simultaneous transformation allows your program to run much faster, especially when rendering
//geometry with millions of vertices.

uniform mat4 u_Model;       // The matrix that defines the transformation of the
                            // object we're rendering. In this assignment,
                            // this will be the result of traversing your scene graph.

uniform mat4 u_ModelInvTr;  // The inverse transpose of the model matrix.
                            // This allows us to transform the object's normals properly
                            // if the object has been non-uniformly scaled.

uniform mat4 u_ViewProj;    // The matrix that defines the camera's transformation.
                            // We've written a static matrix for you to use for HW2,
                            // but in HW3 you'll have to generate one yourself

uniform vec4 u_Color;       // When drawing the cube instance, we'll set our uniform color to represent different block types.

in vec4 vs_Pos;             // The array of vertex positions passed to the shader

in vec4 vs_Nor;             // The array of vertex normals passed to the shader

in vec4 vs_Col;             // The array of vertex colors passed to the shader.

in vec2 vs_UV;              // ADDED

out vec4 fs_Pos;
out vec4 fs_Nor;            // The array of normals that has been transformed by u_ModelInvTr. This is implicitly passed to the fragment shader.
out vec4 fs_LightVec;       // The direction in which our virtual light lies, relative to each vertex. This is implicitly passed to the fragment shader.
out vec4 fs_Col;            // The color of each vertex. This is implicitly passed to the fragment shader.

out vec2 fs_UV;             // ADDED

const vec4 lightDir = normalize(vec4(0.5, 1, 0.75, 0));  // The direction of our virtual light, which is used to compute the shading of
                                        // the geometry in the fragment shader.

// A constantly-increasing value updated in MyGL::tick()
uniform float u_Time;

in float vs_Animated;
out float fs_Animated;

vec2 random2(vec2 p)
{
    return fract(sin(vec2(dot(p, vec2(127.1, 311.7)),
                     dot(p, vec2(269.5,183.3))))
                     * 43758.5453);
}

vec3 random3( vec2 p ) {
    return fract(sin(vec3(dot(p,vec2(127.1, 311.7)),
                          dot(p,vec2(269.5, 183.3)),
                          dot(p, vec2(420.6, 631.2))
                    )) * 43758.5453);
}


float surflet(vec2 P, vec2 gridPoint) {
    // Compute falloff function by converting linear distance to a polynomial
    float distX = abs(P.x - gridPoint.x);
    float distY = abs(P.y - gridPoint.y);
    float tX = 1 - 6 * pow(distX, 5.f) + 15 * pow(distX, 4.f) - 10 * pow(distX, 3.f);
    float tY = 1 - 6 * pow(distY, 5.f) + 15 * pow(distY, 4.f) - 10 * pow(distY, 3.f);

    // Get the random vector for the grid point
    vec2 gradient = 2.f * random2(gridPoint) - vec2(1.f) * sin(u_Time);
    // Get the vector from the grid point to P
    vec2 diff = P - gridPoint;
    // Get the value of our height field by dotting grid->P with our gradient
    float height = dot(diff, gradient);
    // Scale our height field (i.e. reduce it) by our polynomial falloff function
    return height * tX * tY;
}

float surflet(vec3 p, vec3 gridPoint) {
    // Compute the distance between p and the grid point along each axis, and warp it with a
    // quintic function so we can smooth our cells
    vec3 t2 = abs(p - gridPoint);
    vec3 t = vec3(1.f) - 6.f * pow(t2.x, 5.f) * pow(t2.y, 5.f) + 15.f * pow(t2.x, 4.f) * pow(t2.y, 4.f) - 10.f * pow(t2.x, 3.f) * pow(t2.y, 3.f);
    // Get the random vector for the grid point (assume we wrote a function random2
    // that returns a vec2 in the range [0, 1])
    vec3 gradient = random3(gridPoint.xy) * 2. - vec3(1., 1., 1.);
    // Get the vector from the grid point to P
    vec3 diff = p - gridPoint;
    // Get the value of our height field by dotting grid->P with our gradient
    float height = dot(diff, gradient);
    // Scale our height field (i.e. reduce it) by our polynomial falloff function
    return height * t.x * t.y * t.z;
}

float perlinNoise3D(vec3 p) {
    float surfletSum = 0.f;
    // Iterate over the four integer corners surrounding uv
    for(int dx = 0; dx <= 1; ++dx) {
        for(int dy = 0; dy <= 1; ++dy) {
                for(int dz = 0; dz <= 1; ++dz) {
                        surfletSum += surflet(p, floor(p) + vec3(dx, dy, dz));
                }
        }
    }
    return surfletSum;
}

void main()
{
    fs_Pos = vs_Pos;
    fs_Col = vs_Col;                         // Pass the vertex colors to the fragment shader for interpolation
    fs_UV = vs_UV;
    fs_Animated = vs_Animated;

    mat3 invTranspose = mat3(u_ModelInvTr);
    fs_Nor = vec4(invTranspose * vec3(vs_Nor), 0);          // Pass the vertex normals to the fragment shader for interpolation.
                                                            // Transform the geometry's normals by the inverse transpose of the
                                                            // model matrix. This is necessary to ensure the normals remain
                                                            // perpendicular to the surface after the surface is transformed by
                                                            // the model matrix.


    vec4 modelposition = u_Model * vs_Pos;   // Temporarily store the transformed vertex positions for use below
    vec3 normal = vec3(vs_Nor);
    if (vs_Animated > 0.5) { // Only animate vertices marked as "animated"

        float waveAmplitude = 0.15; // Amplitude of the wave
        float waveFrequency = 2.0; // Frequency of the wave
        vec2 waveDirection = normalize(vec2(1.0, 0.5)); // Direction of the wave

        float noise = perlinNoise3D(vs_Pos.xyz);

        float waveOffset = dot(vec2(modelposition.x, modelposition.z), waveDirection); // Project onto wave direction
        modelposition.y += waveAmplitude * sin(noise * waveOffset - u_Time); // Apply sine wave

        // derivates for normal adjustment
        float dWave_dx = waveAmplitude * waveDirection.x * cos(noise * waveOffset - u_Time);
        float dWave_dz = waveAmplitude * waveDirection.y * cos(noise * waveOffset - u_Time);

        // normal based on the wave slopes
        vec3 tangentX = vec3(1.0, dWave_dx, 0.0); // Tangent in the x-direction
        vec3 tangentZ = vec3(0.0, dWave_dz, 1.0); // Tangent in the z-direction

        vec3 normalDistorted = normalize(cross(tangentZ, tangentX)); // Compute the new normal
        normal = normalize(mix(normalDistorted, vec3(vs_Nor), 0.3));
    }

    fs_Nor = vec4(invTranspose * normal, 0);

    modelposition = u_Model * modelposition;
    fs_LightVec = lightDir;

    gl_Position = u_ViewProj * modelposition;
}
