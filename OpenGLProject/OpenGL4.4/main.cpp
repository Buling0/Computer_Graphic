// ============================================================================
// OpenGL 4.4 - 矩阵栈（Matrix Stack）示例
// 教材：第 4 章 Program 4（Prog4_4_matrixStack）
//
// 本程序演示如何用 C++ 标准库的 stack<glm::mat4> 模拟 OpenGL 固定管线时代的
// 矩阵栈（glPushMatrix / glPopMatrix），在可编程管线中手动管理层级变换。
//
// 场景：简化的“太阳系”
//   - 金字塔 = 太阳（中心，绕 X 轴自转）
//   - 立方体 = 行星（绕太阳公转，绕 Y 轴自转）
//   - 小立方体 = 卫星（绕行星公转，缩小为 0.25 倍，绕 Z 轴自转）
//
// 变换层级（从根到叶）：
//   视图矩阵 → 太阳局部变换 → 行星轨道/自转 → 卫星轨道/自转/缩放
// ============================================================================

#include <GL/glew.h>                              // GLEW：加载 OpenGL 扩展函数
#include <GLFW/glfw3.h>                           // GLFW：窗口创建、上下文、输入与事件
#include <string>                                 // std::string
#include <iostream>                               // 标准输入输出流
#include <fstream>                                  // 文件流（Utils 读取着色器文件时使用）
#include <cmath>                                    // sin、cos 等数学函数
#include <stack>                                    // 用于实现矩阵栈
#include <glm/glm.hpp>                              // GLM 核心：vec3、mat4 等
#include <glm/gtc/type_ptr.hpp>                     // glm::value_ptr，将 mat4 转为 float 指针
#include <glm/gtc/matrix_transform.hpp>             // translate、rotate、scale、perspective
#include "Utils.h"                                  // 着色器编译、错误检查等工具类

using namespace std;                                // 使用标准命名空间，简化 string、cout 等写法

#define numVAOs 1                                   // 只需 1 个顶点数组对象
#define numVBOs 2                                   // 2 个顶点缓冲：立方体 + 金字塔

// ------------------------------ 全局变量 ------------------------------

float cameraX, cameraY, cameraZ;                    // 相机在世界空间中的位置
GLuint renderingProgram;                            // 编译链接后的着色器程序 ID
GLuint vao[numVAOs];                                // 顶点数组对象数组
GLuint vbo[numVBOs];                                // 顶点缓冲对象数组
GLuint mvLoc;                                       // 着色器 uniform：模型-视图矩阵 mv_matrix 的位置
GLuint projLoc;                                     // 着色器 uniform：投影矩阵 proj_matrix 的位置
int width, height;                                  // 帧缓冲（窗口）的像素宽度和高度
float aspect;                                       // 宽高比 = width / height，用于透视投影
glm::mat4 pMat;                                     // 投影矩阵（Perspective）
glm::mat4 vMat;                                     // 视图矩阵（View）
glm::mat4 mMat;                                     // 模型矩阵（Model，本例主要用栈顶矩阵代替）
glm::mat4 mvMat;                                    // 模型-视图矩阵 = vMat * mMat
stack<glm::mat4> mvStack;                           // 矩阵栈：push 进入子层级，pop 返回父层级

// ------------------------------ 顶点数据 ------------------------------

// 创建立方体（vbo[0]）和金字塔（vbo[1]）的顶点缓冲
void setupVertices(void) {
	// 立方体：36 个顶点 × 3 分量 = 108 个 float，12 个三角形，中心在原点，边长 2
	float vertexPositions[108] = {
		// ---- 前面（Z = -1）----
		-1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  // 三角形 1
		 1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f,  // 三角形 2
		// ---- 右面（X = +1）----
		 1.0f, -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  // 三角形 3
		 1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  // 三角形 4
		// ---- 后面（Z = +1）----
		 1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  // 三角形 5
		-1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  // 三角形 6
		// ---- 左面（X = -1）----
		-1.0f, -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f,  1.0f,  // 三角形 7
		-1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,  1.0f,  // 三角形 8
		// ---- 底面（Y = -1）----
		-1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  // 三角形 9
		 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f,  // 三角形 10
		// ---- 顶面（Y = +1）----
		-1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  // 三角形 11
		 1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f   // 三角形 12
	};

	// 金字塔：6 个三角形 × 3 顶点 × 3 分量 = 54 个 float
	float pyramidPositions[54] = {
		-1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  1.0f,  0.0f,  // 前面
		 1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  1.0f,  0.0f,  // 右面
		 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  1.0f,  0.0f,  // 后面
		-1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  1.0f,  0.0f,  // 左面
		-1.0f, -1.0f, -1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,  // 底面左三角
		 1.0f, -1.0f,  1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f   // 底面右三角
	};

	glGenVertexArrays(1, vao);                      // 生成 1 个 VAO，ID 写入 vao[0]
	glBindVertexArray(vao[0]);                      // 绑定 VAO，后续 VBO/属性配置记录到该 VAO
	glGenBuffers(numVBOs, vbo);                     // 生成 numVBOs 个 VBO，ID 写入 vbo 数组

	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);          // 绑定 vbo[0] 为当前数组缓冲
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexPositions), vertexPositions, GL_STATIC_DRAW);
	                                                // 将立方体顶点数据上传到 GPU，静态绘制

	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);          // 绑定 vbo[1] 为当前数组缓冲
	glBufferData(GL_ARRAY_BUFFER, sizeof(pyramidPositions), pyramidPositions, GL_STATIC_DRAW);
	                                                // 将金字塔顶点数据上传到 GPU，静态绘制
}

