#version 330 core

in vec3 vPos;
in vec3 vColor;
in vec3 vNormal;

out vec3 passColor;
out vec3 FragPos;
out vec3 Normal;

uniform mat4 transform;
uniform mat4 model;
uniform mat3 normalMatrix;

void main()
{
	gl_Position =  transform * vec4(vPos, 1.0);

	FragPos = vec3(model * vec4(vPos, 1.0));
	Normal  = normalMatrix * vNormal;
	passColor = vColor;
}
