#include "cbase.h"


#include "zmr_player_shared.h"

#ifdef CLIENT_DLL
#include "prediction.h"

#include "zmr/c_zmr_player.h"
#include "zmr/npcs/c_zmr_zombiebase.h"
#else
#include "zmr/zmr_player.h"
#include "zmr/npcs/zmr_zombiebase.h"
#endif


#ifdef CLIENT_DLL
//#define CZMPlayer C_ZMPlayer
#define CZMBaseZombie C_ZMBaseZombie


extern ConVar zm_cl_participation;
#endif

extern ConVar zm_sv_resource_max;


bool CZMPlayer::HasEnoughResToSpawn( ZombieClass_t zclass )
{
    return GetResources() >= CZMBaseZombie::GetCost( zclass );
}

bool CZMPlayer::HasEnoughRes( int cost )
{
    return GetResources() >= cost;
}

int CZMPlayer::GetWeaponSlotFlags()
{
    return m_ZMLocal.m_fWeaponSlotFlags;
}

int CZMPlayer::GetResources()
{
    return m_ZMLocal.m_nResources;
}

void CZMPlayer::IncResources( int res, bool bLimit )
{
#ifdef CLIENT_DLL

#else
    int oldres = GetResources();
    int newres = oldres + res;
    int max = zm_sv_resource_max.GetInt();


    if ( bLimit && newres > max )
    {
        if ( oldres < max )
            newres = max;
        else
            return;
    }

    SetResources( newres );
#endif
}

void CZMPlayer::SetResources( int res )
{
#ifdef CLIENT_DLL

#else
    if ( res < 0 ) res = 0;

    m_ZMLocal.m_nResources = res;
#endif
}

float CZMPlayer::GetFlashlightBattery()
{
    return m_ZMLocal.m_flFlashlightBattery;
}

void CZMPlayer::SetFlashlightBattery( float battery )
{
#ifdef CLIENT_DLL

#else
    m_ZMLocal.m_flFlashlightBattery = battery;
#endif
}

// This + AllowsAutoSwitchFrom was causing problems with switching to an empty weapon.
bool CZMPlayer::Weapon_CanSwitchTo( CBaseCombatWeapon *pWeapon )
{
    if ( !IsAlive() )
        return false;

    if ( !pWeapon->CanDeploy() )
        return false;
    

    CBaseCombatWeapon* pActive = GetActiveWeapon();
    if ( pActive && !pActive->CanHolster() )
    {
        return false;
    }

    return true;
}

Participation_t CZMPlayer::GetParticipation()
{
    Participation_t part;
#ifdef CLIENT_DLL
    part = (Participation_t)zm_cl_participation.GetInt();
#else
    part =  (Participation_t)atoi( engine->GetClientConVarValue( entindex(), "zm_cl_participation" ) );
#endif

    if ( part <= ZMPART_INVALID || part >= ZMPART_MAX ) part = ZMPART_ALLOWZM;

    return part;
}

#ifdef CLIENT_DLL
Participation_t CZMPlayer::GetLocalParticipation()
{
    Participation_t part = (Participation_t)zm_cl_participation.GetInt();

    if ( part <= ZMPART_INVALID || part >= ZMPART_MAX ) part = ZMPART_ALLOWZM;

    return part;
}

void CZMPlayer::SetLocalParticipation( Participation_t part )
{
    zm_cl_participation.SetValue( (int)part );
}
#endif


Vector CZMPlayer::GetAttackSpread( CBaseCombatWeapon *pWeapon, CBaseEntity *pTarget )
{
	if ( pWeapon )
		return pWeapon->GetBulletSpread( WEAPON_PROFICIENCY_PERFECT );
	
	return VECTOR_CONE_15DEGREES;
}

Vector CZMPlayer::GetAutoaimVector( float flScale )
{
    Vector fwd;
    AngleVectors( EyeAngles() + m_Local.m_vecPunchAngle, &fwd );

    return fwd;
}

// Play normal footsteps instead of HL2DM ones.
void CZMPlayer::PlayStepSound( Vector& vecOrigin, surfacedata_t* psurface, float fvol, bool force )
{
    CBasePlayer::PlayStepSound( vecOrigin, psurface, fvol, force );
}

// Shared version of Weapon_GetWpnForAmmo.
CBaseCombatWeapon* CZMPlayer::GetWeaponForAmmo( int iAmmoType )
{
    for ( int i = 0; i < MAX_WEAPONS; i++ )
    {
        CBaseCombatWeapon* pWep = GetWeapon( i );

        if ( !pWep ) continue;


        if ( pWep->GetPrimaryAmmoType() == iAmmoType )
            return pWep;
    }
    
    return nullptr;
}

#ifndef CLIENT_DLL
void TE_PlayerAnimEvent( CBasePlayer* pPlayer, PlayerAnimEvent_t playerAnim, int nData );
#endif

void CZMPlayer::DoAnimationEvent( PlayerAnimEvent_t playerAnim, int nData )
{
#ifdef CLIENT_DLL
	if ( IsLocalPlayer() )
	{
		if ( ( prediction->InPrediction() && !prediction->IsFirstTimePredicted() ) )
			return;
	}

	MDLCACHE_CRITICAL_SECTION();
#endif

    m_pPlayerAnimState->DoAnimationEvent( playerAnim, nData );

#ifndef CLIENT_DLL
    TE_PlayerAnimEvent( this, playerAnim, nData );
#endif
}
