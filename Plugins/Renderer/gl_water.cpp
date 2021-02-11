#include "gl_local.h"

//renderer
qboolean drawreflect;
qboolean drawrefract;
vec3_t water_view;

//water
r_water_t waters[MAX_WATERS];
r_water_t *curwater;
r_water_t *waters_free;
r_water_t *waters_active;

//shader
SHADER_DEFINE(water);

int water_normalmap;

SHADER_DEFINE(drawdepth);

int save_userfogon;
int *g_bUserFogOn;

int saved_cl_waterlevel;

//cvar
cvar_t *r_water = NULL;
cvar_t *r_water_debug = NULL;
cvar_t *r_water_fresnel = NULL;
cvar_t *r_water_depthfactor = NULL;
cvar_t *r_water_normfactor = NULL;
cvar_t *r_water_novis = NULL;
cvar_t *r_water_texscale = NULL;
cvar_t *r_water_minheight = NULL;

void RotatePointAroundVector(vec3_t dst, const vec3_t dir, const vec3_t point, float degrees);

void R_FreeWater(void)
{
	for (int i = 0; i < MAX_WATERS; ++i)
	{
		if (waters[i].depthreflmap)
		{
			GL_DeleteTexture(waters[i].depthreflmap);
			waters[i].depthreflmap = 0;
		}
		if (waters[i].depthrefrmap)
		{
			GL_DeleteTexture(waters[i].depthrefrmap);
			waters[i].depthrefrmap = 0;
		}
		if (waters[i].reflectmap)
		{
			GL_DeleteTexture(waters[i].reflectmap);
			waters[i].reflectmap = 0;
		}
		if (waters[i].refractmap)
		{
			GL_DeleteTexture(waters[i].refractmap);
			waters[i].refractmap = 0;
		}
		waters[i].reflectmap_ready = false;
		waters[i].refractmap_ready = false;
	}
}

void R_ClearWater(void)
{
	for(int i = 0; i < MAX_WATERS - 1; ++i)
		waters[i].next = &waters[i+1];
	waters[MAX_WATERS-1].next = NULL;
	waters_free = &waters[0];
	waters_active = NULL;
}

const char *drawdepth_vscode = 
"varying vec4 projpos;"
"void main(void)"
"{"
"	projpos = gl_ModelViewProjectionMatrix * gl_Vertex;"
"	gl_Position = ftransform();"
"}";

const char *drawdepth_fscode = 
"uniform sampler2D depthmap;"
"varying vec4 projpos;"
"void main(void)"
"{"
"	vec2 vBaseTexCoord = projpos.xy / projpos.w * 0.5 + 0.5;"
"	vec4 vDepthColor = texture2D(depthmap, vBaseTexCoord);"
"	gl_FragColor = vec4(vec3(pow(vDepthColor.z, 50.0)), 1.0);"
"}";

