/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Sample Extension
 * Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 *
 * Version: $Id$
 */

#define swap V_swap

#define BASEENTITY_H
#define BASEENTITY_SHARED_H
#define NEXT_BOT
#define GLOWS_ENABLE
//#define TF_DLL
//#define USES_ECON_ITEMS
#define USE_NAV_MESH
#define RAD_TELEMETRY_DISABLED
#define CBASE_H

#include "extension.h"

extern int g_nActivityListVersion;
extern int g_nEventListVersion;

class IFileSystem;

IVModelInfo *modelinfo = nullptr;
IFileSystem *filesystem = nullptr;

/**
 * @file extension.cpp
 * @brief Implement extension code here.
 */

Sample g_Sample;		/**< Global singleton for extension's main interface */

SMEXT_LINK(&g_Sample);

CGlobalVars *gpGlobals = nullptr;

int m_pStudioHdrOffset = -1;
int m_nSequenceOffset = -1;
int m_flPoseParameterOffset = -1;
int m_AnimOverlayOffset = -1;
int m_nBodyOffset = -1;
int m_flEncodedControllerOffset = -1;
int m_flexWeightOffset = -1;
int m_rgflCoordinateFrameOffset = -1;
int m_iEFlagsOffset = -1;

int CBaseAnimatingStudioFrameAdvance = -1;
int CBaseAnimatingDispatchAnimEvents = -1;
int CBaseAnimatingGetAttachment = -1;
int CBaseAnimatingGetBoneTransform = -1;
int CBaseEntityFireBullets = -1;
int CBaseEntityWorldSpaceCenter = -1;

void *CBaseAnimatingResetSequenceInfo = nullptr;
void *CBaseAnimatingLockStudioHdr = nullptr;
void *CBaseEntitySetAbsOrigin = nullptr;
void *CBaseEntityCalcAbsolutePosition = nullptr;

template <typename R, typename T, typename ...Args>
R call_mfunc(T *pThisPtr, void *offset, Args ...args)
{
	class VEmptyClass {};
	
	void **this_ptr = *reinterpret_cast<void ***>(&pThisPtr);
	
	union
	{
		R (VEmptyClass::*mfpnew)(Args...);
#ifndef PLATFORM_POSIX
		void *addr;
	} u;
	u.addr = offset;
#else
		struct  
		{
			void *addr;
			intptr_t adjustor;
		} s;
	} u;
	u.s.addr = offset;
	u.s.adjustor = 0;
#endif
	
	return (R)(reinterpret_cast<VEmptyClass *>(this_ptr)->*u.mfpnew)(args...);
}

template <typename R, typename T, typename ...Args>
R call_vfunc(T *pThisPtr, size_t offset, Args ...args)
{
	void **vtable = *reinterpret_cast<void ***>(pThisPtr);
	void *vfunc = vtable[offset];
	
	return call_mfunc<R, T, Args...>(pThisPtr, vfunc, args...);
}

extern "C"
{
__attribute__((__visibility__("default"), __cdecl__)) double __pow_finite(double a, double b)
{
	return pow(a, b);
}

__attribute__((__visibility__("default"), __cdecl__)) double __log_finite(double a)
{
	return log(a);
}

__attribute__((__visibility__("default"), __cdecl__)) double __exp_finite(double a)
{
	return exp(a);
}

__attribute__((__visibility__("default"), __cdecl__)) double __atan2_finite(double a, double b)
{
	return atan2(a, b);
}

__attribute__((__visibility__("default"), __cdecl__)) float __atan2f_finite(float a, float b)
{
	return atan2f(a, b);
}

__attribute__((__visibility__("default"), __cdecl__)) float __powf_finite(float a, float b)
{
	return powf(a, b);
}

__attribute__((__visibility__("default"), __cdecl__)) float __logf_finite(float a)
{
	return logf(a);
}

__attribute__((__visibility__("default"), __cdecl__)) float __expf_finite(float a)
{
	return expf(a);
}

__attribute__((__visibility__("default"), __cdecl__)) float __acosf_finite(float a)
{
	return acosf(a);
}

__attribute__((__visibility__("default"), __cdecl__)) double __asin_finite(double a)
{
	return asin(a);
}

__attribute__((__visibility__("default"), __cdecl__)) double __acos_finite(double a)
{
	return acos(a);
}
}

class CStudioHdr;

enum TOGGLE_STATE : int;
struct AI_CriteriaSet;
struct FireBulletsInfo_t;

#include <shareddefs.h>

class CBaseEntity : public IServerEntity
{
public:
	bool  KeyValue( const char *szKeyName, Vector vec ) { return false; }
	bool  KeyValue( const char *szKeyName, float flValue ) { return false; }
	static void *GetPredictionPlayer() { return nullptr; }
	
	model_t *GetModel( void )
	{
		return (model_t *)modelinfo->GetModel( GetModelIndex() );
	}
	
	void FireBullets( const FireBulletsInfo_t &info )
	{
		call_vfunc<void, CBaseEntity, const FireBulletsInfo_t &>(this, CBaseEntityFireBullets, info);
	}
	
	void SetAbsOrigin( const Vector& origin )
	{
		call_mfunc<void, CBaseEntity, const Vector &>(this, CBaseEntitySetAbsOrigin, origin);
	}
	
	int GetIEFlags()
	{
		if(m_iEFlagsOffset == -1) {
			sm_datatable_info_t info{};
			datamap_t *pMap = gamehelpers->GetDataMap(this);
			gamehelpers->FindDataMapInfo(pMap, "m_iEFlags", &info);
			m_iEFlagsOffset = info.actual_offset;
		}
		
		return *(int *)((unsigned char *)this + m_iEFlagsOffset);
	}
	
	matrix3x4_t &EntityToWorldTransform()
	{
		if(m_rgflCoordinateFrameOffset == -1) {
			sm_datatable_info_t info{};
			datamap_t *pMap = gamehelpers->GetDataMap(this);
			gamehelpers->FindDataMapInfo(pMap, "m_rgflCoordinateFrame", &info);
			m_rgflCoordinateFrameOffset = info.actual_offset;
		}
		
		if(GetIEFlags() & EFL_DIRTY_ABSTRANSFORM) {
			CalcAbsolutePosition();
		}
		
		return *(matrix3x4_t *)((unsigned char *)this + m_rgflCoordinateFrameOffset);
	}
	
	void CalcAbsolutePosition()
	{
		call_mfunc<void, CBaseEntity>(this, CBaseEntityCalcAbsolutePosition);
	}
	
	const Vector &WorldSpaceCenter()
	{
		return call_vfunc<const Vector &>(this, CBaseEntityWorldSpaceCenter);
	}
};

#include <mathlib/vmatrix.h>
#include <ehandle.h>
#include <predictioncopy.h>
#include <ai_activity.h>
#include <activitylist.h>
#include <eventlist.h>
#include <studio.h>
#include <engine/ivmodelinfo.h>
#include <filesystem.h>
#include "stringregistry.h"
#include "isaverestore.h"
#include <KeyValues.h>
#include "utldict.h"

#define min(a,b) (((a)<(b))?(a):(b))
#define max(a,b) (((a)>(b))?(a):(b))

#define typeof __typeof__

#include <animation.cpp>
#include <studio.cpp>
#include <studio_shared.cpp>
#include <bone_setup.cpp>
#include <stringregistry.cpp>
#define ListFromString ListFromStringEvent
#include <eventlist.cpp>
#undef ListFromString
#define ListFromString ListFromStringActivity
#include "activitylist.cpp"
#undef ListFromString
#include <collisionutils.cpp>

void SetEdictStateChanged(CBaseEntity *pEntity, int offset)
{
	IServerNetworkable *pNet = pEntity->GetNetworkable();
	edict_t *edict = pNet->GetEdict();
	
	gamehelpers->SetEdictStateChanged(edict, offset);
}

class CBaseAnimating : public CBaseEntity
{
public:
	CStudioHdr *GetModelPtr()
	{
		CStudioHdr *m_pStudioHdr = *(CStudioHdr **)((unsigned char *)this + m_pStudioHdrOffset);
		
		if ( !m_pStudioHdr && GetModel() )
		{
			call_mfunc<void>(this, CBaseAnimatingLockStudioHdr);
			
			m_pStudioHdr = *(CStudioHdr **)((unsigned char *)this + m_pStudioHdrOffset);
		}
		
		return ( m_pStudioHdr && m_pStudioHdr->IsValid() ) ? m_pStudioHdr : nullptr;
	}
	
	int GetSequence()
	{
		return *(int *)((unsigned char *)this + m_nSequenceOffset);
	}
	
	float *GetPoseParameterArray()
	{
		return *(float **)((unsigned char *)this + m_flPoseParameterOffset);
	}
	
	float *GetEncodedControllerArray()
	{
		return *(float **)((unsigned char *)this + m_flEncodedControllerOffset);
	}
	
	void StudioFrameAdvance()
	{
		call_vfunc<void>(this, CBaseAnimatingStudioFrameAdvance);
	}
	
	void DispatchAnimEvents()
	{
		call_vfunc<void, CBaseAnimating, CBaseAnimating *>(this, CBaseAnimatingDispatchAnimEvents, this);
	}
	
	int FindBodygroupByName( const char *name );

	bool GetAttachment( int iAttachment, matrix3x4_t &attachmentToWorld )
	{
		return call_vfunc<bool, CBaseAnimating, int, matrix3x4_t &>(this, CBaseAnimatingGetAttachment, iAttachment, attachmentToWorld);
	}
	
	bool GetBoneTransform( int iAttachment, matrix3x4_t &attachmentToWorld )
	{
		return call_vfunc<bool, CBaseAnimating, int, matrix3x4_t &>(this, CBaseAnimatingGetBoneTransform, iAttachment, attachmentToWorld);
	}
	
	void ResetSequenceInfo()
	{
		call_mfunc<void>(this, CBaseAnimatingResetSequenceInfo);
	}
	
	int GetBody()
	{
		return *(int *)((unsigned char *)this + m_nBodyOffset);
	}
	
