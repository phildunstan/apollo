#include <vector>
#include <glm/gtc/matrix_transform.hpp>

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


static glm::mat4 CreateDebugDrawSpriteModelviewMatrix(const Sprite& sprite, const Vector3& position, const Vector3& facing)
{
	auto y = facing / sprite.dimensions.y;
	auto z = glm::vec3(0.0f, 0.0f, 1.0f);
	auto x = glm::normalize(glm::cross(y, z)) / sprite.dimensions.x;
	auto modelviewMatrix = glm::mat4(glm::vec4(x, 0.0f), glm::vec4(y, 0.0f), glm::vec4(z, 0.0f), glm::vec4(position, 1.0f));
	return modelviewMatrix;
}

static glm::mat4 CreateDebugDrawSpriteBottomLeftModelviewMatrix(const Sprite& sprite, const Vector3& position, const Vector3& facing)
{
	return CreateDebugDrawSpriteModelviewMatrix(sprite, position - facing / 2.0f, facing);
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

void DebugDrawRender(const glm::mat4& projectionMatrix)
{
	for (const auto& line : lines)
	{
		float lineLength = glm::length(line.end - line.begin);
		if (lineLength == 0.0f)
			continue;

		auto modelviewMatrix = CreateDebugDrawSpriteBottomLeftModelviewMatrix(lineSprite, Vector3(0.0f, lineLength, 0.0f), line.end - line.begin);
		DrawSprite(lineSprite, shaderProgram, modelviewMatrix, projectionMatrix);
	}
}
