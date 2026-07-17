// ============================================================================
// OpenGL 6.1 - 程序化球体模型示例
// 教材：第 6 章 Program 1（Prog6_1_sphere）
//
// 本程序演示：
//   1. 用 Sphere 类程序化生成球面网格（顶点、纹理坐标、法线、索引）
//   2. 将索引化网格展开为 glDrawArrays 可用的交错顶点流
//   3. 对球体贴 2D 纹理（earth.jpg）
//   4. 使用背面剔除优化渲染
// ============================================================================

#include <GL/glew.h>                              // GLEW：加载 OpenGL 扩展函数
#include <GLFW/glfw3.h>                           // GLFW：窗口与上下文管理
#include <SOIL2/soil2.h>                          // SOIL2：图像加载（Utils::loadTexture 内部使用）
#include <string>                                 // std::string
#include <iostream>                               // 标准输入输出
#include <fstream>                                // 文件流
#include <cmath>                                  // 数学函数
#include <glm/glm.hpp>                              // GLM 核心类型
#include <glm/gtc/type_ptr.hpp>                     // glm::value_ptr
#include <glm/gtc/matrix_transform.hpp>             // 矩阵变换函数
#include "Sphere.h"                                 // 球体网格生成类
#include "Utils.h"                                  // 着色器与纹理工具类

using namespace std;                                // 使用标准命名空间

#define numVAOs 1                                   // 1 个顶点数组对象
#define numVBOs 3                                   // 3 个 VBO：位置、纹理坐标、法线

// ------------------------------ 全局变量 ------------------------------

float cameraX, cameraY, cameraZ;                    // 相机世界坐标
float sphLocX, sphLocY, sphLocZ;                    // 球体模型世界坐标
GLuint renderingProgram;                            // 着色器程序 ID
GLuint vao[numVAOs];                                // 顶点数组对象
GLuint vbo[numVBOs];                                // 顶点缓冲对象
GLuint earthTexture;                                // 地球纹理对象 ID
float rotAmt = 0.0f;                                // 旋转量（本例未使用，教材保留变量）

GLuint mvLoc;                                       // uniform：模型-视图矩阵
GLuint projLoc;                                     // uniform：投影矩阵
int width, height;                                  // 帧缓冲宽高（像素）
float aspect;                                       // 宽高比 width/height
glm::mat4 pMat;                                     // 投影矩阵
glm::mat4 vMat;                                     // 视图矩阵
glm::mat4 mMat;                                     // 模型矩阵
glm::mat4 mvMat;                                    // 模型-视图矩阵 = vMat * mMat

Sphere mySphere = Sphere(48);                       // 精度 48 的球体（网格密度）

// ------------------------------ 顶点数据准备 ------------------------------

// 从 Sphere 类取出网格数据，按索引展开后上传到 GPU
void setupVertices(void) {
	std::vector<int> ind = mySphere.getIndices();     // 三角形索引列表
	std::vector<glm::vec3> vert = mySphere.getVertices(); // 球面顶点位置
	std::vector<glm::vec2> tex = mySphere.getTexCoords(); // 纹理坐标
	std::vector<glm::vec3> norm = mySphere.getNormals();  // 顶点法线

	std::vector<float> pvalues;                       // 展开后的位置 float 数组
	std::vector<float> tvalues;                       // 展开后的纹理坐标 float 数组
	std::vector<float> nvalues;                       // 展开后的法线 float 数组

	int numIndices = mySphere.getNumIndices();        // 索引总数（= 三角形数 × 3）
	for (int i = 0; i < numIndices; i++) {
		// 按 ind[i] 查找对应顶点，将 vec3/vec2 拆成 float 依次存入
		pvalues.push_back((vert[ind[i]]).x);
		pvalues.push_back((vert[ind[i]]).y);
		pvalues.push_back((vert[ind[i]]).z);
		tvalues.push_back((tex[ind[i]]).s);
		tvalues.push_back((tex[ind[i]]).t);
		nvalues.push_back((norm[ind[i]]).x);
		nvalues.push_back((norm[ind[i]]).y);
		nvalues.push_back((norm[ind[i]]).z);
	}

	glGenVertexArrays(1, vao);                        // 生成 VAO
	glBindVertexArray(vao[0]);                        // 绑定 VAO
	glGenBuffers(numVBOs, vbo);                       // 生成 3 个 VBO

	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);            // 绑定位置 VBO
	glBufferData(GL_ARRAY_BUFFER, pvalues.size() * 4, &pvalues[0], GL_STATIC_DRAW);
	                                                // 上传位置数据（每 float 4 字节）

	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);            // 绑定纹理坐标 VBO
	glBufferData(GL_ARRAY_BUFFER, tvalues.size() * 4, &tvalues[0], GL_STATIC_DRAW);
	                                                // 上传纹理坐标数据

	glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);            // 绑定法线 VBO
	glBufferData(GL_ARRAY_BUFFER, nvalues.size() * 4, &nvalues[0], GL_STATIC_DRAW);
	                                                // 上传法线数据（本例着色器未用法线）
}

