#include "hooks.h"
#include "global.h"
#include "Misc.h"
#include "Menu.h"
#include "BacktrackingHelper.h"
#include "Math.h"
#include "GameUtils.h"
#include "backtrackmanager.h"
#include "Aimbot.h"
#include "PredictionSystem.h"
#include "Antiaim.h"
#include "GrenadePrediction.h"
#include "legitbot.h"
#include <Psapi.h>
#include "knifebot.h"
static CPredictionSystem* Prediction = new CPredictionSystem();

static int Ticks = 0;
static int LastReserve = 0;
namespace FakeLatency
{
	char *clientdllstr = new char[11]{ 25, 22, 19, 31, 20, 14, 84, 30, 22, 22, 0 }; /*client.dll*/
	char *enginedllstr = new char[11]{ 31, 20, 29, 19, 20, 31, 84, 30, 22, 22, 0 }; /*engine.dll*/
	char *cam_tofirstperson_sig = new char[51]{ 59, 75, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 56, 67, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 60, 60, 90, 90, 67, 74, 90, 90, 67, 74, 0 }; /*A1  ??  ??  ??  ??  B9  ??  ??  ??  ??  FF  90  90*/
	char *cam_tothirdperson_sig = new char[51]{ 59, 75, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 56, 67, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 60, 60, 90, 90, 67, 74, 90, 90, 66, 57, 0 }; /*A1  ??  ??  ??  ??  B9  ??  ??  ??  ??  FF  90  8C*/
	char *clientstatestr = new char[51]{ 66, 56, 90, 90, 73, 62, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 66, 59, 90, 90, 60, 67, 90, 90, 60, 73, 90, 90, 74, 60, 90, 90, 75, 75, 90, 90, 78, 79, 90, 90, 60, 57, 0 }; /*8B  3D  ??  ??  ??  ??  8A  F9  F3  0F  11  45  FC*/
	char *updateclientsideanimfnsigstr = new char[95]{ 79, 79, 90, 90, 66, 56, 90, 90, 63, 57, 90, 90, 79, 75, 90, 90, 79, 76, 90, 90, 66, 56, 90, 90, 60, 75, 90, 90, 66, 74, 90, 90, 56, 63, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 74, 74, 90, 90, 74, 74, 90, 90, 74, 74, 90, 90, 77, 78, 90, 90, 73, 76, 90, 90, 66, 56, 90, 90, 74, 76, 90, 90, 60, 60, 90, 90, 67, 74, 90, 90, 69, 69, 90, 90, 69, 69, 90, 90, 74, 74, 90, 90, 74, 74, 0 }; /*55  8B  EC  51  56  8B  F1  80  BE  ??  ??  00  00  00  74  36  8B  06  FF  90  ??  ??  00  00*/
	void DecStr(char *adr, const unsigned len)
	{
		for (unsigned i = 0; i < len; i++)
		{
			adr[i] ^= 50;
			adr[i] ^= 72;
		}
	}

	void EncStr(char *adr, const unsigned len)
	{
		for (unsigned i = 0; i < len; i++)
		{
			adr[i] ^= 72;
			adr[i] ^= 50;
		}
	}

	void DecStr(char *adr)
	{
		int len = strlen(adr);
		for (unsigned i = 0; i < len; i++)
		{
			adr[i] ^= 50;
			adr[i] ^= 72;
		}
	}

	void EncStr(char *adr)
	{
		int len = strlen(adr);
		for (unsigned i = 0; i < len; i++)
		{
			adr[i] ^= 72;
			adr[i] ^= 50;
		}
	}
	HANDLE FindHandle(std::string name)
	{
		return GetModuleHandle(name.c_str());
	}
	uintptr_t FindMemoryPattern(HANDLE ModuleHandle, char* strpattern, int length)
	{
		unsigned char *signature = new unsigned char[length + 1];
		bool *skippable = new bool[length + 1];
		int signaturelength = 0;
		for (int byteoffset = 0; byteoffset < length - 1; byteoffset += 2)
		{
			char charhex[4]; //4 to keep sscanf happy
			*(short*)charhex = *(short*)&strpattern[byteoffset];
			if (charhex[0] != ' ')
			{
				if (charhex[0] == '?')
				{
					signature[signaturelength] = '?';
					skippable[signaturelength] = true;
				}
				else
				{
					charhex[2] = NULL; //add null terminator
					signature[signaturelength] = (unsigned char)std::stoul(charhex, nullptr, 16);
					skippable[signaturelength] = false;
				}
				signaturelength++;
			}
		}
		int searchoffset = 0;
		int maxoffset = signaturelength - 1;


		MODULEINFO dllinfo;
		GetModuleInformation(GetCurrentProcess(), (HMODULE)ModuleHandle, &dllinfo, sizeof(MODULEINFO));
		DWORD endadr = (DWORD)ModuleHandle + dllinfo.SizeOfImage;
		DWORD adrafterfirstmatch = NULL;
		for (DWORD adr = (DWORD)ModuleHandle; adr < endadr; adr++)
		{
			if (skippable[searchoffset] || *(char*)adr == signature[searchoffset] || *(unsigned char*)adr == signature[searchoffset])
			{
				if (searchoffset == 0)
				{
					adrafterfirstmatch = adr + 1;
				}
				searchoffset++;
				if (searchoffset > maxoffset)
				{
					delete[] signature;
					delete[] skippable;
					//timetakentosearch = QPCTime() - startsearch;
					return adr - maxoffset; //FOUND OFFSET!
				}
			}
			else if (adrafterfirstmatch)
			{
				adr = adrafterfirstmatch;
				searchoffset = 0;
				adrafterfirstmatch = NULL;
			}
		}

		delete[] signature;
		delete[] skippable;
		return NULL; //NOT FOUND!
	}
	HANDLE EngineHandle = NULL;
	HANDLE ClientHandle = NULL;
	DWORD ClientState = NULL;
}

