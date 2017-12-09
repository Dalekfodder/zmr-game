#include "cbase.h"

//#include "hl2/npc_BaseZombie.h"

#include "func_break.h"
#include "IEffects.h"
#include "envspark.h"


#include "npcs/zmr_zombiebase.h"
#include "npcs/zmr_fastzombie.h"
#include "zmr/zmr_global_shared.h"
#include "zmr/zmr_util.h"

#include "zmr_player.h"
#include "zmr_entities.h"


/*
    Move selected zombies.
*/
void ZM_Cmd_Move( const CCommand &args )
{
    CZMPlayer* pPlayer = ToZMPlayer( UTIL_GetCommandClient() );

    if ( !pPlayer ) return;
    
    if ( !pPlayer->IsZM() ) return;

    if ( args.ArgC() < 4 ) return;

    
    Vector pos;
    pos.x = atof( args.Arg( 1 ) );
    pos.y = atof( args.Arg( 2 ) );
    pos.z = atof( args.Arg( 3 ) );
    

    float tolerance = atof( args.Arg( 4 ) );


    CZMBaseZombie* pZombie;

    for ( int i = 0; i < g_pZombies->Count(); i++ )
    {
        pZombie = g_pZombies->Element( i );

        if ( pZombie && pZombie->GetSelector() == pPlayer )
        {
            pZombie->Command( pos, true, tolerance );
        }
    }
}

static ConCommand zm_cmd_move( "zm_cmd_move", ZM_Cmd_Move, "Move da zombies!", FCVAR_HIDDEN );


/*
    Swat objects / target players.
*/
void ZM_Cmd_Target( const CCommand &args )
{
    CZMPlayer* pPlayer = ToZMPlayer( UTIL_GetCommandClient() );

    if ( !pPlayer ) return;
    
    if ( !pPlayer->IsZM() ) return;



    CBaseEntity* pTarget = UTIL_EntityByIndex( atoi( args.Arg( 1 ) ) );


    if ( !pTarget ) return;


    // ZM wants to break it.
    bool bForceBreakable = atoi( args.Arg( 2 ) ) ? true : false;


    bool bTarget = false; // Just target an enemy.
    bool bSwat = false; // Just swat the object away/towards an enemy.
    bool bBreak = false; // Tell zombie to break this object.
    bool bCanBeDamaged = pTarget->GetHealth() > 0 && pTarget->m_takedamage == DAMAGE_YES;

    if ( pTarget->IsPlayer() )
    {
        bTarget = true;
    }
    else
    {
        // If we're a physics object then swat.
        IPhysicsObject* phys = pTarget->VPhysicsGetObject();

        bSwat = phys && phys->IsMoveable();


        if ( bCanBeDamaged )
        {
            // If we're breakable (func_breakable, etc.) then force to break.
            // If we're a physics object and we can't be moved, force to break.
            CBreakable* pBreak = dynamic_cast<CBreakable*>( pTarget );

            bBreak = (phys && !phys->IsMotionEnabled()) || (pBreak && pBreak->IsBreakable());
        }
    }


    Vector pos;
    pos.x = atof( args.Arg( 3 ) );
    pos.y = atof( args.Arg( 4 ) );
    pos.z = atof( args.Arg( 5 ) );
    

    CZMBaseZombie* pZombie;
    
    for ( int i = 0; i < g_pZombies->Count(); i++ )
    {
        pZombie = g_pZombies->Element( i );

        if ( !pZombie ) continue;
        
        if ( pZombie->GetSelector() != pPlayer ) continue;


        if ( (pZombie->CanSwatPhysicsObjects() && bSwat) || bBreak )
        {
            // If we're allowed to swat and we're forced to break it, break it.
            bool bForcedBreak = pZombie->CanSwatPhysicsObjects() && bForceBreakable;

            pZombie->Swat( pTarget, bCanBeDamaged && (bBreak || bForcedBreak) );
        }
        else
        {
            if ( bTarget )
            {
                pZombie->TargetEnemy( pTarget );
            }
            else
            {
                pZombie->Command( pos );
            }
        }
    }
}

