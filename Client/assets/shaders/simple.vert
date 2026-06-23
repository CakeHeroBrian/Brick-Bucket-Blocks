#version 460 core

const vec3 vertices[36] = vec3[](
	// back
	vec3(1.0, 1.0, 0.0),
	vec3(1.0, 0.0, 0.0),
	vec3(0.0, 0.0, 0.0),
	vec3(0.0, 0.0, 0.0),
	vec3(0.0, 1.0, 0.0),
	vec3(1.0, 1.0, 0.0),
	
	// front
	vec3(0.0, 1.0, 1.0),
	vec3(0.0, 0.0, 1.0),
	vec3(1.0, 0.0, 1.0),
	vec3(1.0, 0.0, 1.0),
	vec3(1.0, 1.0, 1.0),
	vec3(0.0, 1.0, 1.0),

	// left 
	vec3(0.0, 1.0, 0.0),
	vec3(0.0, 0.0, 0.0),
	vec3(0.0, 0.0, 1.0),
	vec3(0.0, 0.0, 1.0),
	vec3(0.0, 1.0, 1.0),
	vec3(0.0, 1.0, 0.0),

	// right
	vec3(1.0, 1.0, 1.0),
	vec3(1.0, 0.0, 1.0),
	vec3(1.0, 0.0, 0.0),
	vec3(1.0, 0.0, 0.0),
	vec3(1.0, 1.0, 0.0),
	vec3(1.0, 1.0, 1.0),

	// top
	vec3(0.0, 1.0, 0.0),
	vec3(0.0, 1.0, 1.0),
	vec3(1.0, 1.0, 1.0),
	vec3(1.0, 1.0, 1.0),
	vec3(1.0, 1.0, 0.0),
	vec3(0.0, 1.0, 0.0),

	// bottom
	vec3(1.0, 0.0, 0.0),
	vec3(1.0, 0.0, 1.0),
	vec3(0.0, 0.0, 1.0),
	vec3(0.0, 0.0, 1.0),
	vec3(0.0, 0.0, 0.0),
	vec3(1.0, 0.0, 0.0)
);

const vec2 texCoords[6] = vec2[](
    vec2(0.0, 0.0),  // tri 1
    vec2(0.0, 1.0),
    vec2(1.0, 1.0),
    vec2(1.0, 1.0),  // tri 2
    vec2(1.0, 0.0),
    vec2(0.0, 0.0)
);

const vec3 normals[6] = vec3[](
	vec3(0.0, 0.0, -1.0),
	vec3(0.0, 0.0, 1.0),
	vec3(-1.0, 0.0, 0.0),
	vec3(1.0, 0.0, 0.0),
	vec3(0.0, 1.0, 0.0),
	vec3(0.0, -1.0, 0.0)
);

uniform mat4 uProjectionView;
uniform vec3 uViewPosition;
uniform mat4 uTransform;

//		 x		y		z	 f		 t
// 0b |000000|000000|000000|000|0000000000|0
layout(location = 0) in uint data;
layout(location = 1) in uint lightData;

layout(location = 0) out vec3 fColor;
layout(location = 1) out vec2 fTexCoord;
layout(location = 2) out flat uint fTexLayer;
layout(location = 3) out vec3 fVertexPosion;

layout(location = 4) out float fSunlightStrength;

void main() {
	float xOffset = float((data >> 26u) & 63u);
	float yOffset = float((data >> 20u) & 63u);
	float zOffset = float((data >> 14u) & 63u);
	uint face = (data >> 11u) & 7u;
	uint textureLayer = (data >> 1u) & 1023u;
	uint isReducedHeight = clamp(data & 1u,  0u, 1u);

	uint packedSunlightStrength = ((lightData >> 0u) & 0xF);

	vec3 vertex = vertices[face * 6 + gl_VertexID] * vec3(1.0, 1.0 - 0.125 * float(isReducedHeight), 1.0);
	//if (isReducedHeight == 1u) {
	//	vertex.y *= 0.875;
	//}

	vec3 vertexPosition = vec3(xOffset, yOffset, zOffset) + vertex;
	vec4 worldPosition = uTransform * vec4(vertexPosition, 1.0);

	gl_Position = uProjectionView * worldPosition;
	fTexCoord = texCoords[gl_VertexID];
	fTexLayer = textureLayer;
	fVertexPosion = worldPosition.xyz;

	fSunlightStrength = float(packedSunlightStrength) / 15.0;
}