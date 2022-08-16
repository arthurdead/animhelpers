#ifndef IANIMHELPERS_H
#define IANIMHELPERS_H

#pragma once

#include <IShareSys.h>

#define SMINTERFACE_ANIMHELPERS_NAME "IAnimHelpers"
#define SMINTERFACE_ANIMHELPERS_VERSION 1

class CBaseAnimating;

class IAnimHelpers : public SourceMod::SMInterface
{
public:
	virtual const char *GetInterfaceName()
	{ return SMINTERFACE_ANIMHELPERS_NAME; }
	virtual unsigned int GetInterfaceVersion()
	{ return SMINTERFACE_ANIMHELPERS_VERSION; }

	virtual int SelectWeightedSequence(CBaseAnimating *pEntity, int activity) = 0;
	virtual void StudioFrameAdvance(CBaseAnimating *pEntity) = 0;
	virtual void DispatchAnimEvents(CBaseAnimating *pEntity) = 0;
	virtual void ResetSequenceInfo(CBaseAnimating *pEntity) = 0;
};

#endif
