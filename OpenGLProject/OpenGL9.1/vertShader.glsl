#version 430
// 顶点着色器：传递纹理坐标，将顶点变换到裁剪空间

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 tex_coord;
out vec2 tc;

uniform mat4 mv_matrix;
uniform mat4 proj_matrix;
layout (binding = 0) uniform sampler2D s;

void main(void)
{
	tc = tex_coord;
	gl_Position = proj_matrix * mv_matrix * vec4(position,1.0);
}