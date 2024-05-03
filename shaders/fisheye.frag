/* vim: set filetype=glsl : */

in vec2 vs_uv;

uniform sampler2D frame;
uniform float strength;

out vec4 fs_color;

vec2 apply_fisheye(vec2 p) {
    vec2 center = vec2(0.5, 0.5);
    vec2 d = p - center;
    float r = sqrt(dot(d, d));
    float power = (2.0 * PI / (2.0 * sqrt(dot(center, center)))) * strength;

    float bind = power > 0.0 ? sqrt(dot(center, center)) : center.y;

    vec2 uv = p;
    if (power > 0.0) {
        uv = center + normalize(d) * tan(r * power) * bind / tan(bind * power);
    } else if (power < 0.0) {
        uv = center + normalize(d) * atan(r * -power * 10.0) * bind / atan(-power * bind * 10.0);
    }

    return uv;
}

void main(void) {
    vec2 uv = apply_fisheye(vs_uv);
    vec3 color = texture(frame, uv).rgb;
    fs_color = vec4(color, 1.0);
}
