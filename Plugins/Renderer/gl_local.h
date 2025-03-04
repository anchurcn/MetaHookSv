#pragma once

#include <metahook.h>
#include <math.h>
#include <assert.h>
#include <mathlib.h>
#include <archtypes.h>
#include <const.h>
#include <custom.h>
#include <ref_params.h>
#include <cvardef.h>
#include <studio.h>
#include <r_studioint.h>
#include <pm_movevars.h>
#include <pm_shared.h>
#include <particledef.h>
#include <triangleapi.h>
#include <entity_types.h>

#include "plugins.h"
#include "exportfuncs.h"
#include "qgl.h"
#include "ref_int_internal.h"

#include "gl_profile.h"
#include "gl_shader.h"
#include "gl_model.h"
#include "gl_water.h"
#include "gl_sprite.h"
#include "gl_studio.h"
#include "gl_hud.h"
#include "gl_shadow.h"
#include "gl_light.h"
#include "gl_wsurf.h"
#include "gl_draw.h"
#include "gl_cvar.h"

typedef struct
{
	GLboolean cullface;
	GLboolean alphatest;
	GLboolean depthtest;
	GLboolean depthmask;
	GLboolean blend;
	int blendsrc;
	int blenddst;
	qboolean mtex;
}gl_draw_context;

typedef struct refdef_s
{
	vrect_t *vrect;
	vec3_t *vieworg;
	vec3_t *viewangles;
	color24 *ambientlight;
	qboolean *onlyClientDraws;
}refdef_t;

extern refdef_t r_refdef;
extern refdef_GoldSrc_t *r_refdef_GoldSrc;
extern refdef_SvEngine_t *r_refdef_SvEngine;
extern ref_params_t r_params;
extern float *scrfov;
extern float r_xfov;
extern float r_yfov;
extern float r_screenaspect;
extern cl_entity_t *r_worldentity;
extern model_t *r_worldmodel;
extern model_t *r_playermodel;
extern int *cl_numvisedicts;
extern cl_entity_t **cl_visedicts;
extern cl_entity_t **currententity;
extern int *maxTransObjs;
extern int *numTransObjs;
extern transObjRef **transObjects;
extern mleaf_t **r_viewleaf;
extern mleaf_t **r_oldviewleaf;
extern int *r_loading_skybox;

extern RECT *window_rect;

extern float *videowindowaspect;
extern float *windowvideoaspect;

extern vec_t *vup;
extern vec_t *vpn;
extern vec_t *vright;
extern vec_t *r_origin;
extern vec_t *modelorg;
extern vec_t *r_entorigin;
extern float *r_world_matrix;
extern float *r_projection_matrix;
extern float *gWorldToScreen;
extern float *gScreenToWorld;
extern overviewInfo_t *gDevOverview;
extern mplane_t *frustum;

extern int *r_framecount;
extern int *r_visframecount;

extern void *cl_frames;
extern int size_of_frame;
extern int *cl_parsecount;
extern int *cl_waterlevel;
extern int *envmap;
extern int *cl_stats;
extern double *cl_time;
extern double *cl_oldtime;
extern float *cl_weaponstarttime;
extern int *cl_weaponsequence;
extern int *cl_light_level;
extern int *c_alias_polys;
extern int *c_brush_polys;
extern int(*rtable)[20][20];
extern void *tmp_palette;

//fog
extern int *g_bUserFogOn;
extern float *g_UserFogColor;
extern float *g_UserFogDensity;
extern float *g_UserFogStart;
extern float *g_UserFogEnd;

extern model_t *mod_known;
extern int *mod_numknown;

//gl extension

extern int gl_max_texture_size;
extern float gl_max_ansio;
extern int *gl_msaa_fbo;
extern int *gl_backbuffer_fbo;
extern int *gl_mtexable;

extern qboolean *mtexenabled;

extern int glx;
extern int gly;
extern int glwidth;
extern int glheight;

extern bool bNoStretchAspect;
extern bool bUseBindless;
extern bool bUseOITBlend;

extern FBO_Container_t s_GBufferFBO;
extern FBO_Container_t s_BackBufferFBO;
extern FBO_Container_t s_BackBufferFBO2;
extern FBO_Container_t s_BlendBufferFBO;
extern FBO_Container_t s_DownSampleFBO[DOWNSAMPLE_BUFFERS];
extern FBO_Container_t s_LuminFBO[LUMIN_BUFFERS];
extern FBO_Container_t s_Lumin1x1FBO[LUMIN1x1_BUFFERS];
extern FBO_Container_t s_BrightPassFBO;
extern FBO_Container_t s_BlurPassFBO[BLUR_BUFFERS][2];
extern FBO_Container_t s_BrightAccumFBO;
extern FBO_Container_t s_ToneMapFBO;
extern FBO_Container_t s_DepthLinearFBO;
extern FBO_Container_t s_HBAOCalcFBO;
extern FBO_Container_t s_ShadowFBO;
extern FBO_Container_t s_WaterFBO;

