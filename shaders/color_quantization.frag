/* vim: set filetype=glsl : */

in vec2 vs_uv;

uniform sampler2D frame;
uniform int n_levels;
uniform int n_samples;
uniform int radius;

out vec4 fs_color;

float quantize(float x, float n_levels) {
    return round(x * n_levels) / n_levels;
}

vec3 quantize_color(vec3 color, float n_levels) {
    if (n_levels == 0.0) {
        return color;
    }
    vec3 hsv = rgb2hsv(color);
    hsv.x = quantize(hsv.x, n_levels);
    hsv.z = quantize(hsv.z, n_levels);
    return hsv2rgb(hsv);
}

void main() {
    vec3 color = sample_texture(
            frame,
            vs_uv,
            float(n_samples),
            float(radius)
        );
    color = quantize_color(color, float(n_levels));
    fs_color = vec4(color, 1.0);
}
