#version 330 core

layout(location = 0) in vec2 vPos;   // unit quad (0,0) ~ (1,1)
layout(location = 1) in vec2 vTexCoord;

uniform mat4 uProj;   // screen space projection
uniform vec4 uRect;   // x, y, width, height (pixel)

out vec2 vUV;

void main()
{
	gl_Position = uProj * vec4(uRect.xy + vPos * uRect.zw, 0.0, 1.0);
	vUV = vTexCoord;
}
