#version 330 core

in vec3 passColor;
in vec3 FragPos;
in vec3 Normal;

out vec4 FragColor;

uniform vec3 overrideColor;

// 주변조명
uniform vec3 lightColor;
uniform float ambientStrength;
// 산란조명
uniform vec3  lightPos;
uniform float diffuseStrength;
// 조명 on/off
uniform int LightOn;
// 거울반사
uniform vec3  viewPos;
uniform float specularStrength;
uniform float shininess;

void main()
{
	vec3 baseColor = passColor * overrideColor;

	if(LightOn == 0){
		FragColor = vec4(baseColor, 1.0);
		return;
	}
	vec3 N = normalize(Normal);
	vec3 L = normalize(lightPos - FragPos);

	// 주변
	vec3 ambient = lightColor * ambientStrength;
	// 산란
	float diff = max(dot(N, L), 0.0);
	vec3 diffuse = diffuseStrength * diff * lightColor;
	// 거울
	vec3 V = normalize(viewPos - FragPos);
    vec3 R = reflect(-L, N);
	float spec = 0.0;
    if (diff > 0.0)
        spec = pow(max(dot(V, R), 0.0), shininess);
    vec3 specular = specularStrength * spec * lightColor;

    FragColor = vec4((ambient + diffuse + specular) * baseColor, 1.0);
}
