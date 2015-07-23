#include <vector>
#include <glm/gtc/matrix_transform.hpp>
#include "glm/gtc/type_ptr.hpp"

#include "Sprite.h"
#include "sdl_helpers.h"
#include "gl_helpers.h"
#include "debug_draw.h"

using namespace std;

static GLProgram shaderProgram;
static Sprite lineSprite;

struct Line
{
	Vector3 begin;
	Vector3 end;
	Color color;
};

static vector<Line> lines;


namespace
{
	glm::mat4 CreateDebugDrawSpriteModelviewMatrix(const Sprite& sprite, const Vector3& position, const Vector3& facing)
	{
		auto y = facing / sprite.dimensions.y;
		auto z = glm::vec3(0.0f, 0.0f, 1.0f);
		auto x = glm::normalize(glm::cross(y, z)) / sprite.dimensions.x;
		auto modelviewMatrix = glm::mat4(glm::vec4(x, 0.0f), glm::vec4(y, 0.0f), glm::vec4(z, 0.0f), glm::vec4(position, 1.0f));
		return modelviewMatrix;
	}

	static glm::mat4 CreateDebugDrawSpriteBottomLeftModelviewMatrix(const Sprite& sprite, const Vector3& position, const Vector3& facing)
	{
		return CreateDebugDrawSpriteModelviewMatrix(sprite, position + facing / 2.0f, facing);
	}

	void DrawDebugDrawSprite(const Sprite& sprite, const GLProgram& program, const glm::mat4& modelviewMatrix, const glm::mat4& projectionMatrix, Color color)
	{
		glBindBuffer(GL_ARRAY_BUFFER, sprite.vertexBuffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sprite.indexBuffer);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(position_uv_vertex), (void*)offsetof(position_uv_vertex, position));

		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(position_uv_vertex), (void*)offsetof(position_uv_vertex, uv));

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, sprite.texture);
		auto texUniform = glGetUniformLocation(program, "s_texture");
		if (texUniform == -1)
		{
			printf("Unable to find uniform s_texture in player shader.\n");
			CheckOpenGLErrors();
		}
		glUniform1i(texUniform, 0);

		auto modelviewUniform = glGetUniformLocation(program, "u_modelview");
		if (modelviewUniform == -1)
		{
			printf("Unable to find uniform u_modelview in player shader.\n");
			CheckOpenGLErrors();
		}
		glUniformMatrix4fv(modelviewUniform, 1, GL_FALSE, glm::value_ptr(modelviewMatrix));

		auto projectionUniform = glGetUniformLocation(program, "u_projection");
		if (projectionUniform == -1)
		{
			printf("Unable to find uniform u_projection in player shader.\n");
			CheckOpenGLErrors();
		}
		glUniformMatrix4fv(projectionUniform, 1, GL_FALSE, glm::value_ptr(projectionMatrix));

		auto colorUniform = glGetUniformLocation(program, "u_color");
		if (colorUniform == -1)
		{
			printf("Unable to find uniform u_color in debug draw shader.\n");
			CheckOpenGLErrors();
		}
		float r = static_cast<float>((static_cast<unsigned int>(color) & 0xff000000) >> 24) / 255.0f;
		float g = static_cast<float>((static_cast<unsigned int>(color) & 0x00ff0000) >> 16) / 255.0f;
		float b = static_cast<float>((static_cast<unsigned int>(color) & 0x0000ff00) >> 8) / 255.0f;
		float a = static_cast<float>((static_cast<unsigned int>(color) & 0x000000ff)) / 255.0f;
		glUniform4f(colorUniform, r, g, b, a);
		CheckOpenGLErrors();

		glDrawElements(GL_TRIANGLES, sprite.indexCount, GL_UNSIGNED_SHORT, 0);

		glDisableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		CheckOpenGLErrors();
	}
}


void DebugDrawInit()
{
	shaderProgram = LoadShaders("debug_vs.glsl", "debug_fs.glsl");
	lineSprite = CreateSprite("debug_line.png");
}

void DebugDrawShutdown()
{
}

void DebugDrawClear()
{
	lines.clear();
}

void DebugDrawLine(const Vector3& begin, const Vector3& end, Color color)
{
	lines.push_back(Line { begin, end, color });
}

void DebugDrawBox(const glm::mat4& transform, float w, float h, Color color)
{
	auto a = Vector2 { -w / 2.0f , -h / 2.0f };
	auto b = Vector2 { -w / 2.0f ,  h / 2.0f };
	auto c = Vector2 {  w / 2.0f ,  h / 2.0f };
	auto d = Vector2 {  w / 2.0f , -h / 2.0f };
	auto _a = glm::vec3(transform * glm::vec4(a, 0.0f, 1.0f));
	auto _b = glm::vec3(transform * glm::vec4(b, 0.0f, 1.0f));
	auto _c = glm::vec3(transform * glm::vec4(c, 0.0f, 1.0f));
	auto _d = glm::vec3(transform * glm::vec4(d, 0.0f, 1.0f));
	DebugDrawLine(_a, _b, color);
	DebugDrawLine(_b, _c, color);
	DebugDrawLine(_c, _d, color);
	DebugDrawLine(_d, _a, color);
}

void DebugDrawRender(const glm::mat4& projectionMatrix)
{
	glUseProgram(shaderProgram);

	for (const auto& line : lines)
	{
		float lineLength = glm::length(line.end - line.begin);
		if (lineLength == 0.0f)
			continue;

		auto modelviewMatrix = CreateDebugDrawSpriteBottomLeftModelviewMatrix(lineSprite, line.begin, line.end - line.begin);
		DrawDebugDrawSprite(lineSprite, shaderProgram, modelviewMatrix, projectionMatrix, line.color);
	}
	lines.clear();

	glUseProgram(0);
}
