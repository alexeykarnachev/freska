/* vim: set filetype=glsl : */

in vec2 vs_uv;

uniform sampler2D frame;
uniform vec3 white_balance;
uniform float exposure;
uniform float temperature;
uniform float contrast;
uniform float brightness;
uniform float saturation;
uniform float gamma;

out vec4 fs_color;

vec3 apply_color_correction(vec3 color) {
    // Exposure
    if (exposure >= 0.0) {
        color = vec3(1.0) - exp(-color * exposure);
    }

    // White balance
    vec3 linear_color = pow(color, vec3(2.2));
    linear_color *= white_balance;
    color = pow(linear_color, vec3(1.0 / 2.2));

    // Temperature
    vec3 lab = rgb2lab(color);
    lab *= temperature;
    color = lab2rgb(lab);

    // Contrast
    color = pow(color, vec3(contrast));

    // Brightness
    color = clamp(color + brightness, 0.0, 1.0);

    // Saturation
    vec3 hsv = rgb2hsv(color);
    hsv.y *= saturation;
    color = hsv2rgb(hsv);

    // Gamma
    color = pow(color, vec3(1.0 / gamma));

    return color;
}

void main() {
    vec3 color = texture(frame, vs_uv).rgb;
    color = apply_color_correction(color);
    fs_color = vec4(color, 1.0);
}
