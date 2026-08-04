#version 430
// Pass 1 顶点着色器：将顶点变换到光源裁剪空间，仅用于写入深度阴影贴图

layout (location=0) in vec3 vertPos;

uniform mat4 shadowMVP;

void main(void)
{	gl_Position = shadowMVP * vec4(vertPos,1.0);
}