/* vim: set filetype=glsl : */

// Taken from here:
// https://www.shadertoy.com/view/ldXGW4

in vec2 vs_uv;

uniform sampler2D frame;
uniform float vert_jerk;
uniform float vert_movement;
uniform float bottom_static;
uniform float scanlines;
uniform float rgb_offset;
uniform float horz_fuzz;
uniform float time;

out vec4 fs_color;

vec3 mod289(vec3 x) {
    return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec2 mod289(vec2 x) {
    return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec3 permute(vec3 x) {
    return mod289(((x * 34.0) + 1.0) * x);
}

float snoise(vec2 v) {
    const vec4 C = vec4(
            0.211324865405187, // (3.0-sqrt(3.0))/6.0
            0.366025403784439, // 0.5*(sqrt(3.0)-1.0)
            -0.577350269189626, // -1.0 + 2.0 * C.x
            0.024390243902439 // 1.0 / 41.0
        );
    // First corner
    vec2 i = floor(v + dot(v, C.yy));
    vec2 x0 = v - i + dot(i, C.xx);

    // Other corners
    vec2 i1;
    i1 = (x0.x > x0.y) ? vec2(1.0, 0.0) : vec2(0.0, 1.0);
    vec4 x12 = x0.xyxy + C.xxzz;
    x12.xy -= i1;

    // Permutations
    i = mod289(i); // Avoid truncation effects in permutation
    vec3 p = permute(permute(i.y + vec3(0.0, i1.y, 1.0))
                + i.x + vec3(0.0, i1.x, 1.0));

    vec3 m = max(0.5 - vec3(dot(x0, x0), dot(x12.xy, x12.xy), dot(x12.zw, x12.zw)), 0.0);
    m = m * m;
    m = m * m;

    // Gradients: 41 points uniformly over a line, mapped onto a diamond.
    // The ring size 17*17 = 289 is close to a multiple of 41 (41*7 = 287)
    vec3 x = 2.0 * fract(p * C.www) - 1.0;
    vec3 h = abs(x) - 0.5;
    vec3 ox = floor(x + 0.5);
    vec3 a0 = x - ox;

    // Normalise gradients implicitly by scaling m
    // Approximation of: m *= inversesqrt( a0*a0 + h*h );
    m *= 1.79284291400159 - 0.85373472095314 * (a0 * a0 + h * h);

    // Compute final noise value at P
    vec3 g;
    g.x = a0.x * x0.x + h.x * x0.y;
    g.yz = a0.yz * x12.xz + h.yz * x12.yw;
    return 130.0 * dot(m, g);
}

float staticV(vec2 uv) {
    float staticHeight = snoise(vec2(9.0, time * 1.2 + 3.0)) * 0.3 + 5.0;
    float staticAmount = snoise(vec2(1.0, time * 1.2 - 6.0)) * 0.1 + 0.3;
    float staticStrength = snoise(vec2(-9.75, time * 0.6 - 3.0)) * 2.0 + 2.0;
    return (1.0 - step(snoise(vec2(5.0 * pow(time, 2.0) + pow(uv.x * 7.0, 1.2), pow((mod(time, 100.0) + 100.0) * uv.y * 0.3 + 3.0, staticHeight))), staticAmount)) * staticStrength;
}

vec3 apply_old_tv(vec2 uv) {
    float jerkOffset = (1.0 - step(snoise(vec2(time * 1.3, 5.0)), 0.8)) * 0.05;

    float fuzzOffset = snoise(vec2(time * 15.0, uv.y * 80.0)) * 0.003;
    float largeFuzzOffset = snoise(vec2(time * 1.0, uv.y * 25.0)) * 0.004;

    float vertMovementOn = (1.0 - step(snoise(vec2(time * 0.2, 8.0)), 0.4)) * vert_movement;
    float vertJerk = (1.0 - step(snoise(vec2(time * 1.5, 5.0)), 0.6)) * vert_jerk;
    float vertJerk2 = (1.0 - step(snoise(vec2(time * 5.5, 5.0)), 0.2)) * vert_jerk;
    float yOffset = abs(sin(time) * 4.0) * vertMovementOn + vertJerk * vertJerk2 * 0.3;
    float y = mod(uv.y + yOffset, 1.0);

    float xOffset = (fuzzOffset + largeFuzzOffset) * horz_fuzz;

    float staticVal = 0.0;

    for (float y = -1.0; y <= 1.0; y += 1.0) {
        float maxDist = 5.0 / 200.0;
        float dist = y / 200.0;
        staticVal += staticV(vec2(uv.x, uv.y + dist)) * (maxDist - abs(dist)) * 1.5;
    }

    staticVal *= bottom_static;

    float red = texture(frame, vec2(uv.x + xOffset - 0.01 * rgb_offset, y)).r + staticVal;
    float green = texture(frame, vec2(uv.x + xOffset, y)).g + staticVal;
    float blue = texture(frame, vec2(uv.x + xOffset + 0.01 * rgb_offset, y)).b + staticVal;

    vec3 color = vec3(red, green, blue);
    float scanline = sin(uv.y * 800.0) * 0.04 * scanlines;
    color -= scanline;

    return color;
}

void main(void) {
    vec3 color = texture(frame, vs_uv).rgb;
    color = apply_old_tv(vs_uv);
    fs_color = vec4(color, 1.0);
}