	void SetBody(int value)
	{
		*(int *)((unsigned char *)this + m_nBodyOffset) = value;
		SetEdictStateChanged(this, m_nBodyOffset);
	}
	
	bool GetAttachment( const char *szName, Vector &absOrigin, QAngle &absAngles )
	{																
		return GetAttachment( LookupAttachment( szName ), absOrigin, absAngles );
	}
	
	bool GetAttachment( int iAttachment, Vector &absOrigin, QAngle &absAngles )
	{
		matrix3x4_t attachmentToWorld;

		bool bRet = GetAttachment( iAttachment, attachmentToWorld );
		MatrixAngles( attachmentToWorld, absAngles, absOrigin );
		return bRet;
	}
	
	bool GetAttachmentLocal( const char *szName, Vector &origin, QAngle &angles );
	bool GetAttachmentLocal( int iAttachment, Vector &origin, QAngle &angles );
	bool GetAttachmentLocal( int iAttachment, matrix3x4_t &attachmentToLocal );
	
	void SetBodygroup( int iGroup, int iValue );
	
	void GetBonePosition ( int iBone, Vector &origin, QAngle &angles );
	int LookupBone( const char *szName );
	int LookupAttachment( const char *szName );
	int SelectWeightedSequence(int activity);
	int SelectWeightedSequence(int activity, int sequence);
	int LookupSequence(const char *name);
	int LookupActivity(const char *name);
	float GetPoseParameter(int index);
	void SetPoseParameter(int index, float value);
	int LookupPoseParameter(const char *name);
	float SequenceDuration(int sequence);
	int GetSequenceFlags(int sequence);
	
	void SetBoneController ( int iController, float flValue );
	float GetBoneController ( int iController );
	
	int SelectHeaviestSequence ( Activity activity );
	
	int GetNumSequences();
	int GetNumPoseParameters();
	int GetNumAttachments();
	int GetNumBones();
	int GetNumBodygroups();
	int GetNumBoneControllers();
	
	const char *GetPoseParameterName(int id);
	const char *GetAttachmentName(int id);
	const char *GetBoneName(int id);
	const char *GetBodygroupName(int id);
	const char *GetSequenceName(int id);
	const char *GetSequenceActivityName(int id);
	
	int GetAttachmentBone( int iAttachment );
};

bool CBaseAnimating::GetAttachmentLocal( const char *szName, Vector &origin, QAngle &angles )
{
	return GetAttachmentLocal( LookupAttachment( szName ), origin, angles );
}

bool CBaseAnimating::GetAttachmentLocal( int iAttachment, Vector &origin, QAngle &angles )
{
	matrix3x4_t attachmentToEntity;

	bool bRet = GetAttachmentLocal( iAttachment, attachmentToEntity );
	MatrixAngles( attachmentToEntity, angles, origin );
	return bRet;
}

bool CBaseAnimating::GetAttachmentLocal( int iAttachment, matrix3x4_t &attachmentToLocal )
{
	matrix3x4_t attachmentToWorld;
	bool bRet = GetAttachment(iAttachment, attachmentToWorld);
	matrix3x4_t worldToEntity;
	MatrixInvert( EntityToWorldTransform(), worldToEntity );
	ConcatTransforms( worldToEntity, attachmentToWorld, attachmentToLocal ); 
	return bRet;
}

int CBaseAnimating::GetAttachmentBone( int iAttachment )
{
	CStudioHdr *pStudioHdr = GetModelPtr( );
	if (!pStudioHdr || iAttachment < 1 || iAttachment > pStudioHdr->GetNumAttachments() )
	{
		return -1;
	}

	return pStudioHdr->GetAttachmentBone( iAttachment-1 );
}

const char *CBaseAnimating::GetSequenceActivityName(int id)
{
	CStudioHdr *pStudioHdr = GetModelPtr( );
	if (!pStudioHdr)
	{
		return "";
	}

	if (id < 0 || id >= pStudioHdr->GetNumSeq())
	{
		return "";
	}
	
	return pStudioHdr->pSeqdesc( id ).pszActivityName();
}

const char *CBaseAnimating::GetSequenceName(int id)
{
	CStudioHdr *pStudioHdr = GetModelPtr( );
	if (!pStudioHdr)
	{
		return "";
	}

	if (id < 0 || id >= pStudioHdr->GetNumSeq())
	{
		return "";
	}
	
	return pStudioHdr->pSeqdesc( id ).pszLabel();
}

int CBaseAnimating::GetNumBoneControllers()
{
	CStudioHdr *pmodel = (CStudioHdr*)GetModelPtr();
	if (!pmodel)
	{
		return 0;
	}
	
	return pmodel->numbonecontrollers();
}

const char *CBaseAnimating::GetBodygroupName(int id)
{
	CStudioHdr *pStudioHdr = GetModelPtr( );
	if (!pStudioHdr)
	{
		return "";
	}

	if (id < 0 || id >= pStudioHdr->numbodyparts())
	{
		return "";
	}
	
	return pStudioHdr->pBodypart( id )->pszName();
}

int CBaseAnimating::GetNumBodygroups()
{
	CStudioHdr *pmodel = (CStudioHdr*)GetModelPtr();
	if (!pmodel)
	{
		return 0;
	}
	
	return pmodel->numbodyparts();
}

const char *CBaseAnimating::GetBoneName(int id)
{
	CStudioHdr *pStudioHdr = GetModelPtr( );
	if (!pStudioHdr)
	{
		return "";
	}

	if (id < 0 || id >= pStudioHdr->numbones())
	{
		return "";
	}
	
	return pStudioHdr->pBone( id )->pszName();
}

int CBaseAnimating::GetNumBones()
{
	CStudioHdr *pmodel = (CStudioHdr*)GetModelPtr();
	if (!pmodel)
	{
		return 0;
	}
	
	return pmodel->numbones();
}

int CBaseAnimating::GetNumAttachments()
{
	CStudioHdr *pmodel = (CStudioHdr*)GetModelPtr();
	if (!pmodel)
	{
		return 0;
	}
	
	return pmodel->GetNumAttachments();
}

const char *CBaseAnimating::GetAttachmentName(int id)
{
	CStudioHdr *pStudioHdr = GetModelPtr( );
	if (!pStudioHdr)
	{
		return "";
	}

	if (id < 0 || id >= pStudioHdr->GetNumAttachments())
	{
		return "";
	}
	
	return pStudioHdr->pAttachment( id ).pszName();
}

const char *CBaseAnimating::GetPoseParameterName(int id)
{
	CStudioHdr *pStudioHdr = GetModelPtr( );
	if (!pStudioHdr)
	{
		return "";
	}

	if (id < 0 || id >= pStudioHdr->GetNumPoseParameters())
	{
		return "";
	}
	
	return pStudioHdr->pPoseParameter( id ).pszName();
}

int CBaseAnimating::SelectHeaviestSequence ( Activity activity )
{
	return ::SelectHeaviestSequence( GetModelPtr(), activity );
}

int CBaseAnimating::GetNumPoseParameters()
{
	CStudioHdr *pmodel = (CStudioHdr*)GetModelPtr();
	if (!pmodel)
	{
		return 0;
	}
	
	return pmodel->GetNumPoseParameters();
}

int CBaseAnimating::GetNumSequences()
{
	CStudioHdr *pmodel = (CStudioHdr*)GetModelPtr();
	if (!pmodel)
	{
		return 0;
	}
	
	return pmodel->GetNumSeq();
}

void CBaseAnimating::SetBoneController ( int iController, float flValue )
{
	CStudioHdr *pmodel = (CStudioHdr*)GetModelPtr();
	if (!pmodel)
	{
		return;
	}

	float newValue;
	float retVal = Studio_SetController( pmodel, iController, flValue, newValue );
	GetEncodedControllerArray()[iController] = newValue;
	SetEdictStateChanged(this, m_flEncodedControllerOffset + (iController * sizeof(float)));
}

float CBaseAnimating::GetBoneController ( int iController )
{
	CStudioHdr *pmodel = (CStudioHdr*)GetModelPtr();
	if (!pmodel)
	{
		return 0.0;
	}
	
	return Studio_GetController( pmodel, iController, GetEncodedControllerArray()[iController] );
}

void CBaseAnimating::GetBonePosition ( int iBone, Vector &origin, QAngle &angles )
{
	CStudioHdr *pStudioHdr = GetModelPtr( );
	if (!pStudioHdr)
	{
		return;
	}

	if (iBone < 0 || iBone >= pStudioHdr->numbones())
	{
		return;
	}

	matrix3x4_t bonetoworld;
	GetBoneTransform( iBone, bonetoworld );
	
	MatrixAngles( bonetoworld, angles, origin );
}

int CBaseAnimating::LookupBone( const char *szName )
{
	const CStudioHdr *pStudioHdr = GetModelPtr();
	if ( !pStudioHdr )
		return -1;
	return Studio_BoneIndexByName( pStudioHdr, szName );
}

int CBaseAnimating::LookupAttachment( const char *szName )
{
	CStudioHdr *pStudioHdr = GetModelPtr( );
	if (!pStudioHdr)
	{
		return 0;
	}

	return Studio_FindAttachment( pStudioHdr, szName ) + 1;
}

void CBaseAnimating::SetBodygroup( int iGroup, int iValue )
{
	int newBody = GetBody();
	::SetBodygroup( GetModelPtr( ), newBody, iGroup, iValue );
	SetBody(newBody);
}

int CBaseAnimating::FindBodygroupByName( const char *name )
{
	CStudioHdr *pStudioHdr = GetModelPtr();

	if(!pStudioHdr)
		return -1;

	return ::FindBodygroupByName(pStudioHdr, name);
}

int CBaseAnimating::GetSequenceFlags(int sequence)
{
	CStudioHdr *pStudioHdr = GetModelPtr();

	if(!pStudioHdr)
		return 0;

	if(!pStudioHdr->SequencesAvailable())
		return 0;

	if(sequence >= pStudioHdr->GetNumSeq() || sequence < 0)
		return 0;

	return ::GetSequenceFlags(pStudioHdr, sequence);
}

