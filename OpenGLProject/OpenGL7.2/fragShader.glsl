#version 430

in vec3 varyingNormal;
in vec3 varyingLightDir;
in vec3 varyingVertPos;
in vec3 varyingHalfVector;
in vec2 tc;

out vec4 fragColor;

layout (binding=0) uniform sampler2D s;

struct PositionalLight
{	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	vec3 position;
};

struct Material
{	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	float shininess;
};

uniform vec4 globalAmbient;
uniform PositionalLight light;
uniform Material material;
uniform mat4 mv_matrix;
uniform mat4 proj_matrix;
uniform mat4 norm_matrix;

void main(void)
{	vec3 L = normalize(varyingLightDir);
	vec3 N = normalize(varyingNormal);
	vec3 V = normalize(-varyingVertPos);

	float cosTheta = dot(L,N);
	vec3 H = normalize(varyingHalfVector);
	float cosPhi = dot(H,N);

	vec4 texColor = texture(s, tc);

	vec3 ambient = ((globalAmbient * material.ambient)
		+ (light.ambient * material.ambient)).xyz * texColor.rgb;
	vec3 diffuse = light.diffuse.xyz * material.diffuse.xyz
		* texColor.rgb * max(cosTheta,0.0);
	vec3 specular = light.specular.xyz * material.specular.xyz
		* pow(max(cosPhi,0.0), material.shininess*3.0);

	fragColor = vec4((ambient + diffuse + specular), 1.0);
}