#version 430 core

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

uniform vec3 objectColor;
uniform vec3 cameraPos;

uniform sampler2D textureSampler;


in vec2 interpTexCoord;
in vec3 interpNormal;
in vec3 fragPos;

void main()
{
	vec4 color = texture2D(textureSampler, -interpTexCoord);
	vec3 texture = vec3(color.x, color.y, color.z);


	vec3 normal = normalize(interpNormal);
	vec3 V = normalize(cameraPos-fragPos);
	float coef = max(0,dot(V,normal));

	//fragColor += mix(texture, texture * diffuse + vec3(1) * specular, 0.9f) * lights[i].Color;

	FragColor = vec4(texture * 3, 1.0f);
    //float brightness = dot(FragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    BrightColor = vec4(FragColor.rgb, 1.0);


}