// ------------------------------ 初始化 ------------------------------

void init(GLFWwindow* window) {
	renderingProgram = Utils::createShaderProgram("vertShader.glsl", "fragShader.glsl");
	                                                // 编译并链接着色器，返回程序 ID

	glfwGetFramebufferSize(window, &width, &height); // 获取帧缓冲实际像素尺寸（可能不同于窗口尺寸）
	aspect = (float)width / (float)height;          // 计算宽高比
	pMat = glm::perspective(1.0472f, aspect, 0.1f, 1000.0f);
	                                                // 构建透视投影矩阵（60° FOV，近 0.1，远 1000）

	cameraX = 0.0f; cameraY = 0.0f; cameraZ = 12.0f; // 相机位于世界空间 (0, 0, 12)

	setupVertices();                                // 创建并上传顶点缓冲
}

// ------------------------------ 渲染一帧 ------------------------------

void display(GLFWwindow* window, double currentTime) {
	glClear(GL_DEPTH_BUFFER_BIT);                   // 清除深度缓冲（每帧深度测试从零开始）
	glClearColor(0.0, 0.0, 0.0, 1.0);             // 设置清除颜色为黑色 (R,G,B,A)
	glClear(GL_COLOR_BUFFER_BIT);                   // 用清除颜色填充颜色缓冲

	glUseProgram(renderingProgram);                 // 激活着色器程序

	mvLoc = glGetUniformLocation(renderingProgram, "mv_matrix");
	                                                // 查询 mv_matrix 在着色器中的 uniform 位置
	projLoc = glGetUniformLocation(renderingProgram, "proj_matrix");
	                                                // 查询 proj_matrix 在着色器中的 uniform 位置

	vMat = glm::translate(glm::mat4(1.0f), glm::vec3(-cameraX, -cameraY, -cameraZ));
	                                                // 视图矩阵：平移世界，等效相机反向移动

	mvStack.push(vMat);                             // 将视图矩阵压栈，作为场景变换树根节点

	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(pMat));
	                                                // 上传投影矩阵：1 个矩阵，不转置，传 float 指针

	// ========================================================================
	// 第一层级：太阳（金字塔）
	// ========================================================================
	mvStack.push(mvStack.top());                    // 复制栈顶（视图矩阵），进入太阳层级
	mvStack.top() *= glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
	                                                // 太阳位于原点，恒等平移（保留教材结构）

	mvStack.push(mvStack.top());                    // 再压一层，专用于太阳自转变换
	mvStack.top() *= glm::rotate(glm::mat4(1.0f), (float)currentTime, glm::vec3(1.0f, 0.0f, 0.0f));
	                                                // 绕 X 轴自转，角度 = 当前时间（秒，弧度）

	glUniformMatrix4fv(mvLoc, 1, GL_FALSE, glm::value_ptr(mvStack.top()));
	                                                // 将栈顶矩阵（太阳 MV 矩阵）传给顶点着色器
	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);          // 绑定金字塔顶点缓冲
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	                                                // 属性 0：每顶点 3 个 float，不归一化，紧密排列
	glEnableVertexAttribArray(0);                     // 启用顶点属性 0
	glEnable(GL_DEPTH_TEST);                        // 开启深度测试（近处遮挡远处）
	glDepthFunc(GL_LEQUAL);                         // 深度比较：小于等于当前深度则通过
	glDrawArrays(GL_TRIANGLES, 0, 18);              // 绘制 18 个顶点（6 个三角形）	
	mvStack.pop();                                  // 弹出太阳自转层

	// ========================================================================
	// 第二层级：行星（立方体）
	// ========================================================================
	mvStack.push(mvStack.top());                    // 进入行星层级（父节点 = 太阳轨道层）
	mvStack.top() *= glm::translate(glm::mat4(1.0f),
		glm::vec3(sin((float)currentTime) * 4.0f, 0.0f, cos((float)currentTime) * 4.0f));
	                                                // XZ 平面公转，半径 4，角速度 1 弧度/秒

	mvStack.push(mvStack.top());                    // 行星自转变换层
	mvStack.top() *= glm::rotate(glm::mat4(1.0f), (float)currentTime, glm::vec3(0.0f, 1.0f, 0.0f));
	                                                // 绕 Y 轴自转

	glUniformMatrix4fv(mvLoc, 1, GL_FALSE, glm::value_ptr(mvStack.top()));
	                                                // 上传行星 MV 矩阵
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);          // 绑定立方体顶点缓冲
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	                                                // 配置顶点属性 0（位置 xyz）
	glEnableVertexAttribArray(0);                   // 启用顶点属性 0
	glEnable(GL_DEPTH_TEST);                        // 开启深度测试
	glDepthFunc(GL_LEQUAL);                         // 深度比较函数
	glDrawArrays(GL_TRIANGLES, 0, 36);              // 绘制立方体：36 个顶点（12 个三角形）
	mvStack.pop();                                  // 弹出行星自转层

	// ========================================================================
	// 第三层级：卫星（小立方体）
	// ========================================================================
	mvStack.push(mvStack.top());                    // 进入卫星层级（父节点 = 行星轨道层）
	mvStack.top() *= glm::translate(glm::mat4(1.0f),
		glm::vec3(0.0f, sin((float)currentTime) * 2.0f, cos((float)currentTime) * 2.0f));
	                                                // 相对行星在 YZ 平面公转，半径 2
	mvStack.top() *= glm::rotate(glm::mat4(1.0f), (float)currentTime, glm::vec3(0.0f, 0.0f, 1.0f));
	                                                // 绕 Z 轴自转
	mvStack.top() *= glm::scale(glm::mat4(1.0f), glm::vec3(0.25f, 0.25f, 0.25f));
	                                                // 各轴缩放至 0.25，得到小立方体

	glUniformMatrix4fv(mvLoc, 1, GL_FALSE, glm::value_ptr(mvStack.top()));
	                                                // 上传卫星 MV 矩阵
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);          // 复用立方体几何体
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	                                                // 配置顶点属性 0
	glEnableVertexAttribArray(0);                   // 启用顶点属性 0
	glEnable(GL_DEPTH_TEST);                        // 开启深度测试
	glDepthFunc(GL_LEQUAL);                         // 深度比较函数
	glDrawArrays(GL_TRIANGLES, 0, 36);              // 绘制小立方体（36 个顶点）

	mvStack.pop();                                  // 弹出卫星层
	mvStack.pop();                                  // 弹出行星轨道层

	// ========================================================================
	// 第二层级：卫星（小金字塔） 作业4.3
	// ========================================================================
	mvStack.push(mvStack.top());
	mvStack.top() *= glm::translate(glm::mat4(1.0f), glm::vec3(sin((float)currentTime) * 6.0f, 0.0f, cos((float)currentTime) * 9.0f));

	mvStack.push(mvStack.top());
	mvStack.top() *= glm::rotate(glm::mat4(1.0f), (float)currentTime, glm::vec3(0.0f, 1.0f, 0.0f));

	glUniformMatrix4fv(mvLoc, 1, GL_FALSE, glm::value_ptr(mvStack.top()));
	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glEnableVertexAttribArray(0);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	mvStack.pop();

	// 帧末清空矩阵栈：卫星层 → 行星轨道层 → 太阳轨道层 → 视图根节点
	mvStack.pop();                                  // 弹出太阳轨道层
	mvStack.pop();                                  // 弹出视图矩阵（根节点）
}

