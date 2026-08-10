#version 430
// 细分控制着色器（TCS）：设置 patch 的内外细分级别
// layout(vertices=1) 表示每个 patch 有 1 个控制点

uniform mat4 mvp_matrix;
layout (vertices = 1) out;

void main(void)
{	gl_TessLevelOuter[0] = 6;   // 四边形四条边的细分段数
	gl_TessLevelOuter[2] = 6;
	gl_TessLevelOuter[1] = 6;
	gl_TessLevelOuter[3] = 6;
	gl_TessLevelInner[0] = 12;  // 四边形内部两个方向的细分段数
	gl_TessLevelInner[1] = 12;
}