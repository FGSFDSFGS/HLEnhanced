/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
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
#include "entities/NPCs/Monsters.h"
#include "Weapons.h"
#include "CTripmine.h"
#include "nodes/Nodes.h"
#include "CBasePlayer.h"
#include "Effects.h"
#include "gamerules/GameRules.h"

#define	TRIPMINE_PRIMARY_VOLUME		450

#ifndef CLIENT_DLL

//TODO: move - Solokiller
class CTripmineGrenade : public CGrenade
{
public:
	DECLARE_CLASS( CTripmineGrenade, CGrenade );
	DECLARE_DATADESC();

	void Spawn( void ) override;
	void Precache( void ) override;

	void UpdateOnRemove() override;

	void OnTakeDamage( const CTakeDamageInfo& info ) override;
	
	void EXPORT WarningThink( void );
	void EXPORT PowerupThink( void );
	void EXPORT BeamBreakThink( void );
	void EXPORT DelayDeathThink( void );
	void Killed( const CTakeDamageInfo& info, GibAction gibAction ) override;

	void MakeBeam( void );
	void KillBeam( void );

	float		m_flPowerUp;
	Vector		m_vecDir;
	Vector		m_vecEnd;
	float		m_flBeamLength;

	EHANDLE		m_hOwner;
	CBeam		*m_pBeam;
	Vector		m_posOwner;
	Vector		m_angleOwner;
	edict_t		*m_pRealOwner;// tracelines don't hit PEV->OWNER, which means a player couldn't detonate his own trip mine, so we store the owner here.
};