// ------------------------------ 窗口回调 ------------------------------

void window_size_callback(GLFWwindow* win, int newWidth, int newHeight) {
	aspect = (float)newWidth / (float)newHeight;    // 根据新尺寸重新计算宽高比
	glViewport(0, 0, newWidth, newHeight);          // 设置 OpenGL 视口为整个窗口区域
	pMat = glm::perspective(1.0472f, aspect, 0.1f, 1000.0f);
	                                                // 用新宽高比重建投影矩阵
}

// ------------------------------ 程序入口 ------------------------------

int main(void) {
	if (!glfwInit()) {                              // 初始化 GLFW 库
		exit(EXIT_FAILURE);                         // 初始化失败则退出，返回失败码
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);  // 请求 OpenGL 主版本号 4
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);  // 请求 OpenGL 次版本号 3
	GLFWwindow* window = glfwCreateWindow(1600, 800, "Chapter4 - program4", NULL, NULL);
	                                                // 创建 1600×800 窗口，无共享上下文、无监视器指定
	glfwMakeContextCurrent(window);                 // 将 OpenGL 上下文绑定到当前线程

	if (glewInit() != GLEW_OK) {                    // 初始化 GLEW，加载 OpenGL 函数指针
		exit(EXIT_FAILURE);                         // GLEW 初始化失败则退出
	}

	glfwSwapInterval(1);                            // 开启垂直同步，帧率与显示器刷新率对齐
	glfwSetWindowSizeCallback(window, window_size_callback);
	                                                // 注册窗口尺寸变化回调

	init(window);                                   // 执行一次性初始化

	while (!glfwWindowShouldClose(window)) {        // 主循环：直到用户关闭窗口
		display(window, glfwGetTime());             // 渲染一帧，传入当前时间（秒）
		glfwSwapBuffers(window);                    // 交换前后缓冲，显示本帧画面
		glfwPollEvents();                           // 处理窗口、键盘、鼠标等事件
	}

	glfwDestroyWindow(window);                      // 销毁窗口，释放相关资源
	glfwTerminate();                                // 终止 GLFW，清理全局状态
	exit(EXIT_SUCCESS);                             // 正常退出，返回成功码
}
