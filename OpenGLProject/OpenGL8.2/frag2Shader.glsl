#version 430
// Pass 2 片段着色器：PCF 软阴影（与 8.1 硬阴影的唯一区别在此文件）
// 对阴影贴图多个偏移位置采样并平均，得到 0~1 的 shadowFactor，实现边缘柔化

in vec3 vNormal, vLightDir, vVertPos, vHalfVec;
in vec4 shadow_coord;
out vec4 fragColor;

struct PositionalLight
{	vec4 ambient, diffuse, specular;
	vec3 position;
};

struct Material
{	vec4 ambient, diffuse, specular;
	float shininess;
};

uniform vec4 globalAmbient;
uniform PositionalLight light;
uniform Material material;
uniform mat4 mv_matrix;
uniform mat4 proj_matrix;
uniform mat4 norm_matrix;
uniform mat4 shadowMVP;
layout (binding=0) uniform sampler2DShadow shadowTex;

// 在 shadow_coord 附近偏移 (x,y) 后采样阴影贴图；返回 0（阴影）或 1（照亮）
float lookup(float x, float y)
{  	float t = textureProj(shadowTex, shadow_coord + vec4(x * 0.001 * shadow_coord.w,
                                                         y * 0.001 * shadow_coord.w,
                                                         -0.01, 0.0));
	return t;
}

void main(void)
{	float shadowFactor=0.0;
	
	vec3 L = normalize(vLightDir);
	vec3 N = normalize(vNormal);
	vec3 V = normalize(-vVertPos);
	vec3 H = normalize(vHalfVec);

	// 4 点 PCF：根据像素坐标 mod 2 抖动采样位置，减少规则网格带来的带状伪影
	float swidth = 2.5;
	vec2 o = mod(floor(gl_FragCoord.xy), 2.0) * swidth;
	shadowFactor += lookup(-1.5*swidth + o.x,  1.5*swidth - o.y);
	shadowFactor += lookup(-1.5*swidth + o.x, -0.5*swidth - o.y);
	shadowFactor += lookup( 0.5*swidth + o.x,  1.5*swidth - o.y);
	shadowFactor += lookup( 0.5*swidth + o.x, -0.5*swidth - o.y);
	shadowFactor = shadowFactor / 4.0;

	// 阴影区域仅保留环境光；照亮区域叠加漫反射与高光
	vec4 shadowColor = globalAmbient * material.ambient
				+ light.ambient * material.ambient;
	
	vec4 lightedColor = light.diffuse * material.diffuse * max(dot(L,N),0.0)
				+ light.specular * material.specular
				* pow(max(dot(H,N),0.0),material.shininess*3.0);
	
	// shadowFactor 混合：边缘处 0~1 过渡 → 软阴影
	fragColor = vec4((shadowColor.xyz + shadowFactor*(lightedColor.xyz)),1.0);
}