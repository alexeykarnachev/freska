/* vim: set filetype=glsl : */

in vec2 vs_uv;

uniform sampler2D frame;
uniform vec3 color;
uniform float threshold;
uniform int n_samples;
uniform int radius;

out vec4 fs_color;

float sample_outline(sampler2D tex, vec2 uv, float threshold, float n_samples, float radius) {
    vec2 uv_step = 1.0 / vec2(textureSize(tex, 0));
    vec3 prev_color = rgb2hsv(texture(tex, uv).rgb);
    float max_dist = 0.0;
    for (int i = 0; i < int(n_samples); ++i) {
        vec2 disc = sample_poisson_disc(uv, i);
        vec2 uv_ = uv + radius * uv_step * disc;
        if (uv_.x >= 0.0 && uv_.x <= 1.0 && uv_.y >= 0.0 && uv_.y <= 1.0) {
            vec3 curr_color = rgb2hsv(texture(tex, uv_).rgb);
            float dist = abs(prev_color.z - curr_color.z);
            max_dist = max(max_dist, dist);
            prev_color = curr_color;
        }
    }
    return float(max_dist > threshold);
}

void main(void) {
    float outline = sample_outline(
        frame,
        vs_uv,
        threshold,
        float(n_samples),
        float(radius)
    );

    vec3 frame_color = texture(frame, vs_uv).rgb;
    vec3 final_color = mix(frame_color, color, outline);
    fs_color = vec4(final_color, 1.0);
}

