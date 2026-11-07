#version 330 core

in vec3 passColor;
out vec4 FragColor;
uniform vec3 overrideColor;

void main()
{
	FragColor = vec4(passColor * overrideColor, 1.0);
}
