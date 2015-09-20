#include "gl_helpers.h"

#include "GL/glew.h"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <memory>

using namespace std;

GLShader CreateShader(GLenum shaderType, const char* source)
{
	auto shader = GLShader(glCreateShader(shaderType), glDeleteShader);
	if (shader == 0)
	{
		printf("Error creating OpenGL shader: %s\n", glewGetErrorString(glGetError()));
		return GLShader();
	}

	glShaderSource(shader, 1, &source, nullptr);

	glCompileShader(shader);

	GLint compiled = GL_FALSE;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	if (compiled == GL_FALSE)
	{
		GLint logLength = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
		auto log = unique_ptr<GLchar[]>(new GLchar[logLength]);
		glGetShaderInfoLog(shader, logLength, nullptr, log.get());
		printf("Error compiling OpenGL shader source: %s\n", glewGetErrorString(glGetError()));
		printf("%s\n", log.get());
		return GLShader();
	}

	return shader;
}

GLShader LoadShader(GLenum shaderType, const string& filename)
{
	// read in the full filename
	ifstream in { filename.c_str() };
	if (!in.good())
	{
		printf("Error opening file %s\n", filename.c_str());
	}
	auto ss = ostringstream {};
	ss << in.rdbuf();

	return CreateShader(shaderType, ss.str().c_str());
}

GLProgram CreateProgram(GLShader vertexShader, GLShader fragmentShader)
{
	auto program = GLProgram(glCreateProgram(), glDeleteProgram);
	if (program == 0)
	{
		printf("Error creating OpenGL program: %s\n", glewGetErrorString(glGetError()));
		return GLProgram();
	}

	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);

	glLinkProgram(program);

	GLint linked = GL_FALSE;
	glGetProgramiv(program, GL_LINK_STATUS, &linked);
	if (linked == GL_FALSE)
	{
		GLint logLength = 0;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
		auto log = unique_ptr<GLchar[]>(new GLchar[logLength]);
		glGetShaderInfoLog(program, logLength, nullptr, log.get());
		printf("Error linking OpenGL shaders: %s\n", glewGetErrorString(glGetError()));
		printf("%s\n", log.get());
		return GLShader();
	}

	return program;
}

GLProgram LoadShaders(const std::string & vertexShaderFilename, const std::string & fragmentShaderFilename)
{
	auto vertexShader = LoadShader(GL_VERTEX_SHADER, vertexShaderFilename);
	if (vertexShader == 0)
		return GLProgram();

	auto fragmentShader = LoadShader(GL_FRAGMENT_SHADER, fragmentShaderFilename);
	if (fragmentShader == 0)
		return GLProgram();

	return move(CreateProgram(move(vertexShader), move(fragmentShader)));
}

bool CheckOpenGLErrors()
{
#if defined(DEBUG)
	bool errorExists = false;
	auto glError = glGetError();
	while (glError != GL_NO_ERROR)
	{
		printf("OpenGL error: %s(0x%x)\n", glewGetErrorString(glError), glError);
		errorExists = true;
	}
	return !errorExists;
#else
	return true;
#endif
}