float CBaseAnimating::SequenceDuration(int sequence)
{
	CStudioHdr *pStudioHdr = GetModelPtr();

	if(!pStudioHdr)
		return 0.1f;

	if(!pStudioHdr->SequencesAvailable())
		return 0.1f;

	if(sequence >= pStudioHdr->GetNumSeq() || sequence < 0)
		return 0.1f;

	float *m_flPoseParameter = GetPoseParameterArray();
	if(!m_flPoseParameter)
		return 0.1f;

	return Studio_Duration(pStudioHdr, sequence, m_flPoseParameter);
}

class CBaseAnimatingOverlay;

class CAnimationLayer
{
	public:
		#define ANIM_LAYER_ACTIVE 0x0001
		#define ANIM_LAYER_AUTOKILL 0x0002
		#define ANIM_LAYER_KILLME 0x0004
		#define ANIM_LAYER_DONTRESTORE 0x0008
		#define ANIM_LAYER_DYING 0x0020

		void Init(CBaseAnimatingOverlay *pOverlay);
		bool IsActive();
		void MarkActive();
		void KillMe();
		bool IsDying();
		bool IsKillMe();

		int m_fFlags = 0;
		bool m_bSequenceFinished = false;
		bool m_bLooping = false;
		int m_nSequence = -1;
		float m_flCycle = 0.0f;
		float m_flPrevCycle = 0.0f;
		float m_flWeight = 0.0f;
		float m_flPlaybackRate = 0.0f;
		float m_flBlendIn = 0.0f;
		float m_flBlendOut = 0.0f;
		float m_flKillRate = 0.0f;
		float m_flKillDelay = 0.0f;
		float m_flLayerAnimtime = 0.0f;
		float m_flLayerFadeOuttime = 0.0f;
		Activity m_nActivity = ACT_INVALID;
		int m_nPriority = 0;
		int m_nOrder = 0;
		float m_flLastEventCheck = 0.0f;
		float m_flLastAccess = 0.0f;
		CBaseAnimatingOverlay *m_pOwnerEntity = nullptr;
};

class CBaseAnimatingOverlay : public CBaseAnimating
{
public:
	CUtlVector<CAnimationLayer> &GetAnimOverlay()
	{
		return *(CUtlVector<CAnimationLayer> *)((unsigned char *)this + m_AnimOverlayOffset);
	}
	
	int AddGestureSequence(int sequence, bool autokill=true);
	int AddGestureSequenceEx(int sequence, float flDuration, bool autokill=true);
	int AddGesture(Activity activity, bool autokill=true);
	int AddGestureEx(Activity activity, float flDuration, bool autokill=true);
	bool IsPlayingGesture(Activity activity);
	void RestartGesture(Activity activity, bool addifmissing=true, bool autokill=true);
	void RemoveGesture(Activity activity);
	void RemoveAllGestures();
	int AddLayeredSequence(int sequence, int iPriority);
	void SetLayerPriority(int iLayer, int iPriority);
	bool IsValidLayer(int iLayer);
	void SetLayerDuration(int iLayer, float flDuration);
	float GetLayerDuration(int iLayer);
	void SetLayerCycle(int iLayer, float flCycle);
	void SetLayerCycleEx(int iLayer, float flCycle, float flPrevCycle);
	float GetLayerCycle(int iLayer);
	void SetLayerPlaybackRate(int iLayer, float flPlaybackRate);
	void SetLayerWeight(int iLayer, float flWeight);
	float GetLayerWeight(int iLayer);
	void SetLayerBlendIn(int iLayer, float flBlendIn);
	void SetLayerBlendOut(int iLayer, float flBlendOut);
	void SetLayerAutokill(int iLayer, bool bAutokill);
	void SetLayerLooping(int iLayer, bool bLooping);
	void SetLayerNoRestore(int iLayer, bool bNoRestore);
	Activity GetLayerActivity(int iLayer);
	int GetLayerSequence(int iLayer);
	int FindGestureLayer(Activity activity);
	void RemoveLayer(int iLayer, float flKillRate=0.2, float flKillDelay=0.0);
	void FastRemoveLayer(int iLayer);
	int GetNumAnimOverlays();
	void SetNumAnimOverlays(int num);
	void VerifyOrder();
	bool HasActiveLayer();
	int AllocateLayer(int iPriority=0);
	static int MaxOverlays();
};

void CAnimationLayer::KillMe()
{
	m_fFlags |= ANIM_LAYER_KILLME;
}

bool CAnimationLayer::IsDying()
{
	return ((m_fFlags & ANIM_LAYER_DYING) != 0);
}

bool CAnimationLayer::IsKillMe()
{
	return ((m_fFlags & ANIM_LAYER_KILLME) != 0);
}

void CAnimationLayer::MarkActive()
{
	m_flLastAccess = gpGlobals->curtime;
}

bool CAnimationLayer::IsActive()
{
	return ((m_fFlags & ANIM_LAYER_ACTIVE) != 0);
}

void CAnimationLayer::Init(CBaseAnimatingOverlay *pOverlay)
{
	m_pOwnerEntity = pOverlay;
	m_fFlags = 0;
	m_flWeight = 0.0f;
	m_flCycle = 0.0f;
	m_flPrevCycle = 0.0f;
	m_bSequenceFinished = false;
	m_nActivity = ACT_INVALID;
	m_nSequence = 0;
	m_nPriority = 0;
	m_nOrder = CBaseAnimatingOverlay::MaxOverlays();
	m_flKillRate = 100.0;
	m_flKillDelay = 0.0f;
	m_flPlaybackRate = 1.0f;
	m_flLastAccess = gpGlobals->curtime;
	m_flLayerAnimtime = 0.0f;
	m_flLayerFadeOuttime = 0.0f;
}

int CBaseAnimatingOverlay::AddGestureSequence(int sequence, bool autokill)
{
	int i = AddLayeredSequence(sequence, 0);

	if(IsValidLayer(i))
		SetLayerAutokill(i, autokill);

	return i;
}

int CBaseAnimatingOverlay::AddGestureSequenceEx(int nSequence, float flDuration, bool autokill)
{
	int iLayer = AddGestureSequence(nSequence, autokill);

	if(iLayer >= 0 && flDuration > 0)
		GetAnimOverlay().Element(iLayer).m_flPlaybackRate = SequenceDuration(nSequence) / flDuration;

	return iLayer;
}

int CBaseAnimatingOverlay::AddGesture(Activity activity, bool autokill)
{
	if(IsPlayingGesture(activity))
		return FindGestureLayer(activity);

	int seq = SelectWeightedSequence(activity);
	if(seq == -1)
		return -1;

	int i = AddGestureSequence(seq, autokill);
	if(i != -1)
		GetAnimOverlay().Element(i).m_nActivity = activity;

	return i;
}

int CBaseAnimatingOverlay::AddGestureEx(Activity activity, float flDuration, bool autokill)
{
	int iLayer = AddGesture(activity, autokill);
	SetLayerDuration(iLayer, flDuration);
	return iLayer;
}

void CBaseAnimatingOverlay::SetLayerDuration(int iLayer, float flDuration)
{
	if(IsValidLayer(iLayer) && flDuration > 0)
	{
		CAnimationLayer *layer = &GetAnimOverlay().Element(iLayer);
		layer->m_flPlaybackRate = SequenceDuration(layer->m_nSequence ) / flDuration;
	}
}

float CBaseAnimatingOverlay::GetLayerDuration(int iLayer)
{
	if(IsValidLayer(iLayer))
	{
		CAnimationLayer *layer = &GetAnimOverlay().Element(iLayer);

		if(layer->m_flPlaybackRate != 0.0f)
			return (1.0f - layer->m_flCycle) * SequenceDuration(layer->m_nSequence ) / layer->m_flPlaybackRate;

		return SequenceDuration(layer->m_nSequence);
	}

	return 0.0f;
}

int CBaseAnimatingOverlay::AddLayeredSequence(int sequence, int iPriority)
{
	int i = AllocateLayer(iPriority);
	if(IsValidLayer(i))
	{
		CAnimationLayer *layer = &GetAnimOverlay().Element(i);

		layer->m_flCycle = 0.0f;
		layer->m_flPrevCycle = 0.0f;
		layer->m_flPlaybackRate = 1.0f;
		layer->m_nActivity = ACT_INVALID;
		layer->m_nSequence = sequence;
		layer->m_flWeight = 1.0f;
		layer->m_flBlendIn = 0.0f;
		layer->m_flBlendOut = 0.0f;
		layer->m_bSequenceFinished = false;
		layer->m_flLastEventCheck = 0.0f;
		layer->m_bLooping = ((GetSequenceFlags(sequence) & STUDIO_LOOPING) != 0);
	}

	return i;
}

bool CBaseAnimatingOverlay::IsValidLayer(int iLayer)
{
	return (iLayer >= 0 && iLayer < GetAnimOverlay().Count() && GetAnimOverlay().Element(iLayer).IsActive());
}

int CBaseAnimatingOverlay::AllocateLayer(int iPriority)
{
	int i = 0;
	int iNewOrder = 0;
	int iOpenLayer = -1;
	int iNumOpen = 0;

	for(i = 0; i < GetAnimOverlay().Count(); i++)
	{
		CAnimationLayer *layer = &GetAnimOverlay().Element(i);

		if(layer->IsActive()) {
			if(layer->m_nPriority <= iPriority)
				iNewOrder = MAX(iNewOrder, layer->m_nOrder + 1);
		}
		else if(layer->IsDying())
		{
			continue;
		}
		else if(iOpenLayer == -1)
		{
			iOpenLayer = i;
		}
		else
		{
			iNumOpen++;
		}
	}

	if(iOpenLayer == -1)
	{
		if(GetAnimOverlay().Count() >= MaxOverlays())
			return -1;

		iOpenLayer = GetAnimOverlay().AddToTail();
		GetAnimOverlay().Element(iOpenLayer).Init(this);
	}

	if(iNumOpen == 0)
	{
		if(GetAnimOverlay().Count() < MaxOverlays()) {
			i = GetAnimOverlay().AddToTail();
			GetAnimOverlay().Element(i).Init(this);
		}
	}

	for(i = 0; i < GetAnimOverlay().Count(); i++)
	{
		CAnimationLayer *layer = &GetAnimOverlay().Element(i);

		if(layer->m_nOrder >= iNewOrder && layer->m_nOrder < MaxOverlays())
			layer->m_nOrder++;
	}

	CAnimationLayer *layer = &GetAnimOverlay().Element(iOpenLayer);

	layer->m_fFlags = ANIM_LAYER_ACTIVE;
	layer->m_nOrder = iNewOrder;
	layer->m_nPriority = iPriority;
	layer->MarkActive();

	VerifyOrder();

	return iOpenLayer;
}