static ConCommand zm_cmd_target( "zm_cmd_target", ZM_Cmd_Target, "Selected zombies target given entity.", FCVAR_HIDDEN );


ConVar zm_sv_cost_hiddenshambler( "zm_sv_cost_hiddenshambler", "100", FCVAR_NOTIFY );

void ZM_Cmd_CreateHidden( const CCommand &args )
{
    CZMPlayer* pPlayer = ToZMPlayer( UTIL_GetCommandClient() );

    if ( !pPlayer ) return;
    
    if ( !UTIL_IsCommandIssuedByServerAdmin() && !pPlayer->IsZM() ) return;

    if ( args.ArgC() < 4 ) return;
    

    if ( !CZMBaseZombie::HasEnoughPopToSpawn( ZMCLASS_SHAMBLER ) )
    {
        ZMUtil::PrintNotify( pPlayer, ZMCHATNOTIFY_ZM, "#ZMNotEnoughPop" );
        return;
    }
        

    if ( !pPlayer->HasEnoughRes( zm_sv_cost_hiddenshambler.GetInt() ) )
    {
        ZMUtil::PrintNotify( pPlayer, ZMCHATNOTIFY_ZM, "#ZMNotEnoughRes" );
        return;
    }


    Vector pos;
    trace_t trace;

    pos.x = atof( args.Arg( 1 ) );
    pos.y = atof( args.Arg( 2 ) );
    pos.z = atof( args.Arg( 3 ) ) + 1.0f;


    CZMEntTriggerBlockHidden* pBlock;
    for ( int i = 0; i < g_pBlockHidden->Count(); i++ )
    {
        pBlock = g_pBlockHidden->Element( i );

        if (pBlock && pBlock->IsActive()
        &&  pBlock->CollisionProp()
        &&  pBlock->CollisionProp()->IsPointInBounds( pos ) )
        {
            ZMUtil::PrintNotify( pPlayer, ZMCHATNOTIFY_ZM, "#ZMHiddenCreateBlocked" );
            return;
        }
    }

    bool bVisible = false;


    for ( int i = 1; i <= gpGlobals->maxClients; i++ )
    {
        CZMPlayer* pPlayer = ToZMPlayer( UTIL_PlayerByIndex( i ) );

        if ( !pPlayer ) continue;

        if ( !pPlayer->IsHuman() ) continue;

        if ( !pPlayer->IsAlive() ) continue;


        // ZMRTODO: Throw this somewhere else.
        UTIL_TraceLine( pPlayer->EyePosition(), pos, MASK_VISIBLE, pPlayer, COLLISION_GROUP_NONE, &trace );

        if ( trace.fraction == 1.0f )
        {
            bVisible = true;
            break;
        }

        
        UTIL_TraceLine( pPlayer->EyePosition(), pos + Vector( 0.0f, 0.0f, 64.0f ), MASK_VISIBLE, pPlayer, COLLISION_GROUP_NONE, &trace );

        if ( trace.fraction == 1.0f )
        {
            bVisible = true;
            break;
        }
    }

    if ( bVisible )
    {
        ZMUtil::PrintNotify( pPlayer, ZMCHATNOTIFY_ZM, "#ZMHiddenCreateHumanSee" );
        return;
    }




    CZMBaseZombie* pZombie = dynamic_cast<CZMBaseZombie*>( CreateEntityByName( CZMBaseZombie::ClassToName( ZMCLASS_SHAMBLER ) ) );

    if ( !pZombie ) return;


    DispatchSpawn( pZombie );
    
    const Vector findground( 0.0f, 0.0f, 2.0f );

    // Trace down to get ground position.
    UTIL_TraceHull(
        pos + Vector( 0.0f, 0.0f, 8.0f ),
        pos - findground,
        pZombie->GetHullMins(), pZombie->GetHullMaxs(),
        MASK_NPCSOLID, pZombie, COLLISION_GROUP_NONE, &trace );
    
    const Vector up = Vector( 0.0f, 0.0f, 1.0f );
    const float dot = DotProduct( up, trace.plane.normal );

    if (trace.startsolid || trace.fraction == 1.0f
    ||  (trace.fraction != 1.0f && dot < 0.6f)) // Is our spawn location ground?
    {
        ZMUtil::PrintNotify( pPlayer, ZMCHATNOTIFY_ZM, "#ZMInvalidSpot" );

        pZombie->SUB_Remove();

        return;
    }

    
    pZombie->Teleport( &trace.endpos, nullptr, nullptr );

    // Face away from ZM.
    QAngle ang = pPlayer->GetAbsAngles();
    ang.x = ang.z = 0.0f;
    pZombie->SetAbsAngles( ang );

    pZombie->Activate();

    pPlayer->IncResources( -zm_sv_cost_hiddenshambler.GetInt() );
}

