/* vim: set filetype=glsl : */

in vec2 vs_uv;

uniform sampler2D frame;
uniform int pixel_size;

out vec4 fs_color;

vec2 pixelize(vec2 uv) {
    if (pixel_size <= 1) {
        return uv;
    }
    vec2 texture_size = vec2(textureSize(frame, 0));
    vec2 pixel_step = float(pixel_size) / texture_size;
    vec2 i_pixel = vec2(ivec2(uv / pixel_step));
    uv = i_pixel * pixel_step + pixel_step * 0.5;
    return uv;
}


void main(void) {
    vec2 uv = pixelize(vs_uv);
    vec3 color = texture(frame, uv).rgb;
    fs_color = vec4(color, 1.0);
}