void CBaseAnimatingOverlay::SetLayerPriority(int iLayer, int iPriority)
{
	if(!IsValidLayer(iLayer))
		return;

	CAnimationLayer *layer = &GetAnimOverlay().Element(iLayer);

	if(layer->m_nPriority == iPriority)
		return;

	// look for an open slot and for existing layers that are lower priority
	int i = 0;
	for(i = 0; i < GetAnimOverlay().Count(); i++)
	{
		CAnimationLayer *l = &GetAnimOverlay().Element(i);

		if(l->IsActive()) {
			if(l->m_nOrder > layer->m_nOrder)
				l->m_nOrder--;
		}
	}

	int iNewOrder = 0;
	for(i = 0; i < GetAnimOverlay().Count(); i++)
	{
		CAnimationLayer *l = &GetAnimOverlay().Element(i);

		if(i != iLayer && l->IsActive()) {
			if(l->m_nPriority <= iPriority)
				iNewOrder = MAX(iNewOrder, l->m_nOrder + 1);
		}
	}

	for(i = 0; i < GetAnimOverlay().Count(); i++)
	{
		CAnimationLayer *l = &GetAnimOverlay().Element(i);

		if(i != iLayer && l->IsActive()) {
			if(l->m_nOrder >= iNewOrder)
				l->m_nOrder++;
		}
	}

	layer->m_nOrder = iNewOrder;
	layer->m_nPriority = iPriority;
	layer->MarkActive();

	VerifyOrder();
}

int	CBaseAnimatingOverlay::FindGestureLayer(Activity activity)
{
	for(int i = 0; i < GetAnimOverlay().Count(); i++)
	{
		CAnimationLayer *layer = &GetAnimOverlay().Element(i);

		if(!(layer->IsActive()))
			continue;

		if(layer->IsKillMe())
			continue;

		if(layer->m_nActivity == ACT_INVALID)
			continue;

		if(layer->m_nActivity == activity)
			return i;
	}

	return -1;
}

bool CBaseAnimatingOverlay::IsPlayingGesture(Activity activity)
{
	return FindGestureLayer(activity) != -1 ? true : false;
}

void CBaseAnimatingOverlay::RestartGesture(Activity activity, bool addifmissing, bool autokill)
{
	int idx = FindGestureLayer(activity);
	if(idx == -1) {
		if(addifmissing)
			AddGesture( activity, autokill );
		return;
	}

	CAnimationLayer *layer = &GetAnimOverlay().Element(idx);

	layer->m_flCycle = 0.0f;
	layer->m_flPrevCycle = 0.0f;
	layer->m_flLastEventCheck = 0.0f;
}

void CBaseAnimatingOverlay::RemoveGesture(Activity activity)
{
	int iLayer = FindGestureLayer(activity);
	if(iLayer == -1)
		return;

	RemoveLayer(iLayer);
}

void CBaseAnimatingOverlay::RemoveAllGestures()
{
	for(int i = 0; i < GetAnimOverlay().Count(); i++)
		RemoveLayer(i);
}

void CBaseAnimatingOverlay::SetLayerCycle(int iLayer, float flCycle)
{
	if(!IsValidLayer(iLayer))
		return;

	CAnimationLayer *layer = &GetAnimOverlay().Element(iLayer);

	if(!layer->m_bLooping)
		flCycle = clamp(flCycle, 0.0, 1.0);

	layer->m_flCycle = flCycle;
	layer->MarkActive();
}

void CBaseAnimatingOverlay::SetLayerCycleEx(int iLayer, float flCycle, float flPrevCycle)
{
	if(!IsValidLayer(iLayer))
		return;

	CAnimationLayer *layer = &GetAnimOverlay().Element(iLayer);

	if(!layer->m_bLooping){
		flCycle = clamp(flCycle, 0.0, 1.0);
		flPrevCycle = clamp(flPrevCycle, 0.0, 1.0);
	}

	layer->m_flCycle = flCycle;
	layer->m_flPrevCycle = flPrevCycle;
	layer->m_flLastEventCheck = flPrevCycle;
	layer->MarkActive();
}

float CBaseAnimatingOverlay::GetLayerCycle(int iLayer)
{
	if(!IsValidLayer(iLayer))
		return 0.0f;

	return GetAnimOverlay().Element(iLayer).m_flCycle;
}

void CBaseAnimatingOverlay::SetLayerPlaybackRate(int iLayer, float flPlaybackRate)
{
	if(!IsValidLayer(iLayer))
		return;

	GetAnimOverlay().Element(iLayer).m_flPlaybackRate = flPlaybackRate;
}

void CBaseAnimatingOverlay::SetLayerWeight(int iLayer, float flWeight)
{
	if(!IsValidLayer(iLayer))
		return;

	flWeight = clamp(flWeight, 0.0f, 1.0f);

	CAnimationLayer *layer = &GetAnimOverlay().Element(iLayer);
	layer->m_flWeight = flWeight;
	layer->MarkActive();
}

float CBaseAnimatingOverlay::GetLayerWeight(int iLayer)
{
	if(!IsValidLayer(iLayer))
		return 0.0f;

	return GetAnimOverlay().Element(iLayer).m_flWeight;
}

void CBaseAnimatingOverlay::SetLayerBlendIn(int iLayer, float flBlendIn)
{
	if(!IsValidLayer(iLayer))
		return;

	GetAnimOverlay().Element(iLayer).m_flBlendIn = flBlendIn;
}

void CBaseAnimatingOverlay::SetLayerBlendOut(int iLayer, float flBlendOut)
{
	if(!IsValidLayer(iLayer))
		return;

	GetAnimOverlay().Element(iLayer).m_flBlendOut = flBlendOut;
}

void CBaseAnimatingOverlay::SetLayerAutokill(int iLayer, bool bAutokill)
{
	if(!IsValidLayer(iLayer))
		return;

	CAnimationLayer *layer = &GetAnimOverlay().Element(iLayer);

	if(bAutokill)
		layer->m_fFlags |= ANIM_LAYER_AUTOKILL;
	else
		layer->m_fFlags &= ~ANIM_LAYER_AUTOKILL;
}

void CBaseAnimatingOverlay::SetLayerLooping(int iLayer, bool bLooping)
{
	if(!IsValidLayer(iLayer))
		return;

	GetAnimOverlay().Element(iLayer).m_bLooping = bLooping;
}

void CBaseAnimatingOverlay::SetLayerNoRestore(int iLayer, bool bNoRestore)
{
	if(!IsValidLayer(iLayer))
		return;

	CAnimationLayer *layer = &GetAnimOverlay().Element(iLayer);

	if(bNoRestore)
		layer->m_fFlags |= ANIM_LAYER_DONTRESTORE;
	else
		layer->m_fFlags &= ~ANIM_LAYER_DONTRESTORE;
}

Activity CBaseAnimatingOverlay::GetLayerActivity(int iLayer)
{
	if(!IsValidLayer(iLayer))
		return ACT_INVALID;

	return GetAnimOverlay().Element(iLayer).m_nActivity;
}

int CBaseAnimatingOverlay::GetLayerSequence(int iLayer)
{
	if(!IsValidLayer(iLayer))
		return ACT_INVALID;

	return GetAnimOverlay().Element(iLayer).m_nSequence;
}

void CBaseAnimatingOverlay::RemoveLayer(int iLayer, float flKillRate, float flKillDelay)
{
	if(!IsValidLayer(iLayer))
		return;

	CAnimationLayer *layer = &GetAnimOverlay().Element(iLayer);

	if(flKillRate > 0)
		layer->m_flKillRate = layer->m_flWeight / flKillRate;
	else
		layer->m_flKillRate = 100;

	layer->m_flKillDelay = flKillDelay;

	layer->KillMe();
}

void CBaseAnimatingOverlay::FastRemoveLayer(int iLayer)
{
	if(!IsValidLayer(iLayer))
		return;

	CAnimationLayer *layer = &GetAnimOverlay().Element(iLayer);

	for(int j = 0; j < GetAnimOverlay().Count(); j++)
	{
		CAnimationLayer *l = &GetAnimOverlay().Element(j);

		if((l->IsActive()) && l->m_nOrder > layer->m_nOrder)
			l->m_nOrder--;
	}

	layer->Init(this);

	VerifyOrder();
}

void CBaseAnimatingOverlay::SetNumAnimOverlays(int num)
{
	if(GetAnimOverlay().Count() < num)
		GetAnimOverlay().AddMultipleToTail(num - GetAnimOverlay().Count());
	else if(GetAnimOverlay().Count() > num)
		GetAnimOverlay().RemoveMultiple(num, GetAnimOverlay().Count() - num);
}

bool CBaseAnimatingOverlay::HasActiveLayer()
{
	for(int j = 0; j < GetAnimOverlay().Count(); j++)
	{
		if(GetAnimOverlay().Element(j).IsActive())
			return true;
	}

	return false;
}

int CBaseAnimatingOverlay::MaxOverlays()
{
	return 15;
}

void CBaseAnimatingOverlay::VerifyOrder()
{
	
}

int CBaseAnimatingOverlay::GetNumAnimOverlays()
{
	return GetAnimOverlay().Count();
}

float CBaseAnimating::GetPoseParameter(int index)
{
	CStudioHdr *pStudioHdr = GetModelPtr();

	if(!pStudioHdr)
		return 0.0f;

	if(!pStudioHdr->SequencesAvailable())
		return 0.0f;

	if(index <= 0 || index >= pStudioHdr->GetNumPoseParameters())
		return 0.0f;

	return Studio_GetPoseParameter(pStudioHdr, index, GetPoseParameterArray()[index]);
}

