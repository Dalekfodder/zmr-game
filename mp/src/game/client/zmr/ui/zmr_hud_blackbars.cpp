#include "cbase.h"
#include "hudelement.h"
#include "iclientmode.h"
#include <vgui/ISurface.h>
#include <vgui_controls/AnimationController.h>


#include "zmr_hud_blackbars.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


using namespace vgui;


#define SHOW_BARS           g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "ZMBarsShow" )
#define HIDE_BARS           g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "ZMBarsHide" )


CON_COMMAND( zm_hudbars_show, "" )
{
    CZMHudBars* blackbars = GET_HUDELEMENT( CZMHudBars );
    if ( blackbars )
    {
        blackbars->ShowBars( 9999.0f );
    }
}

CON_COMMAND( zm_hudbars_hide, "" )
{
    CZMHudBars* blackbars = GET_HUDELEMENT( CZMHudBars );
    if ( blackbars )
    {
        blackbars->HideBars();
    }
}


DECLARE_HUDELEMENT( CZMHudBars );


CZMHudBars::CZMHudBars( const char *pElementName ) : CHudElement( pElementName ), BaseClass( g_pClientMode->GetViewport(), "ZMHudBars" )
{
    SetPaintBackgroundEnabled( true );
    SetZPos( 9000 );
}

void CZMHudBars::Init()
{
    LevelInit();
}

void CZMHudBars::VidInit()
{
    LevelInit();

    SetSize( ScreenWidth(), ScreenHeight() );
}

void CZMHudBars::LevelInit()
{
    HideBars();
}

void CZMHudBars::OnThink()
{
    if ( m_flNextHide != 0.0f && m_flNextHide <= gpGlobals->curtime )
    {
        HideBars();
    }
}

void CZMHudBars::Paint()
{
    if ( m_flAlpha <= 0.0f ) return;
    

    Color clr = Color( 0, 0, 0, m_flAlpha );

    surface()->DrawSetColor( clr );

    int w = ScreenWidth();
    int h = ScreenHeight();

    if ( m_flTopBarY > 0.0f )
    {
        surface()->DrawFilledRect( 0, 0, w, YRES( m_flTopBarY ) );
    }

    if ( m_flBottomBarY < 480.0f )
    {
        surface()->DrawFilledRect( 0, YRES( m_flBottomBarY ), w, h );
    }
}

void CZMHudBars::ShowBars( float displaytime )
{
    SHOW_BARS;

    if ( displaytime > 0.0f )
        m_flNextHide = gpGlobals->curtime + displaytime;
    else
        m_flNextHide = 0.0f;
}

void CZMHudBars::HideBars()
{
    HIDE_BARS;
    m_flNextHide = 0.0f;
}