static ConCommand zm_cmd_createhidden( "zm_cmd_createhidden", ZM_Cmd_CreateHidden, "Create da zombies!", FCVAR_HIDDEN );


void ZM_Cmd_Queue( const CCommand &args )
{
    CZMPlayer* pPlayer = ToZMPlayer( UTIL_GetCommandClient() );

    if ( !pPlayer ) return;
    
    if ( !pPlayer->IsZM() ) return;

    if ( args.ArgC() < 3 ) return;


    CZMEntZombieSpawn* pSpawn = dynamic_cast<CZMEntZombieSpawn*>( UTIL_EntityByIndex( atoi( args.Arg( 1 ) ) ) );
    if ( !pSpawn ) return;


    ZombieClass_t zclass = static_cast<ZombieClass_t>( atoi( args.Arg( 2 ) ) );

    if ( !CZMBaseZombie::IsValidClass( zclass ) )
    {
        zclass = ZMCLASS_SHAMBLER;
    }


    int amount = atoi( args.Arg( 3 ) );
    if ( amount < 1 ) amount = 1;


    pSpawn->QueueUnit( pPlayer, zclass, amount );
}

static ConCommand zm_cmd_queue( "zm_cmd_queue", ZM_Cmd_Queue, "Usage: zm_cmd_queue <entindex> <number> | Create da zombies!", FCVAR_HIDDEN );


/*
    Clear zombie spawn queue
*/
void ZM_Cmd_QueueClear( const CCommand &args )
{
    CZMPlayer* pPlayer = ToZMPlayer( UTIL_GetCommandClient() );

    if ( !pPlayer ) return;
    
    if ( !pPlayer->IsZM() ) return;

    if ( args.ArgC() < 2 ) return;


    CZMEntZombieSpawn* pSpawn = dynamic_cast<CZMEntZombieSpawn*>( UTIL_EntityByIndex( atoi( args.Arg( 1 ) ) ) );

    if ( !pSpawn ) return;


    int amount = atoi( args.Arg( 2 ) );

    int pos = atoi( args.Arg( 3 ) );
    if ( pos < 1 ) pos = -1; // Last?
    else --pos;

    pSpawn->QueueClear( amount, pos );
}

static ConCommand zm_cmd_queueclear( "zm_cmd_queueclear", ZM_Cmd_QueueClear, "Usage: zm_cmd_queueclear <entindex> <amount (def: 1)> <pos (def: last)>", FCVAR_HIDDEN );




void ZM_Cmd_SelectAll( const CCommand &args )
{
    CZMPlayer* pPlayer = ToZMPlayer( UTIL_GetCommandClient() );

    if ( !pPlayer ) return;
    
    if ( !pPlayer->IsZM() ) return;

    // No zombies to select...
    if ( g_pZombies->Count() < 1 ) return;


    int entindex = pPlayer->entindex();

    CZMBaseZombie* pZombie;
    for ( int i = 0; i < g_pZombies->Count(); i++ )
    {
        pZombie = g_pZombies->Element( i );

        if ( pZombie )
        {
            pZombie->SetSelector( entindex );
        }
    }

    ZMUtil::PrintNotify( pPlayer, ZMCHATNOTIFY_ZM, "#ZMSelectAll" );
}

