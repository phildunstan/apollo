#include <vector>
#include <glm/gtc/matrix_transform.hpp>
#include "glm/gtc/type_ptr.hpp"

#include "debug_draw.h"
#include "gl_helpers.h"
#include "profiler.h"
#include "sprite.h"

using namespace std;

namespace
{
	struct DebugDrawShader
	{
		GLProgram program {};
		GLint texUniform;
		GLint modelviewUniform;
		GLint projectionUniform;
		GLint colorUniform;
	};

	static DebugDrawShader debugDrawShader;
	static Sprite lineSprite;

	struct Line
	{
		Vector2 begin;
		Vector2 end;
		Color color;
	};

	static vector<Line> lines;


	glm::mat4 CreateDebugDrawSpriteModelviewMatrix(const Sprite& sprite, const Vector2& position, const Vector2& facing)
	{
		auto y = glm::vec3 { facing / sprite.dimensions.y, 0.0f };
		auto z = glm::vec3 { 0.0f, 0.0f, 1.0f };
		auto x = glm::normalize(glm::cross(y, z)) / sprite.dimensions.x;
		auto modelviewMatrix = glm::mat4(glm::vec4(x, 0.0f), glm::vec4(y, 0.0f), glm::vec4(z, 0.0f), glm::vec4(position, 0.0f, 1.0f));
		return modelviewMatrix;
	}

	static glm::mat4 CreateDebugDrawSpriteBottomLeftModelviewMatrix(const Sprite& sprite, const Vector2& position, const Vector2& facing)
	{
		return CreateDebugDrawSpriteModelviewMatrix(sprite, position + facing / 2.0f, facing);
	}

	void DrawDebugDrawSprite(const Sprite& sprite, const DebugDrawShader& shader, const glm::mat4& modelviewMatrix, const glm::mat4& projectionMatrix, Color color)
	{
		glBindBuffer(GL_ARRAY_BUFFER, sprite.vertexBuffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sprite.indexBuffer);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(position_uv_vertex), (void*)offsetof(position_uv_vertex, position));

		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(position_uv_vertex), (void*)offsetof(position_uv_vertex, uv));

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, sprite.texture);
		glUniform1i(shader.texUniform, 0);
		glUniformMatrix4fv(shader.modelviewUniform, 1, GL_FALSE, glm::value_ptr(modelviewMatrix));
		glUniformMatrix4fv(shader.projectionUniform, 1, GL_FALSE, glm::value_ptr(projectionMatrix));
		float r = static_cast<float>((static_cast<unsigned int>(color) & 0xff000000) >> 24) / 255.0f;
		float g = static_cast<float>((static_cast<unsigned int>(color) & 0x00ff0000) >> 16) / 255.0f;
		float b = static_cast<float>((static_cast<unsigned int>(color) & 0x0000ff00) >> 8) / 255.0f;
		float a = static_cast<float>((static_cast<unsigned int>(color) & 0x000000ff)) / 255.0f;
		glUniform4f(shader.colorUniform, r, g, b, a);

		glDrawElements(GL_TRIANGLES, sprite.indexCount, GL_UNSIGNED_SHORT, 0);

		glDisableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		CheckOpenGLErrors();
	}
}


void DebugDrawInit()
{
	debugDrawShader.program = LoadShaders("debug_vs.glsl", "debug_fs.glsl");

	debugDrawShader.texUniform = glGetUniformLocation(debugDrawShader.program, "s_texture");
	if (debugDrawShader.texUniform == -1)
	{
		printf("Unable to find uniform s_texture in player shader.\n");
		CheckOpenGLErrors();
	}

	debugDrawShader.modelviewUniform = glGetUniformLocation(debugDrawShader.program, "u_modelview");
	if (debugDrawShader.modelviewUniform == -1)
	{
		printf("Unable to find uniform u_modelview in player shader.\n");
		CheckOpenGLErrors();
	}

	debugDrawShader.projectionUniform = glGetUniformLocation(debugDrawShader.program, "u_projection");
	if (debugDrawShader.projectionUniform == -1)
	{
		printf("Unable to find uniform u_projection in player shader.\n");
		CheckOpenGLErrors();
	}

	debugDrawShader.colorUniform = glGetUniformLocation(debugDrawShader.program, "u_color");
	if (debugDrawShader.colorUniform == -1)
	{
		printf("Unable to find uniform u_color in debug draw shader.\n");
		CheckOpenGLErrors();
	}

	lineSprite = CreateSprite("debug_line.png");
	CheckOpenGLErrors();
}

void DebugDrawShutdown()
{
}

void DebugDrawClear()
{
	lines.clear();
}

void DebugDrawLine(const Vector2& begin, const Vector2& end, Color color)
{
	lines.push_back(Line { begin, end, color });
}

void DebugDrawBox(const glm::mat4& transform, float w, float h, Color color)
{
	auto a = Vector2 { -w / 2.0f , -h / 2.0f };
	auto b = Vector2 { -w / 2.0f ,  h / 2.0f };
	auto c = Vector2 {  w / 2.0f ,  h / 2.0f };
	auto d = Vector2 {  w / 2.0f , -h / 2.0f };
	auto _a = glm::vec2(transform * glm::vec4(a, 0.0f, 1.0f));
	auto _b = glm::vec2(transform * glm::vec4(b, 0.0f, 1.0f));
	auto _c = glm::vec2(transform * glm::vec4(c, 0.0f, 1.0f));
	auto _d = glm::vec2(transform * glm::vec4(d, 0.0f, 1.0f));
	DebugDrawLine(_a, _b, color);
	DebugDrawLine(_b, _c, color);
	DebugDrawLine(_c, _d, color);
	DebugDrawLine(_d, _a, color);
}

void DebugDrawBox2d(const Vector2& min, const Vector2& max, Color color)
{
	auto a = Vector2 { min.x, min.y };
	auto b = Vector2 { min.x, max.y };
	auto c = Vector2 { max.x, max.y };
	auto d = Vector2 { max.x, min.y };
	DebugDrawLine(a, b, color);
	DebugDrawLine(b, c, color);
	DebugDrawLine(c, d, color);
	DebugDrawLine(d, a, color);
}

void DebugDrawRender(const Time& /*time*/, int windowWidth, int windowHeight)
{
	PROFILER_TIMER_FUNCTION();

	glUseProgram(debugDrawShader.program);

	auto projectionMatrix = glm::ortho(-windowWidth / 2.0f, windowWidth / 2.0f, -windowHeight / 2.0f, windowHeight / 2.0f, -10.0f, 10.0f);

	for (const auto& line : lines)
	{
		float lineLength = glm::length(line.end - line.begin);
		if (lineLength == 0.0f)
			continue;

		auto modelviewMatrix = CreateDebugDrawSpriteBottomLeftModelviewMatrix(lineSprite, line.begin, line.end - line.begin);
		DrawDebugDrawSprite(lineSprite, debugDrawShader, modelviewMatrix, projectionMatrix, line.color);
	}
	lines.clear();

	glUseProgram(0);

	CheckOpenGLErrors();
}
