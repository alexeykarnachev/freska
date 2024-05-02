/* vim: set filetype=glsl : */

in vec3 vertexPosition;

out vec2 vs_uv;

void main() {
    vs_uv = vertexPosition.xy;
    gl_Position = vec4(vertexPosition.xy * 2.0 - 1.0, 0.0, 1.0);
}

