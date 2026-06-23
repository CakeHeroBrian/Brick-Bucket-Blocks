#version 460 core

vec2 vertices[4] = vec2[](
	vec2(-0.012, 0.00),
	vec2(0.012, 0.00),

	vec2(0.0, -0.012),
	vec2(0.00, 0.012)
);

uniform float uAspectRatio;

void main() {
	vec2 position = vertices[gl_VertexID];
	position.x /= uAspectRatio;
	gl_Position = vec4(position, 0.0, 1.0);
}