void R_InitWater(void)
{
	if(gl_shader_support)
	{
		const char *water_vscode = (const char *)gEngfuncs.COM_LoadFile("resource\\shader\\water_shader.vsh", 5, 0);
		const char *water_fscode = (const char *)gEngfuncs.COM_LoadFile("resource\\shader\\water_shader.fsh", 5, 0);
		if(water_vscode && water_fscode)
		{
			water.program = R_CompileShader(water_vscode, water_fscode, "water_shader.vsh", "water_shader.fsh");
			if(water.program)
			{
				SHADER_UNIFORM(water, waterfogcolor, "waterfogcolor");
				SHADER_UNIFORM(water, eyepos, "eyepos");
				SHADER_UNIFORM(water, eyedir, "eyedir");
				SHADER_UNIFORM(water, time, "time");
				SHADER_UNIFORM(water, fresnel, "fresnel");
				SHADER_UNIFORM(water, depthfactor, "depthfactor");
				SHADER_UNIFORM(water, normfactor, "normfactor");
				SHADER_UNIFORM(water, abovewater, "abovewater");
				
				SHADER_UNIFORM(water, normalmap, "normalmap");
				SHADER_UNIFORM(water, refractmap, "refractmap");
				SHADER_UNIFORM(water, reflectmap, "reflectmap");
				SHADER_UNIFORM(water, depthrefrmap, "depthrefrmap");
			}
		}	
		if (!water_vscode)
		{
			Sys_ErrorEx("shader file \"resource\\shader\\water_shader.vsh\" not found!");
		}
		if (!water_fscode)
		{
			Sys_ErrorEx("shader file \"resource\\shader\\water_shader.fsh\" not found!");
		}

		gEngfuncs.COM_FreeFile((void *)water_vscode);
		gEngfuncs.COM_FreeFile((void *)water_fscode);

		if(drawdepth_vscode && drawdepth_fscode)
		{
			drawdepth.program = R_CompileShader(drawdepth_vscode, drawdepth_fscode, "drawdepth_shader.vsh", "drawdepth_shader.fsh");
			if(drawdepth.program)
			{
				SHADER_UNIFORM(drawdepth, depthmap, "depthmap");
			}
		}
	}

	r_water = gEngfuncs.pfnRegisterVariable("r_water", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_water_debug = gEngfuncs.pfnRegisterVariable("r_water_debug", "0", FCVAR_CLIENTDLL);
	r_water_fresnel = gEngfuncs.pfnRegisterVariable("r_water_fresnel", "1.5", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_water_depthfactor = gEngfuncs.pfnRegisterVariable("r_water_depthfactor", "50", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_water_normfactor = gEngfuncs.pfnRegisterVariable("r_water_normfactor", "1.5", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_water_novis = gEngfuncs.pfnRegisterVariable("r_water_novis", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_water_texscale = gEngfuncs.pfnRegisterVariable("r_water_texscale", "0.5", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);
	r_water_minheight = gEngfuncs.pfnRegisterVariable("r_water_minheight", "7.5", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);

	curwater = NULL;
	drawreflect = false;
	drawrefract = false;

	R_ClearWater();
}

int R_ShouldReflect(void) 
{
	//The camera is above water?
	if(curwater->vecs[2] > r_refdef->vieworg[2] || *cl_waterlevel >= 3)
		return 0;

	return 1;
}

void R_AddEntityWater(cl_entity_t *ent, vec3_t p, colorVec *color)
{
	r_water_t *w;

	curwater = NULL;

	for (w = waters_active; w; w = w->next)
	{
		if ((ent != r_worldentity && w->ent == ent) || (fabs(w->vecs[2] - p[2]) < 1.0f) )
		{
			//found one
			VectorCopy(p, w->vecs);
			curwater = w;
			curwater->free = false;
			curwater->framecount = (*r_framecount);
			return;
		}
	}

	//no free water slot
	if (!waters_free)
	{
		gEngfuncs.Con_Printf("R_AddEntityWater: MAX_WATER exceeded!");
		return;
	}

	//not found, try to create
	curwater = waters_free;
	waters_free = curwater->next;

	curwater->next = waters_active;
	waters_active = curwater;

	int water_texture_width = glwidth;
	int water_texture_height = glheight;

	int desired_height = glheight * clamp(r_water_texscale->value, 0.1, 2.0);
	while ((water_texture_height >> 1) >= desired_height)
	{
		water_texture_width >>= 1;
		water_texture_height >>= 1;
	}

	if (!water_normalmap)
		water_normalmap = R_LoadTexture("resource\\tga\\water_normalmap.tga", "resource\\tga\\water_normalmap.tga", NULL, NULL, GLT_SYSTEM);

	if (!curwater->reflectmap)
	{
		curwater->reflectmap = R_GLGenTextureRGBA8(water_texture_width, water_texture_height);
	}
	else if (curwater->texwidth != water_texture_width)
	{
		R_GLUploadTextureColorFormat(curwater->reflectmap, water_texture_width, water_texture_height, GL_RGBA8);
	}

	if (!curwater->refractmap)
	{
		curwater->refractmap = R_GLGenTextureRGBA8(water_texture_width, water_texture_height);
	}
	else if (curwater->texwidth != water_texture_width)
	{
		R_GLUploadTextureColorFormat(curwater->refractmap, water_texture_width, water_texture_height, GL_RGBA8);
	}

	if (!curwater->depthrefrmap)
	{
		curwater->depthrefrmap = R_GLGenDepthTexture(water_texture_width, water_texture_height);
	}
	else if (curwater->texwidth != water_texture_width)
	{
		R_GLUploadDepthTexture(curwater->depthrefrmap, water_texture_width, water_texture_height);
	}

	if (!curwater->depthreflmap)
	{
		curwater->depthreflmap = R_GLGenDepthTexture(water_texture_width, water_texture_height);
	}
	else if (curwater->texwidth != water_texture_width)
	{
		R_GLUploadDepthTexture(curwater->depthreflmap, water_texture_width, water_texture_height);
	}

	curwater->texwidth = water_texture_width;
	curwater->texheight = water_texture_height;

	curwater->reflectmap_ready = false;
	curwater->refractmap_ready = false;

	VectorCopy(p, curwater->vecs);
	curwater->ent = ent;
	curwater->org[0] = (ent->curstate.mins[0] + ent->curstate.maxs[0]) / 2;
	curwater->org[1] = (ent->curstate.mins[1] + ent->curstate.maxs[1]) / 2;
	curwater->org[2] = (ent->curstate.mins[2] + ent->curstate.maxs[2]) / 2;
	memcpy(&curwater->color, color, sizeof(*color));
	curwater->is3dsky = (draw3dsky) ? true : false;
	curwater->free = false;
	curwater->framecount = (*r_framecount);
	curwater = NULL;
}

void R_EnableClip(qboolean isdrawworld)
{
	double clipPlane[] = {0, 0, 0, 0};

	if(drawreflect)
	{
		if(saved_cl_waterlevel > 2)
		{
			clipPlane[2] = -1.0;
			clipPlane[3] = curwater->vecs[2];
		}
		else
		{
			clipPlane[2] = 1.0;
			clipPlane[3] = -curwater->vecs[2];
		}
	}
	if(drawrefract)
	{
		return;

		if (saved_cl_waterlevel > 2)
		{
			return;
			//clipPlane[2] = 1.0;
			//clipPlane[3] = curwater->vecs[2]; 
		}
		else
		{
			clipPlane[2] = -1.0;
			clipPlane[3] = curwater->vecs[2];
		}
	}

	//bugfix
	/*if(isdrawworld && drawrefract && saved_cl_waterlevel <= 2)
	{
		clipPlane[3] -= 16.05f;
	}*/

	qglEnable(GL_CLIP_PLANE0);
	qglClipPlane(GL_CLIP_PLANE0, clipPlane);
}

void R_RenderReflectView(void)
{
	if (s_BackBufferFBO.s_hBackBufferFBO)
	{
		qglBindFramebufferEXT(GL_FRAMEBUFFER, s_BackBufferFBO.s_hBackBufferFBO);
		qglFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, curwater->reflectmap, 0);
		qglFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, curwater->depthreflmap, 0);
	}

	qglClearColor(curwater->color.r / 255.0f, curwater->color.g / 255.0f, curwater->color.b / 255.0f, 1);
	qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	R_PushRefDef();

	if(curwater->is3dsky)
	{
		VectorCopy(_3dsky_view, water_view);
	}
	else
	{
		VectorCopy(r_refdef->vieworg, water_view);
	}

	VectorCopy(water_view, r_refdef->vieworg);

	r_refdef->vieworg[2] = (2 * curwater->vecs[2]) - r_refdef->vieworg[2];
	r_refdef->viewangles[0] = -r_refdef->viewangles[0];
	r_refdef->viewangles[2] = -r_refdef->viewangles[2];

	drawreflect = true;

	//int saved_framecount = *r_framecount;
	//mleaf_t *saved_oldviewleaf = *r_oldviewleaf;
	//*r_oldviewleaf = NULL;
	saved_cl_waterlevel = *cl_waterlevel;
	*cl_waterlevel = 0;
	auto saved_r_drawentities = r_drawentities->value;
	if (r_water->value >= 2)
	{
		r_drawentities->value = 1;
	}
	else
	{
		r_drawentities->value = 0;
	}

	R_RenderScene();

	r_drawentities->value = saved_r_drawentities;
	*cl_waterlevel = saved_cl_waterlevel;
	//*r_oldviewleaf = saved_oldviewleaf;
	//*r_framecount = saved_framecount;

	qglDisable(GL_CLIP_PLANE0);

	if (!s_BackBufferFBO.s_hBackBufferFBO)
	{
		qglBindTexture(GL_TEXTURE_2D, curwater->reflectmap);
		qglCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, curwater->texwidth, curwater->texheight, 0);
	}
	else
	{
		qglBindFramebufferEXT(GL_FRAMEBUFFER, s_BackBufferFBO.s_hBackBufferFBO);
		qglFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s_BackBufferFBO.s_hBackBufferTex, 0);
		qglFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, s_BackBufferFBO.s_hBackBufferDepthTex, 0);
	}

	R_PopRefDef();

	drawreflect = false;

	curwater->reflectmap_ready = true;
}

