#version 430
// 环境映射片段着色器：反射向量采样立方体贴图
// r = reflect(-V, N)，模拟镜面反射看到的环境

in vec3 vNormal;
in vec3 vVertPos;
out vec4 fragColor;

uniform mat4 mv_matrix;
uniform mat4 proj_matrix;
uniform mat4 normalMat;
layout (binding = 0) uniform samplerCube t;

void main(void)
{
	vec3 r = -reflect(normalize(-vVertPos), normalize(vNormal));
	fragColor = texture(t, r);
}