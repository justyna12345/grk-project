#version 430 core

uniform vec3 objectColor;
uniform vec3 lightPos;
uniform vec3 cameraPos;

in vec3 interpNormal;
in vec3 vertexPos;

void main()
{
	vec3 lightDir = vertexPos - lightPos;
	lightDir = normalize(lightDir);
	float diffuse = dot(normalize(interpNormal), -lightDir);
	diffuse = max(diffuse, 0.0); 
	vec3 V = normalize(cameraPos - vertexPos);
	vec3 R = reflect(lightDir, normalize(interpNormal));
	float specular = dot(V, R);
	specular = max(specular, 0.0);
	specular = pow(specular, 10);
	gl_FragColor = vec4(objectColor * diffuse + vec3(1.0) * specular, 1.0);
}