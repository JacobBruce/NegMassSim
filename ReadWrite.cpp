#include "ReadWrite.h"

bool DirExists(std::string dirname)
{
    struct stat st;
    if (stat(dirname.c_str(), &st) == 0) {
        if (S_ISDIR(st.st_mode)) { return true; }
    }
    return false;
}

bool CreateDir(std::string dirname)
{
    return (mkdir(dirname.c_str()) == 0) ? true : false;
}

bool FileExists(const std::string& filename)
{
    return access(filename.c_str(), 0) == 0;
}

long long FileSize(std::string& filename)
{
    struct stat64 stat_buf;
    int rc = stat64(filename.c_str(), &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
}

void PrintLine(std::string str)
{
    GLOBALS::COUT_MUTEX.lock();
    std::cout << str << std::endl;
    std::cout.flush();
    GLOBALS::COUT_MUTEX.unlock();
}

void PrintLine(std::stringstream& str_stream)
{
    GLOBALS::COUT_MUTEX.lock();
    std::cout << str_stream.str() << std::endl;
    std::cout.flush();
    str_stream.str("");
    str_stream.clear();
    GLOBALS::COUT_MUTEX.unlock();
}

void HandleFatalError(const int ecode, std::string emsg)
{
	std::cout << "FATAL ERROR (" << ecode << "): " << emsg << std::endl;
	exit(EXIT_FAILURE);
}

std::string ReadFileStr(const std::string filename)
{
	std::ifstream sourceFile(filename);
    std::string sourceCode(std::istreambuf_iterator<char>(sourceFile),
                          (std::istreambuf_iterator<char>()));
	return sourceCode;
}

bool LoadConfigFile(const std::string filename)
{
	std::ifstream config_file(filename);
	std::string line, key, data;
	size_t bpos;

	if (config_file.is_open()) {

		while (!config_file.eof()) {
			getline(config_file, line);
			bpos = line.find("=");
			if (bpos == std::string::npos) continue;
			key = line.substr(0, bpos);
			data = line.substr(bpos+1);
			GLOBALS::config_map[key] = data;
		}

		config_file.close();
		return true;
	}

	return false;
}

void CLBLog(const std::string logstr)
{
	if (CL_COMPLOG) {
		std::ofstream clfile(GLOBALS::DATA_FOLDER+CL_BUILD_LOG);
		if (clfile.is_open()) {
			clfile << logstr.c_str() << "\n";
			clfile.close();
		}
	}
}
