#version 430
// 天空盒片段着色器：按方向采样立方体贴图

in vec3 tc;
out vec4 fragColor;

uniform mat4 v_matrix;
uniform mat4 p_matrix;
layout (binding = 0) uniform samplerCube samp;

void main(void)
{
	fragColor = texture(samp, tc);
}