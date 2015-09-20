#include "Sprite.h"

#include <memory>
#include <future>

#include "glm/gtc/type_ptr.hpp"
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "SDL.h"

#include "gl_helpers.h"


using namespace std;


SpriteShader CreateSpriteShader(const std::string& vertexShaderFilename, const std::string& pixelShaderFilename)
{
	SpriteShader spriteShader;
	spriteShader.program = LoadShaders(vertexShaderFilename, pixelShaderFilename);

	spriteShader.texUniform = glGetUniformLocation(spriteShader.program, "s_texture");
	if (spriteShader.texUniform == -1)
	{
		printf("Unable to find uniform s_texture in player shader.\n");
		CheckOpenGLErrors();
	}

	spriteShader.modelviewUniform = glGetUniformLocation(spriteShader.program, "u_modelview");
	if (spriteShader.modelviewUniform == -1)
	{
		printf("Unable to find uniform u_modelview in player shader.\n");
		CheckOpenGLErrors();
	}

	spriteShader.projectionUniform = glGetUniformLocation(spriteShader.program, "u_projection");
	if (spriteShader.projectionUniform == -1)
	{
		printf("Unable to find uniform u_projection in player shader.\n");
		CheckOpenGLErrors();
	}

	return spriteShader;
}


Sprite CreateSprite(const std::string& spriteFilename)
{
	Sprite sprite;

	int w = 0;
	int h = 0;
	auto pixels = std::unique_ptr<uint8_t, void(*)(void*)>(stbi_load(spriteFilename.c_str(), &w, &h, 0, 4), stbi_image_free);
	if (!pixels)
	{
		printf("Unable to load the image from %s\n", spriteFilename.c_str());
		return sprite;
	}

	sprite.dimensions = Vector2(w, h);
	glGenTextures(1, &sprite.texture);
	glBindTexture(GL_TEXTURE_2D, sprite.texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.get());
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glBindTexture(GL_TEXTURE_2D, 0);


	glGenBuffers(1, &sprite.vertexBuffer);
	if (sprite.vertexBuffer == 0)
	{
		printf("Unable to allocate sprite vertex buffer: %s\n", glewGetErrorString(glGetError()));
		return Sprite {};
	}
	float minX = -w / 2.0f;
	float maxX = w / 2.0f;
	float minY = -h / 2.0f;
	float maxY = h / 2.0f;
	position_uv_vertex vertices[]
	{ { { minX, minY, 0.0f }, { 0.0f, 1.0f } },
	{ { maxX, minY, 0.0f }, { 1.0f, 1.0f } },
	{ { maxX, maxY, 0.0f }, { 1.0f, 0.0f } },
	{ { minX, maxY, 0.0f }, { 0.0f, 0.0f } } };
	sprite.vertexCount = sizeof(vertices) / sizeof(position_uv_vertex);
	glBindBuffer(GL_ARRAY_BUFFER, sprite.vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sprite.vertexCount * sizeof(position_uv_vertex), vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);


	glGenBuffers(1, &sprite.indexBuffer);
	if (sprite.indexBuffer == 0)
	{
		printf("Unable to allocate sprite index buffer: %s\n", glewGetErrorString(glGetError()));
		return Sprite {};
	}
	GLushort indices[] { 0, 1, 3, 3, 1, 2 };
	sprite.indexCount = sizeof(indices) / sizeof(GLushort);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sprite.indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sprite.indexCount * sizeof(GLushort), indices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	CheckOpenGLErrors();

	return sprite;
}


void DrawSprite(const Sprite& sprite, const SpriteShader& spriteShader, const glm::mat4& modelviewMatrix, const glm::mat4& projectionMatrix)
{
	glBindBuffer(GL_ARRAY_BUFFER, sprite.vertexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sprite.indexBuffer);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(position_uv_vertex), (void*)offsetof(position_uv_vertex, position));

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(position_uv_vertex), (void*)offsetof(position_uv_vertex, uv));

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, sprite.texture);
	glUniform1i(spriteShader.texUniform, 0);
	glUniformMatrix4fv(spriteShader.modelviewUniform, 1, GL_FALSE, glm::value_ptr(modelviewMatrix));
	glUniformMatrix4fv(spriteShader.projectionUniform, 1, GL_FALSE, glm::value_ptr(projectionMatrix));

	glDrawElements(GL_TRIANGLES, sprite.indexCount, GL_UNSIGNED_SHORT, 0);

	glDisableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	CheckOpenGLErrors();
}

glm::mat4 CreateSpriteModelviewMatrix(const Sprite& /*sprite*/, const Vector3& position, const Vector3& facing)
{
	return CalculateObjectTransform(position, facing);
}

glm::mat4 CreateSpriteModelviewMatrix(const Sprite& /*sprite*/, const Vector2& position, const Vector2& facing)
{
	return CalculateObjectTransform(position, facing);
}

glm::mat4 CreateSpriteBottomLeftModelviewMatrix(const Sprite& sprite, const Vector3& position, const Vector3& facing)
{
	return CreateSpriteModelviewMatrix(sprite, position - facing / 2.0f, facing);
}
