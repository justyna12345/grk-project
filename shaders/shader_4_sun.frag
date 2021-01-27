#version 430 core

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

	gl_FragColor = vec4(texture, 1.0f);


}
