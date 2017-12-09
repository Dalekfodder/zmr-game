#include "cbase.h"

#include "zmr_playerlocaldata.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifndef CLIENT_DLL
BEGIN_SEND_TABLE_NOBASE( CZMPlayerLocalData, DT_ZM_PlyLocal )
    SendPropInt( SENDINFO( m_fWeaponSlotFlags ), -1, SPROP_UNSIGNED ),
    SendPropInt( SENDINFO( m_nResources ), -1, SPROP_UNSIGNED ),
    SendPropFloat( SENDINFO( m_flFlashlightBattery ), 10, SPROP_UNSIGNED | SPROP_ROUNDUP, 0.0f, 100.0f ),
END_SEND_TABLE()
#else
BEGIN_RECV_TABLE_NOBASE( CZMPlayerLocalData, DT_ZM_PlyLocal )
    RecvPropInt( RECVINFO( m_fWeaponSlotFlags ), SPROP_UNSIGNED ),
    RecvPropInt( RECVINFO( m_nResources ), SPROP_UNSIGNED ),
    RecvPropFloat( RECVINFO( m_flFlashlightBattery ), SPROP_UNSIGNED | SPROP_ROUNDUP ),
END_RECV_TABLE()
#endif

CZMPlayerLocalData::CZMPlayerLocalData()
{
    m_fWeaponSlotFlags = 0;
    m_flFlashlightBattery = 100.0f;
    m_nResources = 0;
}
