/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "wpn_balrog7.h"
#include "weapons/WeaponTemplate.hpp"
#ifndef CLIENT_DLL
#include "effects.h"
#include "customentity.h"
#include "monsters.h"
#endif

#ifdef CLIENT_DLL
namespace cl {
#else
namespace sv {
#endif

#ifndef CLIENT_DLL
#include "gamemode/mods.h"
#endif

enum balrog7_e
{
	BALROG7_IDLE1,
	BALROG7_SHOOT1,
	BALROG7_SHOOT2,
	BALROG7_SHOOT3,
	BALROG7_RELOAD,
	BALROG7_DRAW
};

LINK_ENTITY_TO_CLASS(weapon_balrog7, CBalrog7)

void CBalrog7::Spawn(void)
{
	pev->classname = MAKE_STRING("weapon_balrog7");

	Precache();
	m_iId = WEAPON_M249;
	SET_MODEL(ENT(pev), "models/w_balrog7.mdl");

	m_iDefaultAmmo = BALROG7_DEFAULT_GIVE;
	m_flAccuracy = 0.2;
	m_iShotsFired = 0;

	FallInit();
}

void CBalrog7::Precache(void)
{
	PRECACHE_MODEL("models/v_balrog7.mdl");
	PRECACHE_MODEL("models/w_balrog7.mdl");
	

	PRECACHE_SOUND("weapons/balrog7-1.wav");
	PRECACHE_SOUND("weapons/balrog7_exp.wav");
	PRECACHE_SOUND("weapons/balrog7_clipout1.wav");
	PRECACHE_SOUND("weapons/balrog7_clipout2.wav");
	PRECACHE_SOUND("weapons/balrog7_clipin1.wav");
	PRECACHE_SOUND("weapons/balrog7_clipin2.wav");
	PRECACHE_SOUND("weapons/balrog7_clipin3.wav");

	m_iShell = PRECACHE_MODEL("models/rshell.mdl");
	m_iModelExplo = PRECACHE_MODEL("sprites/eexplo.spr");
	m_iBalrog7Explo = PRECACHE_MODEL("sprites/balrogcritical.spr");
	m_usFireBalrog7 = PRECACHE_EVENT(1, "events/balrog7.sc");
	
}

int CBalrog7::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "556NatoBox";
	p->iMaxAmmo1 = MAX_AMMO_556NATOBOX;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = BALROG7_MAX_CLIP;
	p->iSlot = 0;
	p->iPosition = 4;
	p->iId = m_iId = WEAPON_M249;
	p->iFlags = 0;
	p->iWeight = M249_WEIGHT;

