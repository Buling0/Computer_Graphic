#version 430

// 从顶点着色器接收的纹理坐标
in vec2 tc;
// 输出最终像素颜色
out vec4 color;

uniform mat4 mv_matrix;                           // 本例未使用，保留与教材一致
uniform mat4 proj_matrix;                         // 本例未使用，保留与教材一致
layout (binding=0) uniform sampler2D s;           // 2D 纹理采样器

void main(void)
{
	// 在纹理坐标 tc 处采样地球贴图
	color = texture(s, tc);
}
