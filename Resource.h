#pragma once
#include <string>
#include <mutex>
#include <unordered_map>

#define CAMSPIN_SPEED	0.001
#define CAMMOVE_SPEED	1.0

#define CL_LOGGING		1
#define CL_COMPLOG		1

#define CONFIG_FILE     "settings.cfg"
#define CL_BUILD_LOG    "logs/cl_build.log"

#define WINDOW_TITLE	"Negative Mass Simulator"

#if defined (__APPLE__) || defined(MACOSX)
	#define CL_GL_SHARING_EXT "cl_APPLE_gl_sharing"
#else
	#define CL_GL_SHARING_EXT "cl_khr_gl_sharing"
#endif

namespace GLOBALS {

	extern std::unordered_map<std::string,std::string> config_map;
    extern std::string DATA_FOLDER;
    extern std::mutex COUT_MUTEX;
}
