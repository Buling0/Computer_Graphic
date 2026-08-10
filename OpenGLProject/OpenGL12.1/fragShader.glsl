#version 430
// 片段着色器：固定黄色（线框模式下主要看到边线颜色）

out vec4 color;
uniform mat4 mvp_matrix;

void main(void)
{
	color = vec4(1.0, 1.0, 0.0, 1.0);
}