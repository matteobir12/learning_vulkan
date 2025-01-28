#version 450
layout(location = 0) out vec4 fragColor;
void main() {
    // Hard-coded triangle in clip space
    float x[3] = float[](0.0, 0.5, -0.5);
    float y[3] = float[](0.5, -0.5, -0.5);

    gl_Position = vec4(x[gl_VertexIndex], y[gl_VertexIndex], 0.0, 1.0);
    fragColor   = vec4(1.0, 0.0, 0.0, 1.0); // red
}
