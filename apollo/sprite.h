#pragma once

#include <string>
#include "glm/glm.hpp"
#include "gl_helpers.h"
#include "math_helpers.h"

struct SpriteShader
{
	GLProgram program {};
	GLint texUniform;
	GLint modelviewUniform;
	GLint projectionUniform;
};

SpriteShader CreateSpriteShader(const std::string& vertexShaderFilename, const std::string& pixelShaderFilename);

struct Sprite
{
	Vector2 dimensions { 0.0f, 0.0f };
	GLuint texture { 0 };
	GLuint vertexBuffer { 0 };
	GLuint vertexCount { 0 };
	GLuint indexBuffer { 0 };
	GLuint indexCount { 0 };
};

Sprite CreateSprite(const std::string& spriteFilename);
void DrawSprite(const Sprite& sprite, const SpriteShader& spriteShader, const glm::mat4& modelviewMatrix, const glm::mat4& projectionMatrix);

glm::mat4 CreateSpriteModelviewMatrix(const Sprite& sprite, const Vector3& position, const Vector3& facing);
glm::mat4 CreateSpriteModelviewMatrix(const Sprite& sprite, const Vector2& position, const Vector2& facing);
glm::mat4 CreateSpriteBottomLeftModelviewMatrix(const Sprite& sprite, const Vector3& position, const Vector3& facing);