static ConCommand zm_cmd_selectall( "zm_cmd_selectall", ZM_Cmd_SelectAll, "Select all da zombies!" );


void ZM_Cmd_UnSelectAll( const CCommand &args )
{
    CZMPlayer* pPlayer = ToZMPlayer( UTIL_GetCommandClient() );

    if ( !pPlayer ) return;

    if ( !pPlayer->IsZM() )
    {
        return;
    }


    pPlayer->DeselectAllZombies();
}

static ConCommand zm_cmd_unselectall( "zm_cmd_unselectall", ZM_Cmd_UnSelectAll, "Unselect all da zombies!", FCVAR_HIDDEN );


void ZM_Cmd_Select( const CCommand &args )
{
    CZMPlayer* pPlayer = ToZMPlayer( UTIL_GetCommandClient() );

    if ( !pPlayer ) return;

    if ( !pPlayer->IsZM() ) return;


    CBaseEntity* pEnt = UTIL_EntityByIndex( atoi( args.Arg( 1 ) ) );

    if ( !pEnt ) return;


    // Testing
    CZMEntZombieSpawn* pSpawn = dynamic_cast<CZMEntZombieSpawn*>( pEnt );

    if ( pSpawn )
    {
        pSpawn->QueueUnit( pPlayer, ZMCLASS_SHAMBLER, 1 );
        return;
    }


    CZMBaseZombie* pZombie = dynamic_cast<CZMBaseZombie*>( pEnt );

    if ( pZombie )
    {
        if ( !atoi( args.Arg( 2 ) ) ) // Sticky?
        {
            pPlayer->DeselectAllZombies();
        }

        pZombie->SetSelector( pPlayer );
        return;
    }
}

static ConCommand zm_cmd_select( "zm_cmd_select", ZM_Cmd_Select, "Select spawns/traps/zombies, etc.", FCVAR_HIDDEN );



void ZM_Cmd_SelectMult( const CCommand &args )
{
    CZMPlayer* pPlayer = ToZMPlayer( UTIL_GetCommandClient() );

    if ( !pPlayer ) return;

    if ( !pPlayer->IsZM() ) return;

    if ( args.ArgC() < 3 ) return;
    
    bool bSticky = atoi( args.Arg( 1 ) ) ? true : false;


    if ( !bSticky )
    {
        pPlayer->DeselectAllZombies();
    }


    CZMBaseZombie* pZombie;

    for ( int i = 2; i < args.ArgC(); i++ )
    {
        pZombie = dynamic_cast<CZMBaseZombie*>( UTIL_EntityByIndex( atoi( args.Arg( i ) ) ) );

        if ( pZombie )
        {
            pZombie->SetSelector( pPlayer );
        }
    }
}

static ConCommand zm_cmd_selectmult( "zm_cmd_selectmult", ZM_Cmd_SelectMult, "Select multiple zombies.", FCVAR_HIDDEN );


static ConVar zm_sv_gibdelete( "zm_sv_gibdelete", "1", FCVAR_NOTIFY, "Does deleting zombies gib them?" );

