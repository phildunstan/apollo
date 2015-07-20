#version 300 es
layout(location = 0) in vec4 a_position;
layout(location = 1) in vec2 a_texcoord;
out vec2 v_texCoord;
uniform mat4 u_modelview;
uniform mat4 u_projection;
void main()
{
	gl_Position = u_projection * u_modelview * a_position;
	v_texCoord = a_texcoord;
}
