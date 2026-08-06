#version 430
// 天空盒顶点着色器：顶点位置即立方体贴图采样方向

layout (location = 0) in vec3 position;
out vec3 tc;

uniform mat4 v_matrix;
uniform mat4 p_matrix;
layout (binding = 0) uniform samplerCube samp;

void main(void)
{
	tc = position;
	// 只保留视图旋转，去掉平移，天空不随相机位置移动
	mat4 v3_matrix = mat4(mat3(v_matrix));
	gl_Position = p_matrix * v3_matrix * vec4(position,1.0);
}