void ZM_Cmd_DeleteZombies( const CCommand &args )
{
    CZMPlayer* pPlayer = ToZMPlayer( UTIL_GetCommandClient() );

    if ( !pPlayer ) return;

    if ( !pPlayer->IsZM() ) return;


    CTakeDamageInfo dmg;

    
    int dmgtype = zm_sv_gibdelete.GetBool() ? DMG_ALWAYSGIB : DMG_GENERIC;


    CZMBaseZombie* pZombie;
    for ( int i = 0; i < g_pZombies->Count(); i++ )
    {
        pZombie = g_pZombies->Element( i );

        if ( pZombie && pZombie->GetSelector() == pPlayer )
        {
            dmg.SetDamageType( dmgtype );

            dmg.SetAttacker( pPlayer );
            dmg.SetInflictor( pZombie );

            dmg.SetDamage( pZombie->GetHealth() * 2 );
            dmg.SetDamagePosition( pZombie->GetAbsOrigin() );
            dmg.SetDamageForce( Vector( 0, 0, -10 ) );


            pZombie->TakeDamage( dmg );
        }
    }
}

static ConCommand zm_cmd_delete( "zm_cmd_delete", ZM_Cmd_DeleteZombies, "Delete selected zombies." );

/*
    Trigger a trap.
*/
void ZM_Cmd_Trigger( const CCommand &args )
{
    CZMPlayer* pPlayer = ToZMPlayer( UTIL_GetCommandClient() );

    if ( !pPlayer ) return;
    
    if ( !pPlayer->IsZM() ) return;

    if ( args.ArgC() < 2 ) return;


    int entindex = atoi( args.Arg( 1 ) );

    if ( entindex < 1 ) return;


    CZMEntManipulate* pTrap = dynamic_cast<CZMEntManipulate*>( UTIL_EntityByIndex( entindex ) );

    if ( pTrap )
    {
        pTrap->Trigger( pPlayer );
    }
}

static ConCommand zm_cmd_trigger( "zm_cmd_trigger", ZM_Cmd_Trigger, "Trigger a trap.", FCVAR_HIDDEN );

/*
    Create trap trigger
*/
static ConVar zm_sv_maxtraptriggers( "zm_sv_maxtraptriggers", "3", FCVAR_NOTIFY | FCVAR_ARCHIVE, "How many triggers can the ZM create for one trap?" );

void ZM_Cmd_CreateTrigger( const CCommand &args )
{
    CZMPlayer* pPlayer = ToZMPlayer( UTIL_GetCommandClient() );

    if ( !pPlayer ) return;
    
    if ( !pPlayer->IsZM() ) return;

    if ( args.ArgC() < 5 ) return;


    int entindex = atoi( args.Arg( 1 ) );

    if ( entindex < 1 ) return;

    
    Vector pos;
    pos.x = atof( args.Arg( 2 ) );
    pos.y = atof( args.Arg( 3 ) );
    pos.z = atof( args.Arg( 4 ) );
    

    CZMEntManipulate* pTrap = dynamic_cast<CZMEntManipulate*>( UTIL_EntityByIndex( entindex ) );

    if ( pTrap )
    {
        if ( !pPlayer->HasEnoughRes( pTrap->GetTrapCost() ) )
        {
            ZMUtil::PrintNotify( pPlayer, ZMCHATNOTIFY_ZM, "#ZMNotEnoughRes" );
            return;
        }

        if ( pTrap->GetTriggerCount() >= zm_sv_maxtraptriggers.GetInt() )
        {
            ZMUtil::PrintNotify( pPlayer, ZMCHATNOTIFY_ZM, "#ZMMaxTrapTriggers" );
            return;
        }


        pTrap->CreateTrigger( pos );

        
        pPlayer->IncResources( -pTrap->GetTrapCost() );

        ZMUtil::PrintNotify( pPlayer, ZMCHATNOTIFY_ZM, "#ZMCreatedTrap" );
    }
}

static ConCommand zm_cmd_createtrigger( "zm_cmd_createtrigger", ZM_Cmd_CreateTrigger, "Create a trap trigger.", FCVAR_HIDDEN );


