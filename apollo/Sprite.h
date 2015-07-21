#pragma once

#include <string>
#include "gl_helpers.h"
#include "glm/glm.hpp"

struct Sprite
{
	GLuint texture = 0;
	GLuint vertexBuffer = 0;
	GLuint vertexCount = 0;
	GLuint indexBuffer = 0;
	GLuint indexCount = 0;
};

Sprite CreateSprite(const std::string& spriteFilename);
void DrawSprite(const Sprite& sprite, const GLProgram& program, const glm::mat4& modelviewMatrix, const glm::mat4& projectionMatrix);


