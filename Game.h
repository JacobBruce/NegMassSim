#pragma once
#include "GLGraphics.h"
#include "CLTypes.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "Timer.h"
#include "Camera.h"

class Game
{
public:
	Game(GLFWwindow* window, KeyboardServer& kServer, MouseServer& mServer);
	~Game();
	void Go();
	void ComputeStage1();
	void ComputeStage2();
	void ComputeStage3();
private:
	void RenderScene();
	void HandleInput();
	void BeginActions();
	void ComposeFrame();
private:
	KeyboardClient kbd;
	MouseClient mouse;
	GLGraphics gfx;

	CL openCL;
	cl_int cl_error;
	cl_RenderInfo rInfo;

	cl::Buffer cl_posBuff;
	cl::Buffer cl_negBuff;
	cl::Buffer cl_fragBuff;

	//Scene scene;
	Camera camera;
	cl_AAInfo aaInfo;
	//SCREEN::Resolution resolution;

	int32_t aa_level;
	uint32_t pixCount, fragCount;
	uint32_t heightSpan, widthSpan;
	uint32_t heightRays, widthRays;
	uint32_t heightHalf, widthHalf;
	float dX, dY;

	float deltaTime;
	Timer deltaTimer;
};

namespace GLOBALS {

	static const cl_AAInfo AA_X1  {1,   1,   1.0,   1.0};
	static const cl_AAInfo AA_X4  {4,   2,   0.5,   0.25};
	static const cl_AAInfo AA_X9  {9,   3,   1./3,  1./6};
	static const cl_AAInfo AA_X16 {16,  4,   0.25,  0.125};
}