void R_RenderRefractView(void)
{
	if (s_BackBufferFBO.s_hBackBufferFBO)
	{
		qglBindFramebufferEXT(GL_FRAMEBUFFER, s_BackBufferFBO.s_hBackBufferFBO);
		qglFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, curwater->refractmap, 0);
		qglFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, curwater->depthrefrmap, 0);
	}

	qglClearColor(curwater->color.r / 255.0f, curwater->color.g / 255.0f, curwater->color.b / 255.0f, 1);
	qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	R_PushRefDef();

	if(curwater->is3dsky)
	{
		VectorCopy(_3dsky_view, water_view);
	}
	else
	{
		VectorCopy(r_refdef->vieworg, water_view);
	}

	VectorCopy(water_view, r_refdef->vieworg);

	drawrefract = true;

	//int saved_framecount = *r_framecount;
	//mleaf_t *saved_oldviewleaf = *r_oldviewleaf;
	//*r_oldviewleaf = NULL;
	saved_cl_waterlevel = *cl_waterlevel;
	//*cl_waterlevel = 0;
	auto saved_r_drawentities = r_drawentities->value;
	if (r_water->value >= 2)
	{
		r_drawentities->value = 1;
	}
	else
	{
		r_drawentities->value = 0;
	}

	R_RenderScene();

	r_drawentities->value = saved_r_drawentities;
	*cl_waterlevel = saved_cl_waterlevel;
	//*r_oldviewleaf = saved_oldviewleaf;
	//*r_framecount = saved_framecount;

	qglDisable(GL_CLIP_PLANE0);

	if (!s_BackBufferFBO.s_hBackBufferFBO)
	{
		qglBindTexture(GL_TEXTURE_2D, curwater->refractmap);
		qglCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, curwater->texwidth, curwater->texheight, 0);

		qglBindTexture(GL_TEXTURE_2D, curwater->depthrefrmap);
		qglCopyTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 0, 0, curwater->texwidth, curwater->texheight, 0);
	}
	else
	{
		qglBindFramebufferEXT(GL_FRAMEBUFFER, s_BackBufferFBO.s_hBackBufferFBO);
		qglFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s_BackBufferFBO.s_hBackBufferTex, 0);
		qglFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, s_BackBufferFBO.s_hBackBufferDepthTex, 0);
	}

	R_PopRefDef();

	drawrefract = false;

	curwater->refractmap_ready = true;
}

void R_FreeDeadWaters(void)
{
	r_water_t *kill;
	r_water_t *p;

	for (;; )
	{
		kill = waters_active;

		if (kill && kill->free)
		{
			waters_active = kill->next;
			kill->next = waters_free;
			waters_free = kill;
			continue;
		}

		break;
	}

	for (p = waters_active; p; p = p->next)
	{
		for (;; )
		{
			kill = p->next;
			if (kill && kill->free)
			{
				p->next = kill->next;
				kill->next = waters_free;
				waters_free = kill;
				continue;
			}
			break;
		}
	}
}

void R_RenderWaterView(void)
{
	R_PushFrameBuffer();

	for(r_water_t *w = waters_active; w; w = w->next)
	{
		curwater = w;

		if(R_ShouldReflect())
		{
			R_RenderReflectView();
		}

		R_RenderRefractView();

		curwater = NULL;
	}

	R_PopFrameBuffer();
}