BEGIN_DATADESC(	CTripmineGrenade )
	DEFINE_FIELD( m_flPowerUp, FIELD_TIME ),
	DEFINE_FIELD( m_vecDir, FIELD_VECTOR ),
	DEFINE_FIELD( m_vecEnd, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_flBeamLength, FIELD_FLOAT ),
	DEFINE_FIELD( m_hOwner, FIELD_EHANDLE ),
	DEFINE_FIELD( m_pBeam, FIELD_CLASSPTR ),
	DEFINE_FIELD( m_posOwner, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_angleOwner, FIELD_VECTOR ),
	DEFINE_FIELD( m_pRealOwner, FIELD_EDICT ),
	DEFINE_THINKFUNC( WarningThink ),
	DEFINE_THINKFUNC( PowerupThink ),
	DEFINE_THINKFUNC( BeamBreakThink ),
	DEFINE_THINKFUNC( DelayDeathThink ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( monster_tripmine, CTripmineGrenade );

void CTripmineGrenade :: Spawn( void )
{
	Precache( );
	// motor
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_NOT;

	SetModel( "models/v_tripmine.mdl");
	pev->frame = 0;
	pev->body = 3;
	pev->sequence = TRIPMINE_WORLD;
	ResetSequenceInfo( );
	pev->framerate = 0;
	
	SetSize( Vector( -8, -8, -8), Vector(8, 8, 8) );
	SetAbsOrigin( GetAbsOrigin() );

	if (pev->spawnflags & 1)
	{
		// power up quickly
		m_flPowerUp = gpGlobals->time + 1.0;
	}
	else
	{
		// power up in 2.5 seconds
		m_flPowerUp = gpGlobals->time + 2.5;
	}

	SetThink( &CTripmineGrenade::PowerupThink );
	pev->nextthink = gpGlobals->time + 0.2;

	pev->takedamage = DAMAGE_YES;
	pev->dmg = gSkillData.GetPlrDmgTripmine();
	pev->health = 1; // don't let die normally

	if (pev->owner != NULL)
	{
		// play deploy sound
		EMIT_SOUND( this, CHAN_VOICE, "weapons/mine_deploy.wav", 1.0, ATTN_NORM );
		EMIT_SOUND( this, CHAN_BODY, "weapons/mine_charge.wav", 0.2, ATTN_NORM ); // chargeup

		m_pRealOwner = pev->owner;// see CTripmineGrenade for why.
	}

	UTIL_MakeAimVectors( pev->angles );

	m_vecDir = gpGlobals->v_forward;
	m_vecEnd = GetAbsOrigin() + m_vecDir * 2048;
}


void CTripmineGrenade :: Precache( void )
{
	PRECACHE_MODEL("models/v_tripmine.mdl");
	PRECACHE_SOUND("weapons/mine_deploy.wav");
	PRECACHE_SOUND("weapons/mine_activate.wav");
	PRECACHE_SOUND("weapons/mine_charge.wav");
}

void CTripmineGrenade::UpdateOnRemove()
{
	BaseClass::UpdateOnRemove();

	KillBeam();
}

void CTripmineGrenade :: WarningThink( void  )
{
	// play warning sound
	// EMIT_SOUND( this, CHAN_VOICE, "buttons/Blip2.wav", 1.0, ATTN_NORM );

	// set to power up
	SetThink( &CTripmineGrenade::PowerupThink );
	pev->nextthink = gpGlobals->time + 1.0;
}


void CTripmineGrenade :: PowerupThink( void  )
{
	TraceResult tr;

	if (m_hOwner == NULL)
	{
		// find an owner
		edict_t *oldowner = pev->owner;
		pev->owner = NULL;
		UTIL_TraceLine( GetAbsOrigin() + m_vecDir * 8, GetAbsOrigin() - m_vecDir * 32, dont_ignore_monsters, ENT( pev ), &tr );
		if (tr.fStartSolid || (oldowner && tr.pHit == oldowner))
		{
			pev->owner = oldowner;
			m_flPowerUp += 0.1;
			pev->nextthink = gpGlobals->time + 0.1;
			return;
		}
		if (tr.flFraction < 1.0)
		{
			pev->owner = tr.pHit;
			m_hOwner = CBaseEntity::Instance( pev->owner );
			m_posOwner = m_hOwner->GetAbsOrigin();
			m_angleOwner = m_hOwner->pev->angles;
		}
		else
		{
			STOP_SOUND( this, CHAN_VOICE, "weapons/mine_deploy.wav" );
			STOP_SOUND( this, CHAN_BODY, "weapons/mine_charge.wav" );
			SetThink(&CTripmineGrenade::SUB_Remove );
			pev->nextthink = gpGlobals->time + 0.1;
			ALERT( at_console, "WARNING:Tripmine at %.0f, %.0f, %.0f removed\n", GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z );
			KillBeam();
			return;
		}
	}
	else if (m_posOwner != m_hOwner->GetAbsOrigin() || m_angleOwner != m_hOwner->pev->angles)
	{
		// disable
		STOP_SOUND( this, CHAN_VOICE, "weapons/mine_deploy.wav" );
		STOP_SOUND( this, CHAN_BODY, "weapons/mine_charge.wav" );
		CBaseEntity *pMine = Create( "weapon_tripmine", GetAbsOrigin() + m_vecDir * 24, pev->angles );
		pMine->pev->spawnflags |= SF_NORESPAWN;

		SetThink( &CTripmineGrenade::SUB_Remove );
		KillBeam();
		pev->nextthink = gpGlobals->time + 0.1;
		return;
	}
	// ALERT( at_console, "%d %.0f %.0f %0.f\n", pev->owner, m_pOwner->GetAbsOrigin().x, m_pOwner->GetAbsOrigin().y, m_pOwner->GetAbsOrigin().z );
 
	if (gpGlobals->time > m_flPowerUp)
	{
		// make solid
		pev->solid = SOLID_BBOX;
		SetAbsOrigin( GetAbsOrigin() );

		MakeBeam( );

		// play enabled sound
        EMIT_SOUND_DYN( this, CHAN_VOICE, "weapons/mine_activate.wav", 0.5, ATTN_NORM, 1.0, 75 );
	}
	pev->nextthink = gpGlobals->time + 0.1;
}


void CTripmineGrenade :: KillBeam( void )
{
	if ( m_pBeam )
	{
		UTIL_Remove( m_pBeam );
		m_pBeam = NULL;
	}
}


void CTripmineGrenade :: MakeBeam( void )
{
	TraceResult tr;

	// ALERT( at_console, "serverflags %f\n", gpGlobals->serverflags );

	UTIL_TraceLine( GetAbsOrigin(), m_vecEnd, dont_ignore_monsters, ENT( pev ), &tr );

	m_flBeamLength = tr.flFraction;

	// set to follow laser spot
	SetThink( &CTripmineGrenade::BeamBreakThink );
	pev->nextthink = gpGlobals->time + 0.1;

	Vector vecTmpEnd = GetAbsOrigin() + m_vecDir * 2048 * m_flBeamLength;

	m_pBeam = CBeam::BeamCreate( g_pModelNameLaser, 10 );
	m_pBeam->PointEntInit( vecTmpEnd, entindex() );
	m_pBeam->SetColor( 0, 214, 198 );
	m_pBeam->SetScrollRate( 255 );
	m_pBeam->SetBrightness( 64 );
}


void CTripmineGrenade :: BeamBreakThink( void  )
{
	bool bBlowup = false;

	TraceResult tr;

	// HACKHACK Set simple box using this really nice global!
	gpGlobals->trace_flags = FTRACE_SIMPLEBOX;
	UTIL_TraceLine( GetAbsOrigin(), m_vecEnd, dont_ignore_monsters, ENT( pev ), &tr );

	// ALERT( at_console, "%f : %f\n", tr.flFraction, m_flBeamLength );

	// respawn detect. 
	if ( !m_pBeam )
	{
		MakeBeam( );
		if ( tr.pHit )
			m_hOwner = CBaseEntity::Instance( tr.pHit );	// reset owner too
	}

	if (fabs( m_flBeamLength - tr.flFraction ) > 0.001)
	{
		bBlowup = true;
	}
	else
	{
		if (m_hOwner == NULL)
			bBlowup = true;
		else if (m_posOwner != m_hOwner->GetAbsOrigin())
			bBlowup = true;
		else if (m_angleOwner != m_hOwner->pev->angles)
			bBlowup = true;
	}

	if (bBlowup)
	{
		// a bit of a hack, but all CGrenade code passes pev->owner along to make sure the proper player gets credit for the kill
		// so we have to restore pev->owner from pRealOwner, because an entity's tracelines don't strike it's pev->owner which meant
		// that a player couldn't trigger his own tripmine. Now that the mine is exploding, it's safe the restore the owner so the 
		// CGrenade code knows who the explosive really belongs to.
		pev->owner = m_pRealOwner;
		pev->health = 0;
		Killed( CTakeDamageInfo( Instance( pev->owner ), 0, 0 ), GIB_NORMAL );
		return;
	}

	pev->nextthink = gpGlobals->time + 0.1;
}

void CTripmineGrenade::OnTakeDamage( const CTakeDamageInfo& info )
{
	if (gpGlobals->time < m_flPowerUp && info.GetDamage() < pev->health)
	{
		// disable
		// Create( "weapon_tripmine", GetAbsOrigin() + m_vecDir * 24, pev->angles );
		SetThink( &CTripmineGrenade::SUB_Remove );
		pev->nextthink = gpGlobals->time + 0.1;
		KillBeam();
		return;
	}
	CGrenade::OnTakeDamage( info );
}

void CTripmineGrenade::Killed( const CTakeDamageInfo& info, GibAction gibAction )
{
	pev->takedamage = DAMAGE_NO;
	
	if ( info.GetAttacker() && ( info.GetAttacker()->pev->flags & FL_CLIENT ) )
	{
		// some client has destroyed this mine, he'll get credit for any kills
		SetOwner( info.GetAttacker() );
	}

	SetThink( &CTripmineGrenade::DelayDeathThink );
	pev->nextthink = gpGlobals->time + RANDOM_FLOAT( 0.1, 0.3 );

	EMIT_SOUND( this, CHAN_BODY, "common/null.wav", 0.5, ATTN_NORM ); // shut off chargeup
}


void CTripmineGrenade::DelayDeathThink( void )
{
	KillBeam();
	TraceResult tr;
	UTIL_TraceLine ( GetAbsOrigin() + m_vecDir * 8, GetAbsOrigin() - m_vecDir * 64,  dont_ignore_monsters, ENT(pev), & tr);

	Explode( &tr, DMG_BLAST );
}
#endif

LINK_ENTITY_TO_CLASS( weapon_tripmine, CTripmine );

CTripmine::CTripmine()
	: BaseClass( WEAPON_TRIPMINE )
{
}

void CTripmine::Spawn( )
{
	Precache( );
	SetModel( "models/v_tripmine.mdl");
	pev->frame = 0;
	pev->body = 3;
	pev->sequence = TRIPMINE_GROUND;
	// ResetSequenceInfo( );
	pev->framerate = 0;

	FallInit();// get ready to fall down

	if ( !bIsMultiplayer() )
	{
		SetSize( Vector(-16, -16, 0), Vector(16, 16, 28) ); 
	}
}

void CTripmine::Precache( void )
{
	BaseClass::Precache();

	PRECACHE_MODEL ("models/v_tripmine.mdl");
	PRECACHE_MODEL ("models/p_tripmine.mdl");
	UTIL_PrecacheOther( "monster_tripmine" );

	m_usTripFire = PRECACHE_EVENT( 1, "events/tripfire.sc" );
}

bool CTripmine::Deploy()
{
	//pev->body = 0;
	return DefaultDeploy( "models/v_tripmine.mdl", "models/p_tripmine.mdl", TRIPMINE_DRAW, "trip" );
}


void CTripmine::Holster()
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;

	if (!m_pPlayer->m_rgAmmo[ PrimaryAmmoIndex() ])
	{
		// out of mines
		m_pPlayer->pev->weapons &= ~(1<<m_iId);
		SetThink( &CTripmine::DestroyItem );
		pev->nextthink = gpGlobals->time + 0.1;
	}

	SendWeaponAnim( TRIPMINE_HOLSTER );
	EMIT_SOUND( m_pPlayer, CHAN_WEAPON, "common/null.wav", 1.0, ATTN_NORM);
}

