#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;


layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform sampler2D texSampler[2];

layout(push_constant) uniform Push {
	mat4 modalMatrix;
	mat4 normalMatrix;
	int textureID;
} push;

void main()
{
	if(push.textureID == 0)
	outColor = texture(texSampler[0], fragTexCoord);
	if(push.textureID == 1)
	outColor = texture(texSampler[1], fragTexCoord);
}