	return 1;
}

BOOL CBalrog7::Deploy(void)
{
	m_flAccuracy = 0.2;
	m_iShotsFired = 0;
	iShellOn = 1;

	return DefaultDeploy("models/v_balrog7.mdl", "models/p_balrog7.mdl", BALROG7_DRAW, "balrog7", UseDecrement() != FALSE);
}

void CBalrog7::PrimaryAttack(void)
{
	if (!FBitSet(m_pPlayer->pev->flags, FL_ONGROUND))
		Balrog7Fire(0.045 + (0.5) * m_flAccuracy, 0.1s, FALSE);
	else if (m_pPlayer->pev->velocity.Length2D() > 140)
		Balrog7Fire(0.045 + (0.095) * m_flAccuracy, 0.1s, FALSE);
	else if (m_pPlayer->pev->fov == 90)
		Balrog7Fire((0.02) * m_flAccuracy, 0.1s, FALSE);
	else	
		Balrog7Fire((0.03) * m_flAccuracy, 0.135s, FALSE);
}

void CBalrog7::SecondaryAttack(void)
{
	if (m_pPlayer->m_iFOV != 90)
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 90;
	else
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 55;

	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.3s;
}

Vector Get_Aiming(CBaseEntity *pevAttacker)
{
	Vector start, view_ofs, end;
	start = pevAttacker->pev->origin;
	view_ofs = pevAttacker->pev->view_ofs;
	start = start + view_ofs;

	end = pevAttacker->pev->v_angle;
	UTIL_MakeVectors(end);
	end = gpGlobals->v_forward;
	end = end * 8120.0;
	end = start + end;

	TraceResult tr;
	UTIL_TraceLine(start, end, dont_ignore_monsters, pevAttacker->edict(), &tr);
	end = tr.vecEndPos;

	return end;
}

#ifndef CLIENT_DLL
void CBalrog7::RadiusDamage(Vector vecAiming , float flDamage)
{
	const float flRadius = 100.0;
	const Vector vecSrc = pev->origin;
	entvars_t * const pevAttacker = VARS(pev->owner);
	entvars_t * const pevInflictor = this->pev;
	int bitsDamageType = DMG_BULLET;

	TraceResult tr;
	const float falloff = flRadius ? flDamage / flRadius : 1;
	const int bInWater = (UTIL_PointContents(vecSrc) == CONTENTS_WATER);

	CBaseEntity *pEntity = NULL;
	while ((pEntity = UTIL_FindEntityInSphere(pEntity, vecSrc, flRadius)) != NULL)
	{
		if (pEntity->pev->takedamage != DAMAGE_NO)
		{
			if (bInWater && !pEntity->pev->waterlevel)
				continue;

			if (!bInWater && pEntity->pev->waterlevel == 3)
				continue;

			if (pEntity->IsBSPModel())
				continue;

			if (pEntity->pev == pevAttacker)
				continue;

			Vector vecSpot = pEntity->BodyTarget(vecSrc);
			UTIL_TraceLine(vecSrc, vecSpot, missile, ENT(pevInflictor), &tr);

			if (tr.flFraction == 1.0f || tr.pHit == pEntity->edict())
			{
				if (tr.fStartSolid)
				{
					tr.vecEndPos = vecSrc;
					tr.flFraction = 0;
				}
				float flAdjustedDamage = flDamage - (vecSrc - pEntity->pev->origin).Length() * falloff;
				flAdjustedDamage = Q_max(0, flAdjustedDamage);

				if (tr.flFraction == 1.0f)
				{
					pEntity->TakeDamage(pevInflictor, pevAttacker, flAdjustedDamage, bitsDamageType);
				}
				else
				{
					tr.iHitgroup = HITGROUP_CHEST;
					ClearMultiDamage();
					pEntity->TraceAttack(pevInflictor, flAdjustedDamage, (tr.vecEndPos - vecSrc).Normalize(), &tr, bitsDamageType);
					ApplyMultiDamage(pevInflictor, pevAttacker);
				}

				/*CBasePlayer *pVictim = dynamic_cast<CBasePlayer *>(pEntity);
				if (pVictim->m_bIsZombie) // Zombie Knockback...
				{
				ApplyKnockbackData(pVictim, vecSpot - vecSrc, GetKnockBackData());
				}*/
			}
		}
	}

	MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
	WRITE_BYTE(TE_EXPLOSION);
	WRITE_COORD(vecAiming[0]);
	WRITE_COORD(vecAiming[1]);
	WRITE_COORD(vecAiming[2]);
	WRITE_SHORT(m_iModelExplo);
	WRITE_BYTE(10);
	WRITE_BYTE(16);
	WRITE_BYTE(TE_EXPLFLAG_NOPARTICLES);
	MESSAGE_END();

	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY);
	WRITE_BYTE(TE_EXPLOSION);
	WRITE_COORD(vecAiming[0]);
	WRITE_COORD(vecAiming[1]);
	WRITE_COORD(vecAiming[2] + 20);
	WRITE_SHORT(m_iBalrog7Explo);
	WRITE_BYTE(10);
	WRITE_BYTE(16);
	WRITE_BYTE(TE_EXPLFLAG_NONE);
	MESSAGE_END();
}
#endif