/*
    Create zombie spawn rally point
*/
void ZM_Cmd_SetRally( const CCommand &args )
{
    CZMPlayer* pPlayer = ToZMPlayer( UTIL_GetCommandClient() );

    if ( !pPlayer ) return;
    
    if ( !pPlayer->IsZM() ) return;

    if ( args.ArgC() < 5 ) return;


    int entindex = atoi( args.Arg( 1 ) );

    if ( entindex < 1 ) return;

    
    Vector pos;
    pos.x = atof( args.Arg( 2 ) );
    pos.y = atof( args.Arg( 3 ) );
    pos.z = atof( args.Arg( 4 ) );
    

    CZMEntZombieSpawn* pSpawn = dynamic_cast<CZMEntZombieSpawn*>( UTIL_EntityByIndex( entindex ) );

    if ( pSpawn )
    {
        pSpawn->SetRallyPoint( pos );

        ZMUtil::PrintNotify( pPlayer, ZMCHATNOTIFY_ZM, "#ZMCreatedRally" );
    }
}

static ConCommand zm_cmd_setrally( "zm_cmd_setrally", ZM_Cmd_SetRally, "Set rally point.", FCVAR_HIDDEN );


/*
    Open build menu
*/
void ZM_Cmd_OpenBuildMenu( const CCommand &args )
{
    CZMPlayer* pPlayer = ToZMPlayer( UTIL_GetCommandClient() );

    if ( !pPlayer ) return;
    
    if ( !pPlayer->IsZM() ) return;

    if ( args.ArgC() < 2 ) return;


    int entindex = atoi( args.Arg( 1 ) );

    if ( entindex < 1 ) return;



    CZMEntZombieSpawn* pSpawn = dynamic_cast<CZMEntZombieSpawn*>( UTIL_EntityByIndex( entindex ) );

    if ( pSpawn )
    {
        pPlayer->SetBuildSpawn( pSpawn );
        pSpawn->SendMenuUpdate();
    }
}

static ConCommand zm_cmd_openbuildmenu( "zm_cmd_openbuildmenu", ZM_Cmd_OpenBuildMenu, "", FCVAR_HIDDEN );


void ZM_Cmd_CloseBuildMenu( const CCommand &args )
{
    CZMPlayer* pPlayer = ToZMPlayer( UTIL_GetCommandClient() );

    if ( !pPlayer ) return;
    
    if ( !pPlayer->IsZM() ) return;


    pPlayer->SetBuildSpawn( 0 );
}

static ConCommand zm_cmd_closebuildmenu( "zm_cmd_closebuildmenu", ZM_Cmd_CloseBuildMenu, "", FCVAR_HIDDEN );



/*
    Open manipulate menu
*/
void ZM_Cmd_OpenManiMenu( const CCommand &args )
{
    CZMPlayer* pPlayer = ToZMPlayer( UTIL_GetCommandClient() );

    if ( !pPlayer ) return;
    
    if ( !pPlayer->IsZM() ) return;

    if ( args.ArgC() < 2 ) return;


    int entindex = atoi( args.Arg( 1 ) );

    if ( entindex < 1 ) return;



    CZMEntManipulate* pTrap = dynamic_cast<CZMEntManipulate*>( UTIL_EntityByIndex( entindex ) );

    if ( pTrap && pTrap->GetDescription()[0] )
    {
		CSingleUserRecipientFilter filter( pPlayer );
		filter.MakeReliable();


		UserMessageBegin( filter, "ZMManiMenuUpdate" );
			WRITE_STRING( pTrap->GetDescription() );
		MessageEnd();
    }
}

static ConCommand zm_cmd_openmanimenu( "zm_cmd_openmanimenu", ZM_Cmd_OpenManiMenu, "", FCVAR_HIDDEN );


/*
    Open manipulate menu
*/
static ConVar zm_sv_physexp_cost( "zm_sv_physexp_cost", "400", FCVAR_NOTIFY | FCVAR_ARCHIVE );
static ConVar zm_sv_physexp_radius( "zm_sv_physexp_radius", "222", FCVAR_NOTIFY | FCVAR_ARCHIVE );
static ConVar zm_sv_physexp_magnitude( "zm_sv_physexp_magnitude", "17500", FCVAR_NOTIFY | FCVAR_ARCHIVE );
static ConVar zm_sv_physexp_delay( "zm_sv_physexp_delay", "7.4", FCVAR_NOTIFY | FCVAR_ARCHIVE );

