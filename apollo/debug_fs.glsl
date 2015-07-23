#version 300 es
precision mediump float;

in vec2 v_texCoord;

layout(location = 0) out vec4 fragColor;

uniform sampler2D s_texture;
uniform vec4 u_color;

void main()
{
	fragColor = u_color * texture(s_texture, v_texCoord);
}
