#pragma once
#include "OpenCL.h"

#pragma pack(push,1)

struct cl_RGB32
{
	cl_uchar CL_ALIGNED(4) s[4];
}; // 4 bytes

struct cl_Material
{
	cl_float3 ambient;
	cl_float3 diffuse;
	cl_float3 specular;
	cl_float shininess;
	cl_float glossiness;
	cl_float reflectivity;
	cl_float transparency;
}; // 64 bytes

struct cl_AAInfo
{
	cl_short lvl;
	cl_short dim;
	cl_float inc;
	cl_float div;
}; // 16 bytes

struct cl_CamInfo {
	cl_double3 bl_ray;
	cl_double3 cam_pos;
	cl_double3 cam_ori;
	cl_double3 cam_fwd;
	cl_double3 cam_rgt;
	cl_double3 cam_up;
	cl_double cam_foc;
	cl_double cam_apt;
}; // 208 bytes

struct cl_RenderInfo
{
	cl_CamInfo cam_info;
	cl_float d_time;
	cl_uint span_X;
	cl_uint span_Y;
	cl_uint pixels_X;
	cl_uint pixels_Y;
	cl_uint half_X;
	cl_uint half_Y;
	cl_uint rand_int;
	cl_uint particles;
	cl_AAInfo aa_info;
}; // 256 bytes

typedef struct {
	cl_double3 new_pos;
	cl_double3 position;
	cl_double3 velocity;
	cl_float mass;
	cl_float radius;
} cl_Particle; // 104 bytes

#pragma pack(pop)