extern msurface_t **skychain;
extern msurface_t **waterchain;

extern int *gSkyTexNumber;

extern float gldepthmin;
extern float gldepthmax;

extern cvar_t *dev_overview_color;

extern cvar_t *r_bmodelinterp;
extern cvar_t *r_bmodelhighfrac;
extern cvar_t *r_norefresh;
extern cvar_t *r_drawentities;
extern cvar_t *r_drawviewmodel;
extern cvar_t *r_speeds;
extern cvar_t *r_fullbright;
extern cvar_t *r_decals;
extern cvar_t *r_lightmap;
extern cvar_t *r_shadows;
extern cvar_t *r_mirroralpha;
extern cvar_t *r_wateralpha;
extern cvar_t *r_dynamic;
extern cvar_t *r_mmx;
extern cvar_t *r_traceglow;
extern cvar_t *r_wadtextures;
extern cvar_t *r_glowshellfreq;
extern cvar_t *r_detailtextures;

extern cvar_t *gl_vsync;
extern cvar_t *gl_ztrick;
extern cvar_t *gl_finish;
extern cvar_t *gl_clear;
extern cvar_t *gl_clearcolor;
extern cvar_t *gl_cull;
extern cvar_t *gl_texsort;
extern cvar_t *gl_smoothmodels;
extern cvar_t *gl_affinemodels;
extern cvar_t *gl_flashblend;
extern cvar_t *gl_playermip;
extern cvar_t *gl_nocolors;
extern cvar_t *gl_keeptjunctions;
extern cvar_t *gl_reporttjunctions;
extern cvar_t *gl_wateramp;
extern cvar_t *gl_dither;
extern cvar_t *gl_spriteblend;
extern cvar_t *gl_polyoffset;
extern cvar_t *gl_lightholes;
extern cvar_t *gl_zmax;
extern cvar_t *gl_alphamin;
extern cvar_t *gl_overdraw;
extern cvar_t *gl_watersides;
extern cvar_t *gl_overbright;
extern cvar_t *gl_envmapsize;
extern cvar_t *gl_flipmatrix;
extern cvar_t *gl_monolights;
extern cvar_t *gl_fog;
extern cvar_t *gl_wireframe;
extern cvar_t *gl_ansio;
extern cvar_t *developer;
extern cvar_t *gl_round_down;
extern cvar_t *gl_picmip;
extern cvar_t *gl_max_size;

extern cvar_t *v_texgamma;
extern cvar_t *v_lightgamma;
extern cvar_t *v_brightness;
extern cvar_t *v_gamma;
extern cvar_t *v_lambert;

extern cvar_t *cl_righthand;
extern cvar_t *chase_active;

void R_FillAddress(void);
void R_InstallHook(void);

void GammaToLinear(float *color);
void R_LoadSkyName_SvEngine(const char *name);
void R_LoadSkys(void);
void Mod_LoadStudioModel(model_t *mod, void *buffer);
void BuildGammaTable(float g);
void R_RenderView(void);
void R_RenderScene(void);
void R_RenderView_SvEngine(int a1);
qboolean R_CullBox(vec3_t mins, vec3_t maxs);
void R_ForceCVars(qboolean mp);
void R_NewMap(void);
void R_Init(void);
void R_VidInit(void);
void R_Shutdown(void);
void R_InitTextures(void);
void R_FreeTextures(void);
void R_SetupGL(void);
void R_MarkLeaves(void);
void R_DrawWorld(void);
void R_DrawSkyBox(void);
mleaf_t *Mod_PointInLeaf(vec3_t p, model_t *model);
void R_RecursiveWorldNode(mnode_t *node);
void R_RecursiveWorldNodeVBO(mnode_t *node);
void R_DrawParticles(void);
void R_RotateForEntity(float *origin, cl_entity_t *ent);
void R_SetRenderMode(cl_entity_t *pEntity);
float *R_GetAttachmentPoint(int entity, int attachment);
void R_DrawBrushModel(cl_entity_t *entity);
void R_DrawSpriteModel(cl_entity_t *entity);
entity_state_t *R_GetPlayerState(int index);
bool CL_IsDevOverviewMode(void);
int CL_FxBlend(cl_entity_t *entity);
void R_DrawCurrentEntity(bool bTransparent);
void R_DrawTEntitiesOnList(int onlyClientDraw);
void R_AddTEntity(cl_entity_t *pEnt);
void GL_Shutdown(void);
void GL_Init(void);
void GL_BeginRendering(int *x, int *y, int *width, int *height);
void GL_EndRendering(void);
GLuint GL_GenTexture(void);
GLuint GL_GenBuffer(void);
void GL_DeleteTexture(GLuint tex);
void GL_DeleteBuffer(GLuint buf);
void GL_Bind(int texnum);
void GL_SelectTexture(GLenum target);
void GL_DisableMultitexture(void);
void GL_EnableMultitexture(void);
void triapi_RenderMode(int mode);
int GL_LoadTexture(char *identifier, GL_TEXTURETYPE textureType, int width, int height, byte *data, qboolean mipmap, int iType, byte *pPal);
int GL_LoadTexture2(char *identifier, GL_TEXTURETYPE textureType, int width, int height, byte *data, qboolean mipmap, int iType, byte *pPal, int filter);
void GL_InitShaders(void);
void GL_FreeShaders(void);
texture_t *Draw_DecalTexture(int index);
void Draw_MiptexTexture(cachewad_t *wad, byte *data);
void EmitWaterPolys(msurface_t *fa, int direction);
void R_DecalShootInternal(texture_t *ptexture, int index, int entity, int modelIndex, vec3_t position, int flags, float flScale);
void __fastcall enginesurface_drawFlushText(void *pthis, int dummy);
float CalcFov(float fov_x, float width, float height);
int SignbitsForPlane(mplane_t *out);
qboolean R_ParseVectorCvar(cvar_t *a1, float *vec);
void R_ForceCVars(qboolean mp);
colorVec R_LightPoint(vec3_t p);
void *R_GetRefDef(void);
int R_GetDrawPass(void);
GLuint GL_GenTextureRGBA8(int w, int h);

