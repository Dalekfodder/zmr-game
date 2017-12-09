#pragma once

//#include "hl2mp/hl2mp_player_shared.h"
#include "basecombatweapon_shared.h"
//#include "hl2mp/weapon_hl2mpbase.h"
//#include "hl2mp/hl2mp_weapon_parse.h"


#include "zmr/zmr_weapon_parse.h"

#include "zmr/zmr_player_shared.h"



#ifdef CLIENT_DLL
#define CZMBaseWeapon C_ZMBaseWeapon
#endif

/*
    NOTE:

    You must add custom animation events in eventlist.h && eventlist.cpp
*/

class CZMBaseWeapon : public CBaseCombatWeapon
{
public:
	DECLARE_CLASS( CZMBaseWeapon, CBaseCombatWeapon );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
    DECLARE_DATADESC();

	CZMBaseWeapon();
	~CZMBaseWeapon();
    
#ifdef CLIENT_DLL
    virtual void Spawn() OVERRIDE;
#endif
    virtual bool Reload() OVERRIDE;
    // NOTE: Always use this to get the damage from .txt file.
    virtual void FireBullets( const FireBulletsInfo_t &info ) OVERRIDE;
    virtual void PrimaryAttack() OVERRIDE;
    virtual void SecondaryAttack() OVERRIDE;
    
    const CZMWeaponInfo& GetWpnData() const;

#ifdef CLIENT_DLL
    virtual void    ClientThink() OVERRIDE;
    virtual void    GetGlowEffectColor( float& r, float& g, float& b ) OVERRIDE;
    void            UpdateGlow();

    virtual bool    GlowOccluded() OVERRIDE { return false; };
    virtual bool    GlowUnoccluded() OVERRIDE { return true; };


    virtual void OnDataChanged( DataUpdateType_t ) OVERRIDE;
    virtual bool ShouldPredict() OVERRIDE;
#endif
    virtual bool IsPredicted() const OVERRIDE { return true; };

    void WeaponSound( WeaponSound_t, float soundtime = 0.0f ) OVERRIDE;

    void DoMachineGunKick( float, float, float, float );


    virtual int GetMinBurst( void ) OVERRIDE { return 1; };
    virtual int GetMaxBurst( void ) OVERRIDE { return 1; };
    
#ifndef CLIENT_DLL
    virtual void Materialize( void ) OVERRIDE;
#endif
    // Makes our weapons not cry about spawning.
    virtual void FallInit( void );
    // Override this so our guns don't disappear.
    virtual void SetPickupTouch( void ) OVERRIDE;
    virtual bool CanBeSelected( void ) OVERRIDE;
    // Never let anybody tell you're not beautiful even without any ammo, alright?
    // Let us always select this weapon even when we don't have any ammo for it.
    virtual bool AllowsAutoSwitchFrom( void ) const OVERRIDE { return false; };
    virtual void Drop( const Vector& ) OVERRIDE;

    // Add weapon slot flag.
    virtual void Equip( CBaseCombatCharacter* ) OVERRIDE;


    // Viewmodel/misc stuff
    float   CalcViewmodelBob( void ) OVERRIDE;
    void    AddViewmodelBob( CBaseViewModel*, Vector&, QAngle& ) OVERRIDE;
    Vector  GetBulletSpread( WeaponProficiency_t ) OVERRIDE;
    float   GetSpreadBias( WeaponProficiency_t ) OVERRIDE;

    const WeaponProficiencyInfo_t*          GetProficiencyValues() OVERRIDE;
    static const WeaponProficiencyInfo_t*   GetDefaultProficiencyValues();
    
    // Our stuff
    CZMPlayer*      GetPlayerOwner();
    virtual bool    CanBeDropped() { return true; };
    virtual bool    CanPickupAmmo() { return true; };
    virtual bool    IsInReload() { return CanReload() && m_bInReload; };
    virtual bool    CanAct(); // Can we reload/attack?


    inline int GetSlotFlag() { return m_iSlotFlag; };
#ifndef CLIENT_DLL
    void                FreeWeaponSlot();


    virtual const char* GetDropAmmoName() { return nullptr; };
    virtual int         GetDropAmmoAmount() { return 1; };

    inline int          GetReserveAmmo( ) { return m_nReserveAmmo; };
    inline void         SetReserveAmmo( int ammo ) { m_nReserveAmmo = ammo; };
#endif
    
protected:
#ifndef CLIENT_DLL
    void SaveReserveAmmo( CBaseCombatCharacter* );
    void TransferReserveAmmo( CBaseCombatCharacter* );
    // No support for secondary ammo since we'll never use it anyway, RIGHT?
    int m_nReserveAmmo;
#endif

    inline void SetSlotFlag( int flags ) { m_iSlotFlag = flags; };
    int m_iSlotFlag;
};
