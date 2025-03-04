#pragma once

typedef struct light_dynamic_s
{
	int type;
	vec3_t origin;
	float color[3];
	float distance;
	float ambient;
	float diffuse;
	float specular;
	float specularpow;
}light_dynamic_t;

extern std::vector<light_dynamic_t> g_DynamicLights;

extern cvar_t *r_light_dynamic;
extern cvar_t *r_light_debug;
extern MapConVar *r_flashlight_ambient;
extern MapConVar *r_flashlight_diffuse;
extern MapConVar *r_flashlight_specular;
extern MapConVar *r_flashlight_specularpow;
extern MapConVar *r_dynlight_ambient;
extern MapConVar *r_dynlight_diffuse;
extern MapConVar *r_dynlight_specular;
extern MapConVar *r_dynlight_specularpow;

extern cvar_t *r_ssr;
extern MapConVar *r_ssr_ray_step;
extern MapConVar *r_ssr_iter_count;
extern MapConVar *r_ssr_distance_bias;
extern MapConVar *r_ssr_exponential_step;
extern MapConVar *r_ssr_adaptive_step;
extern MapConVar *r_ssr_binary_search;
extern MapConVar *r_ssr_fade;

extern bool drawgbuffer;

typedef struct
{
	int program;
	int u_lightdir;
	int u_lightpos;
	int u_lightcolor;
	int u_lightcone;
	int u_lightradius;
	int u_lightambient;
	int u_lightdiffuse;
	int u_lightspecular;
	int u_lightspecularpow;
	int u_modelmatrix;
}dlight_program_t;

typedef struct
{
	int program;
	int u_ssrRayStep;
	int u_ssrIterCount;
	int u_ssrDistanceBias;
	int u_ssrFade;
}dfinal_program_t;

void R_InitLight(void);
void R_ShutdownLight(void);
bool R_BeginRenderGBuffer(void);
void R_EndRenderGBuffer(void);
void R_SetGBufferMask(int mask);
void R_SetGBufferBlend(int blendsrc, int blenddst);
void R_SaveDLightProgramStates(void);
void R_SaveDFinalProgramStates(void);
void R_LoadDLightProgramStates(void);
void R_LoadDFinalProgramStates(void);

#define GBUFFER_INDEX_DIFFUSE		0
#define GBUFFER_INDEX_LIGHTMAP		1
#define GBUFFER_INDEX_WORLDNORM		2
#define GBUFFER_INDEX_SPECULAR		3
#define GBUFFER_INDEX_ADDITIVE		4
#define GBUFFER_INDEX_MAX			5

#define GBUFFER_MASK_DIFFUSE		(1<<GBUFFER_INDEX_DIFFUSE)
#define GBUFFER_MASK_LIGHTMAP		(1<<GBUFFER_INDEX_LIGHTMAP)
#define GBUFFER_MASK_WORLDNORM		(1<<GBUFFER_INDEX_WORLDNORM)
#define GBUFFER_MASK_SPECULAR		(1<<GBUFFER_INDEX_SPECULAR)
#define GBUFFER_MASK_ADDITIVE		(1<<GBUFFER_INDEX_ADDITIVE)

#define GBUFFER_MASK_ALL			(GBUFFER_MASK_DIFFUSE | GBUFFER_MASK_LIGHTMAP | GBUFFER_MASK_WORLDNORM | GBUFFER_MASK_SPECULAR | GBUFFER_MASK_ADDITIVE)

#define DLIGHT_SPOT_ENABLED			1
#define DLIGHT_POINT_ENABLED		2
#define DLIGHT_VOLUME_ENABLED		4

#define DFINAL_LINEAR_FOG_ENABLED				1
#define DFINAL_EXP2_FOG_ENABLED					2
#define DFINAL_SSR_ENABLED						4
#define DFINAL_SSR_ADAPTIVE_STEP_ENABLED		8
#define DFINAL_SSR_EXPONENTIAL_STEP_ENABLED		0x10
#define DFINAL_SSR_BINARY_SEARCH_ENABLED		0x20

#define DLIGHT_POINT					0
#define DLIGHT_SPOT						1