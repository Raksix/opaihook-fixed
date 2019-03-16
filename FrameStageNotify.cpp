#include "hooks.h"
#include "global.h"
#include "Menu.h"
#include "Animfix.h"
#include "BacktrackingHelper.h"
#include "xor.h"
#include <intrin.h>
#include "SpoofedConvar.h"
#include "Math.h"
#include "GameUtils.h"
#include "Items.h"
#include "Aimbot.h"
#include "resolver.h"
#include "Misc.h"

std::vector<const char*> smoke_materials = {
	"particle/vistasmokev1/vistasmokev1_smokegrenade",
				"particle/vistasmokev1/vistasmokev1_emods",
				"particle/vistasmokev1/vistasmokev1_emods_impactdust",
				"particle/vistasmokev1/vistasmokev1_fire",
};

void DrawBeam(Vector src, Vector end, Color color)
{
	BeamInfo_t beamInfo;
	beamInfo.m_nType = TE_BEAMTESLA;
	beamInfo.m_pszModelName = "sprites/purplelaser1.vmt";
	beamInfo.m_nModelIndex = -1;
	beamInfo.m_flHaloScale = 0.0f;
	beamInfo.m_flLife = 3.0f;
	beamInfo.m_flWidth = 2.0f;
	beamInfo.m_flEndWidth = 2.0f;
	beamInfo.m_flFadeLength = 0.0f;
	beamInfo.m_flAmplitude = 2.0f;
	beamInfo.m_flBrightness = 255.f;
	beamInfo.m_flSpeed = 0.2f;
	beamInfo.m_nStartFrame = 0;
	beamInfo.m_flFrameRate = 0.f;
	beamInfo.m_flRed = color.r();
	beamInfo.m_flGreen = color.g();
	beamInfo.m_flBlue = color.b();
	beamInfo.m_nSegments = 2;
	beamInfo.m_bRenderable = true;
	beamInfo.m_vecStart = src;
	beamInfo.m_vecEnd = end;
	Beam_t* myBeam = g_pViewRenderBeams->CreateBeamPoints(beamInfo);
	if (myBeam)
		g_pViewRenderBeams->DrawBeam(myBeam);
}