void CBaseAnimating::SetPoseParameter(int index, float value)
{
	CStudioHdr *pStudioHdr = GetModelPtr();

	if(!pStudioHdr)
		return;

	if(!pStudioHdr->SequencesAvailable())
		return;

	if(index <= 0 || index >= pStudioHdr->GetNumPoseParameters())
		return;

	float flNewValue = 0.0f;
	Studio_SetPoseParameter(pStudioHdr, index, value, flNewValue);
	GetPoseParameterArray()[index] = flNewValue;
	SetEdictStateChanged(this, m_flPoseParameterOffset + (index * sizeof(float)));
}

int CBaseAnimating::LookupPoseParameter(const char *name)
{
	CStudioHdr *pStudioHdr = GetModelPtr();

	if(!pStudioHdr)
		return -1;

	if(!pStudioHdr->SequencesAvailable())
		return -1;

	for(int i = 0; i < pStudioHdr->GetNumPoseParameters(); i++) {
		if(Q_stricmp(pStudioHdr->pPoseParameter(i).pszName(), name) == 0)
			return i;
	}

	return -1;
}

int CBaseAnimating::SelectWeightedSequence(int activity, int sequence)
{
	CStudioHdr *pStudioHdr = GetModelPtr();

	if(!pStudioHdr)
		return -1;

	if(!pStudioHdr->SequencesAvailable())
		return -1;

	if(sequence >= pStudioHdr->GetNumSeq() || sequence < 0)
		return -1;
	
	if(activity <= 0)
		return -1;

	return ::SelectWeightedSequence(pStudioHdr, activity, sequence);
}

int CBaseAnimating::SelectWeightedSequence(int activity)
{
	CStudioHdr *pStudioHdr = GetModelPtr();

	if(!pStudioHdr)
		return -1;

	if(!pStudioHdr->SequencesAvailable())
		return -1;

	int sequence = GetSequence();
	if(sequence >= pStudioHdr->GetNumSeq() || sequence < 0)
		return -1;
	
	if(activity <= 0)
		return -1;

	return ::SelectWeightedSequence(pStudioHdr, activity, sequence);
}

int CBaseAnimating::LookupSequence(const char *name)
{
	CStudioHdr *pStudioHdr = GetModelPtr();

	if(!pStudioHdr)
		return -1;

	if(!pStudioHdr->SequencesAvailable())
		return -1;

	return ::LookupSequence(pStudioHdr, name);
}

int CBaseAnimating::LookupActivity(const char *name)
{
	CStudioHdr *pStudioHdr = GetModelPtr();

	if(!pStudioHdr)
		return ACT_INVALID;

	if(!pStudioHdr->SequencesAvailable())
		return ACT_INVALID;

	return ::LookupActivity(pStudioHdr, name);
}

class CBaseFlex : public CBaseAnimatingOverlay
{
public:
	LocalFlexController_t FindFlexController( const char *szName );
	LocalFlexController_t GetNumFlexControllers( void );
	const char *GetFlexControllerName( LocalFlexController_t iFlexController );
	void SetFlexWeight( LocalFlexController_t index, float value );
	float GetFlexWeight( LocalFlexController_t index );
	
	float *GetFlexWeightArray()
	{
		return *(float **)((unsigned char *)this + m_flexWeightOffset);
	}
};

LocalFlexController_t CBaseFlex::GetNumFlexControllers( void )
{
	CStudioHdr *pstudiohdr = GetModelPtr( );
	if (! pstudiohdr)
		return LocalFlexController_t(0);

	return pstudiohdr->numflexcontrollers();
}

const char *CBaseFlex::GetFlexControllerName( LocalFlexController_t iFlexController )
{
	CStudioHdr *pstudiohdr = GetModelPtr( );
	if (! pstudiohdr)
		return "";

	mstudioflexcontroller_t *pflexcontroller = pstudiohdr->pFlexcontroller( iFlexController );

	return pflexcontroller->pszName( );
}

LocalFlexController_t CBaseFlex::FindFlexController( const char *szName )
{
	for (LocalFlexController_t i = LocalFlexController_t(0); i < GetNumFlexControllers(); i++)
	{
		if (stricmp( GetFlexControllerName( i ), szName ) == 0)
		{
			return i;
		}
	}

	return LocalFlexController_t(-1);
}

void CBaseFlex::SetFlexWeight( LocalFlexController_t index, float value )
{
	if (index >= 0 && index < GetNumFlexControllers())
	{
		CStudioHdr *pstudiohdr = GetModelPtr( );
		if (! pstudiohdr)
			return;

		mstudioflexcontroller_t *pflexcontroller = pstudiohdr->pFlexcontroller( index );

		if (pflexcontroller->max != pflexcontroller->min)
		{
			value = (value - pflexcontroller->min) / (pflexcontroller->max - pflexcontroller->min);
			value = clamp( value, 0.0f, 1.0f );
		}

		GetFlexWeightArray()[index] = value;
		SetEdictStateChanged(this, m_flexWeightOffset + (index * sizeof(float)));
	}
}

float CBaseFlex::GetFlexWeight( LocalFlexController_t index )
{
	if (index >= 0 && index < GetNumFlexControllers())
	{
		CStudioHdr *pstudiohdr = GetModelPtr( );
		if (! pstudiohdr)
			return 0.0f;

		mstudioflexcontroller_t *pflexcontroller = pstudiohdr->pFlexcontroller( index );

		if (pflexcontroller->max != pflexcontroller->min)
		{
			return GetFlexWeightArray()[index] * (pflexcontroller->max - pflexcontroller->min) + pflexcontroller->min;
		}
				
		return GetFlexWeightArray()[index];
	}
	return 0.0f;
}

static cell_t BaseAnimatingSelectWeightedSequenceEx(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimating *pEntity = (CBaseAnimating *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}

	return pEntity->SelectWeightedSequence((Activity)params[2]);
}

static cell_t BaseAnimatingSelectHeaviestSequenceEx(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimating *pEntity = (CBaseAnimating *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}

	return pEntity->SelectHeaviestSequence((Activity)params[2]);
}

static cell_t BaseAnimatingLookupSequence(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimating *pEntity = (CBaseAnimating *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}

	char *name = nullptr;
	pContext->LocalToString(params[2], &name);

	return pEntity->LookupSequence(name);
}

static cell_t BaseAnimatingLookupActivity(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimating *pEntity = (CBaseAnimating *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}

	char *name = nullptr;
	pContext->LocalToString(params[2], &name);

	return pEntity->LookupActivity(name);
}

static cell_t BaseAnimatingLookupPoseParameter(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimating *pEntity = (CBaseAnimating *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}

	char *name = nullptr;
	pContext->LocalToString(params[2], &name);

	return pEntity->LookupPoseParameter(name);
}

static cell_t BaseAnimatingGetAttachmentBone(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimating *pEntity = (CBaseAnimating *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}

	return pEntity->GetAttachmentBone(params[2]);
}

static cell_t BaseAnimatingSetPoseParameter(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimating *pEntity = (CBaseAnimating *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}

	pEntity->SetPoseParameter(params[2], sp_ctof(params[3]));
	return 0;
}

static cell_t BaseAnimatingGetPoseParameter(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimating *pEntity = (CBaseAnimating *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}

	return sp_ftoc(pEntity->GetPoseParameter(params[2]));
}

static cell_t BaseAnimatingOverlayAddGestureSequence(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimatingOverlay *pEntity = (CBaseAnimatingOverlay *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}

	return pEntity->AddGestureSequence(params[2], params[3]);
}

static cell_t BaseAnimatingOverlayAddGestureSequenceEx(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimatingOverlay *pEntity = (CBaseAnimatingOverlay *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}

	return pEntity->AddGestureSequenceEx(params[2], sp_ctof(params[3]), params[4]);
}

static cell_t BaseAnimatingOverlayAddGesture(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimatingOverlay *pEntity = (CBaseAnimatingOverlay *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}

	return pEntity->AddGesture((Activity)params[2], params[3]);
}

static cell_t BaseAnimatingOverlayAddGestureEx(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimatingOverlay *pEntity = (CBaseAnimatingOverlay *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}

	return pEntity->AddGestureEx((Activity)params[2], sp_ctof(params[3]), params[4]);
}

static cell_t BaseAnimatingOverlayIsPlayingGesture(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimatingOverlay *pEntity = (CBaseAnimatingOverlay *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}

	return pEntity->IsPlayingGesture((Activity)params[2]);
}

static cell_t BaseAnimatingOverlayRestartGesture(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimatingOverlay *pEntity = (CBaseAnimatingOverlay *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}

	pEntity->RestartGesture((Activity)params[2], params[3], params[4]);
	return 0;
}

static cell_t BaseAnimatingOverlayRemoveGesture(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimatingOverlay *pEntity = (CBaseAnimatingOverlay *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}

	pEntity->RemoveGesture((Activity)params[2]);
	return 0;
}

static cell_t BaseAnimatingOverlayRemoveAllGestures(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimatingOverlay *pEntity = (CBaseAnimatingOverlay *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}

	pEntity->RemoveAllGestures();
	return 0;
}

static cell_t BaseAnimatingOverlayAddLayeredSequence(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimatingOverlay *pEntity = (CBaseAnimatingOverlay *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}

	return pEntity->AddLayeredSequence(params[2], params[3]);
}

static cell_t BaseAnimatingOverlaySetLayerPriority(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimatingOverlay *pEntity = (CBaseAnimatingOverlay *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}

	pEntity->SetLayerPriority(params[2], params[3]);
	return 0;
}

static cell_t BaseAnimatingOverlayIsValidLayer(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimatingOverlay *pEntity = (CBaseAnimatingOverlay *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}

	return pEntity->IsValidLayer(params[2]);
}

static cell_t BaseAnimatingOverlaySetLayerDuration(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimatingOverlay *pEntity = (CBaseAnimatingOverlay *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}

	pEntity->SetLayerDuration(params[2], sp_ctof(params[3]));
	return 0;
}

