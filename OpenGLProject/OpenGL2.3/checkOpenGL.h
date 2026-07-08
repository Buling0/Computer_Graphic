#pragma once
#include <GL/glew.h>
#include <iostream>

using namespace std;

void printShaderLog(GLuint shader) {
	int len = 0;
	int chWrittn = 0;
	char* log = nullptr;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
	if (len > 0) {
		log = (char*)malloc(len);
		glGetShaderInfoLog(shader, len, &chWrittn, log);
		cout << "Shader Info Log: " << log << endl;
		free(log);
	}
}

void printProgramLog(GLuint program) {
	int len = 0;
	int chWrittn = 0;
	char* log = nullptr;
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
	if (len > 0) {
		log = (char*)malloc(len);
		glGetProgramInfoLog(program, len, &chWrittn, log);
		cout << "Program Info Log: " << log << endl;
		free(log);
	}
}

bool checkOpenGLError() {
	bool foundError = false;
	int glError = glGetError();
	while (glError != GL_NO_ERROR) {
		cout << "glError: " << glError << endl;
		foundError = true;
		glError = glGetError();
	}

	return foundError;
}