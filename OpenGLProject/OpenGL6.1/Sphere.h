// ============================================================================
// Sphere.h - 程序化球体网格生成类
//
// 通过经纬度细分（precision = prec）生成单位球面网格：
//   - vertices  : 顶点位置（单位球面上的 (x,y,z)）
//   - texCoords : 纹理坐标（s,t ∈ [0,1]）
//   - normals   : 顶点法线（单位球上与位置相同）
//   - tangents  : 切线（供后续法线贴图等高级用法，本例 6.1 未使用）
//   - indices   : 三角形索引（每 6 个索引构成 2 个三角形，组成一个网格四边形）
// ============================================================================

#include <cmath>                                  // sin、cos、asin 等
#include <vector>                                 // std::vector 容器
#include <glm/glm.hpp>                              // glm::vec2、glm::vec3

class Sphere
{
private:
	int numVertices;                                // 顶点数量 = (prec+1) × (prec+1)
	int numIndices;                                 // 索引数量 = prec × prec × 6
	std::vector<int> indices;                       // 三角形顶点索引
	std::vector<glm::vec3> vertices;                // 顶点位置
	std::vector<glm::vec2> texCoords;               // 纹理坐标
	std::vector<glm::vec3> normals;                 // 法线
	std::vector<glm::vec3> tangents;                // 切线
	void init(int);                                 // 按精度 prec 初始化网格
	float toRadians(float degrees);                 // 角度转弧度

public:
	Sphere();                                       // 默认构造，精度 48
	Sphere(int prec);                               // 指定精度构造
	int getNumVertices();                           // 返回顶点数
	int getNumIndices();                            // 返回索引数
	std::vector<int> getIndices();                  // 返回索引副本
	std::vector<glm::vec3> getVertices();           // 返回顶点副本
	std::vector<glm::vec2> getTexCoords();          // 返回纹理坐标副本
	std::vector<glm::vec3> getNormals();            // 返回法线副本
	std::vector<glm::vec3> getTangents();           // 返回切线副本
};
