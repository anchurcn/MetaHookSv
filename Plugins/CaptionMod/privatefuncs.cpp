#include <metahook.h>
#include <capstone.h>
#include "plugins.h"
#include "exportfuncs.h"
#include "privatefuncs.h"

#define S_FINDNAME_SIG_SVENGINE "\x53\x55\x8B\x6C\x24\x0C\x56\x33\xF6\x57\x85\xED\x75\x2A\x68"
#define S_STARTDYNAMICSOUND_SIG_SVENGINE "\x83\xEC\x2A\xA1\x2A\x2A\x2A\x2A\x33\xC4\x89\x44\x24\x54\x8B\x44\x24\x5C\x55"
#define S_STARTSTATICSOUND_SIG_SVENGINE "\x83\xEC\x2A\xA1\x2A\x2A\x2A\x2A\x33\xC4\x89\x44\x24\x48\x57\x8B\x7C\x24\x5C"
#define S_LOADSOUND_SIG_SVENGINE "\x81\xEC\x2A\x2A\x00\x00\xA1\x2A\x2A\x2A\x2A\x33\xC4\x89\x84\x24\x2A\x2A\x00\x00\x8B\x8C\x24\x2A\x2A\x00\x00\x56\x8B\xB4\x24\x2A\x2A\x00\x00\x8A\x06\x3C\x2A"

#define S_INIT_SIG_NEW "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x08\x85\xC0"
#define S_FINDNAME_SIG_NEW "\x55\x8B\xEC\x53\x56\x8B\x75\x08\x33\xDB\x85\xF6"
#define S_STARTDYNAMICSOUND_SIG_NEW "\x55\x8B\xEC\x83\xEC\x48\xA1\x2A\x2A\x2A\x2A\x53\x56\x57\x85\xC0\xC7\x45\xFC\x00\x00\x00\x00"
#define S_STARTSTATICSOUND_SIG_NEW "\x55\x8B\xEC\x83\xEC\x44\x53\x56\x57\x8B\x7D\x10\x85\xFF\xC7\x45\xFC\x00\x00\x00\x00"
#define S_LOADSOUND_SIG_NEW "\x55\x8B\xEC\x81\xEC\x44\x05\x00\x00\x53\x56\x8B\x75\x08"
#define S_LOADSOUND_8308_SIG "\x55\x8B\xEC\x81\xEC\x28\x05\x00\x00\x53\x8B\x5D\x08"

#define S_INIT_SIG "\x83\xEC\x08\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x08\x85\xC0"
#define S_FINDNAME_SIG "\x53\x55\x8B\x6C\x24\x0C\x33\xDB\x56\x57\x85\xED"
#define S_STARTDYNAMICSOUND_SIG "\x83\xEC\x48\xA1\x2A\x2A\x2A\x2A\x53\x55\x56\x85\xC0\x57\xC7\x44\x24\x10\x00\x00\x00\x00"
#define S_STARTSTATICSOUND_SIG "\x83\xEC\x44\x53\x55\x8B\x6C\x24\x58\x56\x85\xED\x57"
#define S_LOADSOUND_SIG "\x81\xEC\x2A\x2A\x00\x00\x53\x8B\x9C\x24\x2A\x2A\x00\x00\x55\x56\x8A\x03\x57"

double *cl_time;
double *cl_oldtime;

char m_szCurrentLanguage[128] = { 0 };

private_funcs_t gPrivateFuncs = {0};

