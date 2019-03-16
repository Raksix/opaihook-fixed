#include "sdk.h"
#include "global.h"
#include "Draw.h"
#include "GameUtils.h"
#include <chrono>
/*---------------------ENGINE & UTIL CLASSES---------------------*/
CDataMapUtils* g_pData = new CDataMapUtils;
CEncryption* g_pEncrypt = new CEncryption;
CBaseEntity* g::LocalPlayer = nullptr;
CBaseCombatWeapon* g::MainWeapon = nullptr;
CSWeaponInfo* g::WeaponData = nullptr;
CUserCmd*	g::UserCmd = nullptr;
CUserCmd* g::UserCmdForBacktracking = nullptr;
CBaseEntity* g::Target = nullptr;
CPlayerlistInfo* g_pPlayerlistInfo[64] = { nullptr };
std::deque<std::tuple<Vector, float, Color>> g::hitscan_points;
/*---------------------QANGLE---------------------*/
QAngle g::LastAngle = QAngle();
QAngle g::AAAngle = QAngle();
QAngle g::StrafeAngle = QAngle();
QAngle g::RealAngle = QAngle();
vec_t g::PitchAngle = vec_t();
QAngle g::FakeAngle = QAngle();

/*---------------------BOOLEAN---------------------*/
bool g::Aimbotting = false;
bool g::IsDef  = strstr(GetCommandLineA(), "-legacyscaleformui");
bool g::BreakingLagComp = false;
bool g::Return = true;
bool g::ForceRealAA = false;
bool g::SendPacket = true;
bool g::InNotAntiAim = true;
bool g::ShowMenu = false;
bool g::Opened = false;
bool g::NewRound = false;
bool g::Init = false;
bool g::weaponfirecalled = false;
bool g::playerhurtcalled = false;
bool g::walkbotenabled = false;
bool g::bShouldChoke = false;
bool g::bFakewalking = false;
bool g::lby = false;

/*---------------------INTEGER---------------------*/
int g::wbpoints = -1;
//int g::localtime;
int g::chokedticks;
int g::wbcurpoint = 0;
int g::ChokedPackets = 0;
int g::DamageDealt;
int g::ResolverMode[64];
int g::FakeDetection[64];
int g::Shots[64];
int g::MenuTab;
int g::nChokedTicks = 0;
int g::missed_shots[64];
int g::TargetIDO = -1;

/*---------------------FLOATING POINT---------------------*/
float g::PredictedTime = 0.f;
float g::lby_update_end_time;
float g::CurrTime;
float g::flHurtTime;
float g::spread;
float g::viewMatrix[4][4] = { 0 };
/*---------------------D3DX DEFINES---------------------*/
HWND g::Window = nullptr;
CScreen g::Screen = CScreen();

/*---------------------STANDARD VECTOR MAPS---------------------*/
std::vector<FloatingText> g::DamageHit;
std::vector<Vector> g::walkpoints;
std::string g::resolvermode;

/*---------------------VECTOR---------------------*/
Vector g::vecUnpredictedVel = Vector(0, 0, 0);
Vector g::fakeOrigin = Vector(0, 0, 0);


itemTimer::itemTimer() {
	maxTime = 0;
}

itemTimer::itemTimer(float _maxTime) {
	maxTime = _maxTime;
}

float itemTimer::getTimeRemaining() {
	auto time = (timeStarted - (float(clock()) / float(CLOCKS_PER_SEC))) + maxTime;
	return time ? time : 0.00001f; //ensure we don't ever return a 0
}

float itemTimer::getTimeRemainingRatio() {
	return getTimeRemaining() / getMaxTime();
}

float itemTimer::getMaxTime() {
	return maxTime ? maxTime : 1; //don't ever return 0
}

void itemTimer::startTimer() {
	timeStarted = float(float(clock()) / float(CLOCKS_PER_SEC));
}

void itemTimer::setMaxTime(float time) {
	maxTime = time;
}

FloatingText::FloatingText(CBaseEntity* attachEnt, float lifetime, int Damage)
{
	TimeCreated = g::CurrTime;
	ExpireTime = TimeCreated + lifetime;
	pEnt = attachEnt;
	DamageAmt = Damage;
}

void FloatingText::Draw()
{
	float Red, Green, Blue;
	Red = Menu.Colors.DamageIndicator[0] * 255;
	Green = Menu.Colors.DamageIndicator[1] * 255;
	Blue = Menu.Colors.DamageIndicator[2] * 255;
	auto head = pEnt->GetBonePos(8);
	Vector screen;

	if (GameUtils::WorldToScreen(head, screen))
	{
		auto lifetime = ExpireTime - TimeCreated;
		auto pct = 1 - ((ExpireTime - g::CurrTime) / lifetime);
		int offset = pct * 50;
		int y = screen.y - 15 - offset;

		string str = "- " +std::to_string(DamageAmt) + " HP";
		draw::Textf(screen.x, y, Color(Red, Green, Blue, 255), F::esp, str.c_str());
	}
}

int CDataMapUtils::Find(datamap_t* pMap, const char* szName) {
	while (pMap)
	{
		for (int i = 0; i < pMap->dataNumFields; i++)
		{
			if (pMap->dataDesc[i].fieldName == NULL)
				continue;

			if (strcmp(szName, pMap->dataDesc[i].fieldName) == 0)
				return pMap->dataDesc[i].fieldOffset[TD_OFFSET_NORMAL];

			if (pMap->dataDesc[i].fieldType == FIELD_EMBEDDED)
			{
				if (pMap->dataDesc[i].td)
				{
					unsigned int offset;

					if ((offset = Find(pMap->dataDesc[i].td, szName)) != 0)
						return offset;
				}
			}
		}
		pMap = pMap->baseMap;
	}

	return 0;
}