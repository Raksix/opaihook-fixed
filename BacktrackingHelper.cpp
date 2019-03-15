#include "sdk.h"
#include "BacktrackingHelper.h"
#include "global.h"
#include "xor.h"
#include "Math.h"



template<class T, class U>
inline T clamp(T in, U low, U high)
{
	if (in <= low)
		return low;
	else if (in >= high)
		return high;
	else
		return in;
}
#define TICK_INTERVAL			( g_pGlobals->interval_per_tick )
#define TIME_TO_TICKS( dt )		( floorf(( 0.5f + (float)(dt) / TICK_INTERVAL ) ) )
#define TICKS_TO_TIME( t )		( TICK_INTERVAL *( t ) )
CBacktrackHelper* g_BacktrackHelper = new CBacktrackHelper;

bool CBacktrackHelper::IsTickValid(tick_record record)
{
	float correct = 0;

	correct += g_pEngine->GetNetChannelInfo()->GetLatency(FLOW_OUTGOING);
	correct += g_pEngine->GetNetChannelInfo()->GetLatency(FLOW_INCOMING);
	correct += g_BacktrackHelper->GetLerpTime();

	static ConVar* sv_maxunlag = g_pCvar->FindVar("sv_maxunlag");
	correct = clamp<float>(correct, 0.0f, sv_maxunlag->GetFloat());

	float deltaTime = correct - (g_pGlobals->curtime - record.m_flSimulationTime);

	float latecy = Menu.Ragebot.FakeLatencyAmount;
	float ping = Menu.Ragebot.FakeLatency ? latecy : 0.2f;

	return fabsf(deltaTime) < ping;
}

ConVar* minupdate;
ConVar* maxupdate;
ConVar * updaterate;
ConVar * interprate;
ConVar* cmin;
ConVar* cmax;
ConVar* interp;

bool CBacktrackHelper::IsValidTick(int tick) // gucci i think cant remember
{
	auto nci = g_pEngine->GetNetChannelInfo();

	if (!nci)
		return false;

	auto PredictedCmdArrivalTick = csgo::UserCmd->tick_count + 1 + TIME_TO_TICKS(nci->GetAvgLatency(FLOW_INCOMING) + nci->GetAvgLatency(FLOW_OUTGOING));
	auto Correct = clamp(GetLerpTime() + nci->GetLatency(FLOW_OUTGOING), 0.f, 1.f) - TICKS_TO_TIME(PredictedCmdArrivalTick + TIME_TO_TICKS(GetLerpTime()) - (tick + TIME_TO_TICKS(GetLerpTime())));

	float latecy = Menu.Ragebot.FakeLatencyAmount;
	float ping = Menu.Ragebot.FakeLatency ? latecy : 0.2f;

	return (abs(Correct) <= ping);
}

float CBacktrackHelper::GetLerpTime()
{
	int updaterate = g_pCvar->FindVar("cl_updaterate")->GetInt();
	ConVar* minupdate = g_pCvar->FindVar("sv_minupdaterate");
	ConVar* maxupdate = g_pCvar->FindVar("sv_maxupdaterate");

	if (minupdate && maxupdate)
		updaterate = maxupdate->GetInt();

	float ratio = g_pCvar->FindVar("cl_interp_ratio")->GetFloat();

	if (ratio == 0)
		ratio = 1.0f;

	float lerp = g_pCvar->FindVar("cl_interp")->GetFloat();
	ConVar* cmin = g_pCvar->FindVar("sv_client_min_interp_ratio");
	ConVar* cmax = g_pCvar->FindVar("sv_client_max_interp_ratio");

	if (cmin && cmax && cmin->GetFloat() != 1)
		ratio = clamp(ratio, cmin->GetFloat(), cmax->GetFloat());

	return max(lerp, (ratio / updaterate));
}

