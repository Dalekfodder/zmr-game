#include "cbase.h"

#include "baseviewport.h"
#include "clienteffectprecachesystem.h"
#include "iviewrender_beams.h"
#include "dlight.h"
#include "r_efx.h"
#include "takedamageinfo.h"
#include "view.h"
#include "bone_setup.h"


#include "zmr/ui/zmr_viewport.h"

#include "npcs/c_zmr_zombiebase.h"
#include "zmr/zmr_global_shared.h"
#include "c_zmr_entities.h"
#include "c_zmr_player_ragdoll.h"

#include "c_zmr_player.h"
#include "c_zmr_zmvision.h"


extern bool g_bRenderPostProcess;


// ZMRTODO: Replace these
#define MAT_HPCIRCLE        "effects/zm_healthring"
#define MAT_INNERFLARE      "effects/yellowflare"

CLIENTEFFECT_REGISTER_BEGIN( PrecacheEffectZMPlayerEffect )
CLIENTEFFECT_MATERIAL( MAT_HPCIRCLE )
CLIENTEFFECT_MATERIAL( MAT_INNERFLARE )
CLIENTEFFECT_REGISTER_END()


ConVar zm_cl_participation( "zm_cl_participation", "0", FCVAR_USERINFO | FCVAR_ARCHIVE, "Your participation setting. 0 = Want to be ZM, 1 = Only human, 2 = Only spectator" );


ConVar zm_cl_mwheelmove( "zm_cl_mwheelmove", "1", FCVAR_ARCHIVE, "As the ZM, can you move up/down with mousewheel?" );
ConVar zm_cl_mwheelmovereverse( "zm_cl_mwheelmovereverse", "1", FCVAR_ARCHIVE, "Is mousewheel scrolling reversed?" );
ConVar zm_cl_mwheelmovespd( "zm_cl_mwheelmovespd", "400", FCVAR_ARCHIVE );



#undef CZMPlayer // We need to undefine it so we can get the server class.
IMPLEMENT_CLIENTCLASS_DT( C_ZMPlayer, DT_ZM_Player, CZMPlayer )
    RecvPropDataTable( RECVINFO_DT( m_ZMLocal ), 0, &REFERENCE_RECV_TABLE( DT_ZM_PlyLocal ) ),

    RecvPropFloat( RECVINFO( m_angEyeAngles[0] ) ),
    RecvPropFloat( RECVINFO( m_angEyeAngles[1] ) ),
    RecvPropInt( RECVINFO( m_iSpawnInterpCounter ) ),
    RecvPropEHandle( RECVINFO( m_hRagdoll ) ),
END_RECV_TABLE()

