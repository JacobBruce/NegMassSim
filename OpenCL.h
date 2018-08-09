#pragma once
#define __CL_ENABLE_EXCEPTIONS
#include <CL/cl.hpp>
#include "CLTypes.h"
#include "ReadWrite.h"
#include <stdio.h>
#include <cstdlib>
#include <string>
#include <iostream>

#ifdef linux
    #include <GL/glx.h>
#endif

class CL
{
private:
	cl::Platform platform;
	cl::Device device;
	cl::Program program;
public:
	cl::CommandQueue queue;
	cl::Context context;
	cl::Kernel Init_Kernel;
	cl::Kernel Update_Kernel;
	cl::Kernel Draw_Kernel;
	cl::Kernel FillF_Kernel;
	cl::Kernel CopyF_Kernel;
	uint32_t max_wg_size;
public:
	void Initialize()
	{
		std::cout << "Initializing OpenCL ... ";

		// get all platforms (drivers)
		std::vector<cl::Platform> platforms;
		cl::Platform::get(&platforms);

		if (platforms.size() == 0) {
			HandleFatalError(30, "No OpenCL platforms found");
		}
		// select default platform
		platform = platforms[0];

		#ifdef linux
			cl_context_properties props[] = {
			CL_GL_CONTEXT_KHR, (cl_context_properties) glXGetCurrentContext(),
			CL_GLX_DISPLAY_KHR, (cl_context_properties) glXGetCurrentDisplay(),
			CL_CONTEXT_PLATFORM, (cl_context_properties)(platform)(), 0};
		#elif defined WIN32 || defined _WIN32
			cl_context_properties props[] = {
			CL_GL_CONTEXT_KHR, (cl_context_properties) wglGetCurrentContext(),
			CL_WGL_HDC_KHR, (cl_context_properties) wglGetCurrentDC(),
			CL_CONTEXT_PLATFORM, (cl_context_properties)(platform)(), 0};
		#elif defined(__APPLE__) || defined(MACOSX)
			CGLContextObj glContext = CGLGetCurrentContext();
			CGLShareGroupObj shareGroup = CGLGetShareGroup(glContext);
			cl_context_properties props[] = {
			CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE,
			(cl_context_properties)shareGroup, 0};
		#endif

		context = cl::Context(CL_DEVICE_TYPE_GPU, props);

		// get compute devices on default platform
		std::vector<cl::Device> gpu_devices = context.getInfo<CL_CONTEXT_DEVICES>();
		if (gpu_devices.size() == 0) {
			HandleFatalError(31, "No supported GPU devices found");
		}

		// select a compute device
        std::string gpu_index = GLOBALS::config_map["GPU_INDEX"];
        if (gpu_index == "primary") {
            device = gpu_devices[0];
        } else {
            size_t gi = stoi(gpu_index);
            if (gi < platforms.size()) {
                device = gpu_devices[gi];
            } else {
                HandleFatalError(32, "Invalid GPU_INDEX, check settings");
            }
        }

		// veryify CL/GL sharing is supported on device
		std::string dev_exts = device.getInfo<CL_DEVICE_EXTENSIONS>();
		if (dev_exts.find(CL_GL_SHARING_EXT) == std::string::npos) {
			HandleFatalError(35, "Device does not support CL/GL sharing");
		} else if (device.getInfo<CL_DEVICE_IMAGE_SUPPORT>()!=CL_TRUE) {
			HandleFatalError(36, "Device does not support OpenCL images");
		} else {
			std::cout << "Success!\n";
		}

		// Read kernel source file
		cl::Program::Sources sources;
		std::string sourceCode = ReadFileStr(GLOBALS::DATA_FOLDER+"kernels/compute.cl");
		sources.push_back(std::make_pair(sourceCode.c_str(), sourceCode.length()+1));

		// Set program source code and context
		program = cl::Program(context, sources);

		// build kernel program and check for errors
		std::cout << "Building OpenCL kernels ... ";
		try {
            if (program.build(gpu_devices) != CL_SUCCESS) {
                std::cout << "Failed!\n";
                CLBLog("Build log: "+program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device));
                HandleFatalError(33, "Failed building kernel program.");
            } else {
                std::cout << "Success!\n";
                CLBLog("Build log: "+program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device));
            }
		} catch (...) {
            std::cout << "Failed!\n";
            CLBLog("Build log: "+program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device));
            HandleFatalError(34, "Failed building kernel program.");
		}

		// initialize kernel objects
		Init_Kernel = cl::Kernel(program, "GenParticles");
		Update_Kernel = cl::Kernel(program, "UpdateParticles");
		Draw_Kernel = cl::Kernel(program, "DrawParticles");
		FillF_Kernel = cl::Kernel(program, "FillFragBuff");
		CopyF_Kernel = cl::Kernel(program, "FragsToFrame");

		// create queue to which we will push commands for the device
		queue = cl::CommandQueue(context, device);

		// get maximum workgroup size for device
		max_wg_size = (cl_uint)device.getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>();

		// print OpenCL info to console
		PrintCLInfo();
	}
	void PrintCLInfo()
	{
		std::string device_name = device.getInfo<CL_DEVICE_NAME>();
		std::string device_vendor = device.getInfo<CL_DEVICE_VENDOR>();
		device_name.pop_back(); device_vendor.pop_back();

		std::cout << "\nUsing platform: "+platform.getInfo<CL_PLATFORM_NAME>() + "\n";
		std::cout << "Using device: "+device_name+" ("+device_vendor+")\n";
		std::cout << "OpenCL version: "+device.getInfo<CL_DEVICE_VERSION>() + "\n";
		std::cout << "Max parameter size: "+VarToStr((cl_uint)device.getInfo<CL_DEVICE_MAX_PARAMETER_SIZE>())+" KB\n";
		std::cout << "Constant buffer size: "+VarToStr((cl_ulong)device.getInfo<CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE>()/1024)+" KB\n";
		std::cout << "Local memory size: "+VarToStr((cl_ulong)device.getInfo<CL_DEVICE_LOCAL_MEM_SIZE>()/1024)+" KB\n";
		std::cout << "Global memory size: "+VarToStr((cl_ulong)device.getInfo<CL_DEVICE_GLOBAL_MEM_SIZE>()/1024/1024)+" MB\n";
		std::cout << "Max compute units: "+VarToStr((cl_uint)device.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>()) + "\n";
		std::cout << "Max workgroup size: "+VarToStr(max_wg_size) + "\n\n";
	}
	void GenParticles(uint32_t particles)
	{
		queue.enqueueNDRangeKernel(Init_Kernel, cl::NullRange, cl::NDRange(particles));
	}
	void UpdateParticles(uint32_t particles)
	{
		queue.enqueueNDRangeKernel(Update_Kernel, cl::NullRange, cl::NDRange(particles));
	}
	void DrawParticles(uint32_t particles)
	{
		queue.enqueueNDRangeKernel(Draw_Kernel, cl::NullRange, cl::NDRange(particles));
	}
	void FillFragBuff(uint32_t ww, uint32_t wh)
	{
		queue.enqueueNDRangeKernel(FillF_Kernel, cl::NullRange, cl::NDRange(ww, wh));
	}
	void FragsToFrame(uint32_t ww, uint32_t wh)
	{
		queue.enqueueNDRangeKernel(CopyF_Kernel, cl::NullRange, cl::NDRange(ww, wh));
	}
};