static cell_t BaseAnimatingOverlayGetLayerDuration(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimatingOverlay *pEntity = (CBaseAnimatingOverlay *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}

	return sp_ftoc(pEntity->GetLayerDuration(params[2]));
}

static cell_t BaseAnimatingOverlaySetLayerCycle(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimatingOverlay *pEntity = (CBaseAnimatingOverlay *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}

	pEntity->SetLayerCycle(params[2], sp_ctof(params[3]));
	return 0;
}

static cell_t BaseAnimatingOverlaySetLayerCycleEx(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimatingOverlay *pEntity = (CBaseAnimatingOverlay *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}

	pEntity->SetLayerCycleEx(params[2], sp_ctof(params[3]), sp_ctof(params[4]));
	return 0;
}

static cell_t BaseAnimatingOverlayGetLayerCycle(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimatingOverlay *pEntity = (CBaseAnimatingOverlay *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}

	return sp_ftoc(pEntity->GetLayerCycle(params[2]));
}

static cell_t BaseAnimatingOverlaySetLayerPlaybackRate(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimatingOverlay *pEntity = (CBaseAnimatingOverlay *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}

	pEntity->SetLayerPlaybackRate(params[2], sp_ctof(params[3]));
	return 0;
}

static cell_t BaseAnimatingOverlaySetLayerWeight(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimatingOverlay *pEntity = (CBaseAnimatingOverlay *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}

	pEntity->SetLayerWeight(params[2], sp_ctof(params[3]));
	return 0;
}

static cell_t BaseAnimatingOverlayGetLayerWeight(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimatingOverlay *pEntity = (CBaseAnimatingOverlay *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}

	return sp_ftoc(pEntity->GetLayerWeight(params[2]));
}

static cell_t BaseAnimatingOverlaySetLayerBlendIn(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimatingOverlay *pEntity = (CBaseAnimatingOverlay *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}

	pEntity->SetLayerBlendIn(params[2], sp_ctof(params[3]));
	return 0;
}

static cell_t BaseAnimatingOverlaySetLayerBlendOut(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimatingOverlay *pEntity = (CBaseAnimatingOverlay *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}

	pEntity->SetLayerBlendOut(params[2], sp_ctof(params[3]));
	return 0;
}

static cell_t BaseAnimatingOverlaySetLayerAutokill(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimatingOverlay *pEntity = (CBaseAnimatingOverlay *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}

	pEntity->SetLayerAutokill(params[2], params[3]);
	return 0;
}

static cell_t BaseAnimatingOverlaySetLayerLooping(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimatingOverlay *pEntity = (CBaseAnimatingOverlay *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}

	pEntity->SetLayerLooping(params[2], params[3]);
	return 0;
}

static cell_t BaseAnimatingOverlaySetLayerNoRestore(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimatingOverlay *pEntity = (CBaseAnimatingOverlay *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}

	pEntity->SetLayerNoRestore(params[2], params[3]);
	return 0;
}

static cell_t BaseAnimatingOverlayGetLayerActivity(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimatingOverlay *pEntity = (CBaseAnimatingOverlay *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}

	return pEntity->GetLayerActivity(params[2]);
}

static cell_t BaseAnimatingOverlayGetLayerSequence(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimatingOverlay *pEntity = (CBaseAnimatingOverlay *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}

	return pEntity->GetLayerSequence(params[2]);
}

static cell_t BaseAnimatingOverlayFindGestureLayer(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimatingOverlay *pEntity = (CBaseAnimatingOverlay *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}

	return pEntity->FindGestureLayer((Activity)params[2]);
}

static cell_t BaseAnimatingOverlayRemoveLayer(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimatingOverlay *pEntity = (CBaseAnimatingOverlay *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}

	pEntity->RemoveLayer(params[2], sp_ctof(params[3]), sp_ctof(params[4]));
	return 0;
}

static cell_t BaseAnimatingOverlayFastRemoveLayer(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimatingOverlay *pEntity = (CBaseAnimatingOverlay *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}

	pEntity->FastRemoveLayer(params[2]);
	return 0;
}

static cell_t BaseAnimatingOverlayGetNumAnimOverlays(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimatingOverlay *pEntity = (CBaseAnimatingOverlay *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}

	return pEntity->GetNumAnimOverlays();
}

static cell_t BaseAnimatingOverlaySetNumAnimOverlays(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimatingOverlay *pEntity = (CBaseAnimatingOverlay *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}

	pEntity->SetNumAnimOverlays(params[2]);
	return 0;
}

static cell_t BaseAnimatingOverlayVerifyOrder(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimatingOverlay *pEntity = (CBaseAnimatingOverlay *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}

	pEntity->VerifyOrder();
	return 0;
}

static cell_t BaseAnimatingOverlayHasActiveLayer(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimatingOverlay *pEntity = (CBaseAnimatingOverlay *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}

	return pEntity->HasActiveLayer();
}

static cell_t BaseAnimatingOverlayAllocateLayer(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimatingOverlay *pEntity = (CBaseAnimatingOverlay *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}

	return pEntity->AllocateLayer(params[2]);
}

static cell_t BaseAnimatingOverlayMaxOverlays(IPluginContext *pContext, const cell_t *params)
{
	return CBaseAnimatingOverlay::MaxOverlays();
}

static cell_t BaseAnimatingStudioFrameAdvance(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimating *pEntity = (CBaseAnimating *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}

	pEntity->StudioFrameAdvance();
	return 0;
}

static cell_t BaseAnimatingDispatchAnimEvents(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimating *pEntity = (CBaseAnimating *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}

	pEntity->DispatchAnimEvents();
	return 0;
}

static cell_t BaseAnimatingResetSequenceInfo(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimating *pEntity = (CBaseAnimating *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}

	pEntity->ResetSequenceInfo();
	return 0;
}

static cell_t BaseEntityFireBullets(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimating *pEntity = (CBaseAnimating *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}
	
	FireBulletsInfo_t info{};
	
	cell_t *addr = nullptr;
	pContext->LocalToPhysAddr(params[2], &addr);

	info.m_iShots = addr[0];
	info.m_vecSrc.x = sp_ctof(addr[1]);
	info.m_vecSrc.y = sp_ctof(addr[2]);
	info.m_vecSrc.z = sp_ctof(addr[3]);
	info.m_vecDirShooting.x = sp_ctof(addr[4]);
	info.m_vecDirShooting.y = sp_ctof(addr[5]);
	info.m_vecDirShooting.z = sp_ctof(addr[6]);
	info.m_vecSpread.x = sp_ctof(addr[7]);
	info.m_vecSpread.y = sp_ctof(addr[8]);
	info.m_vecSpread.z = sp_ctof(addr[9]);
	info.m_flDistance = sp_ctof(addr[10]);
	info.m_iAmmoType = addr[11];
	info.m_iTracerFreq = addr[12];
	info.m_flDamage = sp_ctof(addr[13]);
	info.m_iPlayerDamage = addr[14];
	info.m_nFlags = addr[15];
	info.m_flDamageForceScale = sp_ctof(addr[16]);
	info.m_pAttacker = gamehelpers->ReferenceToEntity(addr[17]);
	info.m_pAdditionalIgnoreEnt = gamehelpers->ReferenceToEntity(addr[18]);
	info.m_bPrimaryAttack = addr[19];
	info.m_bUseServerRandomSeed = addr[20];
	
	pEntity->FireBullets(info);
	return 0;
}

static cell_t BaseEntitySetAbsOrigin(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity *pEntity = gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}
	
	cell_t *addr = nullptr;
	pContext->LocalToPhysAddr(params[2], &addr);
	Vector pos(sp_ctof(addr[0]), sp_ctof(addr[1]), sp_ctof(addr[2]));

	pEntity->SetAbsOrigin(pos);
	return 0;
}

static cell_t BaseEntityWorldSpaceCenter(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity *pEntity = gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}
	
	cell_t *addr = nullptr;
	pContext->LocalToPhysAddr(params[2], &addr);

	const Vector &pos = pEntity->WorldSpaceCenter();
	addr[0] = sp_ftoc(pos.x);
	addr[1] = sp_ftoc(pos.y);
	addr[2] = sp_ftoc(pos.z);

	return 0;
}

static cell_t BaseAnimatingLookupAttachment(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimating *pEntity = (CBaseAnimating *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}

	char *name = nullptr;
	pContext->LocalToString(params[2], &name);

	return pEntity->LookupAttachment(name);
}

static cell_t BaseAnimatingFindBodygroupByName(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimating *pEntity = (CBaseAnimating *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}

	char *name = nullptr;
	pContext->LocalToString(params[2], &name);

	return pEntity->FindBodygroupByName(name);
}

static cell_t BaseAnimatingLookupBone(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimating *pEntity = (CBaseAnimating *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}

	char *name = nullptr;
	pContext->LocalToString(params[2], &name);

	return pEntity->LookupBone(name);
}

static cell_t BaseAnimatingGetAttachmentEx(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimating *pEntity = (CBaseAnimating *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}

	cell_t *pNullVec = pContext->GetNullRef(SP_NULL_VECTOR);
	
	Vector origin{};
	QAngle angles{};
	bool ret = pEntity->GetAttachment(params[2], origin, angles);
	
	cell_t *addr = nullptr;
	pContext->LocalToPhysAddr(params[3], &addr);
	
	if(addr != pNullVec) {
		addr[0] = sp_ftoc(origin.x);
		addr[1] = sp_ftoc(origin.y);
		addr[2] = sp_ftoc(origin.z);
	}
	
	pContext->LocalToPhysAddr(params[4], &addr);
	
	if(addr != pNullVec) {
		addr[0] = sp_ftoc(angles.x);
		addr[1] = sp_ftoc(angles.y);
		addr[2] = sp_ftoc(angles.z);
	}
	
	return ret;
}

