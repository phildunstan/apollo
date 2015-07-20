#pragma once

#include <memory>
#include <string>
#include "GL/glew.h"

template <typename DeleterT>
class GLResource
{
public:
	GLResource()
		: m_resource(0)
		, m_deleter()
	{
	}

	GLResource(GLuint resource, DeleterT deleter)
		: m_resource(resource)
		, m_deleter(deleter)
	{
	}

	GLResource(const GLResource&) = delete;

	GLResource(GLResource&& other)
		: m_resource(other.m_resource)
		, m_deleter(std::move(other.m_deleter))
	{
		other.m_resource = 0;
		other.m_deleter = DeleterT();
	}

	~GLResource()
	{
		if (m_deleter)
			m_deleter(m_resource);
	}

	GLResource& operator=(const GLResource&) = delete;

	GLResource& operator=(GLResource&& other)
	{
		m_resource = other.m_resource;
		other.m_resource = 0;
		other.m_deleter = DeleterT();
		return *this;
	}

	operator GLuint() const { return m_resource; }
	//GLuint get() const { return m_resource; }

private:
	GLuint m_resource;
	DeleterT m_deleter;
};

typedef GLResource<void (*)(GLuint)> GLShader;

GLShader CreateShader(GLenum shaderType, const char* source);
GLShader LoadShader(GLenum shaderType, const std::string& filename);

typedef GLResource<void(*)(GLuint)> GLProgram;

GLProgram CreateProgram(GLShader vertexShader, GLShader fragmentShader);
GLProgram LoadShaders(const std::string& vertexShaderFilename, const std::string& pixelShaderFilename);

bool CheckOpenGLErrors();


struct position_uv_vertex
{
	GLfloat position[3];
	GLfloat uv[2];
};