void GL_UploadDepthTexture(int texid, int w, int h);
GLuint GL_GenDepthTexture(int w, int h);

void GL_UploadDepthStencilTexture(int texid, int w, int h);
GLuint GL_GenDepthStencilTexture(int w, int h);

GLuint GL_GenTextureColorFormat(int w, int h, int iInternalFormat);
void GL_UploadTextureColorFormat(int texid, int w, int h, int iInternalFormat);

GLuint GL_GenTextureArrayColorFormat(int w, int h, int levels, int iInternalFormat);
void GL_UploadTextureArrayColorFormat(int texid, int w, int h, int levels, int iInternalFormat);

GLuint GL_GenShadowTexture(int w, int h);
void GL_UploadShadowTexture(int texid, int w, int h);

void GL_GenFrameBuffer(FBO_Container_t *s);
void GL_FrameBufferColorTexture(FBO_Container_t *s, GLuint iInternalFormat);
void GL_FrameBufferDepthTexture(FBO_Container_t *s, GLuint iInternalFormat);
void GL_FrameBufferColorTextureHBAO(FBO_Container_t *s);
void GL_FrameBufferColorTextureDeferred(FBO_Container_t *s, int iInternalColorFormat);
void GL_FrameBufferColorTextureOITBlend(FBO_Container_t *s);
int GL_LoadTextureEx(const char *identifier, GL_TEXTURETYPE textureType, int width, int height, void *data, qboolean mipmap, qboolean ansio);
int R_LoadTextureEx(const char *filepath, const char *name, int *width, int *height, GL_TEXTURETYPE type, qboolean mipmap, qboolean ansio);

void GL_UploadDXT(void *data, int width, int height, qboolean mipmap, qboolean ansio, int wrap);
int LoadDDS(const char *filename, byte *buf, int bufSize, int *width, int *height);
int LoadImageGeneric(const char *filename, byte *buf, int bufSize, int *width, int *height);
int SaveImageGeneric(const char *filename, int width, int height, byte *data);

cubemap_t *R_FindCubemap(float *origin);
void R_LoadCubemap(cubemap_t *cubemap);
void R_BuildCubemaps_f(void);

void R_SaveProgramStates_f(void);
void R_LoadProgramStates_f(void);

void COM_FileBase(const char *in, char *out);

//framebuffer
void GL_PushFrameBuffer(void);
void GL_PopFrameBuffer(void);

//refdef
void R_PushRefDef(void);
void R_UpdateRefDef(void);
void R_PopRefDef(void);
int R_GetDrawPass(void);

void GL_FreeTexture(gltexture_t *glt);
void GL_PushMatrix(void);
void GL_PopMatrix(void);

void GL_PushDrawState(void);
void GL_PopDrawState(void);

void GL_Begin2D(void);
void GL_Begin2DEx(int width, int height);
void GL_End2D(void);

void GL_BeginFullScreenQuad(bool enableDepthTest);
void GL_EndFullScreenQuad(void);

void GL_Texturemode_f(void);
void GL_Texturemode_cb(cvar_t *);

int EngineGetMaxKnownModel(void);
int EngineGetModelIndex(model_t *mod);
model_t *EngineGetModelByIndex(int index);

void DLL_SetModKey(void *pinfo, char *pkey, char *pvalue);

extern GLint r_viewport[4];
extern float r_identity_matrix[4][4];
extern float r_entity_matrix[4][4];
extern float r_entity_color[4];
extern bool r_draw_shadowcaster;
extern bool r_draw_opaque;
extern bool r_draw_oitblend;
extern bool r_draw_legacysprite;

extern bool g_SvEngine_DrawPortalView;
extern int r_draw_pass;

#define BUFFER_OFFSET(i) ((unsigned int *)NULL + (i))