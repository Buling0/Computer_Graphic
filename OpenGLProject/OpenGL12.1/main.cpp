// ============================================================================
// OpenGL 12.1 - 细分曲面网格（Tessellation Grid Only）示例
// 教材：第 12 章 Program 1（Prog12_1_tessellationGridOnly）
//
// 本程序演示 OpenGL 细分着色器管线（Tessellation Shader Pipeline）：
//   1. 顶点着色器：空实现（仅 1 个 patch 控制点，无需顶点数据）
//   2. 细分控制着色器（TCS）：设置内外细分级别 gl_TessLevelOuter / Inner
//   3. 细分评估着色器（TES）：按 gl_TessCoord 生成四边形网格顶点
//   4. 片段着色器：输出固定黄色
//
// 渲染流程：
//   glPatchParameteri(GL_PATCH_VERTICES, 1)  → 每个 patch 1 个控制点
//   glDrawArrays(GL_PATCHES, 0, 1)           → 绘制 1 个 patch
//   TCS 将 patch 细分为 6×6 外圈 + 12×12 内圈 → 线框网格
//
// 与后续 12.2+ 的区别：此处仅生成平面网格，无 Bézier 曲面或高度图
// ============================================================================

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <SOIL2/soil2.h>
#include <string>
#include <iostream>
#include <fstream>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Utils.h"

using namespace std;

float toRadians(float degrees) { return (degrees * 2.0f * 3.14159f) / 360.0f; }

#define numVAOs 1

float cameraX, cameraY, cameraZ;
float terLocX, terLocY, terLocZ;
GLuint renderingProgram;
GLuint vao[numVAOs];

GLuint mvpLoc;
int width, height;
float aspect;
glm::mat4 pMat, vMat, mMat, mvpMat;

// 细分级别参考值（实际级别在 tessCShader.glsl 中硬编码）
float tessInner = 30.0f;
float tessOuter = 20.0f;

void init(GLFWwindow* window) {
	// 四阶段着色器：顶点 + 细分控制 + 细分评估 + 片段
	renderingProgram = Utils::createShaderProgram(
		"vertShader.glsl", "tessCShader.glsl", "tessEShader.glsl", "fragShader.glsl");

	cameraX = 0.5f; cameraY = -0.5f; cameraZ = 2.0f;
	terLocX = 0.0f; terLocY = 0.0f; terLocZ = 0.0f;

	glfwGetFramebufferSize(window, &width, &height);
	aspect = (float)width / (float)height;
	pMat = glm::perspective(1.0472f, aspect, 0.1f, 1000.0f);

	glGenVertexArrays(numVAOs, vao);
	glBindVertexArray(vao[0]);
}

void display(GLFWwindow* window, double currentTime) {
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	glUseProgram(renderingProgram);

	vMat = glm::translate(glm::mat4(1.0f), glm::vec3(-cameraX, -cameraY, -cameraZ));

	mMat = glm::translate(glm::mat4(1.0f), glm::vec3(terLocX, terLocY, terLocZ));
	mMat = glm::rotate(mMat, toRadians(35.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	mvpMat = pMat * vMat * mMat;

	mvpLoc = glGetUniformLocation(renderingProgram, "mvp_matrix");
	glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvpMat));

	glFrontFace(GL_CCW);

	// 【关键】每个 patch 只需 1 个控制点；TES 用 gl_TessCoord 生成完整四边形域
	glPatchParameteri(GL_PATCH_VERTICES, 1);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);        // 线框模式，便于观察细分网格
	glDrawArrays(GL_PATCHES, 0, 1);
}

void window_size_callback(GLFWwindow* win, int newWidth, int newHeight) {
	aspect = (float)newWidth / (float)newHeight;
	glViewport(0, 0, newWidth, newHeight);
	pMat = glm::perspective(1.0472f, aspect, 0.1f, 1000.0f);
}

int main(void) {
	if (!glfwInit()) {
		exit(EXIT_FAILURE);
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	GLFWwindow* window = glfwCreateWindow(800, 800, "Chapter12 - program1", NULL, NULL);
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
