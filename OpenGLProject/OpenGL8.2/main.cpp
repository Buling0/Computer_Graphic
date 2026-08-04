// ============================================================================
// OpenGL 8.2 - PCF 软阴影（Percentage Closer Filtering）示例
// 教材：第 8 章 Program 2（Prog8_2_softShadowsPCF）
//
// 本程序在 8.1 阴影映射基础上，Pass 2 片段着色器改用 PCF：
//   1. Pass 1：与 8.1 相同，从光源视角写入深度阴影贴图
//   2. Pass 2：在阴影贴图上对多个偏移位置采样，取平均值得到 shadowFactor
//      shadowFactor ∈ [0,1]：0=全阴影，1=全照亮，中间值产生软阴影边缘
//   3. 最终颜色：shadowColor + shadowFactor * lightedColor（而非 8.1 的二值 if）
//
// 与 8.1 的唯一代码差异在 frag2Shader.glsl 的 lookup() + 4 点 PCF 采样
//
// 场景：青铜圆环 + 金色金字塔，固定位置光源产生软阴影
// ============================================================================

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <SOIL2/soil2.h>
#include <string>
#include <iostream>
#include <fstream>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Torus.h"
#include "Utils.h"
#include "ImportedModel.h"

using namespace std;

void passOne(void);                                 // 第一遍：生成阴影贴图
void passTwo(void);                                 // 第二遍：正常渲染 + 阴影测试

float toRadians(float degrees) { return (degrees * 2.0f * 3.14159f) / 360.0f; }

#define numVAOs 1
#define numVBOs 5                                       // 环位置/锥位置/环法线/锥法线/环索引

GLuint renderingProgram1, renderingProgram2;          // 两套着色器程序
GLuint vao[numVAOs];
GLuint vbo[numVBOs];

ImportedModel pyramid("pyr.obj");                     // OBJ 金字塔模型
Torus myTorus(0.6f, 0.4f, 48);
int numPyramidVertices, numTorusVertices, numTorusIndices;

glm::vec3 torusLoc(1.6f, 0.0f, -0.3f);              // 圆环世界坐标
glm::vec3 pyrLoc(-1.0f, 0.1f, 0.3f);                // 金字塔世界坐标
glm::vec3 cameraLoc(0.0f, 0.2f, 6.0f);              // 相机世界坐标
glm::vec3 lightLoc(-3.8f, 2.2f, 1.1f);             // 光源世界坐标（固定）

