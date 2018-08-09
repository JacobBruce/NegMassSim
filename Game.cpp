#include "Game.h"
#include <time.h>
#include <cstdlib>
#include <string>
#include <math.h>
#include <iostream>
#include <fstream>

Game::Game(GLFWwindow* window, KeyboardServer& kServer, MouseServer& mServer)
:
	kbd( kServer ),
	mouse( mServer )
{
	// ensure the compiler is playing nice
	assert(sizeof(RGB32) == sizeof(cl_RGB32) && sizeof(RGB32) == 4);
	assert(sizeof(Vec3) == sizeof(cl_float3) && sizeof(Vec3) == 16);
	assert(sizeof(DVec3) == sizeof(cl_double3) && sizeof(DVec3) == 32);
	assert(sizeof(cl_Particle) == 104);
	assert(sizeof(cl_RenderInfo) == 256);

	aa_level = stoi(GLOBALS::config_map["AA_LEVEL"]);

	switch (aa_level) {
		case 1: aaInfo = GLOBALS::AA_X1; break;
		case 4: aaInfo = GLOBALS::AA_X4; break;
		//case 9: aaInfo = GLOBALS::AA_X9; break;
		//case 16: aaInfo = GLOBALS::AA_X16; break;
		default:
			HandleFatalError(1, "Invalid AA level detected: "+GLOBALS::config_map["AA_LEVEL"]);
			break;
	}

	// Initialize OpenCL
	openCL.Initialize();

	// Initialize graphics manager
	gfx.Initialize(window, openCL.context());

	// Get window resolution profile
    //resolution = SCREEN::GetProfile(gfx.windowWidth, gfx.windowHeight);

	// set camera sensitivity and position with user settings
	camera.sensitivity = stof(GLOBALS::config_map["MOUSE_SENSI"]);
    camera.position.x = stof(GLOBALS::config_map["CAM_X_POS"]);
    camera.position.y = stof(GLOBALS::config_map["CAM_Y_POS"]);
    camera.position.z = stof(GLOBALS::config_map["CAM_Z_POS"]);
	srand(time(NULL));

	// calc useful screen info
	widthHalf = gfx.windowWidth / 2;
	heightHalf = gfx.windowHeight / 2;
	widthRays = gfx.windowWidth * aaInfo.lvl;
	heightRays = gfx.windowHeight * aaInfo.lvl;
	widthSpan = gfx.windowWidth - 1;
	heightSpan = gfx.windowHeight - 1;
	pixCount = gfx.windowWidth * gfx.windowHeight;
	fragCount = pixCount * aaInfo.lvl;

	rInfo.aa_info = aaInfo;
	rInfo.span_X = widthSpan;
	rInfo.span_Y = heightSpan;
	rInfo.pixels_X = gfx.windowWidth;
	rInfo.pixels_Y = gfx.windowHeight;
	rInfo.half_X = widthHalf;
	rInfo.half_Y = heightHalf;
	rInfo.particles = stoi(GLOBALS::config_map["PARTICLES"]);
	rInfo.rand_int = rand();

	// allocate memory on GPU for pixel fragment buffer
	cl_fragBuff = cl::Buffer(openCL.context, CL_MEM_READ_WRITE, sizeof(cl_RGB32)*fragCount);

	// allocate memory on GPU for positive particle buffer
	cl_posBuff = cl::Buffer(openCL.context, CL_MEM_READ_WRITE, sizeof(cl_Particle)*rInfo.particles);

	// allocate memory on GPU for positive particle buffer
	cl_negBuff = cl::Buffer(openCL.context, CL_MEM_READ_WRITE, sizeof(cl_Particle)*rInfo.particles);

	// generate random particles
	char isNeg = 0;
	openCL.Init_Kernel.setArg(0, cl_posBuff);
	openCL.Init_Kernel.setArg(1, isNeg);
	openCL.Init_Kernel.setArg(2, rInfo);
	openCL.GenParticles(rInfo.particles);
	openCL.queue.finish();

	rInfo.rand_int = rand();
	isNeg = 1;

	openCL.Init_Kernel.setArg(0, cl_negBuff);
	openCL.Init_Kernel.setArg(1, isNeg);
	openCL.Init_Kernel.setArg(2, rInfo);
	openCL.GenParticles(rInfo.particles);
	openCL.queue.finish();

	deltaTimer.ResetTimer();
}

Game::~Game(){}

void Game::Go()
{
	deltaTime = deltaTimer.MilliCount();
	deltaTimer.ResetTimer();
	ComposeFrame();
	gfx.DisplayFrame();
}

