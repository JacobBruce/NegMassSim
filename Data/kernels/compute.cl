// ------------------------------ //
// ------ TYPE DEFINITIONS ------ //
// ------------------------------ //

#define G 0.0006674
#define ALMOST_TWO 1.999999
#define SPEED_MULT 100.0
#define POS_MOD 10000
#define POS_MIN 10000.0
#define POS_MAX 19999.0
#define MASS_MOD 10000
#define MASS_MIN 1000.0
#define IMP_MAX 24998.5
#define IMP_MIN 5000.5
#define INV_MASS 20000000.0

#pragma pack(push,1)

typedef struct {
	unsigned char blue;
	unsigned char green;
	unsigned char red;
	unsigned char alpha;
} RGB32;

typedef struct {
	union {
		RGB32 colors[4];
		int4 data;
	};
} PFrags;

typedef struct {
	double3 bl_ray;
	double3 cam_pos;
	double3 cam_ori;
	double3 cam_fwd;
	double3 cam_rgt;
	double3 cam_up;
	double2 cam_set;
	float d_time;
	uint span_X;
	uint span_Y;
	uint pixels_X;
	uint pixels_Y;
	uint half_X;
	uint half_Y;
	uint rand_int;
	uint particles;
	short aa_lvl;
	short aa_dim;
	float aa_inc;
	float aa_div;
} RenderInfo;

typedef struct {
	double3 new_pos;
	double3 position;
	double3 velocity;
	float mass;
	float radius;
} Particle;

#pragma pack(pop)

// ------------------------------ //
// ------ VECTOR FUNCTIONS ------ //
// ------------------------------ //

double VectMag(const double3 v)
{
	return sqrt((v.x*v.x) + (v.y*v.y) + (v.z*v.z));
}
double VectSqrd(const double3 v1, const double3 v2)
{
	return pow(v1.x-v2.x, 2) + pow(v1.y-v2.y, 2) + pow(v1.z-v2.z, 2);
}
double VectDist(const double3 v1, const double3 v2)
{
	return sqrt(VectSqrd(v1, v2));
}
double3 VectRotX(const double3 v, const double angle)
{
	return (double3)(v.x, (v.z*sin(angle)) + (v.y*cos(angle)), (v.z*cos(angle)) - (v.y*sin(angle)));
}
double3 VectRotY(const double3 v, const double angle)
{
	return (double3)((v.z*sin(angle)) + (v.x*cos(angle)), v.y, (v.z*cos(angle)) - (v.x*sin(angle)));
}
double3 VectRotZ(const double3 v, const double angle)
{
	return (double3)((v.x*cos(angle)) - (v.y*sin(angle)), (v.x*sin(angle)) + (v.y*cos(angle)), v.z);
}
double3 VectRot(const double3 v, const double3 rot)
{
	double3 result = VectRotY(v, rot.y);
	result = VectRotZ(result, rot.z);
	return VectRotX(result, rot.x);
}

// ------------------------------ //
// ------- MISC FUNCTIONS ------- //
// ------------------------------ //

float4 VectToColor(const float3 v)
{
	return (float4)(v / 255, 1.0f);
}

char is_black(const RGB32 color) {
	if (color.red == 0 && color.green == 0 && color.blue == 0) {
		return 1;
	} else {
		return 0;
	}
}

ulong rand_long(ulong seed) {
	seed ^= seed >> 12;
	seed ^= seed << 25;
	seed ^= seed >> 27;
	return seed * 0x2545F4914F6CDD1D;
}

// ------------------------------ //
// ------ KERNEL FUNCTIONS ------ //
// ------------------------------ //

__kernel void FillFragBuff(__global PFrags* frag_buffer, const RenderInfo render_info)
{
    uint pix_X = get_global_id(0);
	uint pix_Y = get_global_id(1);
	uint pix_index = (pix_Y * render_info.pixels_X) + pix_X;
	PFrags frags;
	
	for (unsigned int r=0; r < render_info.aa_lvl; ++r) {
		frags.colors[r].red = 0;
		frags.colors[r].green = 0;
		frags.colors[r].blue = 0;
	}
	
	frag_buffer[pix_index] = frags;
}