// ------------------------------ 初始化 ------------------------------

void init(GLFWwindow* window) {
	renderingProgram = Utils::createShaderProgram("vertShader.glsl", "fragShader.glsl");
	                                                // 编译链接着色器

	cameraX = 0.0f; cameraY = 0.0f; cameraZ = 2.0f; // 相机位于 (0, 0, 2)
	sphLocX = 0.0f; sphLocY = 0.0f; sphLocZ = -1.0f; // 球心位于 (0, 0, -1)

	glfwGetFramebufferSize(window, &width, &height); // 获取帧缓冲尺寸
	aspect = (float)width / (float)height;          // 计算宽高比
	pMat = glm::perspective(1.0472f, aspect, 0.1f, 1000.0f);
	                                                // 60° 透视投影

	setupVertices();                                // 创建并上传球体顶点缓冲
	earthTexture = Utils::loadTexture("earth.jpg"); // 加载地球纹理到 GPU
}

// ------------------------------ 渲染一帧 ------------------------------

void display(GLFWwindow* window, double currentTime) {
	glClear(GL_DEPTH_BUFFER_BIT);                   // 清除深度缓冲
	glClearColor(0.0, 0.0, 0.0, 1.0);             // 设置清除色为黑色
	glClear(GL_COLOR_BUFFER_BIT);                   // 清除颜色缓冲

	glUseProgram(renderingProgram);                 // 使用着色器程序

	mvLoc = glGetUniformLocation(renderingProgram, "mv_matrix");
	                                                // 获取 mv_matrix uniform 位置
	projLoc = glGetUniformLocation(renderingProgram, "proj_matrix");
	                                                // 获取 proj_matrix uniform 位置

	vMat = glm::translate(glm::mat4(1.0f), glm::vec3(-cameraX, -cameraY, -cameraZ));
	                                                // 构建视图矩阵
	mMat = glm::translate(glm::mat4(1.0f), glm::vec3(sphLocX, sphLocY, sphLocZ));
	                                                // 构建模型矩阵（球体平移）
	mvMat = vMat * mMat;                            // 合并为模型-视图矩阵

	glUniformMatrix4fv(mvLoc, 1, GL_FALSE, glm::value_ptr(mvMat));
	                                                // 上传 MV 矩阵
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(pMat));
	                                                // 上传投影矩阵

	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);          // 绑定位置 VBO
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	                                                // 属性 0：location=0，xyz 三分量
	glEnableVertexAttribArray(0);                   // 启用顶点属性 0

	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);          // 绑定纹理坐标 VBO
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
	                                                // 属性 1：location=1，st 两分量
	glEnableVertexAttribArray(1);                   // 启用顶点属性 1

	glActiveTexture(GL_TEXTURE0);                   // 激活纹理单元 0
	glBindTexture(GL_TEXTURE_2D, earthTexture);     // 绑定 2D 地球纹理

	glEnable(GL_CULL_FACE);                         // 开启背面剔除（不绘制背向相机的面）
	glFrontFace(GL_CCW);                            // 逆时针顶点序列为正面

	glDrawArrays(GL_TRIANGLES, 0, mySphere.getNumIndices());
	                                                // 绘制三角形，顶点数 = 索引数
}

// ------------------------------ 窗口回调 ------------------------------

void window_size_callback(GLFWwindow* win, int newWidth, int newHeight) {
	aspect = (float)newWidth / (float)newHeight;    // 更新宽高比
	glViewport(0, 0, newWidth, newHeight);          // 视口铺满新窗口
	pMat = glm::perspective(1.0472f, aspect, 0.1f, 1000.0f);
	                                                // 重建投影矩阵
}

// ------------------------------ 程序入口 ------------------------------

int main(void) {
	if (!glfwInit()) {                              // 初始化 GLFW
		exit(EXIT_FAILURE);                         // 失败则退出
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);  // OpenGL 主版本 4
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);  // OpenGL 次版本 3
	GLFWwindow* window = glfwCreateWindow(1600, 800, "Chapter6 - program1", NULL, NULL);
	                                                // 创建 1600×800 窗口
	glfwMakeContextCurrent(window);                 // 绑定 OpenGL 上下文

	if (glewInit() != GLEW_OK) {                    // 初始化 GLEW
		exit(EXIT_FAILURE);
	}

	glfwSwapInterval(1);                            // 垂直同步
	glfwSetWindowSizeCallback(window, window_size_callback);
	                                                // 注册窗口尺寸变化回调

	init(window);                                   // 初始化场景

	while (!glfwWindowShouldClose(window)) {        // 主循环
		display(window, glfwGetTime());             // 渲染（currentTime 本例未用于动画）
		glfwSwapBuffers(window);                    // 交换双缓冲
		glfwPollEvents();                           // 处理事件
	}

	glfwDestroyWindow(window);                      // 销毁窗口
	glfwTerminate();                                // 终止 GLFW
	exit(EXIT_SUCCESS);                             // 正常退出
}
