#version 430
// 片段着色器：直接采样 2D 纹理作为颜色（天空盒与圆环共用此简单着色器）

in vec2 tc;
out vec4 fragColor;

uniform mat4 mv_matrix;
uniform mat4 proj_matrix;
layout (binding = 0) uniform sampler2D s;

void main(void)
{
	fragColor = texture(s, tc);
}