__kernel void GenParticles(__global Particle* prtcl_buffer, const char is_neg, const RenderInfo render_info)
{
    uint prtcl_index = get_global_id(0);
	Particle particle;
	ulong seed;
	
	if (is_neg == 1) {
		seed = prtcl_index + (long)9876543210; //+(long)render_info.rand_int
	} else {
		seed = prtcl_index + (long)1234567890; //+(long)render_info.rand_int
	}

	seed = rand_long(seed);
	particle.position.x = (seed % POS_MOD) + POS_MIN;
	seed = rand_long(seed);
	particle.position.y = (seed % POS_MOD) + POS_MIN;
	seed = rand_long(seed);
	particle.position.z = (seed % POS_MOD) + POS_MIN;
	seed = rand_long(seed);
	particle.velocity.x = 0.0;
	seed = rand_long(seed);
	particle.velocity.y = 0.0;
	seed = rand_long(seed);
	particle.velocity.z = 0.0;
	seed = rand_long(seed);
	particle.mass = (seed % MASS_MOD) + MASS_MIN;
	particle.radius = sqrt(particle.mass / M_PI_F);
	
	if (is_neg == 1) particle.mass = -particle.mass;
	
	prtcl_buffer[prtcl_index] = particle;
}

__kernel void UpdateParticles(__global Particle* pos_buffer, __global Particle* neg_buffer, const RenderInfo render_info)
{
    uint prtcl_index = get_global_id(0);
	Particle pos_prtcl = pos_buffer[prtcl_index];
	Particle neg_prtcl = neg_buffer[prtcl_index];
	double3 force_pos = (double3)(0.0, 0.0, 0.0);
	double3 force_neg = (double3)(0.0, 0.0, 0.0);
	double3 diff_pp, diff_np, diff_pn, diff_nn;
	double force, dist_pp, dist_np, dist_pn, dist_nn;
	Particle pos_p, neg_p;
		
	for (uint i=0; i < render_info.particles; ++i)
	{
		if (prtcl_index == i) {
			/*double3 diff_np = neg_prtcl.position - pos_prtcl.position;
			double3 diff_pn = pos_prtcl.position - neg_prtcl.position;
			double rad_sum = pos_prtcl.radius + neg_prtcl.radius;
			double dist_np = length(diff_np);
			dist_np = (dist_np > rad_sum) ? dist_np : rad_sum;
			double gpn_mass = G * pos_prtcl.mass * neg_prtcl.mass;
			double force = gpn_mass / pow(dist_np, 2.0);
			force_pos += normalize(diff_np) * force;
			force_neg += normalize(diff_pn) * force;*/
		} else {
			pos_p = pos_buffer[i];
			neg_p = neg_buffer[i];
			
			diff_pp = pos_p.position - pos_prtcl.position;
			diff_np = neg_p.position - pos_prtcl.position;
			diff_pn = pos_p.position - neg_prtcl.position;
			diff_nn = neg_p.position - neg_prtcl.position;
			
			dist_pp = length(diff_pp); dist_np = length(diff_np);
			dist_pn = length(diff_pn); dist_nn = length(diff_nn);

			if (dist_pp > (pos_p.radius + pos_prtcl.radius)) {
				force = (G * pos_p.mass * pos_prtcl.mass) / pow(dist_pp, 2.0);
				force_pos.x += (diff_pp.x / dist_pp) * force;
				force_pos.y += (diff_pp.y / dist_pp) * force;
				force_pos.z += (diff_pp.z / dist_pp) * force;
			//} else if (dist_pp > 0.1) {
				//TODO: handle collisions here
			}

			if (dist_np > pos_prtcl.radius) {
				force = (G * neg_p.mass * pos_prtcl.mass) / pow(dist_np, 2.0);
				force_pos.x -= (diff_np.x / dist_np) * force;
				force_pos.y -= (diff_np.y / dist_np) * force;
				force_pos.z -= (diff_np.z / dist_np) * force;
			}

			if (dist_pn > neg_prtcl.radius) {
				force = (G * pos_p.mass * neg_prtcl.mass) / pow(dist_pn, 2.0);
				force_neg.x -= (diff_pn.x / dist_pn) * force;
				force_neg.y -= (diff_pn.y / dist_pn) * force;
				force_neg.z -= (diff_pn.z / dist_pn) * force;		
			}

			if (dist_nn > neg_prtcl.radius) {
				force = (G * neg_p.mass * neg_prtcl.mass) / pow(dist_nn, 2.0);
				force_neg.x += (diff_nn.x / dist_nn) * force;
				force_neg.y += (diff_nn.y / dist_nn) * force;
				force_neg.z += (diff_nn.z / dist_nn) * force;		
			}
		}
	}
	
	double dimx_max_pos = IMP_MAX - pos_prtcl.position.x;
	double dimy_max_pos = IMP_MAX - pos_prtcl.position.y;
	double dimz_max_pos = IMP_MAX - pos_prtcl.position.z;
	
	double dimx_min_pos = pos_prtcl.position.x - IMP_MIN;
	double dimy_min_pos = pos_prtcl.position.y - IMP_MIN;
	double dimz_min_pos = pos_prtcl.position.z - IMP_MIN;
	
	/*double dimx_max_neg = IMP_MAX - neg_prtcl.position.x;
	double dimy_max_neg = IMP_MAX - neg_prtcl.position.y;
	double dimz_max_neg = IMP_MAX - neg_prtcl.position.z;
	
	double dimx_min_neg = neg_prtcl.position.x - IMP_MIN;
	double dimy_min_neg = neg_prtcl.position.y - IMP_MIN;
	double dimz_min_neg = neg_prtcl.position.z - IMP_MIN;*/
	
	double gm_pos = G * INV_MASS * pos_prtcl.mass;
	//double gm_neg = G * INV_MASS * neg_prtcl.mass;
	
	force_pos.x += gm_pos / pow(dimx_max_pos, 2.0);
	force_pos.y += gm_pos / pow(dimy_max_pos, 2.0);
	force_pos.z += gm_pos / pow(dimz_max_pos, 2.0);

	force_pos.x -= gm_pos / pow(dimx_min_pos, 2.0);
	force_pos.y -= gm_pos / pow(dimy_min_pos, 2.0);
	force_pos.z -= gm_pos / pow(dimz_min_pos, 2.0);

	/*force_neg.x -= gm_neg / pow(dimx_max_neg, 2.0);
	force_neg.y -= gm_neg / pow(dimy_max_neg, 2.0);
	force_neg.z -= gm_neg / pow(dimz_max_neg, 2.0);
	
	force_neg.x += gm_neg / pow(dimx_min_neg, 2.0);
	force_neg.y += gm_neg / pow(dimy_min_neg, 2.0);
	force_neg.z += gm_neg / pow(dimz_min_neg, 2.0);*/
	
	pos_prtcl.velocity += force_pos / pos_prtcl.mass;
	neg_prtcl.velocity += force_neg / neg_prtcl.mass;
	
	pos_prtcl.new_pos = pos_prtcl.position + (pos_prtcl.velocity * (/*render_info.d_time */ SPEED_MULT));
	neg_prtcl.new_pos = neg_prtcl.position + (neg_prtcl.velocity * (/*render_info.d_time */ SPEED_MULT));
	
	pos_prtcl.new_pos.x -= (pos_prtcl.new_pos.x > POS_MAX) ? POS_MOD : 0.0;
	pos_prtcl.new_pos.y -= (pos_prtcl.new_pos.y > POS_MAX) ? POS_MOD : 0.0;
	pos_prtcl.new_pos.z -= (pos_prtcl.new_pos.z > POS_MAX) ? POS_MOD : 0.0;
		
	pos_prtcl.new_pos.x += (pos_prtcl.new_pos.x < POS_MIN) ? POS_MOD : 0.0;
	pos_prtcl.new_pos.y += (pos_prtcl.new_pos.y < POS_MIN) ? POS_MOD : 0.0;
	pos_prtcl.new_pos.z += (pos_prtcl.new_pos.z < POS_MIN) ? POS_MOD : 0.0;
		
	neg_prtcl.new_pos.x -= (neg_prtcl.new_pos.x > POS_MAX) ? POS_MOD : 0.0;
	neg_prtcl.new_pos.y -= (neg_prtcl.new_pos.y > POS_MAX) ? POS_MOD : 0.0;
	neg_prtcl.new_pos.z -= (neg_prtcl.new_pos.z > POS_MAX) ? POS_MOD : 0.0;;
		
	neg_prtcl.new_pos.x += (neg_prtcl.new_pos.x < POS_MIN) ? POS_MOD : 0.0;
	neg_prtcl.new_pos.y += (neg_prtcl.new_pos.y < POS_MIN) ? POS_MOD : 0.0;
	neg_prtcl.new_pos.z += (neg_prtcl.new_pos.z < POS_MIN) ? POS_MOD : 0.0;
		
	pos_buffer[prtcl_index] = pos_prtcl;
	neg_buffer[prtcl_index] = neg_prtcl;
}