int __fastcall Hooked_SendDatagram(void* netchan, void*, void *datagram)
{

	INetChannel* chan = (INetChannel*)netchan;
	bf_write* data = (bf_write*)datagram;


	int instate = chan->m_nInReliableState;
	int insequencenr = chan->m_nInSequenceNr;

	if (Menu.Ragebot.FakeLatency)
	{
		if (!g_pEngine->IsConnected() || !g_pEngine->IsInGame())
			g_BacktrackHelper->ClearIncomingSequences();

		g_BacktrackHelper->AddLatencyToNetchan(chan, Menu.Ragebot.FakeLatencyAmount);
	}

	int ret = HNetchan->GetOriginalMethod<SendDatagramFn>(46)(chan, data);

	chan->m_nInReliableState = instate;
	chan->m_nInSequenceNr = insequencenr;


	return ret;

}

void InitFakeLatency() {
	if (Menu.Ragebot.FakeLatency)
	{
		if (!HNetchan)
		{
			static DWORD ClientState = *(DWORD*)FakeLatency::ClientState;

			if (ClientState)
			{
				static DWORD NetChannel = *(DWORD*)(*(DWORD*)FakeLatency::ClientState + 0x9C);
				if (NetChannel)
				{
					HNetchan = new VMT((DWORD**)NetChannel);
					HNetchan->HookVM((void*)Hooked_SendDatagram, 46);
					HNetchan->ApplyVMT();
				}
			}
		}
	}
}

TraceRayFn oTraceRay;

bool __fastcall Hooks::CreateMove(void* thisptr, void*, float flInputSampleTime, CUserCmd* cmd)
{
	if (!cmd || !cmd->command_number)
		return true;

	g::LocalPlayer = g_pEntitylist->GetClientEntity(g_pEngine->GetLocalPlayer());
	if (!g::LocalPlayer)
		return true;

	g::UserCmd = cmd;
	g::UserCmdForBacktracking = cmd;

	PVOID pebp;
	__asm mov pebp, ebp;
	bool* pbSendPacket = (bool*)(*(PDWORD)pebp - 0x1C);
	bool& bSendPacket = *pbSendPacket;

	g::SendPacket = bSendPacket;

	if (g_pEngine->IsConnected() && g_pEngine->IsInGame()) {
		g_BacktrackHelper->UpdateIncomingSequences();

		if (g::LocalPlayer->isAlive()) {
			g::MainWeapon = g::LocalPlayer->GetWeapon();
			g::WeaponData = g::MainWeapon->GetCSWpnData();

			g_Aimbot->DropTarget();
			g_Misc->RunMiscFuncs();
			KnifeBot::Run();

			Prediction->EnginePrediction(cmd);
			g_Misc->FakeLag();
			g_Aimbot->Run();
			g_Aimbot->CompensateInaccuracies();
			g_Antiaim->Run(Vector());
			InitFakeLatency();

			g_Misc->FixMovement();
			g_Misc->FixCmd();

			cmd = g::UserCmd;
			bSendPacket = g::SendPacket;
			g::ChokedPackets = g_pEngine->GetNetChannel()->m_nChokedPackets;

			if (g::SendPacket) {
				g::FakeAngle = cmd->viewangles;
				g::fakeOrigin = g::LocalPlayer->GetAbsOrigin();
			}
			else 
				g::RealAngle = cmd->viewangles;
			g::spread = g::MainWeapon->GetSpread() + (g::MainWeapon->GetInaccuracy() * 800);

			grenade_prediction::instance().Tick(g::UserCmd->buttons);
		}
	}

	return false;
}
