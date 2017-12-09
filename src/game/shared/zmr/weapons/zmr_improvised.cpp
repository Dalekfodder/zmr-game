#include "cbase.h"

#include "in_buttons.h"


#include "zmr/zmr_shareddefs.h"
#include "zmr/zmr_player_shared.h"

#include "zmr_basemelee.h"


#ifdef CLIENT_DLL
#define CZMWeaponImprovised C_ZMWeaponImprovised
#endif

class CZMWeaponImprovised : public CZMBaseMeleeWeapon
{
public:
	DECLARE_CLASS( CZMWeaponImprovised, CZMBaseMeleeWeapon );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();

    CZMWeaponImprovised();


    void ItemPostFrame( void ) OVERRIDE;

    float GetRange() OVERRIDE { return 50.0f; };
    float GetFireRate() OVERRIDE { return 1.0f; };
    float GetDamageForActivity( Activity hitActivity ) OVERRIDE { return 20.0f; };

    void AddViewKick() OVERRIDE
    {
        CZMPlayer* pPlayer = GetPlayerOwner();
        if ( !pPlayer ) return;
	    pPlayer->ViewPunch( QAngle( random->RandomFloat( 1.0f, 2.0f ), random->RandomFloat( -2.0f, -1.0f ), 0.0f ) );
    }
};

IMPLEMENT_NETWORKCLASS_ALIASED( ZMWeaponImprovised, DT_ZM_WeaponImprovised )

BEGIN_NETWORK_TABLE( CZMWeaponImprovised, DT_ZM_WeaponImprovised )
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CZMWeaponImprovised )
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_zm_improvised, CZMWeaponImprovised );
PRECACHE_WEAPON_REGISTER( weapon_zm_improvised );

acttable_t	CZMWeaponImprovised::m_acttable[] = 
{
    /*
    { ACT_RANGE_ATTACK1,				ACT_RANGE_ATTACK_SLAM,                  true },
    { ACT_HL2MP_IDLE,					ACT_HL2MP_IDLE_MELEE,					false },
    { ACT_HL2MP_RUN,					ACT_HL2MP_RUN_MELEE,					false },
    { ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_MELEE,			false },
    { ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_MELEE,			false },
    { ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_MELEE,	false },
    { ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_MELEE,			false },
    { ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_MELEE,					false },
    */
    { ACT_MP_STAND_IDLE,				ACT_HL2MP_IDLE_MELEE,                   false },
    { ACT_MP_CROUCH_IDLE,				ACT_HL2MP_IDLE_CROUCH_MELEE,			false },
    { ACT_MP_RUN,					    ACT_HL2MP_RUN_MELEE,					false },
    { ACT_MP_CROUCHWALK,			    ACT_HL2MP_WALK_CROUCH_MELEE,			false },
    { ACT_MP_ATTACK_STAND_PRIMARYFIRE,  ACT_HL2MP_GESTURE_RANGE_ATTACK_MELEE,   false },
    { ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,	ACT_HL2MP_GESTURE_RANGE_ATTACK_MELEE,	false },
    { ACT_MP_RELOAD_STAND,			    ACT_HL2MP_GESTURE_RELOAD_MELEE,			false },
    { ACT_MP_RELOAD_CROUCH,				ACT_HL2MP_GESTURE_RELOAD_MELEE,			false },
    { ACT_MP_JUMP,					    ACT_HL2MP_JUMP_MELEE,					false },
};
IMPLEMENT_ACTTABLE( CZMWeaponImprovised );

CZMWeaponImprovised::CZMWeaponImprovised()
{
    SetSlotFlag( ZMWEAPONSLOT_MELEE );
}

void CZMWeaponImprovised::ItemPostFrame()
{
    CZMPlayer* pPlayer = GetPlayerOwner();
    if ( !pPlayer ) return;


    if ( pPlayer->m_nButtons & IN_ATTACK && m_flNextPrimaryAttack <= gpGlobals->curtime )
    {
        // You fucking idiots. The crowbar doesn't fire any animation events. Thanks.
        Swing( false, false );
    }
    else
    {
        WeaponIdle();
    }
}