void CTripmine::PrimaryAttack( void )
{
	if (m_pPlayer->m_rgAmmo[ PrimaryAmmoIndex() ] <= 0)
		return;

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = gpGlobals->v_forward;

	TraceResult tr;

	UTIL_TraceLine( vecSrc, vecSrc + vecAiming * 128, dont_ignore_monsters, ENT( m_pPlayer->pev ), &tr );

	int flags;
#ifdef CLIENT_WEAPONS
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usTripFire, 0.0, g_vecZero, g_vecZero, 0.0, 0.0, 0, 0, 0, 0 );

	if (tr.flFraction < 1.0)
	{
		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );
		if ( pEntity && !(pEntity->pev->flags & FL_CONVEYOR) )
		{
			Vector angles = UTIL_VecToAngles( tr.vecPlaneNormal );

			CBaseEntity *pEnt = CBaseEntity::Create( "monster_tripmine", tr.vecEndPos + tr.vecPlaneNormal * 8, angles, m_pPlayer->edict() );

			m_pPlayer->m_rgAmmo[ PrimaryAmmoIndex() ]--;

			// player "shoot" animation
			m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
			
			if ( m_pPlayer->m_rgAmmo[ PrimaryAmmoIndex() ] <= 0 )
			{
				// no more mines! 
				RetireWeapon();
				return;
			}
		}
		else
		{
			// ALERT( at_console, "no deploy\n" );
		}
	}
	else
	{

	}
	
	m_flNextPrimaryAttack = GetNextAttackDelay(0.3);
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}

void CTripmine::WeaponIdle( void )
{
	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	if ( m_pPlayer->m_rgAmmo[ PrimaryAmmoIndex() ] > 0 )
	{
		SendWeaponAnim( TRIPMINE_DRAW );
	}
	else
	{
		RetireWeapon(); 
		return;
	}

	int iAnim;
	float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0, 1 );
	if (flRand <= 0.25)
	{
		iAnim = TRIPMINE_IDLE1;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 90.0 / 30.0;
	}
	else if (flRand <= 0.75)
	{
		iAnim = TRIPMINE_IDLE2;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 60.0 / 30.0;
	}
	else
	{
		iAnim = TRIPMINE_FIDGET;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 100.0 / 30.0;
	}

	SendWeaponAnim( iAnim );
}




