#include "sdk.h"
#include "Antiaim.h"
#include "global.h"
#include "GameUtils.h"
#include "Math.h"
#include "Aimbot.h"
#include <time.h>
#include <thread>

float get_curtime() {
	if (!g::LocalPlayer)
		return 0;
	int g_tick = 0;
	CUserCmd* g_pLastCmd = nullptr;
	if (!g_pLastCmd || g_pLastCmd->hasbeenpredicted) {
		g_tick = g::LocalPlayer->GetTickBase();
	}
	else {
		++g_tick;
	}
	g_pLastCmd = g::UserCmd;
	float curtime = g_tick * g_pGlobals->interval_per_tick;
	return curtime;
}

bool next_lby_update() 
{
	if (g::LocalPlayer) 
	{
		auto net_channel = g_pEngine->GetNetChannel();
		static float next_lby_update_time = 0;
		const float current_time = get_curtime();

		if (g::LocalPlayer->GetVelocity().Length2D() > 0.1) {
			next_lby_update_time = current_time + 0.22f;
		}
		else {
			if ((next_lby_update_time < current_time && !net_channel->m_nChokedPackets)) {
				next_lby_update_time = current_time + 1.1f;
				return true;
			}
		}
	}
	return false;
}

namespace binds
{
	static bool left, right, back = true;
	void Do()
	{
		if (GetAsyncKeyState(VK_LEFT))
		{
			left = true;
			right = false;
			back = false;
		}
		else if (GetAsyncKeyState(VK_RIGHT))
		{
			left = false;
			right = true;
			back = false;
		}
		else if (GetAsyncKeyState(VK_DOWN))
		{
			left = false;
			right = false;
			back = true;
		}

		if (g::SendPacket) {
			if (left)
				g::UserCmd->viewangles.y -= 90;
			else if (right)
				g::UserCmd->viewangles.y += 90;
			else if (back)
				g::UserCmd->viewangles.y += 180;
		}
		else {
			if (left)
				g::UserCmd->viewangles.y += 90;
			else if (right)
				g::UserCmd->viewangles.y -= 90;
			else if (back)
				g::UserCmd->viewangles.y += 180;
		}
		
	}
}

namespace freestanding
{

	mstudiobbox_t* get_hitbox(CBaseEntity* entity, int hitbox_index)
	{
		if (entity->IsDormant() || entity->GetHealth() <= 0)
			return NULL;

		const auto pModel = entity->GetModel();
		if (!pModel)
			return NULL;

		auto pStudioHdr = g_pModelInfo->GetStudioModel(pModel);
		if (!pStudioHdr)
			return NULL;

		auto pSet = pStudioHdr->pHitboxSet(0);
		if (!pSet)
			return NULL;

		if (hitbox_index >= pSet->numhitboxes || hitbox_index < 0)
			return NULL;

		return pSet->pHitbox(hitbox_index);
	}

	Vector get_hitbox_pos(CBaseEntity* entity, int hitbox_id)
	{

		auto hitbox = get_hitbox(entity, hitbox_id);
		if (!hitbox)
			return Vector(0, 0, 0);

		auto bone_matrix = entity->GetBoneMatrix(hitbox->bone);

		Vector bbmin, bbmax;
		Math::VectorTransform(hitbox->bbmin, bone_matrix, bbmin);
		Math::VectorTransform(hitbox->bbmax, bone_matrix, bbmax);

		return (bbmin + bbmax) * 0.5f;
	}

	float fov_player(Vector ViewOffSet, Vector View, CBaseEntity* entity, int hitbox)
	{
		// Anything past 180 degrees is just going to wrap around
		CONST FLOAT MaxDegrees = 180.0f;

		// Get local angles
		Vector Angles = View;

		// Get local view / eye position
		Vector Origin = ViewOffSet;

		// Create and intiialize vectors for calculations below
		Vector Delta(0, 0, 0);
		//Vector Origin(0, 0, 0);
		Vector Forward(0, 0, 0);

		// Convert angles to normalized directional forward vector
		Math::AngleVectors(Angles, &Forward);

		Vector AimPos = get_hitbox_pos(entity, hitbox); //pvs fix disabled

		Math::VectorSubtract(AimPos, Origin, Delta);
		//Delta = AimPos - Origin;

		// Normalize our delta vector
		Math::NormalizeNum(Delta, Delta);

		// Get dot product between delta position and directional forward vectors
		FLOAT DotProduct = Forward.Dot(Delta);

		// Time to calculate the field of view
		return (acos(DotProduct) * (MaxDegrees / M_PI));
	}

	int closest_to_crosshair()
	{
		int index = -1;
		float lowest_fov = INT_MAX;

		CBaseEntity* local_player = g::LocalPlayer;

		if (!local_player)
			return -1;

		Vector local_position = local_player->GetOrigin() + local_player->GetViewOffset();

		Vector angles;
		g_pEngine->GetViewAngles(angles);

		for (int i = 1; i <= g_pGlobals->maxClients; i++)
		{
			CBaseEntity *entity = g_pEntitylist->GetClientEntity(i);

			if (!entity || !entity->isAlive() || entity->GetTeamNum() == local_player->GetTeamNum() || entity->IsDormant() || entity == local_player)
				continue;

			float fov = fov_player(local_position, angles, entity, 0);

			if (fov < lowest_fov)
			{
				lowest_fov = fov;
				index = i;
			}
		}

		return index;
	}

