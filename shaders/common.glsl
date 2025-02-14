#define PI 3.14159265359

const vec2 POISSON_DISK[87] = vec2[87](vec2(-0.488690, 0.046349), vec2(0.496064, 0.018367), vec2(-0.027347, -0.461505), vec2(-0.090074, 0.490283), vec2(0.294474, 0.366950), vec2(0.305608, -0.360041), vec2(-0.346198, -0.357278), vec2(-0.308924, 0.353038), vec2(-0.437547, -0.177748), vec2(0.446996, -0.129850), vec2(0.117621, -0.444649), vec2(0.171424, 0.418258), vec2(-0.227789, -0.410446), vec2(0.210264, -0.422608), vec2(-0.414136, -0.268376), vec2(0.368202, 0.316549), vec2(-0.480689, 0.127069), vec2(0.481128, -0.056358), vec2(-0.458004, -0.063002), vec2(0.409361, 0.201972), vec2(-0.176597, 0.424044), vec2(-0.095380, -0.441734), vec2(0.326086, -0.280594), vec2(-0.411327, 0.184757), vec2(-0.291534, -0.300406), vec2(0.400901, -0.002308), vec2(0.020255, 0.445511), vec2(0.302251, 0.275637), vec2(0.387805, -0.223370), vec2(-0.378395, 0.062614), vec2(0.405052, 0.101681), vec2(-0.010340, -0.355322), vec2(-0.034931, 0.383699), vec2(-0.318953, -0.225899), vec2(0.349283, -0.140001), vec2(-0.253974, 0.299183), vec2(0.188226, 0.342914), vec2(0.212083, -0.294545), vec2(-0.188320, -0.308466), vec2(-0.373708, -0.070538), vec2(0.114322, -0.356677), vec2(-0.154401, 0.348207), vec2(-0.321713, 0.260043), vec2(-0.086797, -0.349277), vec2(-0.360294, -0.144808), vec2(-0.323996, 0.188199), vec2(0.277830, -0.204128), vec2(0.087828, 0.351992), vec2(-0.215777, -0.234955), vec2(0.291437, 0.171860), vec2(0.027249, -0.255925), vec2(-0.316361, -0.013941), vec2(0.346679, -0.066942), vec2(-0.103280, -0.273636), vec2(-0.017802, 0.310973), vec2(-0.280809, -0.120043), vec2(-0.282912, 0.117500), vec2(0.267574, -0.036973), vec2(-0.034965, -0.223502), vec2(0.109677, 0.256372), vec2(-0.204519, -0.116846), vec2(0.144105, -0.181736), vec2(-0.140560, 0.215101), vec2(0.271573, 0.102406), vec2(0.220437, 0.203459), vec2(-0.242979, -0.027494), vec2(-0.050135, 0.239871), vec2(-0.152652, -0.193125), vec2(-0.220532, 0.179600), vec2(0.216867, -0.096770), vec2(-0.164884, 0.122109), vec2(0.251078, 0.034090), vec2(0.016515, -0.175206), vec2(0.042304, 0.216117), vec2(-0.133933, -0.060601), vec2(0.184659, 0.135680), vec2(-0.161273, 0.024207), vec2(-0.056532, -0.154410), vec2(-0.082706, 0.083129), vec2(0.081409, -0.088060), vec2(0.115078, 0.156566), vec2(0.133209, 0.061211), vec2(0.002618, -0.101328), vec2(0.132926, -0.013988), vec2(-0.027172, -0.017586), vec2(0.022969, 0.116469), vec2(0.036262, 0.015085));

