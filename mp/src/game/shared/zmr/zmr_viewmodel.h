#pragma once

#include "predicted_viewmodel.h"

#ifdef CLIENT_DLL
#define CZMViewModel C_ZMViewModel
#endif

class CZMViewModel : public CPredictedViewModel
{
public:
	DECLARE_CLASS( CZMViewModel, CPredictedViewModel );
	DECLARE_NETWORKCLASS();

#ifdef CLIENT_DLL
    virtual C_BaseAnimating* FindFollowedEntity() OVERRIDE;
#endif
};
