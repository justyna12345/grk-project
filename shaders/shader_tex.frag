#version 430 core



struct Light {
	vec3 Pos;
	vec3 Color;

	//float diffStrength;
	//float specStrength;
};

#define lightsNumber 2

uniform sampler2D textureSampler;
uniform vec3 lightPos;

uniform vec3 objectColor;
uniform vec3 cameraPos;
uniform Light lights[lightsNumber];


in vec3 interpNormal;
in vec2 interpTexCoord;
in vec3 fragPos;



void main()
{
	vec3 fragColor = vec3(0.0f);
	
	vec4 color = texture2D(textureSampler, -interpTexCoord);
	vec3 normal = normalize(interpNormal);

	for (int i=0; i<lightsNumber; i++) {

		vec3 lightDir = normalize(lights[i].Pos - fragPos);

		vec3 V = normalize(cameraPos - fragPos);
		vec3 R = reflect(-normalize(lightDir), normal);

		//float distance = length(lights[i].Pos - fragPos);
		//float attenuation = 1.0f / (1.0f + 0.0014f * distance + 
		//0.000007f * (distance * distance));

		//if(i == 0){
			//diffStrength = 1.0f;
			//specStrength = 0.1f;
		//}
		//else{
			//diffStrength = 0.4f;
			//specStrength = 0.5f;
		//}

		float specular = pow(max(0, dot(R,V)), 1);
		float diffuse = max(0, dot(normal, normalize(lightDir)));

		vec3 texture = vec3(color.x, color.y, color.z) * lights[i].Color;

		fragColor += mix(texture, texture * diffuse + vec3(1) * specular * 0.3f, 0.9f) * lights[i].Color;
	}

	vec4 ambient = (0.1f, 0.1f, 0.1f, 1.0f) * color;
	gl_FragColor = vec4(fragColor, 1.0f) + ambient * 0.1f;

}
