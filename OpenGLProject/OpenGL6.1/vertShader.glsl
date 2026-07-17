#version 430

// 顶点位置，对应 layout location = 0
layout (location = 0) in vec3 position;
// 纹理坐标，对应 layout location = 1
layout (location = 1) in vec2 tex_coord;

// 传递给片段着色器的纹理坐标
out vec2 tc;

uniform mat4 mv_matrix;                           // 模型-视图矩阵
uniform mat4 proj_matrix;                         // 投影矩阵
layout (binding=0) uniform sampler2D s;           // 2D 纹理采样器，绑定到纹理单元 0

void main(void)
{
	// 将顶点变换到裁剪空间
	gl_Position = proj_matrix * mv_matrix * vec4(position, 1.0);
	// 将纹理坐标传给片段着色器
	tc = tex_coord;
}