static cell_t BaseAnimatingGetAttachmentLocalEx(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimating *pEntity = (CBaseAnimating *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}

	cell_t *pNullVec = pContext->GetNullRef(SP_NULL_VECTOR);
	
	Vector origin{};
	QAngle angles{};
	bool ret = pEntity->GetAttachmentLocal(params[2], origin, angles);
	
	cell_t *addr = nullptr;
	pContext->LocalToPhysAddr(params[3], &addr);
	
	if(addr != pNullVec) {
		addr[0] = sp_ftoc(origin.x);
		addr[1] = sp_ftoc(origin.y);
		addr[2] = sp_ftoc(origin.z);
	}
	
	pContext->LocalToPhysAddr(params[4], &addr);
	
	if(addr != pNullVec) {
		addr[0] = sp_ftoc(angles.x);
		addr[1] = sp_ftoc(angles.y);
		addr[2] = sp_ftoc(angles.z);
	}
	
	return ret;
}

static cell_t BaseAnimatingGetBonePositionEx(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimating *pEntity = (CBaseAnimating *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}

	cell_t *pNullVec = pContext->GetNullRef(SP_NULL_VECTOR);
	
	Vector origin{};
	QAngle angles{};
	pEntity->GetBonePosition(params[2], origin, angles);
	
	cell_t *addr = nullptr;
	pContext->LocalToPhysAddr(params[3], &addr);
	
	if(addr != pNullVec) {
		addr[0] = sp_ftoc(origin.x);
		addr[1] = sp_ftoc(origin.y);
		addr[2] = sp_ftoc(origin.z);
	}
	
	pContext->LocalToPhysAddr(params[4], &addr);
	
	if(addr != pNullVec) {
		addr[0] = sp_ftoc(angles.x);
		addr[1] = sp_ftoc(angles.y);
		addr[2] = sp_ftoc(angles.z);
	}
	
	return 0;
}

static cell_t BaseAnimatingSequenceDuration(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimating *pEntity = (CBaseAnimating *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}
	
	return sp_ftoc(pEntity->SequenceDuration(params[2]));
}

static cell_t BaseAnimatingSetBodygroup(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimating *pEntity = (CBaseAnimating *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}
	
	pEntity->SetBodygroup(params[2], params[3]);
	return 0;
}

static cell_t BaseAnimatingGetBoneControllerEx(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimating *pEntity = (CBaseAnimating *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}
	
	return sp_ftoc(pEntity->GetBoneController(params[2]));
}

static cell_t BaseAnimatingSetBoneControllerEx(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimating *pEntity = (CBaseAnimating *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}
	
	pEntity->SetBoneController(params[2], sp_ctof(params[3]));
	return 0;
}

static cell_t BaseFlexFindFlexController(IPluginContext *pContext, const cell_t *params)
{
	CBaseFlex *pEntity = (CBaseFlex *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}

	char *name = nullptr;
	pContext->LocalToString(params[2], &name);

	return (cell_t)pEntity->FindFlexController(name);
}

static cell_t BaseFlexSetFlexWeight(IPluginContext *pContext, const cell_t *params)
{
	CBaseFlex *pEntity = (CBaseFlex *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}

	pEntity->SetFlexWeight((LocalFlexController_t)params[2], sp_ctof(params[3]));
	return 0;
}

static cell_t BaseFlexGetFlexWeight(IPluginContext *pContext, const cell_t *params)
{
	CBaseFlex *pEntity = (CBaseFlex *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}
	
	return sp_ftoc(pEntity->GetFlexWeight((LocalFlexController_t)params[2]));
}

static cell_t BaseAnimatingGetNumPoseParameters(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimating *pEntity = (CBaseAnimating *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}
	
	return pEntity->GetNumPoseParameters();
}

static cell_t BaseAnimatingGetNumAttachments(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimating *pEntity = (CBaseAnimating *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}
	
	return pEntity->GetNumAttachments();
}

static cell_t BaseAnimatingGetNumBones(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimating *pEntity = (CBaseAnimating *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}
	
	return pEntity->GetNumBones();
}

static cell_t BaseAnimatingGetNumBodygroups(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimating *pEntity = (CBaseAnimating *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}
	
	return pEntity->GetNumBodygroups();
}

static cell_t BaseAnimatingGetNumBoneControllers(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimating *pEntity = (CBaseAnimating *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}
	
	return pEntity->GetNumBoneControllers();
}

static cell_t BaseAnimatingGetNumSequences(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimating *pEntity = (CBaseAnimating *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}
	
	return pEntity->GetNumSequences();
}

static cell_t BaseAnimatingGetPoseParameterName(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimating *pEntity = (CBaseAnimating *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}

	const char *name = pEntity->GetPoseParameterName(params[2]);
	if(!name) {
		name = "";
	}
	pContext->StringToLocal(params[3], params[4], name);
	return 0;
}

static cell_t BaseAnimatingGetAttachmentName(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimating *pEntity = (CBaseAnimating *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}

	const char *name = pEntity->GetAttachmentName(params[2]);
	if(!name) {
		name = "";
	}
	pContext->StringToLocal(params[3], params[4], name);
	return 0;
}

static cell_t BaseAnimatingGetBoneName(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimating *pEntity = (CBaseAnimating *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}

	const char *name = pEntity->GetBoneName(params[2]);
	if(!name) {
		name = "";
	}
	pContext->StringToLocal(params[3], params[4], name);
	return 0;
}

static cell_t BaseAnimatingGetBodygroupName(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimating *pEntity = (CBaseAnimating *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}

	const char *name = pEntity->GetBodygroupName(params[2]);
	if(!name) {
		name = "";
	}
	pContext->StringToLocal(params[3], params[4], name);
	return 0;
}

static cell_t BaseAnimatingGetSequenceName(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimating *pEntity = (CBaseAnimating *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}

	const char *name = pEntity->GetSequenceName(params[2]);
	if(!name) {
		name = "";
	}
	pContext->StringToLocal(params[3], params[4], name);
	return 0;
}

static cell_t BaseAnimatingGetSequenceActivityName(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimating *pEntity = (CBaseAnimating *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}

	const char *name = pEntity->GetSequenceActivityName(params[2]);
	if(!name) {
		name = "";
	}
	pContext->StringToLocal(params[3], params[4], name);
	return 0;
}

static cell_t BaseFlexGetFlexControllerName(IPluginContext *pContext, const cell_t *params)
{
	CBaseFlex *pEntity = (CBaseFlex *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}

	const char *name = pEntity->GetFlexControllerName((LocalFlexController_t)params[2]);
	if(!name) {
		name = "";
	}
	pContext->StringToLocal(params[3], params[4], name);
	return 0;
}

static cell_t BaseFlexGetNumFlexControllers(IPluginContext *pContext, const cell_t *params)
{
	CBaseFlex *pEntity = (CBaseFlex *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}
	
	return pEntity->GetNumFlexControllers();
}

static cell_t ActivityList_IndexForNameNative(IPluginContext *pContext, const cell_t *params)
{
	char *name = nullptr;
	pContext->LocalToString(params[1], &name);
	
	return ActivityList_IndexForName(name);
}

static cell_t EventList_IndexForNameNative(IPluginContext *pContext, const cell_t *params)
{
	char *name = nullptr;
	pContext->LocalToString(params[1], &name);
	
	return EventList_IndexForName(name);
}

static cell_t ActivityList_NameForIndexNative(IPluginContext *pContext, const cell_t *params)
{
	const char *name = ActivityList_NameForIndex(params[1]);
	if(!name) {
		name = "";
	}
	pContext->StringToLocal(params[2], params[3], name);
	return 0;
}

static cell_t EventList_NameForIndexNative(IPluginContext *pContext, const cell_t *params)
{
	const char *name = EventList_NameForIndex(params[1]);
	if(!name) {
		name = "";
	}
	pContext->StringToLocal(params[2], params[3], name);
	return 0;
}

