#version 150 core

uniform sampler2D u_Texture;

in vec2 fs_UV;
out vec4 out_Col;

uniform float u_Time;

vec2 random2(vec2 p)
{
    return fract(sin(vec2(dot(p, vec2(127.1, 311.7)),
                     dot(p, vec2(269.5,183.3))))
                     * 43758.5453);

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

float perlinNoise(vec2 uv) {
        float surfletSum = 0.f;
        // Iterate over the four integer corners surrounding uv
        for(int dx = 0; dx <= 1; ++dx) {
            for(int dy = 0; dy <= 1; ++dy) {
                    surfletSum += surflet(uv, floor(uv) + vec2(dx, dy));
            }
        }
        return surfletSum;
}

float WorleyNoise(vec2 uv, out vec2 cellPoint)
{
    vec2 uvInt = floor(uv);
    vec2 uvFract = fract(uv);

    float minDist = 1.0;
    float secondMinDist = 1.0;
    vec2 closestPoint;

    for (int y = -1; y <=1; y++) {
        for (int x = -1; x <=1; x++) {
            vec2 neighbor = vec2(float(x), float(y));

            vec2 point  = random2(uvInt + neighbor) * sin(u_Time);

            vec2 diff = neighbor + point - uvFract;

            float dist = length(diff);

            if (dist < minDist) {
                minDist = dist;
                cellPoint = neighbor + point + uvInt;
            }
        }
    }
    return minDist;
}

void main() {
    vec2 uv = fs_UV;

    float scalarPerlin = perlinNoise(uv + vec2(u_Time * 0.05));
    vec2 point;
    float scalarWorley = WorleyNoise(uv * 4.0 + scalarPerlin, point);

    // Distortion for lava movement
    vec2 distortion = uv + vec2(sin(u_Time + uv.y * 3.0) * 0.03, cos(u_Time + uv.x * 3.0) * 0.03);

    float smoothed = smoothstep(0.3, 0.7, scalarPerlin) * 0.5;

    vec3 color = texture(u_Texture, distortion).rgb * (smoothed + 0.8);

    // Fade colors for lava effect
    color *= vec3(1.0, 0.5, 0.2);

    out_Col = vec4(color, .9);
}
