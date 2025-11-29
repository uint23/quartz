#include <stdbool.h>
#include <stdio.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengles2.h>

#include "quartz.h"
#include "render.h"

/* shader handles */
static GLuint prog = 0;
static GLint u_res;
static GLint u_color;
static GLuint vbo = 0;

/* helpers */
static GLuint compile(GLenum type, const char* src);

static GLuint compile(GLenum type, const char* src)
{
	GLuint s = glCreateShader(type);
	glShaderSource(s, 1, &src, NULL);
	glCompileShader(s);

	GLint ok;
	glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
	if (!ok) {
		char log[512];
		glGetShaderInfoLog(s, sizeof(log), NULL, log);
		LOG_ERROR("renderer: shader error: %s", log);
		glDeleteShader(s);
		return 0;
	}

	return s;
}

void render_begin(int w, int h)
{
	glViewport(0, 0, w, h);
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(prog);
	glUniform2f(u_res, w, h);
}

void render_end(SDL_Window* win)
{
	SDL_GL_SwapWindow(win);
}

bool render_init(void)
{
	const char* vs =
		"attribute vec2 a_pos;"
		"uniform vec2 u_res;"
		"void main() {"
		"vec2 p = a_pos / u_res;"
		"p.y = 1.0 - p.y;"
		"p = p * 2.0 - 1.0;"
		"  gl_Position = vec4(p, 0.0, 1.0);"
		"}";

	const char* fs =
		"precision mediump float;"
		"uniform vec4 u_color;"
		"void main()"
		"{"
		"  gl_FragColor = u_color;"
		"}";

	GLuint v = compile(GL_VERTEX_SHADER, vs);
	GLuint f = compile(GL_FRAGMENT_SHADER, fs);
	if (!v || !f)
		return false;

	prog = glCreateProgram();
	glAttachShader(prog, v);
	glAttachShader(prog, f);
	glBindAttribLocation(prog, 0, "a_pos");
	glLinkProgram(prog);

	glDeleteShader(v);
	glDeleteShader(f);

	GLint ok;
	glGetProgramiv(prog, GL_LINK_STATUS, &ok);
	if (!ok) {
		LOG_ERROR("renderer: program link failed");
		return false;
	}

	u_res = glGetUniformLocation(prog, "u_res");
	u_color = glGetUniformLocation(prog, "u_color");

	glGenBuffers(1, &vbo);

	return true;
}

void render_rect(float x, float y, float w, float h, float r, float g, float b, float a)
{
	float verts[12] = {
		x,    y,
		x+w,  y,
		x+w,  y+h,

		x,    y,
		x+w,  y+h,
		x,    y+h
	};

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STREAM_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

	glUniform4f(u_color, r, g, b, a);

	glDrawArrays(GL_TRIANGLES, 0, 6);
}

void render_shutdown(void)
{
	glDeleteProgram(prog);
	glDeleteBuffers(1, &vbo);
}