float globalAmbient[4] = { 0.7f, 0.7f, 0.7f, 1.0f };
float lightAmbient[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
float lightDiffuse[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
float lightSpecular[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

float* gMatAmb = Utils::goldAmbient();                // 金字塔：金色
float* gMatDif = Utils::goldDiffuse();
float* gMatSpe = Utils::goldSpecular();
float gMatShi = Utils::goldShininess();

float* bMatAmb = Utils::bronzeAmbient();            // 圆环：青铜
float* bMatDif = Utils::bronzeDiffuse();
float* bMatSpe = Utils::bronzeSpecular();
float bMatShi = Utils::bronzeShininess();

float thisAmb[4], thisDif[4], thisSpe[4], matAmb[4], matDif[4], matSpe[4];
float thisShi, matShi;

// ---- 阴影贴图相关 ----
int scSizeX, scSizeY;                                 // 阴影贴图分辨率
GLuint shadowTex;                                     // 深度纹理（阴影贴图）
GLuint shadowBuffer;                                  // 阴影 FBO
glm::mat4 lightVmatrix;                               // 光源视图矩阵
glm::mat4 lightPmatrix;                               // 光源投影矩阵
glm::mat4 shadowMVP1;                                 // Pass1：lightP * lightV * model
glm::mat4 shadowMVP2;                                 // Pass2：b * lightP * lightV * model
glm::mat4 b;                                          // NDC → 纹理坐标变换矩阵

GLuint mvLoc, projLoc, nLoc, sLoc;
int width, height;
float aspect;
glm::mat4 pMat, vMat, mMat, mvMat, invTrMat;
glm::vec3 currentLightPos, transformed;
float lightPos[3];
GLuint globalAmbLoc, ambLoc, diffLoc, specLoc, posLoc, mambLoc, mdiffLoc, mspecLoc, mshiLoc;
glm::vec3 origin(0.0f, 0.0f, 0.0f);
glm::vec3 up(0.0f, 1.0f, 0.0f);

// 上传当前物体材质与光源参数到指定着色器程序
void installLights(int renderingProgram, glm::mat4 vMatrix) {
	transformed = glm::vec3(vMatrix * glm::vec4(currentLightPos, 1.0));
	lightPos[0] = transformed.x;
	lightPos[1] = transformed.y;
	lightPos[2] = transformed.z;

	matAmb[0] = thisAmb[0]; matAmb[1] = thisAmb[1]; matAmb[2] = thisAmb[2]; matAmb[3] = thisAmb[3];
	matDif[0] = thisDif[0]; matDif[1] = thisDif[1]; matDif[2] = thisDif[2]; matDif[3] = thisDif[3];
	matSpe[0] = thisSpe[0]; matSpe[1] = thisSpe[1]; matSpe[2] = thisSpe[2]; matSpe[3] = thisSpe[3];
	matShi = thisShi;

	globalAmbLoc = glGetUniformLocation(renderingProgram, "globalAmbient");
	ambLoc = glGetUniformLocation(renderingProgram, "light.ambient");
	diffLoc = glGetUniformLocation(renderingProgram, "light.diffuse");
	specLoc = glGetUniformLocation(renderingProgram, "light.specular");
	posLoc = glGetUniformLocation(renderingProgram, "light.position");
	mambLoc = glGetUniformLocation(renderingProgram, "material.ambient");
	mdiffLoc = glGetUniformLocation(renderingProgram, "material.diffuse");
	mspecLoc = glGetUniformLocation(renderingProgram, "material.specular");
	mshiLoc = glGetUniformLocation(renderingProgram, "material.shininess");

	glProgramUniform4fv(renderingProgram, globalAmbLoc, 1, globalAmbient);
	glProgramUniform4fv(renderingProgram, ambLoc, 1, lightAmbient);
	glProgramUniform4fv(renderingProgram, diffLoc, 1, lightDiffuse);
	glProgramUniform4fv(renderingProgram, specLoc, 1, lightSpecular);
	glProgramUniform3fv(renderingProgram, posLoc, 1, lightPos);
	glProgramUniform4fv(renderingProgram, mambLoc, 1, matAmb);
	glProgramUniform4fv(renderingProgram, mdiffLoc, 1, matDif);
	glProgramUniform4fv(renderingProgram, mspecLoc, 1, matSpe);
	glProgramUniform1f(renderingProgram, mshiLoc, matShi);
}

void setupVertices(void) {
	numPyramidVertices = pyramid.getNumVertices();
	std::vector<glm::vec3> vert = pyramid.getVertices();
	std::vector<glm::vec3> norm = pyramid.getNormals();

	std::vector<float> pyramidPvalues;
	std::vector<float> pyramidNvalues;

	for (int i = 0; i < numPyramidVertices; i++) {
		pyramidPvalues.push_back((vert[i]).x);
		pyramidPvalues.push_back((vert[i]).y);
		pyramidPvalues.push_back((vert[i]).z);
		pyramidNvalues.push_back((norm[i]).x);
		pyramidNvalues.push_back((norm[i]).y);
		pyramidNvalues.push_back((norm[i]).z);
	}

	numTorusVertices = myTorus.getNumVertices();
	numTorusIndices = myTorus.getNumIndices();
	std::vector<int> ind = myTorus.getIndices();
	vert = myTorus.getVertices();
	norm = myTorus.getNormals();

	std::vector<float> torusPvalues;
	std::vector<float> torusNvalues;

	for (int i = 0; i < numTorusVertices; i++) {
		torusPvalues.push_back(vert[i].x);
		torusPvalues.push_back(vert[i].y);
		torusPvalues.push_back(vert[i].z);
		torusNvalues.push_back(norm[i].x);
		torusNvalues.push_back(norm[i].y);
		torusNvalues.push_back(norm[i].z);
	}

	glGenVertexArrays(1, vao);
	glBindVertexArray(vao[0]);
	glGenBuffers(numVBOs, vbo);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, torusPvalues.size() * 4, &torusPvalues[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, pyramidPvalues.size() * 4, &pyramidPvalues[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
	glBufferData(GL_ARRAY_BUFFER, torusNvalues.size() * 4, &torusNvalues[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[3]);
	glBufferData(GL_ARRAY_BUFFER, pyramidNvalues.size() * 4, &pyramidNvalues[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[4]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, ind.size() * 4, &ind[0], GL_STATIC_DRAW);
}

// 【关键】创建阴影贴图：深度纹理 + 帧缓冲，供 Pass1 写入
void setupShadowBuffers(GLFWwindow* window) {
	glfwGetFramebufferSize(window, &width, &height);
	scSizeX = width;
	scSizeY = height;

	glGenFramebuffers(1, &shadowBuffer);

	glGenTextures(1, &shadowTex);
	glBindTexture(GL_TEXTURE_2D, shadowTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32,
		scSizeX, scSizeY, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// 启用硬件阴影比较：texture() 返回 0（在阴影中）或 1（被照亮）
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void init(GLFWwindow* window) {
	renderingProgram1 = Utils::createShaderProgram("vert1Shader.glsl", "frag1Shader.glsl");
	renderingProgram2 = Utils::createShaderProgram("vert2Shader.glsl", "frag2Shader.glsl");

	glfwGetFramebufferSize(window, &width, &height);
	aspect = (float)width / (float)height;
	glViewport(0, 0, width, height);
	pMat = glm::perspective(1.0472f, aspect, 0.1f, 1000.0f);

	setupVertices();
	setupShadowBuffers(window);

	// 将裁剪空间 [-1,1] 线性映射到纹理空间 [0,1]（含透视除法后的 w 处理）
	b = glm::mat4(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.5f, 0.0f,
		0.5f, 0.5f, 0.5f, 1.0f);
}

void display(GLFWwindow* window, double currentTime) {
	glClear(GL_DEPTH_BUFFER_BIT);
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	currentLightPos = glm::vec3(lightLoc);

	// 以光源为“相机”构建视图/投影矩阵
	lightVmatrix = glm::lookAt(currentLightPos, origin, up);
	lightPmatrix = glm::perspective(toRadians(60.0f), aspect, 0.1f, 1000.0f);

	// ========== Pass 1：渲染阴影深度图 ==========
	glBindFramebuffer(GL_FRAMEBUFFER, shadowBuffer);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadowTex, 0);

	glDrawBuffer(GL_NONE);                          // 只写深度，不写颜色
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_POLYGON_OFFSET_FILL);               // 减少阴影痤疮（shadow acne）
	glPolygonOffset(2.0f, 4.0f);

	passOne();

	glDisable(GL_POLYGON_OFFSET_FILL);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);           // 切回默认帧缓冲

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, shadowTex);        // Pass2 采样阴影贴图

	glDrawBuffer(GL_BACK);

	// ========== Pass 2：ADS 光照 + PCF 软阴影（frag2Shader） ==========
	passTwo();
}

// Pass 1：仅用 shadowMVP 将顶点变换到光源裁剪空间，写入深度
void passOne(void) {
	glUseProgram(renderingProgram1);
	glBindVertexArray(vao[0]);

	sLoc = glGetUniformLocation(renderingProgram1, "shadowMVP");

	// 绘制圆环
	mMat = glm::translate(glm::mat4(1.0f), torusLoc);
	mMat = glm::rotate(mMat, toRadians(25.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	shadowMVP1 = lightPmatrix * lightVmatrix * mMat;
	glUniformMatrix4fv(sLoc, 1, GL_FALSE, glm::value_ptr(shadowMVP1));

	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	glViewport(0, 0, scSizeX, scSizeY);
	glClear(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	glDepthFunc(GL_LEQUAL);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[4]);
	glDrawElements(GL_TRIANGLES, numTorusIndices, GL_UNSIGNED_INT, 0);

	// 绘制金字塔
	mMat = glm::translate(glm::mat4(1.0f), pyrLoc);
	mMat = glm::rotate(mMat, toRadians(30.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	mMat = glm::rotate(mMat, toRadians(40.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	shadowMVP1 = lightPmatrix * lightVmatrix * mMat;
	glUniformMatrix4fv(sLoc, 1, GL_FALSE, glm::value_ptr(shadowMVP1));

	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	glDrawArrays(GL_TRIANGLES, 0, numPyramidVertices);
}

// Pass 2：ADS 光照；阴影由 frag2 中 PCF 多采样平均得到 shadowFactor
void passTwo(void) {
	glUseProgram(renderingProgram2);
	glBindVertexArray(vao[0]);

	mvLoc = glGetUniformLocation(renderingProgram2, "mv_matrix");
	projLoc = glGetUniformLocation(renderingProgram2, "proj_matrix");
	nLoc = glGetUniformLocation(renderingProgram2, "norm_matrix");
	sLoc = glGetUniformLocation(renderingProgram2, "shadowMVP");

	glViewport(0, 0, width, height);

	vMat = glm::translate(glm::mat4(1.0f), glm::vec3(-cameraLoc.x, -cameraLoc.y, -cameraLoc.z));

	// ---- 青铜圆环 ----
	thisAmb[0] = bMatAmb[0]; thisAmb[1] = bMatAmb[1]; thisAmb[2] = bMatAmb[2];
	thisDif[0] = bMatDif[0]; thisDif[1] = bMatDif[1]; thisDif[2] = bMatDif[2];
	thisSpe[0] = bMatSpe[0]; thisSpe[1] = bMatSpe[1]; thisSpe[2] = bMatSpe[2];
	thisShi = bMatShi;

	mMat = glm::translate(glm::mat4(1.0f), torusLoc);
	mMat = glm::rotate(mMat, toRadians(25.0f), glm::vec3(1.0f, 0.0f, 0.0f));

	currentLightPos = glm::vec3(lightLoc);
	installLights(renderingProgram2, vMat);

	mvMat = vMat * mMat;
	invTrMat = glm::transpose(glm::inverse(mvMat));
	shadowMVP2 = b * lightPmatrix * lightVmatrix * mMat;

	glUniformMatrix4fv(mvLoc, 1, GL_FALSE, glm::value_ptr(mvMat));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(pMat));
	glUniformMatrix4fv(nLoc, 1, GL_FALSE, glm::value_ptr(invTrMat));
	glUniformMatrix4fv(sLoc, 1, GL_FALSE, glm::value_ptr(shadowMVP2));

	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);

	glClear(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[4]);
	glDrawElements(GL_TRIANGLES, numTorusIndices, GL_UNSIGNED_INT, 0);

	// ---- 金色金字塔 ----
	thisAmb[0] = gMatAmb[0]; thisAmb[1] = gMatAmb[1]; thisAmb[2] = gMatAmb[2];
	thisDif[0] = gMatDif[0]; thisDif[1] = gMatDif[1]; thisDif[2] = gMatDif[2];
	thisSpe[0] = gMatSpe[0]; thisSpe[1] = gMatSpe[1]; thisSpe[2] = gMatSpe[2];
	thisShi = gMatShi;

	mMat = glm::translate(glm::mat4(1.0f), pyrLoc);
	mMat = glm::rotate(mMat, toRadians(30.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	mMat = glm::rotate(mMat, toRadians(40.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	currentLightPos = glm::vec3(lightLoc);
	installLights(renderingProgram2, vMat);

	mvMat = vMat * mMat;
	invTrMat = glm::transpose(glm::inverse(mvMat));
	shadowMVP2 = b * lightPmatrix * lightVmatrix * mMat;

	glUniformMatrix4fv(mvLoc, 1, GL_FALSE, glm::value_ptr(mvMat));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(pMat));
	glUniformMatrix4fv(nLoc, 1, GL_FALSE, glm::value_ptr(invTrMat));
	glUniformMatrix4fv(sLoc, 1, GL_FALSE, glm::value_ptr(shadowMVP2));

	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[3]);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);

	glDrawArrays(GL_TRIANGLES, 0, numPyramidVertices);
}

void window_size_callback(GLFWwindow* win, int newWidth, int newHeight) {
	aspect = (float)newWidth / (float)newHeight;
	glViewport(0, 0, newWidth, newHeight);
	pMat = glm::perspective(1.0472f, aspect, 0.1f, 1000.0f);
	scSizeX = newWidth;
	scSizeY = newHeight;
}

int main(void) {
	if (!glfwInit()) {
		exit(EXIT_FAILURE);
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	GLFWwindow* window = glfwCreateWindow(1600, 800, "Chapter8 - program2", NULL, NULL);
	glfwMakeContextCurrent(window);

	if (glewInit() != GLEW_OK) {
		exit(EXIT_FAILURE);
	}

	glfwSwapInterval(1);
	glfwSetWindowSizeCallback(window, window_size_callback);

	init(window);

	while (!glfwWindowShouldClose(window)) {
		display(window, glfwGetTime());
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}
