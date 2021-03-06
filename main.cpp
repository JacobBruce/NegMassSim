#include "Game.h"

std::unordered_map<std::string,std::string> GLOBALS::config_map;
std::string GLOBALS::DATA_FOLDER;
std::mutex GLOBALS::COUT_MUTEX;

KeyboardServer GLFW::kServ;
MouseServer GLFW::mServ;

int main(int argc, char *argv[])
{
    if (argc > 1) {
        GLOBALS::DATA_FOLDER.assign(argv[1]);
    } else {
        if (argc == 0) {
            GLOBALS::DATA_FOLDER.assign(__FILE__);
        } else {
            GLOBALS::DATA_FOLDER.assign(argv[0]);
        }

        size_t found = GLOBALS::DATA_FOLDER.find_last_of("/\\");
        GLOBALS::DATA_FOLDER = GLOBALS::DATA_FOLDER.substr(0, found+1);
    }

	std::cout << "Loading config file... ";
	if (LoadConfigFile(GLOBALS::DATA_FOLDER+CONFIG_FILE)) {
        std::cout << "Success!\n";
	} else {
		std::cout << "Failed!\n";
		PrintLine("Failed!\nFile: "+GLOBALS::DATA_FOLDER+CONFIG_FILE);
		exit(EXIT_FAILURE);
	}

	glfwSetErrorCallback(GLFW::error_callback);
	std::cout << "Initializing GLFW ... ";

	if (glfwInit() == GLFW_TRUE) {
		std::cout << "Success!\n";
	} else {
		std::cout << "Failed!\n";
		exit(EXIT_FAILURE);
	}

	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_SAMPLES, 0);
	glfwWindowHint(GLFW_DEPTH_BITS, 0);
	glfwWindowHint(GLFW_STENCIL_BITS, 0);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	int windowWidth = stoi(GLOBALS::config_map["WINDOW_WIDTH"]);
	int windowHeight = stoi(GLOBALS::config_map["WINDOW_HEIGHT"]);
	int windowMode = stoi(GLOBALS::config_map["WINDOW_MODE"]);

	GLFWmonitor* monitor;
	GLFWwindow* window;

	if (GLOBALS::config_map["MONITOR_INDEX"] == "primary") {
		monitor = glfwGetPrimaryMonitor();
	} else {
		int monitorCount;
		int monitorIndex = stoi(GLOBALS::config_map["MONITOR_INDEX"]);
		GLFWmonitor** monitors = glfwGetMonitors(&monitorCount);
		if (monitorIndex > 0 && monitorIndex < monitorCount) {
			monitor = monitors[monitorIndex];
		} else {
			std::cout << "Error: Invalid monitor index (check settings)\n";
			exit(EXIT_FAILURE);
		}
	}

	const GLFWvidmode* vidMode = glfwGetVideoMode(monitor);

	std::cout << "Creating OpenGL window ... ";
	switch (windowMode) {
	case 1:
		window = glfwCreateWindow(windowWidth, windowHeight, WINDOW_TITLE, monitor, NULL);
		break;
	case 2:
		glfwWindowHint(GLFW_RED_BITS, vidMode->redBits);
		glfwWindowHint(GLFW_GREEN_BITS, vidMode->greenBits);
		glfwWindowHint(GLFW_BLUE_BITS, vidMode->blueBits);
		glfwWindowHint(GLFW_REFRESH_RATE, vidMode->refreshRate);
		window = glfwCreateWindow(vidMode->width, vidMode->height, WINDOW_TITLE, monitor, NULL);
		break;
	default:
		window = glfwCreateWindow(windowWidth, windowHeight, WINDOW_TITLE, NULL, NULL);
		if (windowWidth < vidMode->width && windowHeight < vidMode->height) {
			int x_diff = vidMode->width - windowWidth;
			int y_diff = vidMode->height - windowHeight;
			glfwSetWindowPos(window, x_diff/2.0f, y_diff/2.0f);
		}
		break;
	}

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

    if (!window) {
		std::cout << "Failed!\n";
		GLFW::error_exit();
    } else {
		std::cout << "Success!\n";
	}

	std::cout << "Initializing GLEW ... ";
	GLenum err = glewInit();

	if (err != GLEW_OK) {
		std::cout << "Failed!\nError: " << glewGetErrorString(err);
		GLFW::error_exit(window);
	} else {
		std::cout << "Success!\n";
	}

	//TODO: apply window icon
	//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	//GLFWimage images[2];
	//images[0] = load_icon("__TemplateIcon.png");
	//images[1] = load_icon("32x32.ico");
	//glfwSetWindowIcon(window, 2, images);

	Game theGame(window, GLFW::kServ, GLFW::mServ);

	glfwSetKeyCallback(window, GLFW::key_callback);
	glfwSetCursorPosCallback(window, GLFW::cursor_position_callback);
	glfwSetCursorEnterCallback(window, GLFW::cursor_enter_callback);
	glfwSetMouseButtonCallback(window, GLFW::mouse_button_callback);
	glfwSetScrollCallback(window, GLFW::scroll_callback);

    while (!glfwWindowShouldClose(window))
    {
        theGame.Go();
        glfwPollEvents();
    }

	std::cout << "Stopping engine ..." << std::endl;
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}
