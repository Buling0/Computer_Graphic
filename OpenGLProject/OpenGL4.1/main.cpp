#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <string>
#include <iostream>
#include <fstream>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Utils.h"

using namespace std;

#define numVAOs 1
#define numVBOs 2

float cameraX, cameraY, cameraZ;
float cubeLocX, cubeLocY, cubeLocZ;
GLuint renderingProgram;
GLuint vao[numVAOs];
GLuint vbo[numVBOs];

// 分配在display()函数中使用的变量
GLuint mvLoc, projLoc;
int width, height;
float aspect;
glm::mat4 pMat, vMat, mMat, mvMat;
glm::mat4 tMat, rMat;

// 36个顶点，12个三角形，每个顶点有3个坐标值
void setupVertices(void) {
	float vertexPositions[108] = {
		-1.0f, 1.0f, -1.0f,  -1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,  1.0f, 1.0f, -1.0f,  -1.0f, 1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,  1.0f, -1.0f, 1.0f,  1.0f, 1.0f, -1.0f,
		1.0f, -1.0f, 1.0f,  1.0f, 1.0f, 1.0f,  1.0f, 1.0f, -1.0f,
		1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,  -1.0f, -1.0f, 1.0f,  1.0f, -1.0f, 1.0f,
		-1.0f, -1.0f, 1.0f,  -1.0f, 1.0f, 1.0f,  -1.0f, 1.0f, -1.0f,
		-1.0f, 1.0f, -1.0f,  -1.0f, 1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,  1.0f, 1.0f, 1.0f,  1.0f, -1.0f, 1.0f,
		1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, -1.0f,  1.0f, 1.0f, -1.0f,  1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,  -1.0f, 1.0f, 1.0f,  -1.0f, 1.0f, -1.0f
	};

	glGenVertexArrays(1, vao);
	glBindVertexArray(vao[0]);
	glGenBuffers(numVBOs, vbo);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexPositions), vertexPositions, GL_STATIC_DRAW);
}

void init(GLFWwindow* window) {
	renderingProgram = Utils::createShaderProgram("vertShader.glsl", "fragShader.glsl");
	cameraX = 0.0f; cameraY = 0.0f; cameraZ = 8.0f;
	cubeLocX = 0.0f; cubeLocY = -2.0f; cubeLocZ = 0.0f;
	setupVertices();
}

void display(GLFWwindow* window, double currentTime) {
	glClear(GL_DEPTH_BUFFER_BIT);
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(renderingProgram);

	// 获取MV矩阵和投影矩阵的统一变量
	mvLoc = glGetUniformLocation(renderingProgram, "mv_matrix");
	projLoc = glGetUniformLocation(renderingProgram, "proj_matrix");

	// 构建透视矩阵
	glfwGetFramebufferSize(window, &width, &height);
	aspect = (float)width / (float)height;
	pMat = glm::perspective(1.0472f, aspect, 0.1f, 1000.0f); // 1.0472f radians = 60degree

	// 构建视图矩阵
	vMat = glm::translate(glm::mat4(1.0f), glm::vec3(-cameraX, -cameraY, -cameraZ));
	// mMat = glm::translate(glm::mat4(1.0f), glm::vec3(cubeLocX, cubeLocY, cubeLocZ));

	for (int i = 0; i < 50; i++) {
		float tf = currentTime + i;
		tMat = glm::translate(glm::mat4(1.0f), glm::vec3(sin(0.35f * tf) * 8.0f, cos(0.52f * tf) * 8.0f, sin(0.7f * tf) * 8.0f));
		rMat = glm::rotate(glm::mat4(1.0f), 1.75f * (float)tf, glm::vec3(0.0f, 1.0f, 0.0f));
		rMat = glm::rotate(rMat, 1.75f * (float)tf, glm::vec3(1.0f, 0.0f, 0.0f));
		rMat = glm::rotate(rMat, 1.75f * (float)tf, glm::vec3(0.0f, 0.0f, 1.0f));
		mMat = tMat * rMat;
		mvMat = vMat * mMat;

		// 将透视矩阵和MV矩阵复制给相应的统一变量
		glUniformMatrix4fv(mvLoc, 1, GL_FALSE, glm::value_ptr(mvMat));
		glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(pMat));

		// 将vbo关联给顶点着色器中相应的顶点属性
		glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);

		// 调整OpenGL的设置，绘制模型
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glDrawArrays(GL_TRIANGLES, 0, 36);
	}	
}

int main(void) {
	if (!glfwInit()) {
		exit(EXIT_FAILURE);
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	GLFWwindow* window = glfwCreateWindow(1600, 800, "Chapter4 - program1", NULL, NULL);
	glfwMakeContextCurrent(window);
	if (glewInit() != GLEW_OK) {
          exit(EXIT_FAILURE);
	}
	glfwSwapInterval(1);

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