void CBacktrackHelper::UpdateBacktrackRecords(CBaseEntity* pPlayer)
{
	int i = pPlayer->Index();
	//for (int j = g_BacktrackHelper->PlayerRecord[i].records.size() - 1; j >= 0; j--)
	//{
	//	tick_record rec = g_BacktrackHelper->PlayerRecord[i].records.at(j);
	//	float amount = Menu.Ragebot.FakeLatencyAmount;
	//	float ping = Menu.Ragebot.FakeLatency ? amount : 0.2f;
	//	if (rec.m_flSimulationTime < g_pGlobals->curtime - ping)
	//		g_BacktrackHelper->PlayerRecord[i].records.erase(g_BacktrackHelper->PlayerRecord[i].records.begin() + j);

	//}
	//static Vector old_origin[64];
	//if (PlayerRecord[i].records.size() > 0 && pPlayer->GetSimulationTime() == PlayerRecord[i].records.back().m_flSimulationTime) //already got such a record
	//	return;

	//if (!(PlayerRecord[i].records.empty())) {
	//	static vector<tick_record> backtrack;
	//	backtrack.push_back(g_BacktrackHelper->PlayerRecord[pPlayer->Index()].records.back());
	//	backtrack.push_back(g_BacktrackHelper->PlayerRecord[pPlayer->Index()].records.at(0));

	//	for (auto btrack : backtrack) {
	//		if (btrack.m_flSimulationTime > pPlayer->GetSimulationTime())//Invalid lag record, maybe from diffrent game?
	//		{
	//			PlayerRecord[i].records.clear();
	//			return;
	//		}
	//	}
	//}

	////for (int j = g_BacktrackHelper->PlayerRecord[i].records.size() - 1; j >= 0; j--)
	////{
	////	tick_record rec = g_BacktrackHelper->PlayerRecord[i].records.at(j);

	////	switch (Menu.Ragebot.TickType)
	////	{
	////	case 0:
	////		rec = g_BacktrackHelper->PlayerRecord[i].records.at(0);
	////		break;
	////	case 1:
	////		rec = g_BacktrackHelper->PlayerRecord[i].records.at(j);
	////		break;
	////	}

	////	if (PlayerRecord[i].records.size() > 0 && rec.m_flSimulationTime > pPlayer->GetSimulationTime())//Invalid lag record, maybe from diffrent game?
	////	{
	////		PlayerRecord[i].records.clear();
	////		return;
	////	}

	////}
	//tick_record new_record;
	////Vector cur_origin = pPlayer->GetOrigin();
	////Vector v = cur_origin - old_origin[i];
	////bool breaks_lagcomp = v.LengthSqr() > 4096.f;
	////old_origin[i] = cur_origin;
	////

	////new_record.needs_extrapolation = breaks_lagcomp;
	//static float OldLower[64];

	//PlayerRecord[i].LowerBodyYawTarget = pPlayer->LowerBodyYaw();
	//new_record.m_angEyeAngles = pPlayer->GetEyeAngles();
	//new_record.m_flCycle = pPlayer->GetCycle();

	//new_record.m_nFlags = *pPlayer->GetFlags();
	//new_record.m_absangles = pPlayer->GetAbsAngles();


	//new_record.m_flSimulationTime = pPlayer->GetSimulationTime();
	//new_record.m_flAnimTime = pPlayer->GetAnimationTime();
	//new_record.m_vecAbsOrigin = pPlayer->GetAbsOrigin();
	//new_record.bLowerBodyYawUpdated = false;
	//new_record.m_nSequence = pPlayer->GetSequence();
	//new_record.m_vecOrigin = pPlayer->GetOrigin();
	//new_record.m_vecVelocity = pPlayer->GetVelocity();
	//new_record.m_flUpdateTime = g_pGlobals->curtime;
	//new_record.backtrack_time = new_record.m_flSimulationTime + GetLerpTime();
	//new_record.m_vecMins = pPlayer->GetCollision()->VecMins();
	//new_record.m_vecMax = pPlayer->GetCollision()->VecMaxs();
	//new_record.ragpos = pPlayer->get_ragdoll_pos();

	//if (PlayerRecord[i].LowerBodyYawTarget != OldLower[i])
	//	new_record.bLowerBodyYawUpdated = true;

	//for (int i = 0; i < 24; i++)
	//	new_record.m_flPoseParameter[i] = *(float*)((DWORD)pPlayer + offys.m_flPoseParameter + sizeof(float) * i);

	//pPlayer->SetupBones(new_record.boneMatrix, 128, 0x100, g_pGlobals->curtime);

	//OldLower[i] = PlayerRecord[i].LowerBodyYawTarget;
	//PlayerRecord[i].records.push_back(new_record);

	tick_record new_record;

	static float OldSimtime[65];

	if (PlayerRecord[i].records.size() == 0)
	{
		PlayerRecord[i].LowerBodyYawTarget = pPlayer->LowerBodyYaw();
		new_record.m_angEyeAngles = pPlayer->GetEyeAngles();
		new_record.m_flCycle = pPlayer->GetCycle();

		new_record.m_nFlags = *pPlayer->GetFlags();
		new_record.m_absangles = pPlayer->GetAbsAngles();
		new_record.m_flSimulationTime = pPlayer->GetSimulationTime();
		new_record.m_flAnimTime = pPlayer->GetAnimationTime();
		new_record.m_vecAbsOrigin = pPlayer->GetAbsOrigin();
		new_record.bLowerBodyYawUpdated = false;
		new_record.m_nSequence = pPlayer->GetSequence();
		new_record.m_vecOrigin = pPlayer->GetOrigin();
		new_record.m_vecVelocity = pPlayer->GetVelocity();
		new_record.m_flUpdateTime = g_pGlobals->curtime;
		new_record.backtrack_time = new_record.m_flSimulationTime + GetLerpTime();
		new_record.m_vecMins = pPlayer->GetCollision()->VecMins();
		new_record.m_vecMax = pPlayer->GetCollision()->VecMaxs();
		new_record.ragpos = pPlayer->get_ragdoll_pos();

		for (int i = 0; i < 24; i++)
			new_record.m_flPoseParameter[i] = *(float*)((DWORD)pPlayer + offys.m_flPoseParameter + sizeof(float) * i);

		int Backup = *(int*)((uintptr_t)pPlayer + 0x274);
		*(int*)((uintptr_t)pPlayer + 0x274) = 0;
		pPlayer->SetupBones(new_record.boneMatrix, 128, 0x100, g_pGlobals->curtime);
		pPlayer->SetupBones2(new_record.chamsBoneMatrix, 128, 0x100, g_pGlobals->curtime);
		*(int*)((uintptr_t)pPlayer + 0x274) = Backup;

		PlayerRecord[i].records.push_back(new_record);
	}

	if (OldSimtime[i] != pPlayer->GetSimulationTime())
	{
		PlayerRecord[i].LowerBodyYawTarget = pPlayer->LowerBodyYaw();
		new_record.m_angEyeAngles = pPlayer->GetEyeAngles();
		new_record.m_flCycle = pPlayer->GetCycle();

		new_record.m_nFlags = *pPlayer->GetFlags();
		new_record.m_absangles = pPlayer->GetAbsAngles();
		new_record.m_flSimulationTime = pPlayer->GetSimulationTime();
		new_record.m_flAnimTime = pPlayer->GetAnimationTime();
		new_record.m_vecAbsOrigin = pPlayer->GetAbsOrigin();
		new_record.bLowerBodyYawUpdated = false;
		new_record.m_nSequence = pPlayer->GetSequence();
		new_record.m_vecOrigin = pPlayer->GetOrigin();
		new_record.m_vecVelocity = pPlayer->GetVelocity();
		new_record.m_flUpdateTime = g_pGlobals->curtime;
		new_record.backtrack_time = new_record.m_flSimulationTime + GetLerpTime();
		new_record.m_vecMins = pPlayer->GetCollision()->VecMins();
		new_record.m_vecMax = pPlayer->GetCollision()->VecMaxs();
		new_record.ragpos = pPlayer->get_ragdoll_pos();

		for (int i = 0; i < 24; i++)
			new_record.m_flPoseParameter[i] = *(float*)((DWORD)pPlayer + offys.m_flPoseParameter + sizeof(float) * i);

		int Backup = *(int*)((uintptr_t)pPlayer + 0x274);
		*(int*)((uintptr_t)pPlayer + 0x274) = 0;
		pPlayer->SetupBones(new_record.boneMatrix, 128, 0x100, g_pGlobals->curtime);
		pPlayer->SetupBones2(new_record.chamsBoneMatrix, 128, 0x100, g_pGlobals->curtime);
		*(int*)((uintptr_t)pPlayer + 0x274) = Backup;

		PlayerRecord[i].records.push_back(new_record);

		OldSimtime[i] = pPlayer->GetSimulationTime();
	}

	if (PlayerRecord[i].records.size() > 0)
		for (int tick = 0; tick < PlayerRecord[i].records.size(); tick++)
			if (!IsValidTick(TIME_TO_TICKS(PlayerRecord[i].records.at(tick).m_flSimulationTime)))
				PlayerRecord[i].records.erase(PlayerRecord[i].records.begin() + tick);
}