void ZM_Cmd_PhysExp( const CCommand &args )
{
    CZMPlayer* pPlayer = ToZMPlayer( UTIL_GetCommandClient() );

    if ( !pPlayer ) return;
    
    if ( !pPlayer->IsZM() ) return;

    if ( args.ArgC() < 4 ) return;

    if ( !pPlayer->HasEnoughRes( zm_sv_physexp_cost.GetInt() ) )
    {
        ZMUtil::PrintNotify( pPlayer, ZMCHATNOTIFY_ZM, "#ZMNotEnoughRes" );
        return;
    }

    Vector pos;
    pos.x = atof( args.Arg( 1 ) );
    pos.y = atof( args.Arg( 2 ) );
    pos.z = atof( args.Arg( 3 ) ) + 1.0f;

    if ( UTIL_PointContents( pos ) & CONTENTS_SOLID )
    {
        ZMUtil::PrintNotify( pPlayer, ZMCHATNOTIFY_ZM, "#ZMPhysExpBlocked" );
        return;
    }


    CZMEntTriggerBlockPhysExp* pBlock;
    for ( int i = 0; i < g_pBlockPhysExp->Count(); i++ )
    {
        pBlock = g_pBlockPhysExp->Element( i );

        if (pBlock && pBlock->IsActive()
        &&  pBlock->CollisionProp()
        &&  pBlock->CollisionProp()->IsPointInBounds( pos ) )
        {
            ZMUtil::PrintNotify( pPlayer, ZMCHATNOTIFY_ZM, "#ZMPhysExpBlocked" );
            return;
        }
    }


    CZMPhysExplosion* pExp = dynamic_cast<CZMPhysExplosion*>( CreateEntityByName( "env_delayed_physexplosion" ) );

    if ( !pExp ) return;


    pExp->KeyValue( "magnitude", zm_sv_physexp_magnitude.GetString() );
    pExp->KeyValue( "radius", zm_sv_physexp_radius.GetString() );


#define SF_PHYSEXPLOSION_NODAMAGE           0x0001
#define SF_PHYSEXPLOSION_DISORIENT_PLAYER   0x0010

    pExp->AddSpawnFlags( SF_PHYSEXPLOSION_NODAMAGE );
    pExp->AddSpawnFlags( SF_PHYSEXPLOSION_DISORIENT_PLAYER );


    if ( DispatchSpawn( pExp ) != 0 )
        return;

    pExp->Teleport( &pos, nullptr, nullptr );
    pExp->Activate();
    pExp->DelayedExplode( zm_sv_physexp_delay.GetFloat() );


    pPlayer->IncResources( -zm_sv_physexp_cost.GetInt() );


    ZMUtil::PrintNotify( pPlayer, ZMCHATNOTIFY_ZM, "#ZMCreatedPhysExp" );
}

static ConCommand zm_cmd_physexp( "zm_cmd_physexp", ZM_Cmd_PhysExp, "", FCVAR_HIDDEN );


/*
    Set selected zombies' mode
*/
void ZM_Cmd_SetZombieMode( const CCommand &args )
{
    CZMPlayer* pPlayer = ToZMPlayer( UTIL_GetCommandClient() );

    if ( !pPlayer ) return;
    
    if ( !pPlayer->IsZM() ) return;

    if ( args.ArgC() < 2 ) return;


    ZombieMode_t mode = (ZombieMode_t)atoi( args.Arg( 1 ) );

    if ( mode <= ZOMBIEMODE_INVALID || mode >= ZOMBIEMODE_MAX )
        return;

    CZMBaseZombie* pZombie;
    for ( int i = 0; i < g_pZombies->Count(); i++ )
    {
        pZombie = g_pZombies->Element( i );

        if ( pZombie && pZombie->GetSelector() == pPlayer )
        {
            pZombie->SetZombieMode( mode );
        }
    }


    ZMUtil::PrintNotify( pPlayer, ZMCHATNOTIFY_ZM,
        ( mode == ZOMBIEMODE_OFFENSIVE ) ? "#ZMSetZombieModeOff" : "#ZMSetZombieModeDef" );
}

