#version 430
// 天空盒片段着色器（同 9.2）

in vec3 tc;
out vec4 fragColor;

uniform mat4 v_matrix;
uniform mat4 p_matrix;
layout (binding = 0) uniform samplerCube samp;

void main(void)
{
	fragColor = texture(samp, tc);
}