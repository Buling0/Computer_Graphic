#version 430
// 场景物体片段着色器：采样 2D 砖块纹理

in vec2 tc;
out vec4 fragColor;

uniform mat4 mv_matrix;
uniform mat4 proj_matrix;
layout (binding = 0) uniform sampler2D s;

void main(void)
{
	fragColor = texture(s, tc);
}