void Engine_FillAddress(void)
{
	if (g_iEngineType == ENGINE_SVENGINE)
	{
		gPrivateFuncs.S_Init = (void(*)(void))Search_Pattern(S_INIT_SIG_NEW);
		Sig_FuncNotFound(S_Init); 
		
		gPrivateFuncs.S_FindName = (sfx_t *(*)(char *, int *))Search_Pattern(S_FINDNAME_SIG_SVENGINE);
		Sig_FuncNotFound(S_FindName);

		gPrivateFuncs.S_StartDynamicSound = (void(*)(int, int, sfx_t *, float *, float, float, int, int))Search_Pattern(S_STARTDYNAMICSOUND_SIG_SVENGINE);
		Sig_FuncNotFound(S_StartDynamicSound);

		gPrivateFuncs.S_StartStaticSound = (void(*)(int, int, sfx_t *, float *, float, float, int, int))Search_Pattern(S_STARTSTATICSOUND_SIG_SVENGINE);
		Sig_FuncNotFound(S_StartStaticSound);

		gPrivateFuncs.S_LoadSound = (sfxcache_t *(*)(sfx_t *, channel_t *))Search_Pattern(S_LOADSOUND_SIG_SVENGINE);
		Sig_FuncNotFound(S_LoadSound);
	}
	else if(g_dwEngineBuildnum >= 5953)
	{
		gPrivateFuncs.S_Init = (void (*)(void))Search_Pattern(S_INIT_SIG_NEW);
		Sig_FuncNotFound(S_Init);

		gPrivateFuncs.S_FindName = (sfx_t *(*)(char *, int *))Search_Pattern_From(gPrivateFuncs.S_Init, S_FINDNAME_SIG_NEW);
		Sig_FuncNotFound(S_FindName);

		gPrivateFuncs.S_StartDynamicSound = (void (*)(int, int, sfx_t *, float *, float, float, int, int))Search_Pattern_From(gPrivateFuncs.S_FindName, S_STARTDYNAMICSOUND_SIG_NEW);
		Sig_FuncNotFound(S_StartDynamicSound);

		gPrivateFuncs.S_StartStaticSound = (void (*)(int, int, sfx_t *, float *, float, float, int, int))Search_Pattern_From(gPrivateFuncs.S_StartDynamicSound, S_STARTSTATICSOUND_SIG_NEW);
		Sig_FuncNotFound(S_StartStaticSound);

		gPrivateFuncs.S_LoadSound = (sfxcache_t *(*)(sfx_t *, channel_t *))Search_Pattern(S_LOADSOUND_SIG_NEW);
		if(!gPrivateFuncs.S_LoadSound)
			gPrivateFuncs.S_LoadSound = (sfxcache_t *(*)(sfx_t *, channel_t *))Search_Pattern( S_LOADSOUND_8308_SIG);
		Sig_FuncNotFound(S_LoadSound);
	}
	else
	{
		gPrivateFuncs.S_Init = (void (*)(void))Search_Pattern(S_INIT_SIG);
		Sig_FuncNotFound(S_Init);

		gPrivateFuncs.S_FindName = (sfx_t *(*)(char *, int *))Search_Pattern_From(gPrivateFuncs.S_Init, S_FINDNAME_SIG);
		Sig_FuncNotFound(S_FindName);

		gPrivateFuncs.S_StartDynamicSound = (void (*)(int, int, sfx_t *, float *, float, float, int, int))Search_Pattern_From(S_FindName, S_STARTDYNAMICSOUND_SIG);
		Sig_FuncNotFound(S_StartDynamicSound);

		gPrivateFuncs.S_StartStaticSound = (void (*)(int, int, sfx_t *, float *, float, float, int, int))Search_Pattern_From(S_StartDynamicSound, S_STARTSTATICSOUND_SIG);
		Sig_FuncNotFound(S_StartStaticSound);

		gPrivateFuncs.S_LoadSound = (sfxcache_t *(*)(sfx_t *, channel_t *))Search_Pattern_From(S_StartStaticSound, S_LOADSOUND_SIG);
		Sig_FuncNotFound(S_LoadSound);
	}

	PUCHAR SearchBegin = (PUCHAR)g_dwEngineTextBase;
	PUCHAR SearchEnd = SearchBegin + g_dwEngineTextSize;
	while (1)
	{
#define LANGUAGESTRNCPY_SIG "\x68\x80\x00\x00\x00\x50\x8D"
		PUCHAR LanguageStrncpy = (PUCHAR)g_pMetaHookAPI->SearchPattern(SearchBegin, SearchEnd - SearchBegin, LANGUAGESTRNCPY_SIG, sizeof(LANGUAGESTRNCPY_SIG) - 1);
		if (LanguageStrncpy)
		{
			typedef struct
			{
				bool bHasPushEax;
			}LanguageStrncpy_ctx;

			LanguageStrncpy_ctx ctx = { 0 };

			g_pMetaHookAPI->DisasmRanges(LanguageStrncpy, 0x30, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
			{
				auto ctx = (LanguageStrncpy_ctx *)context;
				auto pinst = (cs_insn *)inst;

				if (pinst->id == X86_INS_PUSH &&
					pinst->detail->x86.op_count == 1 &&
					pinst->detail->x86.operands[0].type == X86_OP_REG &&
					pinst->detail->x86.operands[0].reg == X86_REG_EAX)
				{
					ctx->bHasPushEax = true;
				}

				if (ctx->bHasPushEax)
				{
					if (address[0] == 0xE8)
					{
						gPrivateFuncs.V_strncpy = (decltype(gPrivateFuncs.V_strncpy))GetCallAddress(address);
						PUCHAR pfnNewV_strncpy = (PUCHAR)NewV_strncpy;
						int rva = pfnNewV_strncpy - (address + 5);
						g_pMetaHookAPI->WriteMemory(address + 1, (BYTE *)&rva, 4);
						return TRUE;
					}
					else if (address[0] == 0xEB)
					{
						char jmprva = *(char *)(address + 1);
						PUCHAR jmptarget = address + 2 + jmprva;

						if (jmptarget[0] == 0xE8)
						{
							gPrivateFuncs.V_strncpy = (decltype(gPrivateFuncs.V_strncpy))GetCallAddress(jmptarget);
							PUCHAR pfnNewV_strncpy = (PUCHAR)NewV_strncpy;
							int rva = pfnNewV_strncpy - (jmptarget + 5);
							g_pMetaHookAPI->WriteMemory(jmptarget + 1, (BYTE *)&rva, 4);
							return TRUE;
						}
					}
				}

				if (instCount > 5)
					return TRUE;

				if (address[0] == 0xCC)
					return TRUE;

				if (pinst->id == X86_INS_RET)
					return TRUE;

				return FALSE;
			}, 0, &ctx);

			SearchBegin = LanguageStrncpy + sizeof(LANGUAGESTRNCPY_SIG) - 1;
		}
		else
		{
			break;
		}
	}
}

