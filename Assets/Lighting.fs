#version 330

in vec3 fragPosition;
in vec2 fragTexCoord;
in vec3 fragNormal;
in vec4 fragColor;

uniform vec3 viewPos;

#define MAX_LIGHTS 4
struct Light {
	bool enabled;
	int type;
	vec3 position;
	vec3 target;
	vec4 color;
	float intensity;
};
uniform Light lights[MAX_LIGHTS];

out vec4 finalColor;

void main()
{
	vec3 materialColor = fragColor.rgb;

	vec3 normal = normalize(fragNormal);
	vec3 viewDir = normalize(viewPos - fragPosition);
	vec3 finalAmbient = vec3(0.0);
	vec3 finalDiffuse = vec3(0.0);
	vec3 finalSpecular = vec3(0.0);
	float ambientStrength = 0.1;

	for (int i = 0; i < MAX_LIGHTS; i++)
	{
		if (lights[i].enabled)
		{
			vec3 lightColor = lights[i].color.rgb * lights[i].intensity;
			vec3 lightDir;

			if (lights[i].type == 0)
			{
				lightDir = normalize(lights[i].position - lights[i].target);
			}
			else
			{
				lightDir = normalize(lights[i].position - fragPosition);
			}
			finalAmbient += lightColor * ambientStrength;
			float diff = max(dot(normal, lightDir), 0.0);
			finalDiffuse += diff * lightColor;
			vec3 halfwayDir = normalize(lightDir + viewDir);
			float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
			finalSpecular += spec * lightColor;
		}
	}
	vec3 result = (finalAmbient * materialColor) + (finalDiffuse * materialColor) + finalSpecular;
	finalColor = vec4(result, fragColor.a);
}
