#include "hooks.h"
#include "global.h"
#include "Menu.h"
#include "sdk.h"
#include "GrenadePrediction.h"

ClientVariables Menu;

void __fastcall Hooks::OverrideView(void* _this, void* _edx, CViewSetup* setup)
{
	if (g_pEngine->IsInGame() && g_pEngine->IsConnected())
	{
		if (g::LocalPlayer && g::LocalPlayer->GetHealth() > 0)
		{
			if (!g::LocalPlayer->IsScoped())
			{
				setup->fov = 90 + Menu.Misc.PlayerFOV;
			}
			else
			{
				if (Menu.Misc.static_scope)
					setup->fov = 90 + Menu.Misc.PlayerFOV;
				else {

				}
			}

			if (Menu.Antiaim.AntiaimEnable && GetAsyncKeyState('Z'))
				setup->origin.z = g::LocalPlayer->GetAbsOrigin().z + 64.f;
		}
	}
	grenade_prediction::instance().View(setup);

	clientmodeVMT->GetOriginalMethod<OverrideViewFn>(18)(_this, setup);
}

float __stdcall GGetViewModelFOV()
{
	float fov = g_pClientModeHook->GetMethod<oGetViewModelFOV>(35)();

	if (g_pEngine->IsConnected() && g_pEngine->IsInGame())
	{
		if (g::LocalPlayer)
		{
				fov += Menu.Misc.PlayerViewmodel;
		}
	}
	return fov;
}