__kernel void DrawParticles(__global Particle* prtcl_buffer,
__global PFrags* frag_buffer, const RGB32 color, const RenderInfo render_info)
{
    uint prtcl_index = get_global_id(0);
	Particle prtcl = prtcl_buffer[prtcl_index];
	prtcl_buffer[prtcl_index].position = prtcl.new_pos;
	
	PFrags tmp_frags;
	int frag_index, pix_index;
	int2 pixel_coords, ifrag_coords;
	float2 ffrag_coords, screen_coords;
	
	double3 pprc = VectRot(prtcl.new_pos - render_info.cam_pos, render_info.cam_ori);
	double p_dist = length(pprc);
	
	if (pprc.z > 0.0f) {
		screen_coords.x = render_info.half_X + (pprc.x / pprc.z) * render_info.cam_set.x;
		screen_coords.y = render_info.half_Y + (pprc.y / pprc.z) * render_info.cam_set.x;
		float p_prad = (render_info.cam_set.x / p_dist) * prtcl.radius;
		float p_right = screen_coords.x+p_prad;
		float p_left = screen_coords.x-p_prad;
		float p_top = screen_coords.y+p_prad;
		float p_bot = screen_coords.y-p_prad;
		
		if (p_right > 0.0f && p_left < render_info.pixels_X
		&& p_top > 0.0f && p_bot < render_info.pixels_Y) {
			if (p_prad < 0.5f) {
				pixel_coords.x = screen_coords.x;
				pixel_coords.y = screen_coords.y;
				if (pixel_coords.x < render_info.pixels_X
				&& pixel_coords.y < render_info.pixels_Y) {
					ffrag_coords.x = screen_coords.x - pixel_coords.x;
					ffrag_coords.y = screen_coords.y - pixel_coords.y;
					ifrag_coords.x = ffrag_coords.x * ALMOST_TWO;
					ifrag_coords.y = ffrag_coords.y * ALMOST_TWO;
					pix_index = (pixel_coords.y * render_info.pixels_X) + pixel_coords.x;
					frag_index = (ifrag_coords.y * 2) + ifrag_coords.x;
					tmp_frags = frag_buffer[pix_index];
					if (is_black(tmp_frags.colors[frag_index]) == 1)
						frag_buffer[pix_index].colors[frag_index] = color;
				}
			} else {
				for (float y = p_bot; y < p_top; y += 0.5f) {
					pixel_coords.y = y;
					if (pixel_coords.y >= render_info.pixels_Y) { break; }
					for (float x = p_left; x < p_right; x += 0.5f) {
						pixel_coords.x = x;
						if (pixel_coords.x >= render_info.pixels_X) { break; }
						if (sqrt(((x-screen_coords.x)*(x-screen_coords.x)
						+(y-screen_coords.y)*(y-screen_coords.y))) < p_prad)
						{	
							ffrag_coords.x = x - pixel_coords.x;
							ffrag_coords.y = y - pixel_coords.y;
							ifrag_coords.x = ffrag_coords.x * ALMOST_TWO;
							ifrag_coords.y = ffrag_coords.y * ALMOST_TWO;	
							pix_index = (pixel_coords.y * render_info.pixels_X) + pixel_coords.x;
							frag_index = (ifrag_coords.y * 2) + ifrag_coords.x;
							tmp_frags = frag_buffer[pix_index];
							if (is_black(tmp_frags.colors[frag_index]) == 1)
								frag_buffer[pix_index].colors[frag_index] = color;
						}
					}
				}
			}
		}
	}
}

__kernel void FragsToFrame(__global PFrags* frag_buffer,
write_only image2d_t pix_buffer, const RenderInfo render_info)
{
    uint pix_X = get_global_id(0);
	uint pix_Y = get_global_id(1);
	uint pix_index = (pix_Y * render_info.pixels_X) + pix_X;
	
	float3 sumColor = (float3)(0.0f,0.0f,0.0f);
	PFrags frags = frag_buffer[pix_index];
		
	for (unsigned int r=0; r < render_info.aa_lvl; ++r) {
		sumColor.x += frags.colors[r].red;
		sumColor.y += frags.colors[r].green;
		sumColor.z += frags.colors[r].blue;
	}
	
	write_imagef(pix_buffer, (int2)(pix_X, pix_Y), VectToColor(sumColor * render_info.aa_div));
}