	void Do() {
		static float last_real;
		bool no_active = true;
		float bestrotation = 0.f;
		float highestthickness = 0.f;
		Vector besthead;

		auto leyepos = g::LocalPlayer->GetOrigin() + g::LocalPlayer->GetViewOffset();
		auto headpos = get_hitbox_pos(g::LocalPlayer, 0);
		auto origin = g::LocalPlayer->GetAbsOrigin();

		auto checkWallThickness = [&](CBaseEntity* pPlayer, Vector newhead) -> float
		{
			Vector endpos1, endpos2;
			Vector eyepos = pPlayer->GetOrigin() + pPlayer->GetViewOffset();

			Ray_t ray;
			ray.Init(newhead, eyepos);

			CTraceFilterSkipTwoEntities filter(pPlayer, g::LocalPlayer);

			trace_t trace1, trace2;
			g_pEngineTrace->TraceRay_NEW(ray, MASK_SHOT_BRUSHONLY, &filter, &trace1);

			if (trace1.DidHit())
				endpos1 = trace1.endpos;
			else
				return 0.f;

			ray.Init(eyepos, newhead);
			g_pEngineTrace->TraceRay_NEW(ray, MASK_SHOT_BRUSHONLY, &filter, &trace2);

			if (trace2.DidHit())
				endpos2 = trace2.endpos;

			float add = newhead.Dist(eyepos) - leyepos.Dist(eyepos) + 3.f;
			return endpos1.Dist(endpos2) + add / 3;
		};

		int index = closest_to_crosshair();

		static CBaseEntity* entity;

		if (index != -1)
			entity = g_pEntitylist->GetClientEntity(index);

		float step = (2 * M_PI) / 18.f; // One PI = half a circle ( for stacker cause low iq :sunglasses: ), 28

		float radius = fabs(Vector(headpos - origin).Length2D());

		if (index == -1)
		{
			no_active = true;
		}
		else
		{
			for (float rotation = 0; rotation < (M_PI * 2.0); rotation += step)
			{
				Vector newhead(radius * cos(rotation) + leyepos.x, radius * sin(rotation) + leyepos.y, leyepos.z);

				float totalthickness = 0.f;

				no_active = false;

				totalthickness += checkWallThickness(entity, newhead);

				if (totalthickness > highestthickness)
				{
					highestthickness = totalthickness;
					bestrotation = rotation;
					besthead = newhead;
				}
			}
		}

		if (!no_active)
			g::UserCmd->viewangles.y = RAD2DEG(bestrotation);
		else
			g::UserCmd->viewangles.y += (180);
	}
}

CAntiaim* g_Antiaim = new CAntiaim;

void CAntiaim::DoAntiAims()
{
	g::UserCmd->viewangles.x = 89.f;

	static float add = 0;
	static float old_angle = 0;

	if (g::SendPacket) {
		add = !(g::UserCmd->command_number % 3) ? 5 : 175;
		g::UserCmd->viewangles.y = old_angle + add;
	}
	else {
		if (next_lby_update()) // lol
			g::UserCmd->viewangles.y -= 119;
		switch (Menu.Antiaim.DirectonType)
		{
		case 1://binds
		{
			binds::Do();
		}
		break;
		case 2://freestanding
			freestanding::Do();
			break;
		case 3:
		{
			static float at_target_ang = 0;
			for (int i = 1; i < 65; ++i) {
				auto ent = g_pEntitylist->GetClientEntity(i);
				if (!ent->IsValidTarget())
					continue;

				float dist = 9999.f;
				float cur_dist = GameUtils::CalculateAngle(g::LocalPlayer->GetEyePosition(), ent->GetEyePosition()).y;
				if (dist >= cur_dist) {
					dist = cur_dist;
					at_target_ang = dist;
				}
			}

			g::UserCmd->viewangles.y = 180 + at_target_ang;
		}
		break;
		default:
			g::UserCmd->viewangles.y += 180;
			break;
		}
		old_angle = g::UserCmd->viewangles.y;
	}
}
	


void CAntiaim::Run(QAngle org_view)
{
	if (Menu.Antiaim.AntiaimEnable)
	{
		CGrenade* pCSGrenade = (CGrenade*)g::LocalPlayer->GetWeapon();
		if (g::UserCmd->buttons & IN_ATTACK
			|| g::UserCmd->buttons & IN_USE
			|| g::LocalPlayer->GetMoveType() == MOVETYPE_LADDER && g::LocalPlayer->GetVelocity().Length() > 0
			|| g::LocalPlayer->GetMoveType() == MOVETYPE_NOCLIP
			|| *g::LocalPlayer->GetFlags() & FL_FROZEN
			|| pCSGrenade && pCSGrenade->GetThrowTime() > 0.f)
			return;

		if (!Menu.Misc.FakelagEnable || (*g::LocalPlayer->GetFlags() & FL_ONGROUND && !Menu.Misc.FakelagOnground || *g::LocalPlayer->GetFlags() & FL_ONGROUND && g::LocalPlayer->GetVelocity().Length() < 3))
			g::SendPacket = choke;

		choke = !choke;

		DoAntiAims();

		g::UserCmd->buttons |= IN_BULLRUSH;

		if (GetAsyncKeyState('Z'))
		{
			static bool counter = false;
			static int counters = 0;
			if (counters == 20)
			{
				counters = 0;
				counter = !counter;
			}
			counters++;
			if (counter)
			{
				g::UserCmd->buttons |= IN_DUCK;
				
			}
			else {
				g::UserCmd->buttons &= ~IN_DUCK;
				
			}
		}
	}
}