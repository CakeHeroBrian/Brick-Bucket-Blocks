#version 460 core

layout(location = 0) in vec3 fColor;
layout(location = 1) in vec2 fTexCoord;
layout(location = 2) in flat uint fTexLayer;
layout(location = 3) in vec3 fVertexPosion;
layout(location = 4) in float fSunlightStrength;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2DArray testTexture;

uniform vec4 fogColor;
uniform float fogStart;
uniform float fogEnd;
uniform float fogDensity;

uniform vec3 uViewPosition;

void main() {
	float dist = length(fVertexPosion - uViewPosition);
	float fogFactor = clamp((fogEnd - dist) / (fogEnd - fogStart), 0.0, 1.0);

	if (fogFactor < 1e-6) {
		discard;
	}

	vec4 textureColor = texture(testTexture, vec3(fTexCoord, fTexLayer));
	float shadowStrength = clamp(fSunlightStrength, 0.5 / 15.0, 1.0);
	outColor = mix(fogColor, vec4(textureColor.xyz * shadowStrength, textureColor.w), fogFactor);
}