static std::deque<CIncomingSequence>sequences;
static int lastincomingsequencenumber;
int Real_m_nInSequencenumber;

void CBacktrackHelper::UpdateIncomingSequences() // fake latency
{
	if (!FakeLatency::ClientState || FakeLatency::ClientState == 0)
		return;
	DWORD ClientState = (DWORD)*(DWORD*)FakeLatency::ClientState;
	if (ClientState)
	{
		INetChannel *netchan = (INetChannel*)*(DWORD*)(ClientState + 0x9C);
		if (netchan)
		{
			if (netchan->m_nInSequenceNr > lastincomingsequencenumber)
			{
				lastincomingsequencenumber = netchan->m_nInSequenceNr;
				sequences.push_front(CIncomingSequence(netchan->m_nInReliableState, netchan->m_nOutReliableState, netchan->m_nInSequenceNr, g_pGlobals->realtime));
			}

			if (sequences.size() > 2048)
				sequences.pop_back();
		}
	}
}

void CBacktrackHelper::ClearIncomingSequences()
{
	sequences.clear();
}

void CBacktrackHelper::AddLatencyToNetchan(INetChannel *netchan, float Latency)
{
	for (auto& seq : sequences)
	{
		if (g_pGlobals->realtime - seq.curtime >= Latency || g_pGlobals->realtime - seq.curtime > 1)
		{
			netchan->m_nInReliableState = seq.inreliablestate;
			netchan->m_nInSequenceNr = seq.sequencenr;
			break;
		}
	}
}