void Engine_InstallHook(void)
{
	Install_InlineHook(S_FindName);
	Install_InlineHook(S_StartDynamicSound);
	Install_InlineHook(S_StartStaticSound);
}

void Client_FillAddress(void)
{
	auto pfnClientFactory = g_pMetaHookAPI->GetClientFactory();

	if (pfnClientFactory && pfnClientFactory("SCClientDLL001", 0))
	{
		g_IsSCClient = true;

#define SC_FINDSOUND_SIG "\x51\x55\x8B\x6C\x24\x0C\x89\x4C\x24\x04\x85\xED\x0F\x84\x2A\x2A\x2A\x2A\x80\x7D\x00\x00"
		{
			gPrivateFuncs.ScClient_FindSoundEx = (decltype(gPrivateFuncs.ScClient_FindSoundEx))
				g_pMetaHookAPI->SearchPattern(g_dwClientBase, g_dwClientSize, SC_FINDSOUND_SIG, Sig_Length(SC_FINDSOUND_SIG));

			Sig_FuncNotFound(ScClient_FindSoundEx);
			Install_InlineHook(ScClient_FindSoundEx);
		}

#define SC_GETCLIENTCOLOR_SIG "\x8B\x4C\x24\x04\x85\xC9\x2A\x2A\x6B\xC1\x58"
		{
			gPrivateFuncs.GetClientColor = (decltype(gPrivateFuncs.GetClientColor))
				g_pMetaHookAPI->SearchPattern(g_dwClientBase, g_dwClientSize, SC_GETCLIENTCOLOR_SIG, Sig_Length(SC_GETCLIENTCOLOR_SIG));

			Sig_FuncNotFound(GetClientColor);
		}

#define SC_VIEWPORT_SIG "\x8B\x0D\x2A\x2A\x2A\x2A\x85\xC9\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x84\xC0\x0F"
		{
			DWORD addr = (DWORD)g_pMetaHookAPI->SearchPattern(g_dwClientBase, g_dwClientSize, SC_VIEWPORT_SIG, Sig_Length(SC_VIEWPORT_SIG));

			Sig_AddrNotFound(GameViewport);

			GameViewport = *(decltype(GameViewport) *)(addr + 2);
			gPrivateFuncs.GameViewport_AllowedToPrintText = (decltype(gPrivateFuncs.GameViewport_AllowedToPrintText))GetCallAddress(addr + 10);
		}

#define SC_UPDATECURSORSTATE_SIG "\x8B\x40\x28\xFF\xD0\x84\xC0\x2A\x2A\xC7\x05\x2A\x2A\x2A\x2A\x01\x00\x00\x00"
		{
			DWORD addr = (DWORD)g_pMetaHookAPI->SearchPattern(g_dwClientBase, g_dwClientSize, SC_UPDATECURSORSTATE_SIG, Sig_Length(SC_UPDATECURSORSTATE_SIG));
			Sig_AddrNotFound(g_iVisibleMouse);
			g_iVisibleMouse = *(decltype(g_iVisibleMouse) *)(addr + 11);
		}
	}

	if (!g_iVisibleMouse &&
		(PUCHAR)gExportfuncs.IN_Accumulate > (PUCHAR)g_dwClientBase &&
		(PUCHAR)gExportfuncs.IN_Accumulate < (PUCHAR)g_dwClientBase + g_dwClientSize)
	{
		typedef struct
		{
			DWORD candidate;
			int candidate_register;
		}IN_Accumulate_ctx;

		IN_Accumulate_ctx ctx = { 0 };

		g_pMetaHookAPI->DisasmRanges(gExportfuncs.IN_Accumulate, 0x30, [](void *inst, PUCHAR address, size_t instLen, int instCount, int depth, PVOID context)
		{
			auto ctx = (IN_Accumulate_ctx *)context;
			auto pinst = (cs_insn *)inst;

			if (pinst->id == X86_INS_MOV &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].type == X86_OP_MEM &&
				pinst->detail->x86.operands[1].mem.base == 0 &&
				pinst->detail->x86.operands[1].mem.index == 0 &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp > (PUCHAR)g_dwClientBase &&
				(PUCHAR)pinst->detail->x86.operands[1].mem.disp < (PUCHAR)g_dwClientBase + g_dwClientSize)
			{
				ctx->candidate = pinst->detail->x86.operands[1].mem.disp;
				ctx->candidate_register = pinst->detail->x86.operands[0].reg;
			}

			if (ctx->candidate_register &&
				pinst->id == X86_INS_TEST &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_REG &&
				pinst->detail->x86.operands[0].reg == ctx->candidate_register &&
				pinst->detail->x86.operands[1].type == X86_OP_REG &&
				pinst->detail->x86.operands[1].reg == ctx->candidate_register)
			{
				g_iVisibleMouse = (decltype(g_iVisibleMouse))ctx->candidate;
			}

			if (pinst->id == X86_INS_CMP &&
				pinst->detail->x86.op_count == 2 &&
				pinst->detail->x86.operands[0].type == X86_OP_MEM &&
				pinst->detail->x86.operands[0].mem.base == 0 &&
				pinst->detail->x86.operands[0].mem.index == 0 &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp > (PUCHAR)g_dwClientBase &&
				(PUCHAR)pinst->detail->x86.operands[0].mem.disp < (PUCHAR)g_dwClientBase + g_dwClientSize &&
				pinst->detail->x86.operands[1].type == X86_OP_IMM &&
				pinst->detail->x86.operands[1].imm == 0)
			{
				g_iVisibleMouse = (decltype(g_iVisibleMouse))pinst->detail->x86.operands[0].mem.disp;
			}

			if (g_iVisibleMouse)
				return TRUE;

			if (address[0] == 0xCC)
				return TRUE;

			if (pinst->id == X86_INS_RET)
				return TRUE;

			return FALSE;
		}, 0, &ctx);

		Sig_VarNotFound(g_iVisibleMouse);
	}
}