void Game::HandleInput()
{
	// keys for left and right tilt
	if (kbd.KeyIsPressed(GLFW_KEY_E)) {
		camera.orientation.z -= CAMSPIN_SPEED * deltaTime;
	} else if (kbd.KeyIsPressed(GLFW_KEY_Q)) {
		camera.orientation.z += CAMSPIN_SPEED * deltaTime;
	}

	KeyEvent ke = kbd.ReadKey();
	if (ke.IsPress()) {
		switch (ke.GetCode()) {
		case GLFW_KEY_SCROLL_LOCK:
			gfx.ToggleCursorLock();
			break;
		default: break;
		}
	}

	// handle mouse events
	MouseEvent me = mouse.ReadMouse();
	switch (me.GetType()) {
	case MouseEvent::Move:
		if (gfx.cursorLocked) {
			dX = me.GetX() * camera.sensitivity * deltaTime;
			dY = me.GetY() * camera.sensitivity * deltaTime;
			camera.orientation.x += dY;
			camera.orientation.y -= dX;
			glfwSetCursorPos(gfx.window, 0.0, 0.0);
		}
		break;
	case MouseEvent::WheelUp:
		camera.foclen += 10;
		break;
	case MouseEvent::WheelDown:
		camera.foclen -= 10;
		break;
	case MouseEvent::LPress:
		break;
	case MouseEvent::RPress:
		break;
	case MouseEvent::LRelease:
		break;
	case MouseEvent::RRelease:
		break;
	case MouseEvent::Invalid:
		break;
	}

	// update camera direction
	camera.UpdateDirection();

	// keys for up and down movement
	if (kbd.KeyIsPressed(GLFW_KEY_PAGE_UP)) {
		camera.position += camera.up * (CAMMOVE_SPEED * deltaTime);
	} else if (kbd.KeyIsPressed(GLFW_KEY_PAGE_DOWN)) {
		camera.position -= camera.up * (CAMMOVE_SPEED * deltaTime);
	}

	// keys for forward and backward movement
	if (kbd.KeyIsPressed(GLFW_KEY_W)) {
		camera.position += camera.forward * (CAMMOVE_SPEED * deltaTime);
	} else if (kbd.KeyIsPressed(GLFW_KEY_S)) {
		camera.position -= camera.forward * (CAMMOVE_SPEED * deltaTime);
	}

	// keys for right and left movement
	if (kbd.KeyIsPressed(GLFW_KEY_D)) {
		camera.position += camera.right * (CAMMOVE_SPEED * deltaTime);
	} else if (kbd.KeyIsPressed(GLFW_KEY_A)) {
		camera.position -= camera.right * (CAMMOVE_SPEED * deltaTime);
	}
}

void Game::BeginActions()
{
	// calculate bottom left position of virtual screen
	camera.bl_ray = (camera.forward * camera.foclen).
					VectSub(camera.right * widthHalf).
					VectSub(camera.up * heightHalf);

	// save data to RenderInfo structure
	rInfo.cam_info = camera.GetInfo();
	rInfo.rand_int = rand();
	rInfo.d_time = deltaTime;

	openCL.FillF_Kernel.setArg(0, cl_fragBuff);
	openCL.FillF_Kernel.setArg(1, rInfo);

	openCL.FillFragBuff(gfx.windowWidth, gfx.windowHeight);
	openCL.queue.finish();
}

void Game::ComputeStage1()
{
    // update particle positions

	openCL.Update_Kernel.setArg(0, cl_posBuff);
	openCL.Update_Kernel.setArg(1, cl_negBuff);
	openCL.Update_Kernel.setArg(2, rInfo);

	openCL.UpdateParticles(rInfo.particles);
	openCL.queue.finish();
}

void Game::ComputeStage2()
{
	// compute final pixel colors

	openCL.Draw_Kernel.setArg(0, cl_posBuff);
	openCL.Draw_Kernel.setArg(1, cl_fragBuff);
	openCL.Draw_Kernel.setArg(2, YELLOW.rgba);
	openCL.Draw_Kernel.setArg(3, rInfo);
	openCL.DrawParticles(rInfo.particles);

	openCL.Draw_Kernel.setArg(0, cl_negBuff);
	openCL.Draw_Kernel.setArg(1, cl_fragBuff);
	openCL.Draw_Kernel.setArg(2, BLUE.rgba);
	openCL.Draw_Kernel.setArg(3, rInfo);
	openCL.DrawParticles(rInfo.particles);

	openCL.queue.finish();
}

void Game::ComputeStage3()
{
	// write frag buffer to frame buffer

	openCL.CopyF_Kernel.setArg(0, cl_fragBuff);
	openCL.CopyF_Kernel.setArg(1, gfx.gl_backBuff);
	openCL.CopyF_Kernel.setArg(2, rInfo);

	openCL.FragsToFrame(gfx.windowWidth, gfx.windowHeight);
	openCL.queue.finish();
}

void Game::RenderScene()
{
	// update particles
	ComputeStage1();

	// draw particles
	ComputeStage2();

	// write frame
	ComputeStage3();
}

void Game::ComposeFrame()
{
	// handle keyboard/mouse actions
	HandleInput();

	// reset/update some stuff
	BeginActions();

	// give OCL control of OGL framebuffer
	gfx.AcquireBackBuff(openCL.queue());

	// render the 3D scene using OCL
	RenderScene();

	// make OCL release control of OGL memory
	gfx.ReleaseBackBuff(openCL.queue());
}
