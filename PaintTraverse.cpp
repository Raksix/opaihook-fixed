#include "hooks.h"
#include "GameUtils.h"
#include "Menu.h"
#include "Misc.h"
#include "global.h"
#include "SpoofedConvar.h"
#include "GrenadePrediction.h"
#include "global.h"
#include "AW_hitmarker.h"
#include "event_log.h"
#include <memory>
#include "Math.h"
#include "Antiaim.h"
#include "esp.h"

bool first_frame_passed = false;
std::string sPanel = XorStr("FocusOverlayPanel");

void __fastcall Hooks::PaintTraverse(void* ecx/*thisptr*/, void* edx, unsigned int vgui_panel, bool force_repaint, bool allow_force) // cl
{
	if (Menu.Visuals.EspEnable && Menu.Visuals.Noscope && strcmp("HudZoom", g_pPanel->GetName(vgui_panel)) == 0)
		return;

	panelVMT->GetOriginalMethod<PaintTraverseFn>(41)(ecx, vgui_panel, force_repaint, allow_force);

	static bool bSpoofed = false;

	if (!bSpoofed)
	{
		ConVar* svcheats = g_pCvar->FindVar("sv_cheats");
		SpoofedConvar* svcheatsspoof = new SpoofedConvar(svcheats);
		svcheatsspoof->SetInt(1);
		bSpoofed = true;
	}

	const char* pszPanelName = g_pPanel->GetName(vgui_panel);

	if (!strstr(pszPanelName, sPanel.data()))
		return;

	int cur_height, cur_width; g_pEngine->GetScreenSize(cur_width, cur_height);

	if (!first_frame_passed || cur_width != g::Screen.width || cur_height != g::Screen.height)
	{
		first_frame_passed = true;
		draw::InitFonts();
		g_pEngine->GetScreenSize(cur_width, cur_height);
		g::Screen.height = cur_height;
		g::Screen.width = cur_width;
	}

	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			g::viewMatrix[i][j] = g_pEngine->WorldToScreenMatrix()[i][j];
		}
	}

	if (Menu.Visuals.Hitmarker && g::LocalPlayer && g::LocalPlayer->isAlive()) {
		hitmarker_2->paint();
	}

	static ConVar* PostProcVar = g_pCvar->FindVar("mat_postprocess_enable");
	PostProcVar->SetValue(!Menu.Visuals.RemoveParticles);

	if (g::LocalPlayer) {
		bool enable_force_crosshair = !g::LocalPlayer->IsScoped() && Menu.Visuals.ForceCrosshair;
		static auto weapon_debug = g_pCvar->FindVar("weapon_debug_spread_show");
		weapon_debug->SetValue(enable_force_crosshair ? 3 : 0);
	}

	static float r; r += 0.001f; if (r > 1) r = 0;

	static auto viewmodel_offset_x = g_pCvar->FindVar("viewmodel_offset_x");
	static auto viewmodel_offset_y = g_pCvar->FindVar("viewmodel_offset_y");
	static auto viewmodel_offset_z = g_pCvar->FindVar("viewmodel_offset_z");

	viewmodel_offset_x->m_fnChangeCallbacks.m_Size = 0;
	viewmodel_offset_y->m_fnChangeCallbacks.m_Size = 0;
	viewmodel_offset_z->m_fnChangeCallbacks.m_Size = 0;

	static float old_value_x = viewmodel_offset_x->GetFloat();
	static float old_value_y = viewmodel_offset_y->GetFloat();
	static float old_value_z = viewmodel_offset_z->GetFloat();

	viewmodel_offset_x->SetValue(old_value_x + Menu.Visuals.Viewmodel_X);
	viewmodel_offset_y->SetValue(old_value_y + Menu.Visuals.Viewmodel_Y);
	viewmodel_offset_z->SetValue(old_value_z + Menu.Visuals.Viewmodel_Z);

	visuals::Do();
	draw::DrawPixel(1, 1, Color(0, 0, 0, 0));
	draw::Textf(g::Screen.width - 75, 6, Color::FromHSB(r, 1, 1), F::eventlog, "opaihook fixed");
	grenade_prediction::instance().Paint();
}