float rand(vec2 seed) {
    return fract(sin(dot(seed.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

vec2 sample_poisson_disc(vec2 seed, int idx) {
    int offset_ = int(rand(seed) * 87.0);
    idx = (offset_ + idx) % 87;
    return POISSON_DISK[idx];
}

vec3 sample_texture(sampler2D tex, vec2 uv, float n_samples, float radius) {
    ivec2 size = textureSize(tex, 0);
    vec2 uv_step = 1.0 / vec2(size);
    vec3 color = vec3(0.0);
    float n = 0.0;
    for (int i = 0; i < int(n_samples); ++i) {
        vec2 disc = sample_poisson_disc(uv, i);
        vec2 uv_ = uv + radius * uv_step * disc;
        if (uv_.x >= 0.0 && uv_.x <= 1.0 && uv_.y >= 0.0 && uv_.y <= 1.0) {
            color += texture(tex, uv_).rgb;
            n += 1.0;
        }
    }
    color /= n;
    return color;
}

vec3 rgb2xyz(vec3 c) {
    vec3 tmp;
    tmp.x = (c.r > 0.04045) ? pow((c.r + 0.055) / 1.055, 2.4) : c.r / 12.92;
    tmp.y = (c.g > 0.04045) ? pow((c.g + 0.055) / 1.055, 2.4) : c.g / 12.92, tmp.z = (c.b > 0.04045) ? pow((c.b + 0.055) / 1.055, 2.4) : c.b / 12.92;
    const mat3 mat = mat3(0.4124, 0.3576, 0.1805, 0.2126, 0.7152, 0.0722, 0.0193, 0.1192, 0.9505);
    return 100.0 * tmp * mat;
}

vec3 lab2xyz(vec3 c) {
    float fy = (c.x + 16.0) / 116.0;
    float fx = c.y / 500.0 + fy;
    float fz = fy - c.z / 200.0;
    return vec3(95.047 * ((fx > 0.206897) ? fx * fx * fx : (fx - 16.0 / 116.0) / 7.787), 100.000 * ((fy > 0.206897) ? fy * fy * fy : (fy - 16.0 / 116.0) / 7.787), 108.883 * ((fz > 0.206897) ? fz * fz * fz : (fz - 16.0 / 116.0) / 7.787));
}

vec3 xyz2rgb(vec3 c) {
    const mat3 mat = mat3(3.2406, -1.5372, -0.4986, -0.9689, 1.8758, 0.0415, 0.0557, -0.2040, 1.0570);
    vec3 v = (c / 100.0) * mat;
    vec3 r;
    r.x = (v.r > 0.0031308) ? ((1.055 * pow(v.r, (1.0 / 2.4))) - 0.055) : 12.92 * v.r;
    r.y = (v.g > 0.0031308) ? ((1.055 * pow(v.g, (1.0 / 2.4))) - 0.055) : 12.92 * v.g;
    r.z = (v.b > 0.0031308) ? ((1.055 * pow(v.b, (1.0 / 2.4))) - 0.055) : 12.92 * v.b;
    return r;
}

vec3 xyz2lab(vec3 c) {
    vec3 n = c / vec3(95.047, 100, 108.883);
    vec3 v;
    v.x = (n.x > 0.008856) ? pow(n.x, 1.0 / 3.0) : (7.787 * n.x) + (16.0 / 116.0);
    v.y = (n.y > 0.008856) ? pow(n.y, 1.0 / 3.0) : (7.787 * n.y) + (16.0 / 116.0);
    v.z = (n.z > 0.008856) ? pow(n.z, 1.0 / 3.0) : (7.787 * n.z) + (16.0 / 116.0);
    return vec3((116.0 * v.y) - 16.0, 500.0 * (v.x - v.y), 200.0 * (v.y - v.z));
}

vec3 rgb2lab(vec3 c) {
    vec3 lab = xyz2lab(rgb2xyz(c));
    return vec3(lab.x / 100.0, 0.5 + 0.5 * (lab.y / 127.0), 0.5 + 0.5 * (lab.z / 127.0));
}

vec3 lab2rgb(vec3 c) {
    return xyz2rgb(lab2xyz(vec3(100.0 * c.x, 2.0 * 127.0 * (c.y - 0.5), 2.0 * 127.0 * (c.z - 0.5))));
}

vec3 rgb2hsv(vec3 c) {
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}
