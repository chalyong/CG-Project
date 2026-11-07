#version 330 core

in vec3 vPos;
in vec3 vColor;
out vec3 passColor;

uniform mat4 transform;

void main()
{
	gl_Position =  transform * vec4(vPos, 1.0f);
	passColor = vColor;
}
