#pragma once
#include <cstdlib>
#include <string>
#include <cmath>
#include <cfloat>
#include <sstream>

#define HALF_PI 1.570796326794896
#define PI		3.141592653589793
#define TWO_PI	6.283185307179586

struct float2 {
	float x;
	float y;
};

struct float3 {
	float x;
	float y;
	float z;
};

struct float4 {
	float x;
	float y;
	float z;
	float w;
};

struct UInt2 {
	uint32_t x;
	uint32_t y;
};

struct UInt3 {
	uint32_t x;
	uint32_t y;
	uint32_t z;
};

struct UInt4 {
	uint32_t x;
	uint32_t y;
	uint32_t z;
	uint32_t w;
};

inline std::wstring StrToWstr(std::string str)
{
	return std::wstring(str.begin(), str.end());
}

template<typename T>
inline void NormAngle(T& angle)
{
	while (angle < 0.0) { angle += TWO_PI; }
	while (angle > TWO_PI) { angle -= TWO_PI; }
}

template<typename T>
inline void SwapVars(T& x1, T& x2)
{
	T temp = x1;
	x1 = x2;
	x2 = temp;
}

template<typename T>
inline std::string VarToStr(const T& var)
{
	std::ostringstream s;
	s << var;
	return s.str();
}

template<typename T>
inline std::wstring VarToWstr(const T& var)
{
	return StrToWstr(VarToStr(var));
}

inline UInt2 StrToUInt2(std::string line)
{
	size_t bpos;
	std::string data;
	UInt2 result;
	bpos = line.find(",");
	result.x = stoi(line.substr(0, bpos));
	result.y = stoi(line.substr(bpos+1));
	return result;
}

inline UInt3 StrToUInt3(std::string line)
{
	size_t bpos;
	std::string data;
	UInt3 result;
	bpos = line.find(",");
	result.x = stoi(line.substr(0, bpos));
	data = line.substr(bpos+1);
	bpos = data.find(",");
	result.y = stoi(data.substr(0, bpos));
	result.z = stoi(data.substr(bpos+1));
	return result;
}

inline float2 StrToFlt2(std::string line)
{
	size_t bpos;
	float2 result;
	bpos = line.find(",");
	result.x = stof(line.substr(0, bpos));
	result.y = stof(line.substr(bpos+1));
	return result;
}

inline float3 StrToFlt3(std::string line)
{
	size_t bpos;
	std::string data;
	float3 result;
	bpos = line.find(",");
	result.x = stof(line.substr(0, bpos));
	data = line.substr(bpos+1);
	bpos = data.find(",");
	result.y = stof(data.substr(0, bpos));
	result.z = stof(data.substr(bpos+1));
	return result;
}
