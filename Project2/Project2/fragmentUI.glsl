#version 330 core

in vec2 vUV;

uniform vec4 uColor;
uniform sampler2D uTex;
uniform int uUseTex;   // 0 = solid color, 1 = texture

out vec4 FragColor;

void main()
{
	vec4 tex = (uUseTex == 1) ? texture(uTex, vUV) : vec4(1.0, 1.0, 1.0, 1.0);
	FragColor = tex * uColor;
}