BEGIN_PREDICTION_DATA( C_ZMPlayer )
    DEFINE_PRED_FIELD( m_flCycle, FIELD_FLOAT, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
    //DEFINE_PRED_FIELD( m_fIsWalking, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
    DEFINE_PRED_FIELD( m_nSequence, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
    DEFINE_PRED_FIELD( m_flPlaybackRate, FIELD_FLOAT, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
    DEFINE_PRED_ARRAY_TOL( m_flEncodedController, FIELD_FLOAT, MAXSTUDIOBONECTRLS, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE, 0.02f ),
    DEFINE_PRED_FIELD( m_nNewSequenceParity, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),

    DEFINE_PRED_FIELD( m_blinktoggle, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
    DEFINE_PRED_FIELD( m_viewtarget, FIELD_VECTOR, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
END_PREDICTION_DATA()


CZMPlayerAnimState* CreateZMPlayerAnimState( C_ZMPlayer* pPlayer );

C_ZMPlayer::C_ZMPlayer() : m_iv_angEyeAngles( "C_ZMPlayer::m_iv_angEyeAngles" )
{
    AddVar( &m_angEyeAngles, &m_iv_angEyeAngles, LATCH_SIMULATION_VAR );

    //m_EntClientFlags |= ENTCLIENTFLAG_DONTUSEIK;

    m_pFlashlightBeam = NULL;
    m_iIDEntIndex = 0;
    m_iSpawnInterpCounterCache = 0;


    m_pPlayerAnimState = CreateZMPlayerAnimState( this );



    m_flNextUpMove = 0.0f;
    m_flUpMove = 0.0f;

    m_fxHealth = new CZMCharCircle();
    m_fxHealth->SetYaw( 0.0f );
    m_fxHealth->SetMaterial( MAT_HPCIRCLE );
    m_fxHealth->SetSize( 16.0f );


    m_fxInner = new CZMCharCircle();
    m_fxInner->SetYaw( 0.0f );
    m_fxInner->SetColor( 1.0f, 1.0f, 1.0f );
    m_fxInner->SetAlpha( 0.8f );
    m_fxInner->SetMaterial( MAT_INNERFLARE );
    m_fxInner->SetSize( 22.0f );
}

C_ZMPlayer::~C_ZMPlayer()
{
    ReleaseFlashlight();

    m_pPlayerAnimState->Release();


    delete m_fxHealth;
    delete m_fxInner;

    g_ZMVision.RemoveSilhouette( this );
}

void C_ZMPlayer::Spawn()
{
    BaseClass::Spawn();

    if ( !IsLocalPlayer() )
        g_ZMVision.AddSilhouette( this );
}

C_ZMPlayer* C_ZMPlayer::GetLocalPlayer()
{
    return static_cast<C_ZMPlayer*>( C_BasePlayer::GetLocalPlayer() );
}

void C_ZMPlayer::ClientThink()
{
    UpdateIDTarget();
}

void C_ZMPlayer::PreThink()
{
    // Don't let base class change our max speed (sprinting)
    float spd = MaxSpeed();

    BaseClass::PreThink();

    if ( MaxSpeed() != spd )
        SetMaxSpeed( spd );

    //HandleSpeedChanges();

    //if ( m_HL2Local.m_flSuitPower <= 0.0f )
    //{
    //    if( IsSprinting() )
    //    {
    //        StopSprinting();
    //    }
    //}
}

void C_ZMPlayer::UpdateClientSideAnimation()
{
    m_pPlayerAnimState->Update( EyeAngles()[YAW], EyeAngles()[PITCH] );

    BaseClass::UpdateClientSideAnimation();
}

void C_ZMPlayer::CalculateIKLocks( float currentTime )
{
    if ( !m_pIk ) 
        return;

    int targetCount = m_pIk->m_target.Count();
    if ( targetCount == 0 )
        return;

    // In TF, we might be attaching a player's view to a walking model that's using IK. If we are, it can
    // get in here during the view setup code, and it's not normally supposed to be able to access the spatial
    // partition that early in the rendering loop. So we allow access right here for that special case.
    SpatialPartitionListMask_t curSuppressed = partition->GetSuppressedLists();
    partition->SuppressLists( PARTITION_ALL_CLIENT_EDICTS, false );
    C_BaseEntity::PushEnableAbsRecomputations( false );

    for ( int i = 0; i < targetCount; i++ )
    {
        trace_t trace;
        CIKTarget *pTarget = &m_pIk->m_target[i];

        if ( !pTarget->IsActive() )
            continue;

        switch( pTarget->type )
        {
        case IK_GROUND:
            {
                pTarget->SetPos( Vector( pTarget->est.pos.x, pTarget->est.pos.y, GetRenderOrigin().z ));
                pTarget->SetAngles( GetRenderAngles() );
            }
            break;

        case IK_ATTACHMENT:
            {
                C_BaseEntity *pEntity = NULL;
                float flDist = pTarget->est.radius;

                // FIXME: make entity finding sticky!
                // FIXME: what should the radius check be?
                for ( CEntitySphereQuery sphere( pTarget->est.pos, 64 ); ( pEntity = sphere.GetCurrentEntity() ) != NULL; sphere.NextEntity() )
                {
                    C_BaseAnimating* pAnim = pEntity->GetBaseAnimating();
                    if ( !pAnim )
                        continue;

                    int iAttachment = pAnim->LookupAttachment( pTarget->offset.pAttachmentName );
                    if ( iAttachment <= 0 )
                        continue;

                    Vector origin;
                    QAngle angles;
                    pAnim->GetAttachment( iAttachment, origin, angles );

                    // debugoverlay->AddBoxOverlay( origin, Vector( -1, -1, -1 ), Vector( 1, 1, 1 ), QAngle( 0, 0, 0 ), 255, 0, 0, 0, 0 );

                    float d = (pTarget->est.pos - origin).Length();

                    if ( d >= flDist )
                        continue;

                    flDist = d;
                    pTarget->SetPos( origin );
                    pTarget->SetAngles( angles );
                    // debugoverlay->AddBoxOverlay( pTarget->est.pos, Vector( -pTarget->est.radius, -pTarget->est.radius, -pTarget->est.radius ), Vector( pTarget->est.radius, pTarget->est.radius, pTarget->est.radius), QAngle( 0, 0, 0 ), 0, 255, 0, 0, 0 );
                }

                if ( flDist >= pTarget->est.radius )
                {
                    // debugoverlay->AddBoxOverlay( pTarget->est.pos, Vector( -pTarget->est.radius, -pTarget->est.radius, -pTarget->est.radius ), Vector( pTarget->est.radius, pTarget->est.radius, pTarget->est.radius), QAngle( 0, 0, 0 ), 0, 0, 255, 0, 0 );
                    // no solution, disable ik rule
                    pTarget->IKFailed();
                }
            }
            break;
        }
    }

    C_BaseEntity::PopEnableAbsRecomputations();
    partition->SuppressLists( curSuppressed, true );
}

void C_ZMPlayer::TeamChange( int iNewTeam )
{
    // ZMRTODO: Test if there are any cases when TeamChange isn't fired!!!
    BaseClass::TeamChange( iNewTeam );


    // Update ZM entities' visibility.
    // How visibility works is when player enters a leaf that can see given entity, ShouldDraw is fired.
    // However, if the player changes their team and they remain in the same leaf, the previous ShouldDraw is "active".
    // This makes sure we get updated on our ZM entities when changing teams, so orbs get drawn when changing to ZM and not drawn when changing to human/spec.
    DevMsg( "Updating ZM entity visibility...\n" );

    // The team number hasn't been updated yet.
    int iOldTeam = GetTeamNumber();
    C_BaseEntity::ChangeTeam( iNewTeam );

    C_ZMEntBaseSimple* pZMEnt;
    for ( C_BaseEntity* pEnt = ClientEntityList().FirstBaseEntity(); pEnt; pEnt = ClientEntityList().NextBaseEntity( pEnt ) )
    {
        pZMEnt = dynamic_cast<C_ZMEntBaseSimple*>( pEnt );
        if ( pZMEnt )
        {
            pZMEnt->UpdateVisibility();
        }
    }


    // Turn off nightvision.
    g_ZMVision.TurnOff();


    // Reset back to old team just in case something uses it.
    C_BaseEntity::ChangeTeam( iOldTeam );
    


    if ( g_pZMView )
        g_pZMView->SetVisible( iNewTeam == ZMTEAM_ZM );


    // Execute team config.
    if ( iNewTeam == ZMTEAM_ZM )
    {
        ZMClientUtil::QueueTooltip( "zmintro", 1.0f );
        ZMClientUtil::QueueTooltip( "zmmoving", 12.0f );



        engine->ClientCmd( "exec zm.cfg" );
    }
    else if ( iNewTeam == ZMTEAM_HUMAN )
    {
        engine->ClientCmd( "exec survivor.cfg" );
    }
}

void C_ZMPlayer::Simulate()
{
    BaseClass::Simulate();


    if ( !IsLocalPlayer() )
    {
        if ( IsEffectActive( EF_DIMLIGHT ) )
        {
            if ( m_iAttachmentRH == -1 )
                return;

            Vector vecOrigin;
            QAngle eyeAngles;
    
            GetAttachment( m_iAttachmentRH, vecOrigin, eyeAngles );

            Vector vForward;
            AngleVectors( m_angEyeAngles, &vForward );
                
            trace_t tr;
            UTIL_TraceLine( vecOrigin, vecOrigin + (vForward * 200), MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );

            if ( !m_pFlashlightBeam )
            {
                BeamInfo_t beamInfo;
                beamInfo.m_nType = TE_BEAMPOINTS;
                beamInfo.m_vecStart = tr.startpos;
                beamInfo.m_vecEnd = tr.endpos;
                beamInfo.m_pszModelName = "sprites/glow01.vmt";
                beamInfo.m_pszHaloName = "sprites/glow01.vmt";
                beamInfo.m_flHaloScale = 3.0;
                beamInfo.m_flWidth = 8.0f;
                beamInfo.m_flEndWidth = 35.0f;
                beamInfo.m_flFadeLength = 300.0f;
                beamInfo.m_flAmplitude = 0;
                beamInfo.m_flBrightness = 60.0;
                beamInfo.m_flSpeed = 0.0f;
                beamInfo.m_nStartFrame = 0.0;
                beamInfo.m_flFrameRate = 0.0;
                beamInfo.m_flRed = 255.0;
                beamInfo.m_flGreen = 255.0;
                beamInfo.m_flBlue = 255.0;
                beamInfo.m_nSegments = 8;
                beamInfo.m_bRenderable = true;
                beamInfo.m_flLife = 0.5;
                beamInfo.m_nFlags = FBEAM_FOREVER | FBEAM_ONLYNOISEONCE | FBEAM_NOTILE | FBEAM_HALOBEAM;
                
                m_pFlashlightBeam = beams->CreateBeamPoints( beamInfo );
            }

            if ( m_pFlashlightBeam )
            {
                BeamInfo_t beamInfo;
                beamInfo.m_vecStart = tr.startpos;
                beamInfo.m_vecEnd = tr.endpos;
                beamInfo.m_flRed = 255.0;
                beamInfo.m_flGreen = 255.0;
                beamInfo.m_flBlue = 255.0;

                beams->UpdateBeamInfo( m_pFlashlightBeam, beamInfo );

                dlight_t *el = effects->CL_AllocDlight( 0 );
                el->origin = tr.endpos;
                el->radius = 50; 
                el->color.r = 200;
                el->color.g = 200;
                el->color.b = 200;
                el->die = gpGlobals->curtime + 0.1;
            }
        }
        else if ( m_pFlashlightBeam )
        {
            ReleaseFlashlight();
        }
    }
}

bool C_ZMPlayer::ShouldDraw()
{
    // ZMRTODO: See if this has any side-effects.
    if ( IsDormant() )
        return false;

    if ( IsZM() )
        return false;

    if ( !IsAlive() )
        return false;

    if ( IsLocalPlayer() && IsRagdoll() )
        return true;

    if ( IsRagdoll() )
        return false;

    return BaseClass::ShouldDraw();
}

int C_ZMPlayer::DrawModel( int flags )
{
    if ( !m_bReadyToDraw ) return 0;

    // Again, ShouldDraw is not enough.
    if ( IsZM() ) return 0;


    C_ZMPlayer* pPlayer = C_ZMPlayer::GetLocalPlayer();
    if ( !pPlayer || !pPlayer->IsZM() )
    {
        return BaseClass::DrawModel( flags );
    }


    if ( !g_bRenderPostProcess )
    {
        float ratio = (GetHealth() > 0 ? GetHealth() : 1) / 100.0f;

        float g = ratio;
        float r = 1.0f - g;


        if ( m_fxInner )
        {
            m_fxInner->SetPos( GetAbsOrigin() + Vector( 0.0f, 0.0f, 3.0f ) );
            m_fxInner->SetYaw( random->RandomFloat( 0.0f, 360.0f ) );
            m_fxInner->Draw();
        }


        if ( m_fxHealth )
        {
            m_fxHealth->SetAlpha( 0.5f );
            m_fxHealth->SetColor( r, g, 0 );
            m_fxHealth->SetPos( GetAbsOrigin() + Vector( 0.0f, 0.0f, 3.0f ) );
            m_fxHealth->Draw();
        }
    }


    return BaseClass::DrawModel( flags );
}

ShadowType_t C_ZMPlayer::ShadowCastType() 
{
    if ( !IsVisible() )
         return SHADOWS_NONE;

    return SHADOWS_RENDER_TO_TEXTURE_DYNAMIC;
}

bool C_ZMPlayer::ShouldReceiveProjectedTextures( int flags )
{
    Assert( flags & SHADOW_FLAGS_PROJECTED_TEXTURE_TYPE_MASK );

    if ( IsEffectActive( EF_NODRAW ) )
         return false;

    if( flags & SHADOW_FLAGS_FLASHLIGHT )
    {
        return true;
    }

    return BaseClass::ShouldReceiveProjectedTextures( flags );
}

const QAngle& C_ZMPlayer::GetRenderAngles()
{
    if ( IsRagdoll() )
    {
        return vec3_angle;
    }
    else
    {
        return m_pPlayerAnimState->GetRenderAngles();
    }
}

const QAngle& C_ZMPlayer::EyeAngles()
{
    return IsLocalPlayer() ? BaseClass::EyeAngles() : m_angEyeAngles;
}

float C_ZMPlayer::GetFOV()
{
    float fov = C_BasePlayer::GetFOV() + GetZoom();
    
    return MAX( GetMinFOV(), fov );
}

int C_ZMPlayer::GetIDTarget() const
{
    return m_iIDEntIndex;
}

void C_ZMPlayer::UpdateIDTarget()
{
    if ( !IsLocalPlayer() )
        return;

    // Clear old target and find a new one
    m_iIDEntIndex = 0;

    // don't show IDs in chase spec mode
    if ( GetObserverMode() == OBS_MODE_CHASE )
         return;

    C_BaseEntity* pIgnore = (IsObserver() && GetObserverTarget() && GetObserverMode() == OBS_MODE_IN_EYE) ? GetObserverTarget() : this;

    trace_t tr;
    Vector vecStart, vecEnd;
    VectorMA( MainViewOrigin(), 1500, MainViewForward(), vecEnd );
    VectorMA( MainViewOrigin(), 10, MainViewForward(), vecStart );
    UTIL_TraceLine(
        vecStart,
        vecEnd,
        MASK_SOLID,
        pIgnore,
        COLLISION_GROUP_NONE,
        &tr );

    if ( !tr.startsolid && tr.DidHitNonWorldEntity() )
    {
        C_BaseEntity* pEntity = tr.m_pEnt;

        if ( pEntity && pEntity != pIgnore )
        {
            m_iIDEntIndex = pEntity->entindex();
        }
    }
}

void C_ZMPlayer::TraceAttack( const CTakeDamageInfo& info, const Vector& vecDir, trace_t* ptr, CDmgAccumulator* pAccumulator )
{
    Vector vecOrigin = ptr->endpos - vecDir * 4;

    float flDistance = 0.0f;
    
    if ( info.GetAttacker() )
    {
        flDistance = (ptr->endpos - info.GetAttacker()->GetAbsOrigin()).Length();
    }

    if ( m_takedamage )
    {
        AddMultiDamage( info, this );

        int blood = BloodColor();
        
        C_BaseEntity* pAttacker = info.GetAttacker();

        if ( pAttacker && pAttacker->InSameTeam( this ) )
        {
            return;
        }

        if ( blood != DONT_BLEED )
        {
            UTIL_BloodDrips( vecOrigin, vecDir, blood, info.GetDamage() );// a little surface blood.
            TraceBleed( flDistance, vecDir, ptr, info.GetDamageType() );
        }
    }
}

void C_ZMPlayer::DoImpactEffect( trace_t& tr, int nDamageType )
{
    if ( GetActiveWeapon() )
    {
        GetActiveWeapon()->DoImpactEffect( tr, nDamageType );
        return;
    }

    BaseClass::DoImpactEffect( tr, nDamageType );
}

C_BaseAnimating* C_ZMPlayer::BecomeRagdollOnClient()
{
    return nullptr;
}

// ZMRTODO: When happy, set to 1.
ConVar zm_cl_firstperson_deathcam( "zm_cl_firstperson_deathcam", "0" );

void C_ZMPlayer::CalcView( Vector& eyeOrigin, QAngle& eyeAngles, float& zNear, float& zFar, float& fov )
{
    C_ZMPlayer* pPlayer = this;

    if (IsObserver()
    &&  GetObserverMode() == OBS_MODE_IN_EYE
    &&  GetObserverTarget()
    &&  GetObserverTarget()->IsPlayer() )
    {
        pPlayer = ToZMPlayer( GetObserverTarget() );
    }

    if ( pPlayer->m_lifeState != LIFE_ALIVE && !pPlayer->IsObserver() )
    {
        if ( zm_cl_firstperson_deathcam.GetBool() )
        {
            DeathCam_Firstperson( pPlayer, eyeOrigin, eyeAngles, zNear, zFar, fov );
        }
        else
        {
            DeathCam_Thirdperson( pPlayer, eyeOrigin, eyeAngles, zNear, zFar, fov );
        }

        return;
    }

    BaseClass::CalcView( eyeOrigin, eyeAngles, zNear, zFar, fov );
}

void C_ZMPlayer::DeathCam_Firstperson( C_ZMPlayer* pPlayer, Vector& eyeOrigin, QAngle& eyeAngles, float& zNear, float& zFar, float& fov )
{
    C_ZMRagdoll* pRagdoll = pPlayer->GetRagdoll();

    if ( !pRagdoll || pPlayer->GetEyeAttachment() == -1 )
        DeathCam_Thirdperson( pPlayer, eyeOrigin, eyeAngles, zNear, zFar, fov );


    pRagdoll->GetAttachment( pPlayer->GetEyeAttachment(), eyeOrigin, eyeAngles );
}

void C_ZMPlayer::DeathCam_Thirdperson( C_ZMPlayer* pPlayer, Vector& eyeOrigin, QAngle& eyeAngles, float& zNear, float& zFar, float& fov )
{
    Vector origin = pPlayer->EyePosition();			

    IRagdoll* pRagdoll = pPlayer->GetRepresentativeRagdoll();

    if ( pRagdoll )
    {
        origin = pRagdoll->GetRagdollOrigin();
        origin.z += VEC_DEAD_VIEWHEIGHT_SCALED( pPlayer ).z; // look over ragdoll, not through
    }

    BaseClass::CalcView( eyeOrigin, eyeAngles, zNear, zFar, fov );

    eyeOrigin = origin;
        
    Vector vForward; 
    AngleVectors( eyeAngles, &vForward );

    VectorNormalize( vForward );
    VectorMA( origin, -CHASE_CAM_DISTANCE_MAX, vForward, eyeOrigin );

    Vector WALL_MIN( -WALL_OFFSET, -WALL_OFFSET, -WALL_OFFSET );
    Vector WALL_MAX( WALL_OFFSET, WALL_OFFSET, WALL_OFFSET );

    trace_t trace; // clip against world
    C_BaseEntity::PushEnableAbsRecomputations( false ); // HACK don't recompute positions while doing RayTrace
    UTIL_TraceHull( origin, eyeOrigin, WALL_MIN, WALL_MAX, MASK_SOLID_BRUSHONLY, pPlayer, COLLISION_GROUP_NONE, &trace );
    C_BaseEntity::PopEnableAbsRecomputations();

    if ( trace.fraction < 1.0f )
    {
        eyeOrigin = trace.endpos;
    }
}

IRagdoll* C_ZMPlayer::GetRepresentativeRagdoll() const
{
    if ( GetRagdoll() )
    {
        return GetRagdoll()->GetIRagdoll();
    }
    else
    {
        return nullptr;
    }
}

C_ZMRagdoll* C_ZMPlayer::GetRagdoll() const
{
    return m_hRagdoll.Get();
}

int C_ZMPlayer::GetEyeAttachment() const
{
    return m_iAttachmentEyes;
}

CStudioHdr* C_ZMPlayer::OnNewModel( void )
{
    CStudioHdr* hdr = BaseClass::OnNewModel();
    
    Initialize();

    if ( m_pPlayerAnimState )
    {
        m_pPlayerAnimState->OnNewModel();
    }
    
    return hdr;
}

void C_ZMPlayer::Initialize()
{
    CStudioHdr *hdr = GetModelPtr();
    for ( int i = 0; i < hdr->GetNumPoseParameters() ; i++ )
    {
        SetPoseParameter( hdr, i, 0.0 );
    }


    // This is used for deathcam. Should be safe since we're using the same model as the ragdoll.
    m_iAttachmentEyes = LookupAttachment( "eyes" );
    m_iAttachmentRH = LookupAttachment( "anim_attachment_RH" );
}

bool C_ZMPlayer::ShouldInterpolate()
{
    // Always interpolate our observer target.
    // Mainly here for ZM observering being laggy.
    // ZMRTODO: See if this has any side-effects.
    if ( !IsLocalPlayer() )
    {
        C_ZMPlayer* pPlayer = GetLocalPlayer();

        if ( pPlayer && pPlayer->GetObserverTarget() == this )
        {
            return true;
        }
    }

    return BaseClass::ShouldInterpolate();
}

void C_ZMPlayer::NotifyShouldTransmit( ShouldTransmitState_t state )
{
    if ( state == SHOULDTRANSMIT_END )
    {
        if ( m_pFlashlightBeam != NULL )
        {
            ReleaseFlashlight();
        }
    }

    BaseClass::NotifyShouldTransmit( state );
}

void C_ZMPlayer::OnDataChanged( DataUpdateType_t type )
{
    BaseClass::OnDataChanged( type );

    if ( type == DATA_UPDATE_CREATED )
    {
        SetNextClientThink( CLIENT_THINK_ALWAYS );
    }

    UpdateVisibility();
}

void C_ZMPlayer::PostDataUpdate( DataUpdateType_t updateType )
{
    if ( m_iSpawnInterpCounter != m_iSpawnInterpCounterCache )
    {
        MoveToLastReceivedPosition( true );
        ResetLatched();
        m_iSpawnInterpCounterCache = m_iSpawnInterpCounter;
    }

    BaseClass::PostDataUpdate( updateType );
}

void C_ZMPlayer::ReleaseFlashlight()
{
    if ( m_pFlashlightBeam )
    {
        m_pFlashlightBeam->flags = 0;
        m_pFlashlightBeam->die = gpGlobals->curtime - 1;

        m_pFlashlightBeam = NULL;
    }
}

bool C_ZMPlayer::CreateMove( float delta, CUserCmd* cmd )
{
    bool bResult = BaseClass::CreateMove( delta, cmd );
    

    if ( m_flNextUpMove > gpGlobals->curtime )
    {
        cmd->upmove += m_flUpMove;
    }

    return bResult;
}

void C_ZMPlayer::SetMouseWheelMove( float dir )
{
    if ( !zm_cl_mwheelmove.GetBool() ) return;

    if ( dir == 0.0f ) return;

    if ( m_flNextUpMove > gpGlobals->curtime )
        return;

    if ( zm_cl_mwheelmovereverse.GetBool() )
        dir *= -1.0f;

    m_flNextUpMove = gpGlobals->curtime + 0.1f;
    m_flUpMove = zm_cl_mwheelmovespd.GetFloat() * dir;
}