static const sp_nativeinfo_t g_sNativesInfo[] =
{
	{"BaseEntity.FireBullets", BaseEntityFireBullets},
	{"BaseEntity.SetAbsOrigin", BaseEntitySetAbsOrigin},
	{"BaseEntity.WorldSpaceCenter", BaseEntityWorldSpaceCenter},
	{"BaseAnimating.SelectWeightedSequenceEx", BaseAnimatingSelectWeightedSequenceEx},
	{"BaseAnimating.SelectHeaviestSequenceEx", BaseAnimatingSelectHeaviestSequenceEx},
	{"BaseAnimating.LookupSequence", BaseAnimatingLookupSequence},
	{"BaseAnimating.LookupActivity", BaseAnimatingLookupActivity},
	{"BaseAnimating.LookupPoseParameter", BaseAnimatingLookupPoseParameter},
	{"BaseAnimating.GetPoseParameterName", BaseAnimatingGetPoseParameterName},
	{"BaseAnimating.GetAttachmentName", BaseAnimatingGetAttachmentName},
	{"BaseAnimating.GetAttachmentBoneEx", BaseAnimatingGetAttachmentBone},
	{"BaseAnimating.GetBoneName", BaseAnimatingGetBoneName},
	{"BaseAnimating.GetBodygroupName", BaseAnimatingGetBodygroupName},
	{"BaseAnimating.GetSequenceActivityName", BaseAnimatingGetSequenceActivityName},
	{"BaseAnimating.GetSequenceName", BaseAnimatingGetSequenceName},
	{"BaseFlex.GetFlexControllerName", BaseFlexGetFlexControllerName},
	{"BaseAnimating.GetNumPoseParameters", BaseAnimatingGetNumPoseParameters},
	{"BaseAnimating.GetNumAttachments", BaseAnimatingGetNumAttachments},
	{"BaseAnimating.GetNumBones", BaseAnimatingGetNumBones},
	{"BaseAnimating.GetNumBodygroups", BaseAnimatingGetNumBodygroups},
	{"BaseAnimating.GetNumBoneControllers", BaseAnimatingGetNumBoneControllers},
	{"BaseAnimating.GetNumSequences", BaseAnimatingGetNumSequences},
	{"BaseFlex.GetNumFlexControllers", BaseFlexGetNumFlexControllers},
	{"BaseAnimating.SetPoseParameterEx", BaseAnimatingSetPoseParameter},
	{"BaseAnimating.GetPoseParameterEx", BaseAnimatingGetPoseParameter},
	{"BaseAnimating.StudioFrameAdvance", BaseAnimatingStudioFrameAdvance},
	{"BaseAnimating.DispatchAnimEvents", BaseAnimatingDispatchAnimEvents},
	{"BaseAnimating.ResetSequenceInfo", BaseAnimatingResetSequenceInfo},
	{"BaseAnimating.LookupAttachment", BaseAnimatingLookupAttachment},
	{"BaseAnimating.FindBodygroupByName", BaseAnimatingFindBodygroupByName},
	{"BaseAnimating.GetAttachmentEx", BaseAnimatingGetAttachmentEx},
	{"BaseAnimating.GetAttachmentLocalEx", BaseAnimatingGetAttachmentLocalEx},
	{"BaseAnimating.SetBodygroupEx", BaseAnimatingSetBodygroup},
	{"BaseAnimating.LookupBone", BaseAnimatingLookupBone},
	{"BaseAnimating.GetBonePositionEx", BaseAnimatingGetBonePositionEx},
	{"BaseAnimating.SequenceDurationEx", BaseAnimatingSequenceDuration},
	{"BaseAnimating.GetBoneControllerEx", BaseAnimatingGetBoneControllerEx},
	{"BaseAnimating.SetBoneControllerEx", BaseAnimatingSetBoneControllerEx},
	{"BaseFlex.FindFlexController", BaseFlexFindFlexController},
	{"BaseFlex.SetFlexWeightEx", BaseFlexSetFlexWeight},
	{"BaseFlex.GetFlexWeightEx", BaseFlexGetFlexWeight},
	{"BaseAnimatingOverlay.AddGestureSequenceEx1", BaseAnimatingOverlayAddGestureSequence},
	{"BaseAnimatingOverlay.AddGestureSequenceEx2", BaseAnimatingOverlayAddGestureSequenceEx},
	{"BaseAnimatingOverlay.AddGestureEx1", BaseAnimatingOverlayAddGesture},
	{"BaseAnimatingOverlay.AddGestureEx2", BaseAnimatingOverlayAddGestureEx},
	{"BaseAnimatingOverlay.IsPlayingGestureEx", BaseAnimatingOverlayIsPlayingGesture},
	{"BaseAnimatingOverlay.RestartGestureEx", BaseAnimatingOverlayRestartGesture},
	{"BaseAnimatingOverlay.RemoveGestureEx", BaseAnimatingOverlayRemoveGesture},
	{"BaseAnimatingOverlay.AddLayeredSequenceEx", BaseAnimatingOverlayAddLayeredSequence},
	{"BaseAnimatingOverlay.RemoveAllGestures", BaseAnimatingOverlayRemoveAllGestures},
	{"BaseAnimatingOverlay.SetLayerPriority", BaseAnimatingOverlaySetLayerPriority},
	{"BaseAnimatingOverlay.IsValidLayer", BaseAnimatingOverlayIsValidLayer},
	{"BaseAnimatingOverlay.SetLayerDuration", BaseAnimatingOverlaySetLayerDuration},
	{"BaseAnimatingOverlay.GetLayerDuration", BaseAnimatingOverlayGetLayerDuration},
	{"BaseAnimatingOverlay.SetLayerCycle", BaseAnimatingOverlaySetLayerCycle},
	{"BaseAnimatingOverlay.SetLayerCycleEx", BaseAnimatingOverlaySetLayerCycleEx},
	{"BaseAnimatingOverlay.GetLayerCycle", BaseAnimatingOverlayGetLayerCycle},
	{"BaseAnimatingOverlay.SetLayerPlaybackRate", BaseAnimatingOverlaySetLayerPlaybackRate},
	{"BaseAnimatingOverlay.SetLayerWeight", BaseAnimatingOverlaySetLayerWeight},
	{"BaseAnimatingOverlay.GetLayerWeight", BaseAnimatingOverlayGetLayerWeight},
	{"BaseAnimatingOverlay.SetLayerBlendIn", BaseAnimatingOverlaySetLayerBlendIn},
	{"BaseAnimatingOverlay.SetLayerBlendOut", BaseAnimatingOverlaySetLayerBlendOut},
	{"BaseAnimatingOverlay.SetLayerAutokill", BaseAnimatingOverlaySetLayerAutokill},
	{"BaseAnimatingOverlay.SetLayerLooping", BaseAnimatingOverlaySetLayerLooping},
	{"BaseAnimatingOverlay.SetLayerNoRestore", BaseAnimatingOverlaySetLayerNoRestore},
	{"BaseAnimatingOverlay.GetLayerActivity", BaseAnimatingOverlayGetLayerActivity},
	{"BaseAnimatingOverlay.GetLayerSequence", BaseAnimatingOverlayGetLayerSequence},
	{"BaseAnimatingOverlay.FindGestureLayer", BaseAnimatingOverlayFindGestureLayer},
	{"BaseAnimatingOverlay.RemoveLayer", BaseAnimatingOverlayRemoveLayer},
	{"BaseAnimatingOverlay.FastRemoveLayer", BaseAnimatingOverlayFastRemoveLayer},
	{"BaseAnimatingOverlay.GetNumAnimOverlays", BaseAnimatingOverlayGetNumAnimOverlays},
	{"BaseAnimatingOverlay.SetNumAnimOverlays", BaseAnimatingOverlaySetNumAnimOverlays},
	{"BaseAnimatingOverlay.VerifyOrder", BaseAnimatingOverlayVerifyOrder},
	{"BaseAnimatingOverlay.HasActiveLayer", BaseAnimatingOverlayHasActiveLayer},
	{"BaseAnimatingOverlay.AllocateLayer", BaseAnimatingOverlayAllocateLayer},
	{"BaseAnimatingOverlay.MaxOverlays", BaseAnimatingOverlayMaxOverlays},
	{"ActivityList_IndexForName", ActivityList_IndexForNameNative},
	{"ActivityList_NameForIndex", ActivityList_NameForIndexNative},
	{"EventList_IndexForName", EventList_IndexForNameNative},
	{"EventList_NameForIndex", EventList_NameForIndexNative},
	{nullptr, nullptr},
};

void Sample::OnCoreMapStart(edict_t *pEdictList, int edictCount, int clientMax)
{
	if(g_nActivityListVersion <= 1) {
		ActivityList_Free();
		ActivityList_Init();
		ActivityList_RegisterSharedActivities();
	}

	if(g_nEventListVersion <= 1) {
		EventList_Free();
		EventList_Init();
		EventList_RegisterSharedEvents();
	}
}

bool Sample::SDK_OnMetamodLoad(ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	gpGlobals = ismm->GetCGlobals();
	GET_V_IFACE_CURRENT(GetEngineFactory, modelinfo, IVModelInfo, VMODELINFO_SERVER_INTERFACE_VERSION)
	GET_V_IFACE_CURRENT(GetEngineFactory, filesystem, IFileSystem, FILESYSTEM_INTERFACE_VERSION)
	return true;
}

bool Sample::SDK_OnLoad(char *error, size_t maxlen, bool late)
{
	IGameConfig *g_pGameConf = nullptr;
	gameconfs->LoadGameConfigFile("animhelpers", &g_pGameConf, error, maxlen);
	
	g_pGameConf->GetOffset("CBaseEntity::FireBullets", &CBaseEntityFireBullets);
	g_pGameConf->GetOffset("CBaseEntity::WorldSpaceCenter", &CBaseEntityWorldSpaceCenter);
	g_pGameConf->GetMemSig("CBaseEntity::SetAbsOrigin", &CBaseEntitySetAbsOrigin);
	
	g_pGameConf->GetOffset("CBaseAnimating::m_pStudioHdr", &m_pStudioHdrOffset);
	
	g_pGameConf->GetOffset("CBaseAnimating::StudioFrameAdvance", &CBaseAnimatingStudioFrameAdvance);
	g_pGameConf->GetOffset("CBaseAnimating::DispatchAnimEvents", &CBaseAnimatingDispatchAnimEvents);
	g_pGameConf->GetOffset("CBaseAnimating::GetAttachment", &CBaseAnimatingGetAttachment);
	g_pGameConf->GetOffset("CBaseAnimating::GetBoneTransform", &CBaseAnimatingGetBoneTransform);
	
	g_pGameConf->GetMemSig("CBaseAnimating::ResetSequenceInfo", &CBaseAnimatingResetSequenceInfo);
	g_pGameConf->GetMemSig("CBaseAnimating::LockStudioHdr", &CBaseAnimatingLockStudioHdr);
	g_pGameConf->GetMemSig("CBaseAnimating::CalcAbsolutePosition", &CBaseEntityCalcAbsolutePosition);
	
	gameconfs->CloseGameConfigFile(g_pGameConf);
	
	sm_sendprop_info_t info{};
	gamehelpers->FindSendPropInfo("CBaseAnimating", "m_nSequence", &info);
	m_nSequenceOffset = info.actual_offset;
	
	gamehelpers->FindSendPropInfo("CBaseAnimatingOverlay", "m_AnimOverlay", &info);
	m_AnimOverlayOffset = info.actual_offset;
	
	gamehelpers->FindSendPropInfo("CBaseAnimating", "m_flPoseParameter", &info);
	m_flPoseParameterOffset = info.actual_offset;
	
	gamehelpers->FindSendPropInfo("CBaseAnimating", "m_flEncodedController", &info);
	m_flEncodedControllerOffset = info.actual_offset;
	
	gamehelpers->FindSendPropInfo("CBaseFlex", "m_flexWeight", &info);
	m_flexWeightOffset = info.actual_offset;
	
	gamehelpers->FindSendPropInfo("CBaseAnimating", "m_nBody", &info);
	m_nBodyOffset = info.actual_offset;
	
	gamehelpers->FindSendPropInfo("CBaseAnimating", "m_hLightingOrigin", &info);
	m_pStudioHdrOffset += info.actual_offset;

	sharesys->AddNatives(myself, g_sNativesInfo);
	
	sharesys->RegisterLibrary(myself, "animhelpers");
	
	return true;
}

void Sample::SDK_OnUnload()
{
	
}
