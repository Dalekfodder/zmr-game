#pragma once


#include "zmr/zmr_shareddefs.h"


namespace ZMUtil
{
    int GetSelectedZombieCount( int iPlayerIndex );

    void PrintNotify( CBasePlayer* pPlayer, ZMChatNotifyType_t type, const char* msg );
    void PrintNotifyAll( ZMChatNotifyType_t type, const char* msg );
    void SendNotify( IRecipientFilter& filter, ZMChatNotifyType_t type, const char* msg );
};