void CBalrog7::ItemPostFrame()
{	
	if (!(this->m_pPlayer->pev->button & IN_ATTACK))
		pev->iuser1 = 0;
	return CBasePlayerWeapon::ItemPostFrame();
}

float CBalrog7::GetDamage()
{
#ifndef CLIENT_DLL
	if (g_pModRunning->DamageTrack() == DT_ZBS)
		return 600.0;
	if (g_pModRunning->DamageTrack() == DT_ZB)
		return 400.0;
#endif
	return 105.0;
}

void CBalrog7::Balrog7Fire(float flSpread, duration_t flCycleTime, BOOL fUseAutoAim)
{
	m_bDelayFire = true;
	m_iShotsFired++;
	m_flAccuracy = ((m_iShotsFired * m_iShotsFired * m_iShotsFired) / 175.0) + 0.4;

	if (m_flAccuracy > 0.9)
		m_flAccuracy = 0.9;

	if (m_iClip <= 0)
	{
		if (m_fFireOnEmpty)
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.2s;
		}

		return;
	}

	m_iClip--;
	m_pPlayer->pev->effects |= EF_MUZZLEFLASH;
#ifndef CLIENT_DLL
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);
#endif

	UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecDir = m_pPlayer->FireBullets3(vecSrc, gpGlobals->v_forward, flSpread, 8192, 2, BULLET_PLAYER_556MM, 32, 0.97, m_pPlayer->pev, FALSE, m_pPlayer->random_seed);

	int flags;
#ifdef CLIENT_WEAPONS
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif
#ifndef CLIENT_DLL
	int iShootTime;
	iShootTime = pev->iuser1;

	if (iShootTime > 10)
	{

		iShootTime = 0;
		float flDamage = GetDamage();
		int bitsDamageType;
		CBaseEntity *pevAttacker = this->m_pPlayer;
		auto vecAiming = Get_Aiming(pevAttacker);

		RadiusDamage(vecAiming , flDamage);
	}

	iShootTime++;
	pev->iuser1 = iShootTime;
#endif
	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usFireBalrog7, 0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, (int)(m_pPlayer->pev->punchangle.x * 100), (int)(m_pPlayer->pev->punchangle.y * 100), FALSE, FALSE);

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + flCycleTime;

#ifndef CLIENT_DLL
	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);
#endif
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.6s;

	if (!FBitSet(m_pPlayer->pev->flags, FL_ONGROUND))
		KickBack(1.1, 0.3, 0.2, 0.06, 4, 2.5, 8);
	else if (m_pPlayer->pev->velocity.Length2D() > 0)
		KickBack(1.5, 0.55, 0.3, 0.3, 6, 5, 5);
	else if (FBitSet(m_pPlayer->pev->flags, FL_DUCKING))
		KickBack(0.75, 0.1, 0.1, 0.01, 3.5, 1.2, 9);
	else
		KickBack(0.8, 0.2, 0.18, 0.02, 3.2, 2.25, 7);
}

void CBalrog7::Reload(void)
{
	
	if (m_pPlayer->ammo_556natobox <= 0)
		return;

	if (m_pPlayer->m_iFOV != 90)
		SecondaryAttack();

	if (DefaultReload(BALROG7_MAX_CLIP, BALROG7_RELOAD, 4.0s))
	{
#ifndef CLIENT_DLL
		m_pPlayer->SetAnimation(PLAYER_RELOAD);
#endif
		m_flAccuracy = 0.2;
		m_bDelayFire = false;
		m_iShotsFired = 0;
	}
}

void CBalrog7::WeaponIdle(void)
{
	ResetEmptySound();
	m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 20s;
	SendWeaponAnim(BALROG7_IDLE1, UseDecrement() != FALSE);
}

float CBalrog7::GetMaxSpeed(void)
{
	if (m_pPlayer->m_iFOV == 90)
		return M249_MAX_SPEED;

	return 200;
}
}
