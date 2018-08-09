#pragma once
#include "GLFWFuncs.h"
#include "OpenCL.h"
#include <assert.h>

class GLGraphics
{
public:
	void Initialize(GLFWwindow* pWindow, cl_context& context);
	void AcquireBackBuff(cl_command_queue& queue);
	void ReleaseBackBuff(cl_command_queue& queue);
	void ToggleCursorLock();
	void GetWindowSize(int* width, int* height);
	void SetWindowSize(int width, int height);
	void BeginFrame();
	void DisplayFrame();
private:
	GLuint		gl_fb_id;
	GLuint		gl_rb_id;
	GLuint		gl_tex_id;
	GLenum		gl_status;
	cl_int		cl_error;
	cl_context	cl_con;
public:
	GLFWwindow*	window;
	cl_mem		gl_backBuff;
	int			windowWidth;
	int			windowHeight;
	int			widthHalf;
	int			heightHalf;
	int         widthSpan;
	int         heightSpan;
	bool		cursorLocked;
};