void __stdcall Hooks::FrameStageNotify(ClientFrameStage_t curStage)
{
	static std::string old_Skyname = "";
	static bool OldNightmode;
	static int OldSky;
	if (!g::LocalPlayer || !g_pEngine->IsConnected() || !g_pEngine->IsInGame())
	{
		clientVMT->GetOriginalMethod<FrameStageNotifyFn>(37)(curStage);
		old_Skyname = "";
		OldNightmode = false;
		OldSky = 0;
		return;
	}
	static ConVar*r_DrawSpecificStaticProp;
	if (OldNightmode != Menu.Visuals.nightmode)
	{
		if (!r_DrawSpecificStaticProp)
			r_DrawSpecificStaticProp = g_pCvar->FindVar("r_DrawSpecificStaticProp");
		r_DrawSpecificStaticProp->SetValue(0);
		for (MaterialHandle_t i = g_pMaterialSystem->FirstMaterial(); i != g_pMaterialSystem->InvalidMaterial(); i = g_pMaterialSystem->NextMaterial(i))
		{
			IMaterial* pMaterial = g_pMaterialSystem->GetMaterial(i);
			if (!pMaterial)
				continue;
			if (strstr(pMaterial->GetTextureGroupName(), "World") || strstr(pMaterial->GetTextureGroupName(), "StaticProp"))
			{
				if (Menu.Visuals.nightmode)
					if (strstr(pMaterial->GetTextureGroupName(), "StaticProp"))
						pMaterial->ColorModulate(0.3f, 0.3f, 0.3f);
					else
						pMaterial->ColorModulate(0.05f, 0.05f, 0.05f);
				else
					pMaterial->ColorModulate(1.0f, 1.0f, 1.0f);
			}
		}
		OldNightmode = Menu.Visuals.nightmode;
	}
	if (OldSky != Menu.Visuals.Skybox)
	{
		auto LoadNamedSky = reinterpret_cast<void(__fastcall*)(const char*)>(FindPatternIDA("engine.dll", "55 8B EC 81 EC ? ? ? ? 56 57 8B F9 C7 45"));
		if (old_Skyname == "")
		{
			//old_Skyname = g_pCvar->FindVar("sv_skyname")->GetName();
		}
		int type = Menu.Visuals.Skybox;
		/*if(type == 0)
		LoadNamedSky(old_Skyname.c_str());
		else */if (type == 1)
			LoadNamedSky("cs_baggage_skybox_");
		else if (type == 2)
			LoadNamedSky("cs_tibet");
		else if (type == 3)
			LoadNamedSky("italy");
		else if (type == 4)
			LoadNamedSky("jungle");
		else if (type == 5)
			LoadNamedSky("office");
		else if (type == 6)
			LoadNamedSky("sky_cs15_daylight02_hdr");
		else if (type == 7)
			LoadNamedSky("sky_csgo_night02");
		else if (type == 8)
			LoadNamedSky("vertigo");
		OldSky = Menu.Visuals.Skybox;
	}
	static Vector oldViewPunch;
	static Vector oldAimPunch;
	Vector* view_punch = g::LocalPlayer->GetViewPunchPtr();
	Vector* aim_punch = g::LocalPlayer->GetPunchAnglePtr();
	if (curStage == FRAME_RENDER_START && g::LocalPlayer->isAlive())
	{
	
		static bool enabledtp = false, check = false;

		if (GetAsyncKeyState(Menu.Misc.TPKey))
		{
			if (!check)
				enabledtp = !enabledtp;
			check = true;
		}
		else
			check = false;

		if (enabledtp)
			*reinterpret_cast<QAngle*>(reinterpret_cast<DWORD>(g::LocalPlayer + 0x31D8)) = g::FakeAngle;


		if (enabledtp)
		{
			*(bool*)((DWORD)g_pInput + 0xA5 + 0x8) = true;
			*(float*)((DWORD)g_pInput + 0xA8 + 0x8 + 0x8) = Menu.Visuals.thirdperson_dist;
		}
		else
		{
			*(bool*)((DWORD)g_pInput + 0xA5 + 0x8) = false;
		}

		if (view_punch && aim_punch && Menu.Visuals.Novisrevoil)
		{
			oldViewPunch = *view_punch;
			oldAimPunch = *aim_punch;
			view_punch->Init();
			aim_punch->Init();
		}
	}
	if (curStage == FRAME_NET_UPDATE_START)
	{
		if (Menu.Visuals.BulletTracers && Menu.Visuals.bulletType == 0)
		{
			float Red, Green, Blue;
			Red = Menu.Colors.Bulletracer[0] * 255;
			Green = Menu.Colors.Bulletracer[1] * 255;
			Blue = Menu.Colors.Bulletracer[2] * 255;
			for (unsigned int i = 0; i < trace_logs.size(); i++) {
				auto *shooter = g_pEntitylist->GetClientEntity(g_pEngine->GetPlayerForUserID(trace_logs[i].userid));
				if (!shooter) 
					return;
				DrawBeam(trace_logs[i].start, trace_logs[i].position, Color(Red, Green, Blue, 255));
				trace_logs.erase(trace_logs.begin() + i);
			}
		}
		for (auto material_name : smoke_materials) {
			static IMaterial* mat = g_pMaterialSystem->FindMaterial(material_name, TEXTURE_GROUP_OTHER);
			mat->SetMaterialVarFlag(MATERIAL_VAR_NO_DRAW, Menu.Visuals.Nosmoke ? true : false);
		}
		if (Menu.Visuals.Nosmoke) {
			static int* smokecount = *(int**)(FindPatternIDA(client_dll, "8B 1D ? ? ? ? 56 33 F6 57 85 DB") + 0x2);
			*smokecount = 0;
		}
		if (Menu.Visuals.NoFlash) {
			static IMaterial* Flash = g_pMaterialSystem->FindMaterial("effects\\flashbang", "ClientEffect textures");
			static IMaterial* FlashWhite = g_pMaterialSystem->FindMaterial("effects\\flashbang_white", "ClientEffect textures");
			Flash->SetMaterialVarFlag(MATERIAL_VAR_NO_DRAW, true);
			FlashWhite->SetMaterialVarFlag(MATERIAL_VAR_NO_DRAW, true);
		}
		
	}
	if (curStage == FRAME_RENDER_START)
	{
		for (int i = 1; i < 65; i++)
		{
			if (i == g_pEngine->GetLocalPlayer())
				continue;
			CBaseEntity* pEntity = g_pEntitylist->GetClientEntity(i);
			if (pEntity)
			{
				if (pEntity->GetHealth() > 0 && !pEntity->IsDormant())
				{
					*(int*)((uintptr_t)pEntity + 0xA30) = g_pGlobals->framecount; //we'll skip occlusion checks now
					*(int*)((uintptr_t)pEntity + 0xA28) = 0;//clear occlusion flags
				}
			}
		}
	}

	c_animfix::get().on_fsn(curStage); //eye candy != fix animations

	if (curStage == FRAME_RENDER_START)
	{
		for (int i = 1; i < 65; i++)
		{
			CBaseEntity* pEntity = g_pEntitylist->GetClientEntity(i);
			if (pEntity && pEntity->IsValidTarget())
			{
				g_BacktrackHelper->UpdateBacktrackRecords(pEntity);
			}
		}
	}

	if (curStage == FRAME_NET_UPDATE_END)
	{
		for (int o = 0; o < 64; ++o) {
			auto pPlayerEntity = g_pEntitylist->GetClientEntity(o);
			if (pPlayerEntity == nullptr || !g::LocalPlayer || !pPlayerEntity->isAlive()
				|| pPlayerEntity->IsDormant() || pPlayerEntity == g::LocalPlayer || pPlayerEntity->GetTeamNum() == g::LocalPlayer->GetTeamNum())
				continue;
			if (pPlayerEntity != g::LocalPlayer)
			{
				auto VarMap = reinterpret_cast<uintptr_t>(pPlayerEntity) + 36;
				auto VarMapSize = *reinterpret_cast<int*>(VarMap + 20);

				for (auto index = 0; index < VarMapSize; index++)
					*reinterpret_cast<uintptr_t*>(*reinterpret_cast<uintptr_t*>(VarMap) + index * 12) = 0;
			}
		}
	}

	clientVMT->GetOriginalMethod<FrameStageNotifyFn>(37)(curStage);
	if (curStage == FRAME_RENDER_START && g::LocalPlayer && g::LocalPlayer->GetHealth() > 0)
	{
		if (Menu.Visuals.Novisrevoil)
		{
			*aim_punch = oldAimPunch;
			*view_punch = oldViewPunch;
		}
	}
	/*if (curStage == FRAME_RENDER_START)
		animation_fix::get().post_render_start();*/
}


