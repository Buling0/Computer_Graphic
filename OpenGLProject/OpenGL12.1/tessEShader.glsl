#version 430
// 细分评估着色器（TES）：在 patch 参数域 [0,1]×[0,1] 上生成顶点
// layout(quads) 表示四边形 patch；gl_TessCoord 为当前细分点的 (u,v)

layout (quads, equal_spacing, ccw) in;

uniform mat4 mvp_matrix;

void main (void)
{
	float u = gl_TessCoord.x;
	float v = gl_TessCoord.y;
	// 将 (u,0,v) 映射为 XZ 平面上的网格点（Y=0）
	gl_Position = mvp_matrix * vec4(u, 0.0, v, 1.0);
}