// ============================================================================
// Sphere.cpp - 球体网格生成实现
//
// 网格生成思路：
//   1. 在纬线 i（0..prec）和经线 j（0..prec）上采样球面点
//   2. 相邻四边形拆成 2 个三角形，写入 indices
//   3. 单位球：法线 = 归一化后的顶点位置（此处顶点已在单位球上）
// ============================================================================

#include <cmath>
#include <vector>
#include <iostream>
#include <glm/glm.hpp>
#include "Sphere.h"

using namespace std;

Sphere::Sphere() {
	init(48);                                       // 默认精度 48
}

Sphere::Sphere(int prec) {
	init(prec);                                     // 使用指定精度
}

// 角度转弧度：rad = deg × π / 180
float Sphere::toRadians(float degrees) { return (degrees * 2.0f * 3.14159f) / 360.0f; }

void Sphere::init(int prec) {
	numVertices = (prec + 1) * (prec + 1);          // 网格点总数（含极点行列）
	numIndices = prec * prec * 6;                   // 每个网格单元 2 三角形 × 3 顶点

	// 预分配容器空间（先 push 占位元素）
	for (int i = 0; i < numVertices; i++) { vertices.push_back(glm::vec3()); }
	for (int i = 0; i < numVertices; i++) { texCoords.push_back(glm::vec2()); }
	for (int i = 0; i < numVertices; i++) { normals.push_back(glm::vec3()); }
	for (int i = 0; i < numVertices; i++) { tangents.push_back(glm::vec3()); }
	for (int i = 0; i < numIndices; i++) { indices.push_back(0); }

	// ---- 生成球面顶点、纹理坐标、法线、切线 ----
	for (int i = 0; i <= prec; i++) {               // i：纬线（从北极到南极）
		for (int j = 0; j <= prec; j++) {           // j：经线（绕 Y 轴一圈）
			// 球面参数方程：y 由纬线决定，x/z 由经线和 y 决定
			float y = (float)cos(toRadians(180.0f - i * 180.0f / prec));
			float x = -(float)cos(toRadians(j * 360.0f / prec)) * (float)abs(cos(asin(y)));
			float z = (float)sin(toRadians(j * 360.0f / (float)(prec))) * (float)abs(cos(asin(y)));

			int index = i * (prec + 1) + j;         // 二维网格转一维索引
			vertices[index] = glm::vec3(x, y, z);   // 单位球顶点位置
			texCoords[index] = glm::vec2(((float)j / prec), ((float)i / prec));
			                                        // 纹理坐标：s 沿经线，t 沿纬线
			normals[index] = glm::vec3(x, y, z);    // 单位球法线 = 位置向量

			// 切线：Y 轴与法线叉乘；极点处法线与 Y 轴平行，单独处理
			if (((x == 0) && (y == 1) && (z == 0)) || ((x == 0) && (y == -1) && (z == 0))) {
				tangents[index] = glm::vec3(0.0f, 0.0f, -1.0f); // 北极/南极
			}
			else {
				tangents[index] = glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(x, y, z));
			}
		}
	}

	// ---- 生成三角形索引（每个网格四边形 → 2 个三角形）----
	for (int i = 0; i < prec; i++) {
		for (int j = 0; j < prec; j++) {
			// 四边形四个角顶点索引：
			//   v0 = (i,   j)   v1 = (i,   j+1)
			//   v2 = (i+1, j)   v3 = (i+1, j+1)
			int base = 6 * (i * prec + j);          // 本四边形在 indices 中的起始下标
			indices[base + 0] = i * (prec + 1) + j;           // 三角形1：v0, v1, v2
			indices[base + 1] = i * (prec + 1) + j + 1;
			indices[base + 2] = (i + 1) * (prec + 1) + j;
			indices[base + 3] = i * (prec + 1) + j + 1;     // 三角形2：v1, v3, v2
			indices[base + 4] = (i + 1) * (prec + 1) + j + 1;
			indices[base + 5] = (i + 1) * (prec + 1) + j;
		}
	}
}

// ---- 访问器：返回成员数据的副本 ----
int Sphere::getNumVertices() { return numVertices; }
int Sphere::getNumIndices() { return numIndices; }
std::vector<int> Sphere::getIndices() { return indices; }
std::vector<glm::vec3> Sphere::getVertices() { return vertices; }
std::vector<glm::vec2> Sphere::getTexCoords() { return texCoords; }
std::vector<glm::vec3> Sphere::getNormals() { return normals; }
std::vector<glm::vec3> Sphere::getTangents() { return tangents; }