static ConCommand zm_cmd_zombiemode( "zm_cmd_zombiemode", ZM_Cmd_SetZombieMode, "" );


/*
    Set banshee ceiling cling.
*/
void ZM_Cmd_SetBansheeCeil( const CCommand &args )
{
    CZMPlayer* pPlayer = ToZMPlayer( UTIL_GetCommandClient() );

    if ( !pPlayer ) return;
    
    if ( !pPlayer->IsZM() ) return;


    bool bSet = false;

    CZMBaseZombie* pZombie;
    for ( int i = 0; i < g_pZombies->Count(); i++ )
    {
        pZombie = g_pZombies->Element( i );

        if ( !pZombie ) continue;

        if ( pZombie->GetSelector() != pPlayer && pZombie->GetZombieClass() != ZMCLASS_BANSHEE )
            continue;


        if (!pZombie->IsCurSchedule( SCHED_FASTZOMBIE_CEILING_JUMP ) &&
            !pZombie->IsCurSchedule( SCHED_FASTZOMBIE_CEILING_CLING ))
        {
            pZombie->SetSchedule( SCHED_FASTZOMBIE_CEILING_JUMP );

            bSet = true;
        }
    }

    ZMUtil::PrintNotify( pPlayer, ZMCHATNOTIFY_ZM, bSet ? "#ZMBansheeCeilSet" : "#ZMBansheeCeilNoSel" );
}

static ConCommand zm_cmd_bansheeceiling( "zm_cmd_bansheeceiling", ZM_Cmd_SetBansheeCeil, "" );


/*
    Set zombie ambush.
*/
void ZM_Cmd_CreateAmbush( const CCommand &args )
{
    CZMPlayer* pPlayer = ToZMPlayer( UTIL_GetCommandClient() );

    if ( !pPlayer ) return;
    
    if ( !pPlayer->IsZM() ) return;

    if ( args.ArgC() < 4 ) return;

    if ( !ZMUtil::GetSelectedZombieCount( pPlayer->entindex() ) )
        return;
    

    Vector pos;
    pos.x = atof( args.Arg( 1 ) );
    pos.y = atof( args.Arg( 2 ) );
    pos.z = atof( args.Arg( 3 ) );

    if ( UTIL_PointContents( pos ) & CONTENTS_SOLID )
        return;


    CZMEntAmbushTrigger* pTrigger = dynamic_cast<CZMEntAmbushTrigger*>( CreateEntityByName( "info_ambush_trigger" ) );

    if ( !pTrigger ) return;

    if ( DispatchSpawn( pTrigger ) != 0 )
        return;


    pTrigger->Teleport( &pos, nullptr, nullptr );
    pTrigger->Activate();


    int count = 0;

    CZMBaseZombie* pZombie;
    for ( int i = 0; i < g_pZombies->Count(); i++ )
    {
        pZombie = g_pZombies->Element( i );

        if ( !pZombie ) continue;

        if ( pZombie->GetSelector() != pPlayer )
            continue;


        ++count;


        pZombie->RemoveFromAmbush( false );

        pZombie->SetAmbush( pTrigger );
    }


    pTrigger->SetAmbushZombies( count );



    ZMUtil::PrintNotify( pPlayer, ZMCHATNOTIFY_ZM, "#ZMSetZombieModeAmb" );
}

static ConCommand zm_cmd_createambush( "zm_cmd_createambush", ZM_Cmd_CreateAmbush, "" );
