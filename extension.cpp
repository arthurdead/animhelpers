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

#if SOURCE_ENGINE == SE_TF2
	#define TF_DLL
	#define USES_ECON_ITEMS
#elif SOURCE_ENGINE == SE_LEFT4DEAD2
	#define TERROR
	#define LEFT4DEAD
#endif

#if SOURCE_ENGINE == SE_LEFT4DEAD2
	#define PREDICTIONCOPY_H
#endif

#define BASEENTITY_H
#define BASEENTITY_SHARED_H
#define NEXT_BOT
#define GLOWS_ENABLE
#define USE_NAV_MESH
#define RAD_TELEMETRY_DISABLED
#define CBASE_H

#include "extension.h"
#include <CDetour/detours.h>

class IEngineTrace *enginetrace = nullptr;
class IStaticPropMgrServer *staticpropmgr = nullptr;

#include <public/const.h>
#include <public/dt_send.h>
#include <public/networkvar.h>
#include <shared/shareddefs.h>
#include <shared/util_shared.h>
#include <datacache/imdlcache.h>
#include <materialsystem/imaterial.h>
#include <materialsystem/imaterialsystem.h>
#include <bspfile.h>
#include <vphysics_interface.h>

#include <unordered_map>
#include <vector>
#include <string>
#include <functional>
#include <IForwardSys.h>

using namespace std::literals::string_literals;

int *g_nActivityListVersion = nullptr;
int *g_nEventListVersion = nullptr;

class IFileSystem;

IVModelInfo *modelinfo = nullptr;
IMDLCache *mdlcache = nullptr;
IFileSystem *filesystem = nullptr;
IMaterialSystem *materials = nullptr;
ISpatialPartition *partition{nullptr};
IPhysicsCollision *physcollision{nullptr};

#ifdef __HAS_PROXYSEND
class proxysend *proxysend = nullptr;
#endif

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
int m_flPlaybackRateOffset = -1;
int m_flCycleOffset = -1;
int m_vecOriginOffset = -1;
int m_vecAbsOriginOffset = -1;
int m_angRotationOffset = -1;
int m_hMoveParentOffset = -1;
int touchStampOffset = -1;
int m_fDataObjectTypesOffset = -1;
int m_hMoveChildOffset = -1;
int m_hMovePeerOffset = -1;
int m_bSequenceLoopsOffset = -1;
int m_flAnimTimeOffset = -1;
int m_flPrevAnimTimeOffset = -1;

int CBaseAnimatingStudioFrameAdvance = -1;
int CBaseAnimatingDispatchAnimEvents = -1;
int CBaseAnimatingGetAttachment = -1;
int CBaseAnimatingGetBoneTransform = -1;
int CBaseEntityWorldSpaceCenter = -1;
int CBaseEntityEyePosition = -1;

int CBaseAnimatingHandleAnimEvent = -1;

void *CBaseAnimatingResetSequenceInfo = nullptr;
void *CBaseAnimatingLockStudioHdr = nullptr;
void *CBaseEntitySetAbsOrigin = nullptr;
void *CBaseEntityCalcAbsolutePosition = nullptr;

void *ActivityList_NameForIndexPtr = nullptr;
void *ActivityList_IndexForNamePtr = nullptr;
void *ActivityList_RegisterPrivateActivityPtr = nullptr;

void *EventList_NameForIndexPtr = nullptr;
void *EventList_IndexForNamePtr = nullptr;
void *EventList_RegisterPrivateEventPtr = nullptr;
void *EventList_GetEventTypePtr = nullptr;

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

template <typename T>
T void_to_func(void *ptr)
{
	union { T f; void *p; };
	p = ptr;
	return f;
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

#include <shared/shareddefs.h>

#define DECLARE_PREDICTABLE()

#ifndef FMTFUNCTION
#define FMTFUNCTION(...)
#endif

#include <collisionproperty.h>
#include <ehandle.h>
#include <shared/predictioncopy.h>
#include <util.h>
#include <ServerNetworkProperty.h>
#include <vcollide_parse.h>

CBaseEntityList *g_pEntityList = nullptr;

void *CBaseEntityCollisionRulesChanged{nullptr};
void *EntityTouch_AddPtr{nullptr};

void EntityTouch_Add( CBaseEntity *pEntity )
{
	(void_to_func<void (*)(CBaseEntity *)>(EntityTouch_AddPtr))(pEntity);
}

class CBaseEntity : public IServerEntity
{
public:
	int entindex()
	{
		return gamehelpers->EntityToBCompatRef(this);
	}

	model_t *GetModel( void )
	{
		return (model_t *)modelinfo->GetModel( GetModelIndex() );
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
	
	const Vector &GetLocalOrigin()
	{
		if(m_vecOriginOffset == -1) {
			datamap_t *map = gamehelpers->GetDataMap(this);
			sm_datatable_info_t info{};
			gamehelpers->FindDataMapInfo(map, "m_vecOrigin", &info);
			m_vecOriginOffset = info.actual_offset;
		}
		
		return *(Vector *)(((unsigned char *)this) + m_vecOriginOffset);
	}

	const Vector &GetAbsOrigin()
	{
		if(m_vecAbsOriginOffset == -1) {
			datamap_t *map = gamehelpers->GetDataMap(this);
			sm_datatable_info_t info{};
			gamehelpers->FindDataMapInfo(map, "m_vecAbsOrigin", &info);
			m_vecAbsOriginOffset = info.actual_offset;
		}
		
		if(GetIEFlags() & EFL_DIRTY_ABSTRANSFORM) {
			CalcAbsolutePosition();
		}
		
		return *(Vector *)(((unsigned char *)this) + m_vecAbsOriginOffset);
	}

	const QAngle &GetLocalAngles()
	{
		if(m_angRotationOffset == -1) {
			datamap_t *map = gamehelpers->GetDataMap(this);
			sm_datatable_info_t info{};
			gamehelpers->FindDataMapInfo(map, "m_angRotation", &info);
			m_angRotationOffset = info.actual_offset;
		}
		
		return *(QAngle *)(((unsigned char *)this) + m_angRotationOffset);
	}
	
	void CalcAbsolutePosition()
	{
		call_mfunc<void, CBaseEntity>(this, CBaseEntityCalcAbsolutePosition);
	}
	
	const Vector &WorldSpaceCenter()
	{
		return call_vfunc<const Vector &>(this, CBaseEntityWorldSpaceCenter);
	}

	Vector EyePosition()
	{
		return call_vfunc<Vector>(this, CBaseEntityEyePosition);
	}

	inline edict_t			*edict( void )			{ return NetworkProp()->edict(); }
	inline const edict_t	*edict( void ) const	{ return NetworkProp()->edict(); }

	CServerNetworkProperty *NetworkProp() { return (CServerNetworkProperty *)GetNetworkable(); }
	const CServerNetworkProperty *NetworkProp() const { return (const CServerNetworkProperty *)const_cast<CBaseEntity *>(this)->GetNetworkable(); }

	CCollisionProperty		*CollisionProp() { return (CCollisionProperty		*)GetCollideable(); }
	const CCollisionProperty*CollisionProp() const { return (const CCollisionProperty*)const_cast<CBaseEntity *>(this)->GetCollideable(); }

	CBaseEntity *GetRootMoveParent()
	{
		CBaseEntity *pEntity = this;
		CBaseEntity *pParent = this->GetMoveParent();
		while ( pParent )
		{
			pEntity = pParent;
			pParent = pEntity->GetMoveParent();
		}

		return pEntity;
	}

	CBaseEntity *FirstMoveChild()
	{
		if(m_hMoveChildOffset == -1) {
			datamap_t *map = gamehelpers->GetDataMap(this);
			sm_datatable_info_t info{};
			gamehelpers->FindDataMapInfo(map, "m_hMoveChild", &info);
			m_hMoveChildOffset = info.actual_offset;
		}

		return (*(EHANDLE *)(((unsigned char *)this) + m_hMoveChildOffset)).Get();
	}

	CBaseEntity *NextMovePeer()
	{
		if(m_hMovePeerOffset == -1) {
			datamap_t *map = gamehelpers->GetDataMap(this);
			sm_datatable_info_t info{};
			gamehelpers->FindDataMapInfo(map, "m_hMovePeer", &info);
			m_hMovePeerOffset = info.actual_offset;
		}

		return (*(EHANDLE *)(((unsigned char *)this) + m_hMovePeerOffset)).Get();
	}

	CBaseEntity *GetMoveParent()
	{
		if(m_hMoveParentOffset == -1) {
			datamap_t *map = gamehelpers->GetDataMap(this);
			sm_datatable_info_t info{};
			gamehelpers->FindDataMapInfo(map, "m_hMoveParent", &info);
			m_hMoveParentOffset = info.actual_offset;
		}

		return (*(EHANDLE *)(((unsigned char *)this) + m_hMoveParentOffset)).Get();
	}

	SolidType_t GetSolid() const
	{
		return CollisionProp()->GetSolid();
	}

	void AddSolidFlags( int flags )
	{
		CollisionProp()->AddSolidFlags( flags );
	}

	void RemoveSolidFlags( int flags )
	{
		CollisionProp()->RemoveSolidFlags( flags );
	}

	bool HasDataObjectType( int type ) const
	{
		return ( *(int *)((unsigned char *)this + m_fDataObjectTypesOffset)	& (1<<type) ) ? true : false;
	}

	bool IsCurrentlyTouching()
	{
		if ( HasDataObjectType( TOUCHLINK ) )
		{
			return true;
		}

		return false;
	}

	void SetCheckUntouch( bool check )
	{
		if(touchStampOffset == -1) {
			sm_datatable_info_t info{};
			datamap_t *pMap = gamehelpers->GetDataMap(this);
			gamehelpers->FindDataMapInfo(pMap, "touchStamp", &info);
			touchStampOffset = info.actual_offset;
		}

		// Invalidate touchstamp
		if ( check )
		{
			*(int *)((unsigned char *)this + touchStampOffset) += 1;
			if ( !(GetIEFlags() & EFL_CHECK_UNTOUCH) )
			{
				AddIEFlags( EFL_CHECK_UNTOUCH );
				EntityTouch_Add( this );
			}
		}
		else
		{
			RemoveIEFlags( EFL_CHECK_UNTOUCH );
		}
	}

	bool IsSolid()
	{
		return CollisionProp()->IsSolid( );
	}

	void SetSolid( SolidType_t val )
	{
		CollisionProp()->SetSolid( val );
	}

	void DispatchUpdateTransmitState()
	{
		
	}

	void AddIEFlags(int flags)
	{
		if(m_iEFlagsOffset == -1) {
			sm_datatable_info_t info{};
			datamap_t *pMap = gamehelpers->GetDataMap(this);
			gamehelpers->FindDataMapInfo(pMap, "m_iEFlags", &info);
			m_iEFlagsOffset = info.actual_offset;
		}

		*(int *)((unsigned char *)this + m_iEFlagsOffset) |= flags;

		if ( flags & ( EFL_FORCE_CHECK_TRANSMIT | EFL_IN_SKYBOX ) )
		{
			DispatchUpdateTransmitState();
		}
	}

	void RemoveIEFlags(int flags)
	{
		if(m_iEFlagsOffset == -1) {
			sm_datatable_info_t info{};
			datamap_t *pMap = gamehelpers->GetDataMap(this);
			gamehelpers->FindDataMapInfo(pMap, "m_iEFlags", &info);
			m_iEFlagsOffset = info.actual_offset;
		}

		*(int *)((unsigned char *)this + m_iEFlagsOffset) &= ~flags;

		if ( flags & ( EFL_FORCE_CHECK_TRANSMIT | EFL_IN_SKYBOX ) )
		{
			DispatchUpdateTransmitState();
		}
	}

	void CollisionRulesChanged()
	{
		call_mfunc<void, CBaseEntity>(this, CBaseEntityCollisionRulesChanged);
	}

	//GARBAGE!!!
	bool  KeyValue( const char *szKeyName, Vector vec ) { return false; }
	bool  KeyValue( const char *szKeyName, float flValue ) { return false; }
	static void *GetPredictionPlayer() { return nullptr; }
};

static void GetAllChildren_r( CBaseEntity *pEntity, CUtlVector<CBaseEntity *> &list )
{
	for ( ; pEntity != NULL; pEntity = pEntity->NextMovePeer() )
	{
		list.AddToTail( pEntity );
		GetAllChildren_r( pEntity->FirstMoveChild(), list );
	}
}

int GetAllChildren( CBaseEntity *pParent, CUtlVector<CBaseEntity *> &list )
{
	if ( !pParent )
		return 0;

	GetAllChildren_r( pParent->FirstMoveChild(), list );
	return list.Count();
}

void CCollisionProperty::MarkSurroundingBoundsDirty()
{
	GetOuter()->AddIEFlags( EFL_DIRTY_SURROUNDING_COLLISION_BOUNDS );
	MarkPartitionHandleDirty();

#ifdef CLIENT_DLL
	g_pClientShadowMgr->MarkRenderToTextureShadowDirty( GetOuter()->GetShadowHandle() );
#else
	GetOuter()->NetworkProp()->MarkPVSInformationDirty();
#endif
}

void CCollisionProperty::MarkPartitionHandleDirty()
{
	// don't bother with the world
	if ( m_pOuter->entindex() == 0 )
		return;
	
	if ( !(m_pOuter->GetIEFlags() & EFL_DIRTY_SPATIAL_PARTITION) )
	{
		m_pOuter->AddIEFlags( EFL_DIRTY_SPATIAL_PARTITION );
		//s_DirtyKDTree.AddEntity( m_pOuter );
	}

#ifdef CLIENT_DLL
	GetOuter()->MarkRenderHandleDirty();
	g_pClientShadowMgr->AddToDirtyShadowList( GetOuter() );
#endif
}

void CCollisionProperty::UpdateServerPartitionMask( )
{
#ifndef CLIENT_DLL
	SpatialPartitionHandle_t handle = GetPartitionHandle();
	if ( handle == PARTITION_INVALID_HANDLE )
		return;

	// Remove it from whatever lists it may be in at the moment
	// We'll re-add it below if we need to.
	::partition->Remove( handle );

	// Don't bother with deleted things
	if ( !m_pOuter->edict() )
		return;

	// don't add the world
	if ( m_pOuter->entindex() == 0 )
		return;		

	// Make sure it's in the list of all entities
	bool bIsSolid = IsSolid() || IsSolidFlagSet(FSOLID_TRIGGER);
	if ( bIsSolid || m_pOuter->GetIEFlags() & EFL_USE_PARTITION_WHEN_NOT_SOLID )
	{
		::partition->Insert( PARTITION_ENGINE_NON_STATIC_EDICTS, handle );
	}

	if ( !bIsSolid )
		return;

	// Insert it into the appropriate lists.
	// We have to continually reinsert it because its solid type may have changed
	SpatialPartitionListMask_t mask = 0;
	if ( !IsSolidFlagSet(FSOLID_NOT_SOLID) )
	{
		mask |=	PARTITION_ENGINE_SOLID_EDICTS;
	}
	if ( IsSolidFlagSet(FSOLID_TRIGGER) )
	{
		mask |=	PARTITION_ENGINE_TRIGGER_EDICTS;
	}
	Assert( mask != 0 );
	::partition->Insert( mask, handle );
#endif
}

void CCollisionProperty::CheckForUntouch()
{
#ifndef CLIENT_DLL
	if ( !IsSolid() && !IsSolidFlagSet(FSOLID_TRIGGER))
	{
		// If this ent's touch list isn't empty, it's transitioning to not solid
		if ( m_pOuter->IsCurrentlyTouching() )
		{
			// mark ent so that at the end of frame it will check to 
			// see if it's no longer touching ents
			m_pOuter->SetCheckUntouch( true );
		}
	}
#endif
}

void CCollisionProperty::SetSolidFlags( int flags )
{
	int oldFlags = m_usSolidFlags;
	m_usSolidFlags = (unsigned short)(flags & 0xFFFF);
	if ( oldFlags == m_usSolidFlags )
		return;

	// These two flags, if changed, can produce different surrounding bounds
	if ( (oldFlags & (FSOLID_FORCE_WORLD_ALIGNED | FSOLID_USE_TRIGGER_BOUNDS)) != 
		 (m_usSolidFlags & (FSOLID_FORCE_WORLD_ALIGNED | FSOLID_USE_TRIGGER_BOUNDS)) )
	{
		MarkSurroundingBoundsDirty();
	}

	if ( (oldFlags & (FSOLID_NOT_SOLID|FSOLID_TRIGGER)) != (m_usSolidFlags & (FSOLID_NOT_SOLID|FSOLID_TRIGGER)) )
	{
		m_pOuter->CollisionRulesChanged();
	}

#ifndef CLIENT_DLL
	if ( (oldFlags & (FSOLID_NOT_SOLID | FSOLID_TRIGGER)) != (m_usSolidFlags & (FSOLID_NOT_SOLID | FSOLID_TRIGGER)) )
	{
		UpdateServerPartitionMask( );
		CheckForUntouch();
	}
#endif
}

void CCollisionProperty::SetSolid( SolidType_t val )
{
	if ( m_nSolidType == val )
		return;

#ifndef CLIENT_DLL
	bool bWasNotSolid = IsSolid();
#endif

	MarkSurroundingBoundsDirty();

	// OBB is not yet implemented
	if ( val == SOLID_BSP )
	{
		if ( GetOuter()->GetMoveParent() )
		{
			if ( GetOuter()->GetRootMoveParent()->GetSolid() != SOLID_BSP )
			{
				// must be SOLID_VPHYSICS because parent might rotate
				val = SOLID_VPHYSICS;
			}
		}
#ifndef CLIENT_DLL
		// UNDONE: This should be fine in the client DLL too.  Move GetAllChildren() into shared code.
		// If the root of the hierarchy is SOLID_BSP, then assume that the designer
		// wants the collisions to rotate with this hierarchy so that the player can
		// move while riding the hierarchy.
		if ( !GetOuter()->GetMoveParent() )
		{
			// NOTE: This assumes things don't change back from SOLID_BSP
			// NOTE: This is 100% true for HL2 - need to support removing the flag to support changing from SOLID_BSP
			CUtlVector<CBaseEntity *> list;
			GetAllChildren( GetOuter(), list );
			for ( int i = list.Count()-1; i>=0; --i )
			{
				list[i]->AddSolidFlags( FSOLID_ROOT_PARENT_ALIGNED );
			}
		}
#endif
	}

	m_nSolidType = val;

#ifndef CLIENT_DLL
	m_pOuter->CollisionRulesChanged();

	UpdateServerPartitionMask( );

	if ( bWasNotSolid != IsSolid() )
	{
		CheckForUntouch();
	}
#endif
}

#if SOURCE_ENGINE == SE_LEFT4DEAD2
enum
{
	kActivityLookup_Unknown = -2,			// hasn't been searched for
	kActivityLookup_Missing = -1,			// has been searched for but wasn't found
};
#endif

#include <mathlib/vmatrix.h>
#include <ehandle.h>
#include <predictioncopy.h>
#include <shared/ai_activity.h>
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

#if SOURCE_ENGINE == SE_LEFT4DEAD2
#undef stackalloc
#define stackalloc( _size )		alloca( ALIGN_VALUE( _size, 16 ) )
#endif

#ifndef FMTFUNCTION
#define FMTFUNCTION(...)
#endif

#include <animation.cpp>
#include <studio.cpp>
#include <studio_shared.cpp>
#include <bone_setup.cpp>
#include <stringregistry.cpp>
#define ListFromString ListFromStringEvent
//#include <eventlist.cpp>
#undef ListFromString
#define ListFromString ListFromStringActivity
//#include "activitylist.cpp"
#undef ListFromString
#include <collisionutils.cpp>

Activity ActivityList_RegisterPrivateActivity( const char *pszActivityName )
{
	return ((Activity(*)(const char *))ActivityList_RegisterPrivateActivityPtr)(pszActivityName);
}

int ActivityList_IndexForName( const char *pszActivityName )
{
	return ((int(*)(const char *))ActivityList_IndexForNamePtr)(pszActivityName);
}

const char *ActivityList_NameForIndex( int iActivityIndex )
{
	return ((const char *(*)(int))ActivityList_NameForIndexPtr)(iActivityIndex);
}

Animevent EventList_RegisterPrivateEvent( const char *pszEventName )
{
	return ((Animevent(*)(const char *))EventList_RegisterPrivateEventPtr)(pszEventName);
}

int EventList_IndexForName( const char *pszEventName )
{
	return ((int(*)(const char *))EventList_IndexForNamePtr)(pszEventName);
}

const char *EventList_NameForIndex( int iEventIndex )
{
	return ((const char *(*)(int))EventList_NameForIndexPtr)(iEventIndex);
}

int EventList_GetEventType( int eventIndex )
{
	return ((int(*)(int))EventList_GetEventTypePtr)(eventIndex);
}

int SharedRandomInt( const char *sharedname, int iMinVal, int iMaxVal, int additionalSeed /*=0*/ )
{
	return RandomInt( iMinVal, iMaxVal );
}

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
	
	float GetPlaybackRate()
	{
		return *(float *)((unsigned char *)this + m_flPlaybackRateOffset);
	}
	
	float GetCycle()
	{
		return *(float *)((unsigned char *)this + m_flCycleOffset);
	}
	
	float GetAnimTime()
	{
		return *(int *)((unsigned char *)this + m_flAnimTimeOffset);
	}
	
	float GetPrevAnimTime()
	{
		if(m_flPrevAnimTimeOffset == -1) {
			datamap_t *map = gamehelpers->GetDataMap(this);
			sm_datatable_info_t info{};
			gamehelpers->FindDataMapInfo(map, "m_flPrevAnimTime", &info);
			m_flPrevAnimTimeOffset = info.actual_offset;
		}
		
		return *(float *)(((unsigned char *)this) + m_flPrevAnimTimeOffset);
	}
	
	float *GetPoseParameterArray()
	{
		return &*(float *)((unsigned char *)this + m_flPoseParameterOffset);
	}
	
	float *GetEncodedControllerArray()
	{
		return &*(float *)((unsigned char *)this + m_flEncodedControllerOffset);
	}
	
	bool SequenceLoops()
	{
		if(m_bSequenceLoopsOffset == -1) {
			datamap_t *map = gamehelpers->GetDataMap(this);
			sm_datatable_info_t info{};
			gamehelpers->FindDataMapInfo(map, "m_bSequenceLoops", &info);
			m_bSequenceLoopsOffset = info.actual_offset;
		}
		
		return *(bool *)(((unsigned char *)this) + m_bSequenceLoopsOffset);
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
	
	void GetBoneTransform( int iAttachment, matrix3x4_t &attachmentToWorld )
	{
		call_vfunc<void, CBaseAnimating, int, matrix3x4_t &>(this, CBaseAnimatingGetBoneTransform, iAttachment, attachmentToWorld);
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

	bool GetBonePositionLocal( const char *szName, Vector &origin, QAngle &angles );
	bool GetBonePositionLocal( int iAttachment, Vector &origin, QAngle &angles );
	bool GetBonePositionLocal( int iAttachment, matrix3x4_t &attachmentToLocal );
	
	void SetBodygroup( int iGroup, int iValue );
	
	bool GetBonePosition ( int iBone, Vector &origin, QAngle &angles );
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
	float SequenceDuration( CStudioHdr *pStudioHdr, int sequence);
	int GetSequenceFlags(int sequence);

	int SequenceNumFrames(int sequence);
	float SequenceFPS(int sequence);
	float SequenceCPS(int sequence);
	
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
	
	float GetSequenceCycleRate( CStudioHdr *pStudioHdr, int iSequence );
	float GetSequenceCycleRate( int iSequence );
	
	bool GetIntervalMovement( float flIntervalUsed, bool &bMoveSeqFinished, Vector &newPosition, QAngle &newAngles );
	bool GetSequenceMovement( float flIntervalUsed, bool &bMoveSeqFinished, Vector &deltaPosition, QAngle &deltaAngles );
	
	float GetAnimTimeInterval( void );
	
	bool GetIntervalMovement( bool &bMoveSeqFinished, Vector &newPosition, QAngle &newAngles )
	{ return GetIntervalMovement(GetAnimTimeInterval(), bMoveSeqFinished, newPosition, newAngles); }
	bool GetSequenceMovement( bool &bMoveSeqFinished, Vector &newPosition, QAngle &newAngles )
	{ return GetSequenceMovement(GetAnimTimeInterval(), bMoveSeqFinished, newPosition, newAngles); }
	
	Activity GetSequenceActivity( int iSequence );

	void HandleAnimEvent(animevent_t *pEvent)
	{
		call_vfunc<void, CBaseAnimating, animevent_t *>(this, CBaseAnimatingHandleAnimEvent, pEvent);
	}
};

Activity CBaseAnimating::GetSequenceActivity( int iSequence )
{
	if( iSequence == -1 )
	{
		return ACT_INVALID;
	}

	if ( !GetModelPtr() )
		return ACT_INVALID;

	return (Activity)::GetSequenceActivity( GetModelPtr(), iSequence );
}

float CBaseAnimating::GetSequenceCycleRate( CStudioHdr *pStudioHdr, int iSequence )
{
	float t = SequenceDuration( pStudioHdr, iSequence );

	if ( t != 0.0f )
	{
		return 1.0f / t;
	}
	return t;
}

float CBaseAnimating::GetSequenceCycleRate( int iSequence )
{
	CStudioHdr *pstudiohdr = GetModelPtr( );
	
	return GetSequenceCycleRate(pstudiohdr, iSequence);
}

#define MAX_ANIMTIME_INTERVAL 0.2f

float CBaseAnimating::GetAnimTimeInterval( void )
{
	float flInterval;
	if (GetAnimTime() < gpGlobals->curtime)
	{
		// estimate what it'll be this frame
		flInterval = clamp( gpGlobals->curtime - GetAnimTime(), 0.f, MAX_ANIMTIME_INTERVAL );
	}
	else
	{
		// report actual
		flInterval = clamp( GetAnimTime() - GetPrevAnimTime(), 0.f, MAX_ANIMTIME_INTERVAL );
	}
	return flInterval;
}

bool CBaseAnimating::GetSequenceMovement( float flIntervalUsed, bool &bMoveSeqFinished, Vector &newPosition, QAngle &newAngles )
{
	CStudioHdr *pstudiohdr = GetModelPtr( );
	if (! pstudiohdr || !pstudiohdr->SequencesAvailable())
		return false;

	float flComputedCycleRate = GetSequenceCycleRate( pstudiohdr, GetSequence() );
	
	float flNextCycle = GetCycle() + flIntervalUsed * flComputedCycleRate * GetPlaybackRate();

	if ((!SequenceLoops()) && flNextCycle > 1.0)
	{
		flIntervalUsed = GetCycle() / (flComputedCycleRate * GetPlaybackRate());
		flNextCycle = 1.0;
		bMoveSeqFinished = true;
	}
	else
	{
		bMoveSeqFinished = false;
	}

	Vector deltaPos;
	QAngle deltaAngles;

	if (Studio_SeqMovement( pstudiohdr, GetSequence(), GetCycle(), flNextCycle, GetPoseParameterArray(), deltaPos, deltaAngles ))
	{
		VectorYawRotate( deltaPos, GetLocalAngles().y, deltaPos );
		newPosition = deltaPos;
		newAngles.Init();
		newAngles.y = deltaAngles.y;
		return true;
	}
	else
	{
		return false;
	}
}

bool CBaseAnimating::GetIntervalMovement( float flIntervalUsed, bool &bMoveSeqFinished, Vector &newPosition, QAngle &newAngles )
{
	CStudioHdr *pstudiohdr = GetModelPtr( );
	if (! pstudiohdr || !pstudiohdr->SequencesAvailable())
		return false;

	float flComputedCycleRate = GetSequenceCycleRate( pstudiohdr, GetSequence() );
	
	float flNextCycle = GetCycle() + flIntervalUsed * flComputedCycleRate * GetPlaybackRate();

	if ((!SequenceLoops()) && flNextCycle > 1.0)
	{
		flIntervalUsed = GetCycle() / (flComputedCycleRate * GetPlaybackRate());
		flNextCycle = 1.0;
		bMoveSeqFinished = true;
	}
	else
	{
		bMoveSeqFinished = false;
	}

	Vector deltaPos;
	QAngle deltaAngles;

	if (Studio_SeqMovement( pstudiohdr, GetSequence(), GetCycle(), flNextCycle, GetPoseParameterArray(), deltaPos, deltaAngles ))
	{
		VectorYawRotate( deltaPos, GetLocalAngles().y, deltaPos );
		newPosition = GetLocalOrigin() + deltaPos;
		newAngles.Init();
		newAngles.y = GetLocalAngles().y + deltaAngles.y;
		return true;
	}
	else
	{
		newPosition = GetLocalOrigin();
		newAngles = GetLocalAngles();
		return false;
	}
}

bool CBaseAnimating::GetAttachmentLocal( const char *szName, Vector &origin, QAngle &angles )
{
	return GetAttachmentLocal( LookupAttachment( szName ), origin, angles );
}

bool CBaseAnimating::GetBonePositionLocal( const char *szName, Vector &origin, QAngle &angles )
{
	return GetBonePositionLocal( LookupBone( szName ), origin, angles );
}

bool CBaseAnimating::GetAttachmentLocal( int iAttachment, Vector &origin, QAngle &angles )
{
	matrix3x4_t attachmentToEntity;

	bool bRet = GetAttachmentLocal( iAttachment, attachmentToEntity );
	MatrixAngles( attachmentToEntity, angles, origin );
	return bRet;
}

bool CBaseAnimating::GetBonePositionLocal( int iAttachment, Vector &origin, QAngle &angles )
{
	matrix3x4_t attachmentToEntity;

	bool bRet = GetBonePositionLocal( iAttachment, attachmentToEntity );
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

bool CBaseAnimating::GetBonePositionLocal( int iAttachment, matrix3x4_t &attachmentToLocal )
{
	matrix3x4_t attachmentToWorld;
	GetBoneTransform(iAttachment, attachmentToWorld);
	matrix3x4_t worldToEntity;
	MatrixInvert( EntityToWorldTransform(), worldToEntity );
	ConcatTransforms( worldToEntity, attachmentToWorld, attachmentToLocal ); 
	return true;
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

bool CBaseAnimating::GetBonePosition ( int iBone, Vector &origin, QAngle &angles )
{
	CStudioHdr *pStudioHdr = GetModelPtr( );
	if (!pStudioHdr)
	{
		return false;
	}

	if (iBone < 0 || iBone >= pStudioHdr->numbones())
	{
		return false;
	}

	matrix3x4_t bonetoworld;
	GetBoneTransform( iBone, bonetoworld );
	
	MatrixAngles( bonetoworld, angles, origin );
	return true;
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

float CBaseAnimating::SequenceDuration(CStudioHdr *pStudioHdr, int sequence)
{
	if(!pStudioHdr)
		return 0.1f;

	if(!pStudioHdr->SequencesAvailable())
		return 0.1f;

	if(sequence >= pStudioHdr->GetNumSeq() || sequence < 0)
		return 0.1f;

	return Studio_Duration(pStudioHdr, sequence, GetPoseParameterArray());
}

float CBaseAnimating::SequenceDuration(int sequence)
{
	CStudioHdr *pStudioHdr = GetModelPtr();

	return SequenceDuration(pStudioHdr, sequence);
}

int CBaseAnimating::SequenceNumFrames(int sequence)
{
	CStudioHdr *pStudioHdr = GetModelPtr();

	if(!pStudioHdr)
		return 0;

	if(!pStudioHdr->SequencesAvailable())
		return 0;

	if(sequence >= pStudioHdr->GetNumSeq() || sequence < 0)
		return 0;

	return Studio_MaxFrame(pStudioHdr, sequence, GetPoseParameterArray());
}

float CBaseAnimating::SequenceFPS(int sequence)
{
	CStudioHdr *pStudioHdr = GetModelPtr();

	if(!pStudioHdr)
		return 0.1f;

	if(!pStudioHdr->SequencesAvailable())
		return 0.1f;

	if(sequence >= pStudioHdr->GetNumSeq() || sequence < 0)
		return 0.1f;

	return Studio_FPS(pStudioHdr, sequence, GetPoseParameterArray());
}

float CBaseAnimating::SequenceCPS(int sequence)
{
	CStudioHdr *pStudioHdr = GetModelPtr();

	if(!pStudioHdr)
		return 0.1f;

	if(!pStudioHdr->SequencesAvailable())
		return 0.1f;

	if(sequence >= pStudioHdr->GetNumSeq() || sequence < 0)
		return 0.1f;

	mstudioseqdesc_t &seqdesc = ((CStudioHdr *)pStudioHdr)->pSeqdesc( sequence );

	return Studio_CPS(pStudioHdr, seqdesc, sequence, GetPoseParameterArray());
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

	if(index < 0 || index >= pStudioHdr->GetNumPoseParameters())
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

	if(index < 0 || index >= pStudioHdr->GetNumPoseParameters())
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
	
	if(activity < 0)
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
	
	if(activity < 0)
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

static cell_t BaseEntityEyePosition(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity *pEntity = gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}
	
	cell_t *addr = nullptr;
	pContext->LocalToPhysAddr(params[2], &addr);

	Vector pos = pEntity->EyePosition();
	addr[0] = sp_ftoc(pos.x);
	addr[1] = sp_ftoc(pos.y);
	addr[2] = sp_ftoc(pos.z);

	return 0;
}

static cell_t BaseEntityGetAbsOrigin(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity *pEntity = gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}
	
	cell_t *addr = nullptr;
	pContext->LocalToPhysAddr(params[2], &addr);

	const Vector &pos = pEntity->GetAbsOrigin();
	addr[0] = sp_ftoc(pos.x);
	addr[1] = sp_ftoc(pos.y);
	addr[2] = sp_ftoc(pos.z);

	return 0;
}

static cell_t BaseEntityIsSolid(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity *pEntity = gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}
	
	return pEntity->IsSolid();
}

static cell_t BaseEntitySetSolid(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity *pEntity = gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}
	
	pEntity->SetSolid((SolidType_t)params[2]);
	return 0;
}

static cell_t BaseEntityAddSolidFlags(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity *pEntity = gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}
	
	pEntity->AddSolidFlags(params[2]);
	return 0;
}

static cell_t BaseEntityRemoveSolidFlags(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity *pEntity = gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}
	
	pEntity->RemoveSolidFlags(params[2]);
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

static cell_t BaseAnimatingGetAttachmentMatrixEx(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimating *pEntity = (CBaseAnimating *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}

	matrix3x4_t matrix{};
	bool ret = pEntity->GetAttachment(params[2], matrix);
	
	cell_t *addr = nullptr;
	pContext->LocalToPhysAddr(params[3], &addr);

	if(pContext->GetRuntime()->UsesDirectArrays()) {

	} else {

	}
	
	return false;
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

static cell_t BaseAnimatingGetBonePositionLocalEx(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimating *pEntity = (CBaseAnimating *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}

	cell_t *pNullVec = pContext->GetNullRef(SP_NULL_VECTOR);
	
	Vector origin{};
	QAngle angles{};
	bool ret = pEntity->GetBonePositionLocal(params[2], origin, angles);
	
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
	bool ret = pEntity->GetBonePosition(params[2], origin, angles);
	
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

static cell_t BaseAnimatingSequenceDuration(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimating *pEntity = (CBaseAnimating *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}
	
	return sp_ftoc(pEntity->SequenceDuration(params[2]));
}

static cell_t BaseAnimatingSequenceNumFrames(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimating *pEntity = (CBaseAnimating *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}
	
	return pEntity->SequenceNumFrames(params[2]);
}

static cell_t BaseAnimatingSequenceCPS(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimating *pEntity = (CBaseAnimating *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}
	
	return sp_ftoc(pEntity->SequenceCPS(params[2]));
}

static cell_t BaseAnimatingSequenceFPS(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimating *pEntity = (CBaseAnimating *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}
	
	return sp_ftoc(pEntity->SequenceFPS(params[2]));
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
	size_t written = 0;
	pContext->StringToLocalUTF8(params[3], params[4], name, &written);
	return written;
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
	size_t written = 0;
	pContext->StringToLocalUTF8(params[3], params[4], name, &written);
	return written;
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
	size_t written = 0;
	pContext->StringToLocalUTF8(params[3], params[4], name, &written);
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
	size_t written = 0;
	pContext->StringToLocalUTF8(params[3], params[4], name, &written);
	return written;
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
	size_t written = 0;
	pContext->StringToLocalUTF8(params[3], params[4], name, &written);
	return written;
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
	size_t written = 0;
	pContext->StringToLocalUTF8(params[3], params[4], name, &written);
	return written;
}

static cell_t BaseAnimatingGetSequenceActivity(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimating *pEntity = (CBaseAnimating *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}
	
	return (cell_t)pEntity->GetSequenceActivity(params[2]);
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
	size_t written = 0;
	pContext->StringToLocalUTF8(params[3], params[4], name, &written);
	return written;
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

static cell_t ActivityList_RegisterPrivateActivityNative(IPluginContext *pContext, const cell_t *params)
{
	char *name = nullptr;
	pContext->LocalToString(params[1], &name);
	
	return ActivityList_RegisterPrivateActivity(name);
}

static cell_t EventList_IndexForNameNative(IPluginContext *pContext, const cell_t *params)
{
	char *name = nullptr;
	pContext->LocalToString(params[1], &name);
	
	return EventList_IndexForName(name);
}

static cell_t EventList_RegisterPrivateEventNative(IPluginContext *pContext, const cell_t *params)
{
	char *name = nullptr;
	pContext->LocalToString(params[1], &name);
	
	return EventList_RegisterPrivateEvent(name);
}

static cell_t ActivityList_NameForIndexNative(IPluginContext *pContext, const cell_t *params)
{
	const char *name = ActivityList_NameForIndex(params[1]);
	if(!name) {
		name = "";
	}
	size_t written = 0;
	pContext->StringToLocalUTF8(params[2], params[3], name, &written);
	return written;
}

static cell_t EventList_NameForIndexNative(IPluginContext *pContext, const cell_t *params)
{
	const char *name = EventList_NameForIndex(params[1]);
	if(!name) {
		name = "";
	}
	size_t written = 0;
	pContext->StringToLocalUTF8(params[2], params[3], name, &written);
	return written;
}

SH_DECL_MANUALHOOK0_void(GenericDtor, 1, 0, 0)
SH_DECL_MANUALHOOK1_void(HandleAnimEvent, 0, 0, 0, animevent_t *)

#define ANIMEVENT_OPTIONS_STR_SIZE 64
#define ANIMEVENT_OPTIONS_STR_SIZE_IN_CELL (ANIMEVENT_OPTIONS_STR_SIZE / sizeof(cell_t))
#define ANIMEVENT_STRUCT_SIZE (sizeof(cell_t) + ANIMEVENT_OPTIONS_STR_SIZE + (sizeof(cell_t) * 4))
#define ANIMEVENT_STRUCT_SIZE_IN_CELL (ANIMEVENT_STRUCT_SIZE / sizeof(cell_t))

void AnimEventToAddr(animevent_t *pEvent, cell_t *addr)
{
	cell_t *tmp_addr{addr};

#if SOURCE_ENGINE == SE_TF2
	*tmp_addr = pEvent->event;
#elif SOURCE_ENGINE == SE_LEFT4DEAD2
	*tmp_addr = pEvent->Event();
#endif
	++tmp_addr;

	strncpy((char *)tmp_addr, pEvent->options, strlen(pEvent->options)+1);
	tmp_addr += ANIMEVENT_OPTIONS_STR_SIZE_IN_CELL;

	*tmp_addr = sp_ftoc(pEvent->cycle);
	++tmp_addr;

	*tmp_addr = sp_ftoc(pEvent->eventtime);
	++tmp_addr;

	*tmp_addr = pEvent->type;
	++tmp_addr;

	*tmp_addr = gamehelpers->EntityToBCompatRef(pEvent->pSource);
}

void AddrToAnimEvent(animevent_t *pEvent, const cell_t *addr)
{
	const cell_t *tmp_addr{addr};

#if SOURCE_ENGINE == SE_TF2
	pEvent->event = *tmp_addr;
#elif SOURCE_ENGINE == SE_LEFT4DEAD2
	
#endif
	++tmp_addr;

	pEvent->options = (const char *)tmp_addr;
	tmp_addr += ANIMEVENT_OPTIONS_STR_SIZE_IN_CELL;

	pEvent->cycle = sp_ctof(*tmp_addr);
	++tmp_addr;

	pEvent->eventtime = sp_ctof(*tmp_addr);
	++tmp_addr;

	pEvent->type = sp_ctof(*tmp_addr);
	++tmp_addr;

	pEvent->pSource = (CBaseAnimating *)gamehelpers->ReferenceToEntity(*tmp_addr);
}

SH_DECL_MANUALHOOK0_void(UpdateOnRemove, 0, 0, 0)

struct callback_holder_t
{
	IChangeableForward *fwd = nullptr;
	std::vector<IdentityToken_t *> owners{};
	int ref = -1;
	std::vector<int> hookids{};

	callback_holder_t(CBaseEntity *pEntity, int ref_);
	~callback_holder_t();

	void removed(CBaseEntity *pEntity)
	{
		for(int id : hookids) {
			SH_REMOVE_HOOK_ID(id);
		}
	}

	void HookEntityRemove();
	
	void HookHandleAnimEvent(animevent_t *pEvent)
	{
		if(!fwd || fwd->GetFunctionCount() == 0) {
			RETURN_META(MRES_IGNORED);
		}

		cell_t addr[ANIMEVENT_STRUCT_SIZE_IN_CELL]{0};
		AnimEventToAddr(pEvent, addr);
		
		CBaseEntity *pEntity = META_IFACEPTR(CBaseEntity);
		
		fwd->PushCell(gamehelpers->EntityToBCompatRef(pEntity));
		fwd->PushArray(addr, ANIMEVENT_STRUCT_SIZE_IN_CELL, SM_PARAM_COPYBACK);
		cell_t res = 0;
		fwd->Execute(&res);
		
		switch(res) {
			case Pl_Continue: {
				RETURN_META(MRES_HANDLED);
			}
			case Pl_Changed: {
				animevent_t copy{};
				AddrToAnimEvent(&copy, addr);
				
				RETURN_META_MNEWPARAMS(MRES_HANDLED, HandleAnimEvent, (&copy));
			}
			case Pl_Handled:
			case Pl_Stop: {
				RETURN_META(MRES_SUPERCEDE);
			}
		}
	}
};

using callback_holder_map_t = std::unordered_map<int, callback_holder_t *>;
callback_holder_map_t callbackmap{};

void callback_holder_t::HookEntityRemove()
{
	CBaseEntity *pEntity = META_IFACEPTR(CBaseEntity);
	int this_ref = gamehelpers->EntityToReference(pEntity);
	removed(pEntity);
	callbackmap.erase(this_ref);
	delete this;
	RETURN_META(MRES_HANDLED);
}

callback_holder_t::callback_holder_t(CBaseEntity *pEntity, int ref_)
	: ref{ref_}
{
	hookids.emplace_back(SH_ADD_MANUALHOOK(UpdateOnRemove, pEntity, SH_MEMBER(this, &callback_holder_t::HookEntityRemove), false));
	hookids.emplace_back(SH_ADD_MANUALHOOK(HandleAnimEvent, pEntity, SH_MEMBER(this, &callback_holder_t::HookHandleAnimEvent), false));

	fwd = forwards->CreateForwardEx(nullptr, ET_Hook, 2, nullptr, Param_Cell, Param_Array);

	callbackmap.emplace(ref, this);
}

callback_holder_t::~callback_holder_t()
{
	if(fwd) {
		forwards->ReleaseForward(fwd);
	}
}

static cell_t BaseAnimatingSetHandleAnimEvent(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity *pEntity = gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}
	
	callback_holder_t *holder = nullptr;

	int ref = gamehelpers->EntityToReference(pEntity);
	
	callback_holder_map_t::iterator it{callbackmap.find(ref)};
	if(it != callbackmap.end()) {
		holder = it->second;
	} else {
		holder = new callback_holder_t{pEntity, ref};
	}
	
	IPluginFunction *func = pContext->GetFunctionById(params[2]);

	holder->fwd->RemoveFunction(func);
	holder->fwd->AddFunction(func);

	IdentityToken_t *iden{pContext->GetIdentity()};
	if(std::find(holder->owners.cbegin(), holder->owners.cend(), iden) == holder->owners.cend()) {
		holder->owners.emplace_back(iden);
	}
	
	return 0;
}

static cell_t BaseAnimatingHandleAnimEvent(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimating *pEntity = (CBaseAnimating *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}

	cell_t *addr = nullptr;
	pContext->LocalToPhysAddr(params[2], &addr);

	animevent_t event{};
	AddrToAnimEvent(&event, addr);

	pEntity->HandleAnimEvent(&event);
	return 0;
}

static cell_t BaseAnimatingGetIntervalMovement(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimating *pEntity = (CBaseAnimating *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}
	
	bool bMoveSeqFinished = false;
	Vector newPosition{};
	QAngle newAngles{};
	bool ret = pEntity->GetIntervalMovement(bMoveSeqFinished, newPosition, newAngles);
	
	cell_t *addr = nullptr;
	pContext->LocalToPhysAddr(params[2], &addr);
	
	*addr = bMoveSeqFinished;
	
	cell_t *pNullVec = pContext->GetNullRef(SP_NULL_VECTOR);
	
	pContext->LocalToPhysAddr(params[3], &addr);
	
	if(addr != pNullVec) {
		addr[0] = sp_ftoc(newPosition.x);
		addr[1] = sp_ftoc(newPosition.y);
		addr[2] = sp_ftoc(newPosition.z);
	}
	
	pContext->LocalToPhysAddr(params[4], &addr);
	
	if(addr != pNullVec) {
		addr[0] = sp_ftoc(newAngles.x);
		addr[1] = sp_ftoc(newAngles.y);
		addr[2] = sp_ftoc(newAngles.z);
	}
	
	return ret;
}

static cell_t BaseAnimatingGetSequenceMovement(IPluginContext *pContext, const cell_t *params)
{
	CBaseAnimating *pEntity = (CBaseAnimating *)gamehelpers->ReferenceToEntity(params[1]);
	if(!pEntity) {
		return pContext->ThrowNativeError("Invalid Entity Reference/Index %i", params[1]);
	}
	
	bool bMoveSeqFinished = false;
	Vector newPosition{};
	QAngle newAngles{};
	bool ret = pEntity->GetSequenceMovement(bMoveSeqFinished, newPosition, newAngles);
	
	cell_t *addr = nullptr;
	pContext->LocalToPhysAddr(params[2], &addr);
	
	*addr = bMoveSeqFinished;
	
	cell_t *pNullVec = pContext->GetNullRef(SP_NULL_VECTOR);
	
	pContext->LocalToPhysAddr(params[3], &addr);
	
	if(addr != pNullVec) {
		addr[0] = sp_ftoc(newPosition.x);
		addr[1] = sp_ftoc(newPosition.y);
		addr[2] = sp_ftoc(newPosition.z);
	}
	
	pContext->LocalToPhysAddr(params[4], &addr);
	
	if(addr != pNullVec) {
		addr[0] = sp_ftoc(newAngles.x);
		addr[1] = sp_ftoc(newAngles.y);
		addr[2] = sp_ftoc(newAngles.z);
	}
	
	return ret;
}

static cell_t Model_tTypeget(IPluginContext *pContext, const cell_t *params)
{
	model_t *mod = (model_t *)params[1];
	return modelinfo->GetModelType(mod);
}

static cell_t Model_tGetName(IPluginContext *pContext, const cell_t *params)
{
	model_t *mod = (model_t *)params[1];
	const char *ptr = modelinfo->GetModelName(mod);
	size_t written = 0;
	pContext->StringToLocalUTF8(params[2], params[3], ptr, &written);
	return written;
}

static cell_t Model_tGetBounds(IPluginContext *pContext, const cell_t *params)
{
	model_t *mod = (model_t *)params[1];
	
	Vector mins{};
	Vector maxs{};
	modelinfo->GetModelBounds(mod, mins, maxs);
	
	cell_t *addr = nullptr;
	pContext->LocalToPhysAddr(params[2], &addr);
	addr[0] = sp_ftoc(mins.x);
	addr[1] = sp_ftoc(mins.y);
	addr[2] = sp_ftoc(mins.z);
	
	pContext->LocalToPhysAddr(params[3], &addr);
	addr[0] = sp_ftoc(maxs.x);
	addr[1] = sp_ftoc(maxs.y);
	addr[2] = sp_ftoc(maxs.z);
	
	return 0;
}

static cell_t Model_tGetRenderBounds(IPluginContext *pContext, const cell_t *params)
{
	model_t *mod = (model_t *)params[1];
	
	Vector mins{};
	Vector maxs{};
	modelinfo->GetModelRenderBounds(mod, mins, maxs);
	
	cell_t *addr = nullptr;
	pContext->LocalToPhysAddr(params[2], &addr);
	addr[0] = sp_ftoc(mins.x);
	addr[1] = sp_ftoc(mins.y);
	addr[2] = sp_ftoc(mins.z);
	
	pContext->LocalToPhysAddr(params[3], &addr);
	addr[0] = sp_ftoc(maxs.x);
	addr[1] = sp_ftoc(maxs.y);
	addr[2] = sp_ftoc(maxs.z);
	
	return 0;
}

static cell_t Model_tRadiusget(IPluginContext *pContext, const cell_t *params)
{
	model_t *mod = (model_t *)params[1];
	return sp_ftoc(modelinfo->GetModelRadius(mod));
}

struct msurface2_t;
typedef msurface2_t *SurfaceHandle_t;

struct cplane_t;
struct mleaf_t;
struct mleafwaterdata_t;
struct mvertex_t;
struct doccluderdata_t;
struct doccluderpolydata_t;
struct mtexinfo_t;
struct csurface_t;
struct msurface1_t;
struct msurfacelighting_t;
struct msurfacenormal_t;
struct dfacebrushlist_t;
struct dworldlight_t;
struct lightzbuffer_t;
struct mprimitive_t;
struct mprimvert_t;
struct darea_t;
struct dareaportal_t;
struct mcubemapsample_t;
struct mleafambientindex_t;
struct mleafambientlighting_t;
class CMemoryStack;
using HDISPINFOARRAY = void *;
struct ColorRGBExp32;
struct mnode_t;

struct worldbrushdata_t
{
	int			numsubmodels;
	
	int			numplanes;
	cplane_t	*planes;

	int			numleafs;		// number of visible leafs, not counting 0
	mleaf_t		*leafs;

	int			numleafwaterdata;
	mleafwaterdata_t *leafwaterdata;

	int			numvertexes;
	mvertex_t	*vertexes;

	int			numoccluders;
	doccluderdata_t *occluders;

	int			numoccluderpolys;
	doccluderpolydata_t *occluderpolys;

	int			numoccludervertindices;
	int			*occludervertindices;

	int				numvertnormalindices;	// These index vertnormals.
	unsigned short	*vertnormalindices;

	int			numvertnormals;
	Vector		*vertnormals;

	int			numnodes;
	mnode_t		*nodes;
	unsigned short *m_LeafMinDistToWater;

	int			numtexinfo;
	mtexinfo_t	*texinfo;

	int			numtexdata;
	csurface_t	*texdata;

	int         numDispInfos;
	HDISPINFOARRAY	hDispInfos;	// Use DispInfo_Index to get IDispInfos..

	/*
	int         numOrigSurfaces;
	msurface_t  *pOrigSurfaces;
	*/

	int			numsurfaces;
	msurface1_t	*surfaces1;
	msurface2_t	*surfaces2;
	msurfacelighting_t *surfacelighting;
	msurfacenormal_t *surfacenormals;
	
#if SOURCE_ENGINE == SE_LEFT4DEAD2
	unsigned short *m_pSurfaceBrushes;
	dfacebrushlist_t *m_pSurfaceBrushList;
#elif SOURCE_ENGINE == SE_TF2
	bool		unloadedlightmaps;
#endif
	
	int			numvertindices;
	unsigned short *vertindices;

	int nummarksurfaces;
	SurfaceHandle_t *marksurfaces;

	ColorRGBExp32		*lightdata;
#if SOURCE_ENGINE == SE_LEFT4DEAD2
	int					m_nLightingDataSize;
#endif
	
	int			numworldlights;
	dworldlight_t *worldlights;

	lightzbuffer_t *shadowzbuffers;

	// non-polygon primitives (strips and lists)
	int			numprimitives;
	mprimitive_t *primitives;

	int			numprimverts;
	mprimvert_t *primverts;

	int			numprimindices;
	unsigned short *primindices;

	int				m_nAreas;
	darea_t			*m_pAreas;

	int				m_nAreaPortals;
	dareaportal_t	*m_pAreaPortals;

	int				m_nClipPortalVerts;
	Vector			*m_pClipPortalVerts;

	mcubemapsample_t  *m_pCubemapSamples;
	int				   m_nCubemapSamples;

	int				m_nDispInfoReferences;
	unsigned short	*m_pDispInfoReferences;

	mleafambientindex_t		*m_pLeafAmbient;
	mleafambientlighting_t	*m_pAmbientSamples;

#if SOURCE_ENGINE == SE_LEFT4DEAD2
	// specific technique that discards all the lightmaps after load
	// no lightstyles or dlights are possible with this technique
	bool		m_bUnloadedAllLightmaps;

	CMemoryStack	*m_pLightingDataStack;

	int              m_nBSPFileSize;
#endif
};

class IDispInfo;

typedef unsigned short WorldDecalHandle_t;
typedef unsigned short ShadowDecalHandle_t;
typedef unsigned short OverlayFragmentHandle_t;

#pragma pack(1)
// NOTE: 32-bytes.  Aligned/indexed often
struct msurface2_t
{
	unsigned int			flags;			// see SURFDRAW_ #defines (only 22-bits right now)
	// These are packed in to flags now
	//unsigned char			vertCount;		// number of verts for this surface
	//unsigned char			sortGroup;		// only uses 2 bits, subdivide?
	cplane_t				*plane;			// pointer to shared plane	
	int						firstvertindex;	// look up in model->vertindices[] (only uses 17-18 bits?)
	WorldDecalHandle_t		decals;
	ShadowDecalHandle_t		m_ShadowDecals; // unsigned short
	OverlayFragmentHandle_t m_nFirstOverlayFragment;	// First overlay fragment on the surface (short)
	short					materialSortID;
	unsigned short			vertBufferIndex;

	unsigned short			m_bDynamicShadowsEnabled : 1;	// Can this surface receive dynamic shadows?
	unsigned short			texinfo : 15;

	IDispInfo				*pDispInfo;         // displacement map information
	int						visframe;		// should be drawn when node is crossed
};
#pragma pack()

inline const SurfaceHandle_t SurfaceHandleFromIndex( int surfaceIndex, const worldbrushdata_t *pData )
{
	return &pData->surfaces2[surfaceIndex];
}

struct mtexinfo_t
{
	Vector4D	textureVecsTexelsPerWorldUnits[2];	// [s/t] unit vectors in world space. 
							                        // [i][3] is the s/t offset relative to the origin.
	Vector4D	lightmapVecsLuxelsPerWorldUnits[2];
	float		luxelsPerWorldUnit;
	float		worldUnitsPerLuxel;
	unsigned short flags;		// SURF_ flags.
	unsigned short texinfoFlags;// TEXINFO_ flags.
	IMaterial	*material;
	
	mtexinfo_t( mtexinfo_t const& src )
	{
		// copy constructor needed since Vector4D has no copy constructor
		memcpy( this, &src, sizeof(mtexinfo_t) );
	}
};

inline mtexinfo_t *MSurf_TexInfo( SurfaceHandle_t surfID, worldbrushdata_t *pData )
{
	return &pData->texinfo[surfID->texinfo];
}

#include <model_types.h>

struct brushdata_t
{
	worldbrushdata_t	*pShared;
	int				firstmodelsurface;
	int				nummodelsurfaces;
	
	unsigned short	renderHandle;
	unsigned short	firstnode;
};

class CEngineSprite;

struct spritedata_t
{
	int				numframes;
	int				width;
	int				height;
	CEngineSprite	*sprite;
};

#define	MAX_OSPATH		260

struct model_t
{
	FileNameHandle_t	fnHandle;
#if SOURCE_ENGINE == SE_TF2
	CUtlString			strName;
#elif SOURCE_ENGINE == SE_LEFT4DEAD2
	char				szPathName[MAX_OSPATH];
#endif
	
	int					nLoadFlags;		// mark loaded/not loaded
	int					nServerCount;	// marked at load
#if SOURCE_ENGINE == SE_TF2
	IMaterial			**ppMaterials;	// null-terminated runtime material cache; ((intptr_t*)(ppMaterials))[-1] == nMaterials
#endif
	
	modtype_t			type;
	int					flags;			// MODELFLAG_???

	// volume occupied by the model graphics	
	Vector				mins, maxs;
	float				radius;

	union
	{
		brushdata_t		brush;
		MDLHandle_t		studio;
		spritedata_t	sprite;
	};
};

int GetModelMaterialCount(model_t *pModel)
{
	switch(modelinfo->GetModelType(pModel)) {
		case mod_brush: {
			return modelinfo->GetModelMaterialCount(pModel);
		}
		case mod_studio: {
			MDLHandle_t studio = modelinfo->GetCacheHandle(pModel);
			if(studio == MDLHANDLE_INVALID) {
				return 0;
			}
			studiohwdata_t *pHWDdata = mdlcache->GetHardwareData(studio);
			if(!pHWDdata) {
				return 0;
			}
			studioloddata_t *pLOD = &pHWDdata->m_pLODs[pHWDdata->m_RootLOD];
			return pLOD->numMaterials;
		}
	}

	return 0;
}

const char *GetModelMaterialName(model_t *pModel, int idx)
{
	switch(modelinfo->GetModelType(pModel)) {
		case mod_brush: {
			if(pModel->brush.pShared) {
				SurfaceHandle_t surfID = SurfaceHandleFromIndex(pModel->brush.firstmodelsurface + idx, pModel->brush.pShared);
				IMaterial *pMat = MSurf_TexInfo(surfID, pModel->brush.pShared)->material;
				return pMat->GetName();
			}
		} break;
		case mod_studio: {
			MDLHandle_t studio = modelinfo->GetCacheHandle(pModel);
			if(studio == MDLHANDLE_INVALID) {
				return 0;
			}
			studiohwdata_t *pHWDdata = mdlcache->GetHardwareData(studio);
			if(!pHWDdata) {
				return 0;
			}
			studioloddata_t *pLOD = &pHWDdata->m_pLODs[pHWDdata->m_RootLOD];
			IMaterial *pMat = pLOD->ppMaterials[idx];
			return pMat->GetName();
		}
	}

	return "";
}

static cell_t Model_tGetMaterialName(IPluginContext *pContext, const cell_t *params)
{
	model_t *mod = (model_t *)params[1];
	
	const char *ptr = GetModelMaterialName(mod, params[2]);

	size_t written = 0;
	pContext->StringToLocalUTF8(params[3], params[4], ptr, &written);
	return written;
}

static cell_t Model_tMaterialCountget(IPluginContext *pContext, const cell_t *params)
{
	model_t *mod = (model_t *)params[1];
	return GetModelMaterialCount(mod);
}

static cell_t StudioModelLODCountget(IPluginContext *pContext, const cell_t *params)
{
	model_t *mod = (model_t *)params[1];

	MDLHandle_t studio = modelinfo->GetCacheHandle(mod);
	if(studio == MDLHANDLE_INVALID) {
		return 0;
	}

	studiohwdata_t *pHWDdata = mdlcache->GetHardwareData(studio);
	if(!pHWDdata) {
		return 0;
	}

	return pHWDdata->m_NumLODs;
}

static cell_t StudioModelRootLODget(IPluginContext *pContext, const cell_t *params)
{
	model_t *mod = (model_t *)params[1];

	MDLHandle_t studio = modelinfo->GetCacheHandle(mod);
	if(studio == MDLHANDLE_INVALID) {
		return 0;
	}

	studiohwdata_t *pHWDdata = mdlcache->GetHardwareData(studio);
	if(!pHWDdata) {
		return 0;
	}

	return pHWDdata->m_RootLOD;
}

static cell_t StudioModelGetLOD(IPluginContext *pContext, const cell_t *params)
{
	model_t *mod = (model_t *)params[1];

	MDLHandle_t studio = modelinfo->GetCacheHandle(mod);
	if(studio == MDLHANDLE_INVALID) {
		return 0;
	}

	studiohwdata_t *pHWDdata = mdlcache->GetHardwareData(studio);
	if(!pHWDdata) {
		return 0;
	}

	return (cell_t)(&pHWDdata->m_pLODs[params[2]]);
}

static cell_t StudioModelIncludedModelsget(IPluginContext *pContext, const cell_t *params)
{
	model_t *mod = (model_t *)params[1];

	studiohdr_t *pStudio = modelinfo->GetStudiomodel(mod);

	return pStudio->numincludemodels;
}

static cell_t StudioModelNumTexturesget(IPluginContext *pContext, const cell_t *params)
{
	model_t *mod = (model_t *)params[1];

	studiohdr_t *pStudio = modelinfo->GetStudiomodel(mod);

	return pStudio->numtextures;
}

static cell_t StudioModelNumTextureCDSget(IPluginContext *pContext, const cell_t *params)
{
	model_t *mod = (model_t *)params[1];

	studiohdr_t *pStudio = modelinfo->GetStudiomodel(mod);

	return pStudio->numcdtextures;
}

static cell_t StudioModelGetIncludedModel(IPluginContext *pContext, const cell_t *params)
{
	model_t *mod = (model_t *)params[1];

	studiohdr_t *pStudio = modelinfo->GetStudiomodel(mod);
	mstudiomodelgroup_t *modelgroup = pStudio->pModelGroup(params[2]);

	const char *ptr = modelgroup->pszName();

	size_t written = 0;
	pContext->StringToLocalUTF8(params[3], params[4], ptr, &written);
	return written;
}

static cell_t StudioModelGetTextureName(IPluginContext *pContext, const cell_t *params)
{
	model_t *mod = (model_t *)params[1];

	studiohdr_t *pStudio = modelinfo->GetStudiomodel(mod);
	mstudiotexture_t *pTex = pStudio->pTexture(params[2]);

	const char *ptr = pTex->pszName();

	size_t written = 0;
	pContext->StringToLocalUTF8(params[3], params[4], ptr, &written);
	return written;
}

static cell_t StudioModelGetTextureDirectory(IPluginContext *pContext, const cell_t *params)
{
	model_t *mod = (model_t *)params[1];

	studiohdr_t *pStudio = modelinfo->GetStudiomodel(mod);

	const char *ptr = pStudio->pCdtexture(params[2]);

	size_t written = 0;
	pContext->StringToLocalUTF8(params[3], params[4], ptr, &written);
	return written;
}

static cell_t StudioModelGetAnimBlockName(IPluginContext *pContext, const cell_t *params)
{
	model_t *mod = (model_t *)params[1];

	studiohdr_t *pStudio = modelinfo->GetStudiomodel(mod);

	const char *ptr = pStudio->pszAnimBlockName();

	size_t written = 0;
	pContext->StringToLocalUTF8(params[2], params[3], ptr, &written);
	return written;
}

static cell_t StudioModelLODMaterialCountget(IPluginContext *pContext, const cell_t *params)
{
	studioloddata_t *mod = (studioloddata_t *)params[1];

	return mod->numMaterials;
}

static cell_t StudioModelLODGetMaterialName(IPluginContext *pContext, const cell_t *params)
{
	studioloddata_t *mod = (studioloddata_t *)params[1];

	const char *ptr = mod->ppMaterials[params[2]]->GetName();

	size_t written = 0;
	pContext->StringToLocalUTF8(params[3], params[4], ptr, &written);
	return written;
}

static cell_t ModelInfoGetModelIndex(IPluginContext *pContext, const cell_t *params)
{
	char *name = nullptr;
	pContext->LocalToString(params[1], &name);
	return modelinfo->GetModelIndex(name);
}

static cell_t ModelInfoGetModel(IPluginContext *pContext, const cell_t *params)
{
	return (cell_t)modelinfo->GetModel(params[1]);
}

int RagdollExtractBoneIndices( cell_t *boneIndexOut, int max, CStudioHdr *pStudioHdr, vcollide_t *pCollide )
{
	int elementCount = 0;

	IVPhysicsKeyParser *pParse = physcollision->VPhysicsKeyParserCreate( pCollide->pKeyValues );
	while ( !pParse->Finished() )
	{
		const char *pBlock = pParse->GetCurrentBlockName();
		if ( !strcasecmp( pBlock, "solid" ) )
		{
			solid_t solid;
			pParse->ParseSolid( &solid, NULL );
			if ( elementCount < max )
			{
				boneIndexOut[elementCount] = Studio_BoneIndexByName( pStudioHdr, solid.name );
				elementCount++;
			}
		}
		else
		{
			pParse->SkipBlock();
		}
	}
	physcollision->VPhysicsKeyParserDestroy( pParse );

	return elementCount;
}

CStudioHdr *GetModelPtr(const model_t *mod)
{
	MDLHandle_t hStudioHdr = modelinfo->GetCacheHandle(mod);
	if(hStudioHdr == MDLHANDLE_INVALID) {
		return nullptr;
	}

	const studiohdr_t *pStudioHdr = mdlcache->LockStudioHdr(hStudioHdr);
	if(!pStudioHdr) {
		return nullptr;
	}

	CStudioHdr *pStudioHdrContainer = new CStudioHdr;
	pStudioHdrContainer->Init( pStudioHdr, mdlcache );

	if(pStudioHdrContainer && pStudioHdrContainer->GetVirtualModel()) {
		MDLHandle_t hVirtualModel = (MDLHandle_t)(int)(pStudioHdrContainer->GetRenderHdr()->virtualModel) & 0xffff;
		mdlcache->LockStudioHdr(hVirtualModel);
	}

	return pStudioHdrContainer;
}

void FreeModelPtr(const model_t *mod, CStudioHdr *pStudioHdrContainer)
{
	MDLHandle_t hStudioHdr = modelinfo->GetCacheHandle(mod);
	mdlcache->UnlockStudioHdr(hStudioHdr);

	if(pStudioHdrContainer->GetVirtualModel()) {
		MDLHandle_t hVirtualModel = (MDLHandle_t)(int)(pStudioHdrContainer->GetRenderHdr()->virtualModel) & 0xffff;
		mdlcache->UnlockStudioHdr( hVirtualModel );
	}

	delete pStudioHdrContainer;
}

static cell_t RagdollExtractBoneIndicesNative(IPluginContext *pContext, const cell_t *params)
{
	char *model = nullptr;
	pContext->LocalToString(params[1], &model);

	int idx = modelinfo->GetModelIndex(model);
	if(idx == -1) {
		return pContext->ThrowNativeError("invalid model %s", model);
	}

	vcollide_t *pCollide = modelinfo->GetVCollide(idx);
	if(!pCollide) {
		return pContext->ThrowNativeError("%s missing vcollide data", model);
	}

	const model_t *mod = modelinfo->GetModel(idx);
	if(!mod) {
		return pContext->ThrowNativeError("invalid model %s", model);
	}

	CStudioHdr *pStudioHdrContainer = GetModelPtr(mod);
	if(!pStudioHdrContainer) {
		return pContext->ThrowNativeError("invalid model %s", model);
	}

	cell_t *addr;
	pContext->LocalToPhysAddr(params[2], &addr);

	int num = RagdollExtractBoneIndices(addr, params[3], pStudioHdrContainer, pCollide);

	FreeModelPtr(mod, pStudioHdrContainer);

	return num;
}

#include <tier1/mempool.h>

#if SOURCE_ENGINE == SE_TF2
using CUtlMemoryPool = CMemoryPool;
#endif

class CModelLoader
{
public:
	
};

CModelLoader *modelloader = nullptr;

static cell_t ModelInfoNumWorldSubmodelsget(IPluginContext *pContext, const cell_t *params)
{
	const model_t *mod = modelinfo->GetModel(1);
	if(mod && mod->brush.pShared) {
		return mod->brush.pShared->numsubmodels;
	} else {
		return 0;
	}
}

void Sample::OnCoreMapStart(edict_t *pEdictList, int edictCount, int clientMax)
{
#if 0
	ActivityList_Free();
	ActivityList_Init();
	ActivityList_RegisterSharedActivities();

	EventList_Free();
	EventList_Init();
	EventList_RegisterSharedEvents();
#endif
}

static cell_t register_matproxy(IPluginContext *pContext, const cell_t *params);

static const sp_nativeinfo_t g_sNativesInfo[] =
{
	{"RagdollExtractBoneIndices", RagdollExtractBoneIndicesNative},
	{"RegisterMatProxy", register_matproxy},
	{"EntitySetAbsOrigin", BaseEntitySetAbsOrigin},
	{"EntityWorldSpaceCenter", BaseEntityWorldSpaceCenter},
	{"EntityEyePosition", BaseEntityEyePosition},
	{"EntityGetAbsOrigin", BaseEntityGetAbsOrigin},
	{"EntityIsSolid", BaseEntityIsSolid},
	{"EntitySetSolid", BaseEntitySetSolid},
	{"EntityAddSolidFlags", BaseEntityAddSolidFlags},
	{"EntityRemoveSolidFlags", BaseEntityRemoveSolidFlags},
	{"AnimatingSelectWeightedSequence", BaseAnimatingSelectWeightedSequenceEx},
	{"AnimatingSelectHeaviestSequence", BaseAnimatingSelectHeaviestSequenceEx},
	{"AnimatingLookupSequence", BaseAnimatingLookupSequence},
	{"AnimatingLookupActivity", BaseAnimatingLookupActivity},
	{"AnimatingLookupPoseParameter", BaseAnimatingLookupPoseParameter},
	{"AnimatingGetPoseParameterName", BaseAnimatingGetPoseParameterName},
	{"AnimatingGetAttachmentName", BaseAnimatingGetAttachmentName},
	{"AnimatingGetAttachmentBone", BaseAnimatingGetAttachmentBone},
	{"AnimatingGetBoneName", BaseAnimatingGetBoneName},
	{"AnimatingGetBodygroupName", BaseAnimatingGetBodygroupName},
	{"AnimatingGetSequenceActivityName", BaseAnimatingGetSequenceActivityName},
	{"AnimatingGetSequenceActivity", BaseAnimatingGetSequenceActivity},
	{"AnimatingGetSequenceName", BaseAnimatingGetSequenceName},
	{"FlexGetFlexControllerName", BaseFlexGetFlexControllerName},
	{"AnimatingGetNumPoseParameters", BaseAnimatingGetNumPoseParameters},
	{"AnimatingGetNumAttachments", BaseAnimatingGetNumAttachments},
	{"AnimatingGetNumBones", BaseAnimatingGetNumBones},
	{"AnimatingGetNumBodygroups", BaseAnimatingGetNumBodygroups},
	{"AnimatingGetNumBoneControllers", BaseAnimatingGetNumBoneControllers},
	{"AnimatingGetNumSequences", BaseAnimatingGetNumSequences},
	{"FlexGetNumFlexControllers", BaseFlexGetNumFlexControllers},
	{"AnimatingSetPoseParameter", BaseAnimatingSetPoseParameter},
	{"AnimatingGetPoseParameter", BaseAnimatingGetPoseParameter},
	{"AnimatingStudioFrameAdvance", BaseAnimatingStudioFrameAdvance},
	{"AnimatingDispatchAnimEvents", BaseAnimatingDispatchAnimEvents},
	{"AnimatingResetSequenceInfo", BaseAnimatingResetSequenceInfo},
	{"AnimatingLookupAttachment", BaseAnimatingLookupAttachment},
	{"AnimatingFindBodygroupByName", BaseAnimatingFindBodygroupByName},
	{"AnimatingGetAttachment", BaseAnimatingGetAttachmentEx},
	//{"AnimatingGetAttachmentMatrix", BaseAnimatingGetAttachmentMatrixEx},
	{"AnimatingGetAttachmentLocal", BaseAnimatingGetAttachmentLocalEx},
	{"AnimatingSetBodygroupEx", BaseAnimatingSetBodygroup},
	{"AnimatingLookupBone", BaseAnimatingLookupBone},
	{"AnimatingGetBonePosition", BaseAnimatingGetBonePositionEx},
	{"AnimatingGetBonePositionLocal", BaseAnimatingGetBonePositionLocalEx},
	{"AnimatingSequenceDuration", BaseAnimatingSequenceDuration},
	{"AnimatingSequenceNumFrames", BaseAnimatingSequenceNumFrames},
	{"AnimatingSequenceFPS", BaseAnimatingSequenceFPS},
	{"AnimatingSequenceCPS", BaseAnimatingSequenceCPS},
	{"AnimatingGetBoneController", BaseAnimatingGetBoneControllerEx},
	{"AnimatingSetBoneController", BaseAnimatingSetBoneControllerEx},
	{"AnimatingHookHandleAnimEvent", BaseAnimatingSetHandleAnimEvent},
	{"AnimatingHandleAnimEvent", BaseAnimatingHandleAnimEvent},
	{"AnimatingGetIntervalMovement", BaseAnimatingGetIntervalMovement},
	{"AnimatingGetSequenceMovement", BaseAnimatingGetSequenceMovement},
	{"FlexFindFlexController", BaseFlexFindFlexController},
	{"FlexSetFlexWeight", BaseFlexSetFlexWeight},
	{"FlexGetFlexWeight", BaseFlexGetFlexWeight},
	{"AnimatingOverlayAddGestureSequence1", BaseAnimatingOverlayAddGestureSequence},
	{"AnimatingOverlayAddGestureSequence2", BaseAnimatingOverlayAddGestureSequenceEx},
	{"AnimatingOverlayAddGesture1", BaseAnimatingOverlayAddGesture},
	{"AnimatingOverlayAddGesture2", BaseAnimatingOverlayAddGestureEx},
	{"AnimatingOverlayIsPlayingGesture", BaseAnimatingOverlayIsPlayingGesture},
	{"AnimatingOverlayRestartGesture", BaseAnimatingOverlayRestartGesture},
	{"AnimatingOverlayRemoveGesture", BaseAnimatingOverlayRemoveGesture},
	{"AnimatingOverlayAddLayeredSequence", BaseAnimatingOverlayAddLayeredSequence},
	{"AnimatingOverlayRemoveAllGestures", BaseAnimatingOverlayRemoveAllGestures},
	{"AnimatingOverlaySetLayerPriority", BaseAnimatingOverlaySetLayerPriority},
	{"AnimatingOverlayIsValidLayer", BaseAnimatingOverlayIsValidLayer},
	{"AnimatingOverlaySetLayerDuration", BaseAnimatingOverlaySetLayerDuration},
	{"AnimatingOverlayGetLayerDuration", BaseAnimatingOverlayGetLayerDuration},
	{"AnimatingOverlaySetLayerCycle1", BaseAnimatingOverlaySetLayerCycle},
	{"AnimatingOverlaySetLayerCycle2", BaseAnimatingOverlaySetLayerCycleEx},
	{"AnimatingOverlayGetLayerCycle", BaseAnimatingOverlayGetLayerCycle},
	{"AnimatingOverlaySetLayerPlaybackRate", BaseAnimatingOverlaySetLayerPlaybackRate},
	{"AnimatingOverlaySetLayerWeight", BaseAnimatingOverlaySetLayerWeight},
	{"AnimatingOverlayGetLayerWeight", BaseAnimatingOverlayGetLayerWeight},
	{"AnimatingOverlaySetLayerBlendIn", BaseAnimatingOverlaySetLayerBlendIn},
	{"AnimatingOverlaySetLayerBlendOut", BaseAnimatingOverlaySetLayerBlendOut},
	{"AnimatingOverlaySetLayerAutokill", BaseAnimatingOverlaySetLayerAutokill},
	{"AnimatingOverlaySetLayerLooping", BaseAnimatingOverlaySetLayerLooping},
	{"AnimatingOverlaySetLayerNoRestore", BaseAnimatingOverlaySetLayerNoRestore},
	{"AnimatingOverlayGetLayerActivity", BaseAnimatingOverlayGetLayerActivity},
	{"AnimatingOverlayGetLayerSequence", BaseAnimatingOverlayGetLayerSequence},
	{"AnimatingOverlayFindGestureLayer", BaseAnimatingOverlayFindGestureLayer},
	{"AnimatingOverlayRemoveLayer", BaseAnimatingOverlayRemoveLayer},
	{"AnimatingOverlayFastRemoveLayer", BaseAnimatingOverlayFastRemoveLayer},
	{"AnimatingOverlayGetNumAnimOverlays", BaseAnimatingOverlayGetNumAnimOverlays},
	{"AnimatingOverlaySetNumAnimOverlays", BaseAnimatingOverlaySetNumAnimOverlays},
	{"AnimatingOverlayVerifyOrder", BaseAnimatingOverlayVerifyOrder},
	{"AnimatingOverlayHasActiveLayer", BaseAnimatingOverlayHasActiveLayer},
	{"AnimatingOverlayAllocateLayer", BaseAnimatingOverlayAllocateLayer},
	{"AnimatingOverlayMaxOverlays", BaseAnimatingOverlayMaxOverlays},
	{"ActivityList_IndexForName", ActivityList_IndexForNameNative},
	{"ActivityList_NameForIndex", ActivityList_NameForIndexNative},
	{"ActivityList_RegisterPrivateActivity", ActivityList_RegisterPrivateActivityNative},
	{"EventList_IndexForName", EventList_IndexForNameNative},
	{"EventList_NameForIndex", EventList_NameForIndexNative},
	{"EventList_RegisterPrivateEvent", EventList_RegisterPrivateEventNative},
	{"BaseModel.Type.get", Model_tTypeget},
	{"BaseModel.GetName", Model_tGetName},
	{"BaseModel.GetBounds", Model_tGetBounds},
	{"BaseModel.GetRenderBounds", Model_tGetRenderBounds},
	{"BaseModel.Radius.get", Model_tRadiusget},
	{"BaseModel.MaterialCount.get", Model_tMaterialCountget},
	{"BaseModel.GetMaterialName", Model_tGetMaterialName},
	{"StudioModel.LODCount.get", StudioModelLODCountget},
	{"StudioModel.RootLOD.get", StudioModelRootLODget},
	{"StudioModel.IncludedModels.get", StudioModelIncludedModelsget},
	{"StudioModel.GetIncludedModelPath", StudioModelGetIncludedModel},
	{"StudioModel.GetAnimBlockPath", StudioModelGetAnimBlockName},
	{"StudioModel.GetLOD", StudioModelGetLOD},
	{"StudioModel.NumTextureCDS.get", StudioModelNumTextureCDSget},
	{"StudioModel.GetTextureDirectory", StudioModelGetTextureDirectory},
	{"StudioModel.NumTextures.get", StudioModelNumTexturesget},
	{"StudioModel.GetTextureName", StudioModelGetTextureName},
	{"StudioModelLOD.MaterialCount.get", StudioModelLODMaterialCountget},
	{"StudioModelLOD.GetMaterialName", StudioModelLODGetMaterialName},
	{"ModelInfo.GetModelIndex", ModelInfoGetModelIndex},
	{"ModelInfo.GetModel", ModelInfoGetModel},
	{"ModelInfo.GetNumWorldSubmodels", ModelInfoNumWorldSubmodelsget},
	{nullptr, nullptr},
};

#include <materialsystem/imaterialproxyfactory.h>
#include <materialsystem/imaterialproxy.h>

class CDummyMaterialProxy : public IMaterialProxy
{
public:
	bool Init( IMaterial* pMaterial, KeyValues *pKeyValues ) override
	{
		return true;
	}

	~CDummyMaterialProxy() override
	{
		
	}

	void OnBind( void *pObject ) override
	{
		
	}

	void Release() override
	{
		
	}

	IMaterial *GetMaterial() override
	{ return nullptr; }
};

#include "funnyfile.h"

HandleType_t matproxy_handle = 0;

struct matproxy_entry_t;

class CSPMaterialProxy : public CDummyMaterialProxy
{
public:
	CSPMaterialProxy(std::string &&name_, IPluginFunction *func_)
		: CDummyMaterialProxy{}, name{name_}, func{func_}
	{
	}

	bool Init( IMaterial* pMaterial, KeyValues *pKeyValues ) override
	{
		if(!CDummyMaterialProxy::Init(pMaterial, pKeyValues)) {
			return false;
		}

		m_pMaterial = pMaterial;
		m_pKeyValues = new KeyValues{""};
		m_pKeyValues->RecursiveMergeKeyValues(pKeyValues);

		return true;
	}

	~CSPMaterialProxy() override
	{
		if(m_pKeyValues) {
			m_pKeyValues->deleteThis();
		}
	}

	IMaterial *GetMaterial() override
	{ return m_pMaterial; }

	void OnBind( void *pObject ) override
	{
		CBaseEntity *pEntity = (CBaseEntity *)pObject;
		IMaterial *pMaterial = GetMaterial();
		KeyValues *pKeyValues = m_pKeyValues;

		if(!pMaterial || !pEntity) {
			return;
		}

		if(!func) {
			return;
		}

		HandleError err{};
		Handle_t kvhndl = ((HandleSystemHack *)handlesys)->CreateKeyValuesHandle(pKeyValues, nullptr, &err);
		if(err != HandleError_None) {
			func->GetParentContext()->ReportError("Invalid KeyValues handle %x (error %d).", kvhndl, err);
			return;
		}

		const char *matname{pMaterial->GetName()};
		func->PushCell(gamehelpers->EntityToBCompatRef(pEntity));
		func->PushStringEx((char *)matname, strlen(matname)+1, SM_PARAM_STRING_COPY|SM_PARAM_STRING_UTF8, 0);
		func->PushCell(kvhndl);
		func->Execute(nullptr);
	}

	void Release() override
	{
		m_pMaterial = nullptr;
		m_pKeyValues->deleteThis();
		m_pKeyValues = nullptr;

		CDummyMaterialProxy::Release();

		func = nullptr;
	}

	void EntryDeleted();

	IMaterial *m_pMaterial = nullptr;
	KeyValues *m_pKeyValues = nullptr;
	std::string name;

	IPluginFunction *func = nullptr;
	matproxy_entry_t *entry = nullptr;
};

using matproxy_entries_t = std::unordered_map<std::string, matproxy_entry_t *>;
matproxy_entries_t matproxy_entries;

struct matproxy_entry_t
{
	matproxy_entry_t(std::string &&name_, IPluginFunction *func_)
		: name{name_}, func{func_}
	{
		matproxy_entries.emplace(name, this);
	}

	~matproxy_entry_t()
	{
		for(CSPMaterialProxy *it : childs) {
			it->entry = nullptr;
			it->EntryDeleted();
		}

		matproxy_entries_t::iterator it{matproxy_entries.find(name)};
		if(it != matproxy_entries.cend()) {
			matproxy_entries.erase(it);
		}
	}

	IMaterialProxy *create()
	{
		std::string tmpname{name};
		CSPMaterialProxy *ptr{new CSPMaterialProxy{std::move(tmpname), func}};
		ptr->entry = this;
		childs.emplace_back(ptr);
		return ptr;
	}

	void child_deleted(CSPMaterialProxy *ptr)
	{
		childs_t::iterator it{std::find(childs.begin(), childs.end(), ptr)};
		if(it != childs.end()) {
			childs.erase(it);
		}
	}

	IPluginFunction *func = nullptr;
	using childs_t = std::vector<CSPMaterialProxy *>;
	childs_t childs;
	std::string name;
};

void CSPMaterialProxy::EntryDeleted()
{
	func = nullptr;

	if(entry) {
		entry->child_deleted(this);
		entry = nullptr;
	}
}

void Sample::OnHandleDestroy(HandleType_t type, void *object)
{
	if(type == matproxy_handle) {
		matproxy_entry_t *obj = (matproxy_entry_t *)object;
		delete obj;
	}
}

std::string default_proxies[]{
	"ParticleSphereProxy"s,
	"TextureScroll"s,
	"Sine"s,
	"LinearRamp"s,
	"Add"s,
	"TextureTransform"s,
	"SelectFirstIfNonZero"s,
	"AnimatedTexture"s,
	"WaterLOD"s,
	"Equals"s,
	"PlayerProximity"s,
	"Divide"s,
	"Multiply"s,
	"Subtract"s,
	"Clamp"s,
	"PlayerTeamMatch"s,
	"MaterialModify"s,
	"MaterialModifyAnimated"s,
	"LessOrEqual"s,
	"Monitor"s,
	"lampbeam"s,
#if SOURCE_ENGINE == SE_TF2
	"ItemTintColor"s,
	"vm_invis"s,
	"CustomSteamImageOnModel"s,
	"ModelGlowColor"s,
	"AnimatedWeaponSheen"s,
	"YellowLevel"s,
	"weapon_invis"s,
	"invis"s,
	"BurnLevel"s,
	"CommunityWeapon"s,
	"StickybombGlowColor"s,
	"building_invis"s,
	"ShieldFalloff"s,
	"WeaponSkin"s,
	"spy_invis"s,
	"WheatlyEyeGlow"s,
	"BuildingRescueLevel"s,
	"StatTrakIcon"s,
	"StatTrakDigit"s,
	"StatTrakIllum"s,
	"InvulnLevel"s,
	"HeartbeatScale"s,
	"BenefactorLevel"s,
#endif
};

class CSPMaterialProxyFactory : public IMaterialProxyFactory
{
public:
	static bool is_default_proxy(const std::string &name, bool casesen)
	{
		for(const std::string &def : default_proxies) {
			if(casesen) {
				if(def == name) {
					return true;
				}
			} else {
				if(strcasecmp(def.c_str(), name.c_str()) == 0) {
					return true;
				}
			}
		}
		return false;
	}

	IMaterialProxy *CreateProxy(const char *proxyName) override
	{
		std::string name{proxyName};

		if(is_default_proxy(name, false)) {
			return new CDummyMaterialProxy{};
		}

		matproxy_entries_t::const_iterator it{
			std::find_if(matproxy_entries.cbegin(), matproxy_entries.cend(),
				[&name](const auto &it) noexcept -> bool {
					return (strncasecmp(it.first.c_str(), name.c_str(), it.first.length()) == 0);
				}
			)
		};
		if(it == matproxy_entries.cend()) {
			return nullptr;
		}

		return it->second->create();
	}

	void DeleteProxy(IMaterialProxy *pProxy) override
	{
		CDummyMaterialProxy *dummy = (CDummyMaterialProxy *)pProxy;
		pProxy->Release();
		delete dummy;
	}
};

static CSPMaterialProxyFactory proxyfactory;

static cell_t register_matproxy(IPluginContext *pContext, const cell_t *params)
{
	char *name_ptr = nullptr;
	pContext->LocalToString(params[1], &name_ptr);
	std::string name{name_ptr};
	
	matproxy_entries_t::iterator it{matproxy_entries.find(name)};
	if(CSPMaterialProxyFactory::is_default_proxy(name, true) || it != matproxy_entries.cend()) {
		return pContext->ThrowNativeError("%s is already registered", name_ptr);
	}

	IPluginFunction *func = pContext->GetFunctionById(params[2]);

	matproxy_entry_t *obj = new matproxy_entry_t{std::move(name), func};
	return handlesys->CreateHandle(matproxy_handle, obj, pContext->GetIdentity(), myself->GetIdentity(), nullptr);
}

static CDetour *SV_ComputeClientPacks_detour{nullptr};

class CFrameSnapshot;
class CGameClient;

class CFrameSnapshot
{
public:
	CInterlockedInt			m_ListIndex;	// Index info CFrameSnapshotManager::m_FrameSnapshots.

	// Associated frame. 
	int						m_nTickCount; // = sv.tickcount
	
	// State information
	class CFrameSnapshotEntry		*m_pEntities;	
	int						m_nNumEntities; // = sv.num_edicts

	// This list holds the entities that are in use and that also aren't entities for inactive clients.
	unsigned short			*m_pValidEntities; 
	int						m_nValidEntities;

	// Additional HLTV info
	class CHLTVEntityData			*m_pHLTVEntityData; // is NULL if not in HLTV mode or array of m_pValidEntities entries
	class CReplayEntityData		*m_pReplayEntityData; // is NULL if not in replay mode or array of m_pValidEntities entries

	class CEventInfo				**m_pTempEntities; // temp entities
	int						m_nTempEntities;

	CUtlVector<int>			m_iExplicitDeleteSlots;

private:

	// Snapshots auto-delete themselves when their refcount goes to zero.
	CInterlockedInt			m_nReferences;
};

DETOUR_DECL_STATIC3(SV_ComputeClientPacks, void, int, clientCount, CGameClient **, clients, CFrameSnapshot *, snapshot)
{
#ifdef __HAS_PROXYSEND
	if(!proxysend)
#endif
	{
		for(int i{0}; i < snapshot->m_nValidEntities; ++i) {
			int idx{snapshot->m_pValidEntities[i]};
			int ref{gamehelpers->IndexToReference(idx)};

			CBaseEntity *pEntity{gamehelpers->ReferenceToEntity(ref)};
			if(pEntity) {
				g_Sample.pre_pack_entity(pEntity);
			}
		}
	}

	DETOUR_STATIC_CALL(SV_ComputeClientPacks)(clientCount, clients, snapshot);
}

void Sample::pre_pack_entity(CBaseEntity *pEntity) const noexcept
{
	if(matproxy_entries.empty()) {
		return;
	}

	const char *classname = gamehelpers->GetEntityClassname(pEntity);
	if(strcmp(classname, "phys_bone_follower") == 0) {
		return;
	}

	int mdlidx = pEntity->GetModelIndex();
	if(mdlidx == -1) {
		return;
	}

	const model_t *mdlptr = modelinfo->GetModel(mdlidx);
	if(!mdlptr) {
		return;
	}

	MDLHandle_t mdlhndl = modelinfo->GetCacheHandle(mdlptr);
	if(mdlhndl == MDLHANDLE_INVALID) {
		return;
	}

	studiohwdata_t *pHWDdata = mdlcache->GetHardwareData(mdlhndl);
	if(!pHWDdata) {
		return;
	}

	std::vector<IMaterial *> already_binded{};

	for(int i = 0; i < pHWDdata->m_NumLODs; ++i) {
		studioloddata_t *pLOD = &pHWDdata->m_pLODs[i];
		for(int j = 0; j < pLOD->numMaterials; ++j) {
			IMaterial *pMaterial = pLOD->ppMaterials[j];

			if(std::find(already_binded.begin(), already_binded.end(), pMaterial) != already_binded.end()) {
				continue;
			}

			pMaterial->CallBindProxy( (void *)pEntity );

			already_binded.emplace_back(pMaterial);
		}
	}
}

bool Sample::SDK_OnMetamodLoad(ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	gpGlobals = ismm->GetCGlobals();
	GET_V_IFACE_CURRENT(GetEngineFactory, modelinfo, IVModelInfo, VMODELINFO_SERVER_INTERFACE_VERSION)
	GET_V_IFACE_CURRENT(GetEngineFactory, filesystem, IFileSystem, FILESYSTEM_INTERFACE_VERSION)
	GET_V_IFACE_CURRENT(GetEngineFactory, mdlcache, IMDLCache, MDLCACHE_INTERFACE_VERSION)
	GET_V_IFACE_CURRENT(GetEngineFactory, materials, IMaterialSystem, MATERIAL_SYSTEM_INTERFACE_VERSION)
	GET_V_IFACE_CURRENT(GetEngineFactory, partition, ISpatialPartition, INTERFACEVERSION_SPATIALPARTITION)
	GET_V_IFACE_CURRENT(GetEngineFactory, physcollision, IPhysicsCollision, VPHYSICS_COLLISION_INTERFACE_VERSION)

	materials->SetMaterialProxyFactory(&proxyfactory);

	return true;
}

void Sample::OnPluginUnloaded(IPlugin *plugin)
{
	callback_holder_map_t::iterator it{callbackmap.begin()};
	while(it != callbackmap.end()) {
		callback_holder_t *holder = it->second;
		using owners_t = std::vector<IdentityToken_t *>;
		owners_t &owners{holder->owners};

		owners_t::iterator it_own{std::find(owners.begin(), owners.end(), plugin->GetIdentity())};
		if(it_own != owners.cend()) {
			owners.erase(it_own);

			size_t func_count{0};

			if(holder->fwd) {
				holder->fwd->RemoveFunctionsOfPlugin(plugin);
				func_count += holder->fwd->GetFunctionCount();
			}

			if(func_count == 0) {
				CBaseEntity *pEntity = gamehelpers->ReferenceToEntity(holder->ref);
				if(pEntity) {
					holder->removed(pEntity);
				}
				delete holder;
				it = callbackmap.erase(it);
				continue;
			}
		}
		
		++it;
	}
}

IGameConfig *g_pGameConf = nullptr;

bool Sample::SDK_OnLoad(char *error, size_t maxlen, bool late)
{
	if(!gameconfs->LoadGameConfigFile("animhelpers", &g_pGameConf, error, maxlen)) {
		return false;
	}

	int CBaseEntityUpdateOnRemove{-1};
	g_pGameConf->GetOffset("CBaseEntity::UpdateOnRemove", &CBaseEntityUpdateOnRemove);
	if(CBaseEntityUpdateOnRemove == -1) {
		snprintf(error, maxlen, "could not get CBaseEntity::UpdateOnRemove offset");
		return false;
	}

	g_pGameConf->GetOffset("CBaseAnimating::HandleAnimEvent", &CBaseAnimatingHandleAnimEvent);
	if(CBaseAnimatingHandleAnimEvent == -1) {
		snprintf(error, maxlen, "could not get CBaseAnimating::HandleAnimEvent offset");
		return false;
	}

	g_pGameConf->GetOffset("CBaseEntity::WorldSpaceCenter", &CBaseEntityWorldSpaceCenter);
	if(CBaseEntityWorldSpaceCenter == -1) {
		snprintf(error, maxlen, "could not get CBaseEntity::WorldSpaceCenter offset");
		return false;
	}

	g_pGameConf->GetOffset("CBaseEntity::EyePosition", &CBaseEntityEyePosition);
	if(CBaseEntityEyePosition == -1) {
		snprintf(error, maxlen, "could not get CBaseEntity::EyePosition offset");
		return false;
	}

	g_pGameConf->GetMemSig("CBaseEntity::CollisionRulesChanged", &CBaseEntityCollisionRulesChanged);
	if(CBaseEntityCollisionRulesChanged == nullptr) {
		snprintf(error, maxlen, "could not get CBaseEntity::CollisionRulesChanged address");
		return false;
	}

	g_pGameConf->GetMemSig("EntityTouch_Add", &EntityTouch_AddPtr);
	if(EntityTouch_AddPtr == nullptr) {
		snprintf(error, maxlen, "could not get EntityTouch_Add address");
		return false;
	}

	g_pGameConf->GetOffset("CBaseAnimating::m_pStudioHdr", &m_pStudioHdrOffset);
	if(m_pStudioHdrOffset == -1) {
		snprintf(error, maxlen, "could not get CBaseAnimating::m_pStudioHdr offset");
		return false;
	}

	g_pGameConf->GetOffset("CBaseAnimating::StudioFrameAdvance", &CBaseAnimatingStudioFrameAdvance);
	if(CBaseAnimatingStudioFrameAdvance == -1) {
		snprintf(error, maxlen, "could not get CBaseAnimating::StudioFrameAdvance offset");
		return false;
	}

	g_pGameConf->GetOffset("CBaseAnimating::DispatchAnimEvents", &CBaseAnimatingDispatchAnimEvents);
	if(CBaseAnimatingDispatchAnimEvents == -1) {
		snprintf(error, maxlen, "could not get CBaseAnimating::DispatchAnimEvents offset");
		return false;
	}

	g_pGameConf->GetOffset("CBaseAnimating::GetAttachment", &CBaseAnimatingGetAttachment);
	if(CBaseAnimatingGetAttachment == -1) {
		snprintf(error, maxlen, "could not get CBaseAnimating::GetAttachment offset");
		return false;
	}

	g_pGameConf->GetOffset("CBaseAnimating::GetBoneTransform", &CBaseAnimatingGetBoneTransform);
	if(CBaseAnimatingGetBoneTransform == -1) {
		snprintf(error, maxlen, "could not get CBaseAnimating::GetBoneTransform offset");
		return false;
	}

	g_pGameConf->GetMemSig("CBaseAnimating::ResetSequenceInfo", &CBaseAnimatingResetSequenceInfo);
	if(CBaseAnimatingResetSequenceInfo == nullptr) {
		snprintf(error, maxlen, "could not get CBaseAnimating::ResetSequenceInfo address");
		return false;
	}

	g_pGameConf->GetMemSig("CBaseAnimating::LockStudioHdr", &CBaseAnimatingLockStudioHdr);
	if(CBaseAnimatingLockStudioHdr == nullptr) {
		snprintf(error, maxlen, "could not get CBaseAnimating::LockStudioHdr address");
		return false;
	}

	g_pGameConf->GetMemSig("CBaseEntity::CalcAbsolutePosition", &CBaseEntityCalcAbsolutePosition);
	if(CBaseEntityCalcAbsolutePosition == nullptr) {
		snprintf(error, maxlen, "could not get CBaseEntity::CalcAbsolutePosition address");
		return false;
	}

	g_pGameConf->GetMemSig("CBaseEntity::SetAbsOrigin", &CBaseEntitySetAbsOrigin);
	if(CBaseEntitySetAbsOrigin == nullptr) {
		snprintf(error, maxlen, "could not get CBaseEntity::SetAbsOrigin address");
		return false;
	}

	g_pGameConf->GetMemSig("modelloader", (void **)&modelloader);
	if(modelloader == nullptr) {
		snprintf(error, maxlen, "could not get modelloader address");
		return false;
	}

	g_pGameConf->GetMemSig("ActivityList_NameForIndex", &ActivityList_NameForIndexPtr);
	if(ActivityList_NameForIndexPtr == nullptr) {
		snprintf(error, maxlen, "could not get ActivityList_NameForIndex address");
		return false;
	}

	g_pGameConf->GetMemSig("ActivityList_IndexForName", &ActivityList_IndexForNamePtr);
	if(ActivityList_IndexForNamePtr == nullptr) {
		snprintf(error, maxlen, "could not get ActivityList_IndexForName address");
		return false;
	}

	g_pGameConf->GetMemSig("ActivityList_RegisterPrivateActivity", &ActivityList_RegisterPrivateActivityPtr);
	if(ActivityList_RegisterPrivateActivityPtr == nullptr) {
		snprintf(error, maxlen, "could not get ActivityList_RegisterPrivateActivity address");
		return false;
	}

	g_pGameConf->GetMemSig("EventList_NameForIndex", &EventList_NameForIndexPtr);
	if(EventList_NameForIndexPtr == nullptr) {
		snprintf(error, maxlen, "could not get EventList_NameForIndex address");
		return false;
	}

	g_pGameConf->GetMemSig("EventList_IndexForName", &EventList_IndexForNamePtr);
	if(EventList_IndexForNamePtr == nullptr) {
		snprintf(error, maxlen, "could not get EventList_IndexForName address");
		return false;
	}

	g_pGameConf->GetMemSig("EventList_RegisterPrivateEvent", &EventList_RegisterPrivateEventPtr);
	if(EventList_RegisterPrivateEventPtr == nullptr) {
		snprintf(error, maxlen, "could not get EventList_RegisterPrivateEvent address");
		return false;
	}

	g_pGameConf->GetMemSig("EventList_GetEventType", &EventList_GetEventTypePtr);
	if(EventList_GetEventTypePtr == nullptr) {
		snprintf(error, maxlen, "could not get EventList_GetEventType address");
		return false;
	}

	g_pGameConf->GetMemSig("g_nActivityListVersion", (void **)&g_nActivityListVersion);
	if(g_nActivityListVersion == nullptr) {
		snprintf(error, maxlen, "could not get g_nActivityListVersion address");
		return false;
	}

	g_pGameConf->GetMemSig("g_nEventListVersion", (void **)&g_nEventListVersion);
	if(g_nEventListVersion == nullptr) {
		snprintf(error, maxlen, "could not get g_nEventListVersion address");
		return false;
	}

	CDetourManager::Init(g_pSM->GetScriptingEngine(), g_pGameConf);

	SV_ComputeClientPacks_detour = DETOUR_CREATE_STATIC(SV_ComputeClientPacks, "s_ComputeClientPacks");
	if(!SV_ComputeClientPacks_detour) {
		snprintf(error, maxlen, "could not create SV_ComputeClientPacks detour");
		return false;
	}

	SH_MANUALHOOK_RECONFIGURE(HandleAnimEvent, CBaseAnimatingHandleAnimEvent, 0, 0);
	SH_MANUALHOOK_RECONFIGURE(UpdateOnRemove, CBaseEntityUpdateOnRemove, 0, 0);

	g_pEntityList = reinterpret_cast<CBaseEntityList *>(gamehelpers->GetGlobalEntityList());

	sm_sendprop_info_t info{};
	gamehelpers->FindSendPropInfo("CBaseAnimating", "m_nSequence", &info);
	m_nSequenceOffset = info.actual_offset;
	
	gamehelpers->FindSendPropInfo("CBaseAnimatingOverlay", "m_AnimOverlay", &info);
	m_AnimOverlayOffset = info.actual_offset;
	
	gamehelpers->FindSendPropInfo("CBaseAnimating", "m_flPoseParameter", &info);
	m_flPoseParameterOffset = info.actual_offset;
	
	gamehelpers->FindSendPropInfo("CBaseAnimating", "m_flEncodedController", &info);
	m_flEncodedControllerOffset = info.actual_offset;
	
	gamehelpers->FindSendPropInfo("CBaseAnimating", "m_flPlaybackRate", &info);
	m_flPlaybackRateOffset = info.actual_offset;
	
	gamehelpers->FindSendPropInfo("CBaseAnimating", "m_flCycle", &info);
	m_flCycleOffset = info.actual_offset;
	
	gamehelpers->FindSendPropInfo("CBaseAnimating", "m_flAnimTime", &info);
	m_flAnimTimeOffset = info.actual_offset;
	
	gamehelpers->FindSendPropInfo("CBaseFlex", "m_flexWeight", &info);
	m_flexWeightOffset = info.actual_offset;
	
	gamehelpers->FindSendPropInfo("CBaseAnimating", "m_nBody", &info);
	m_nBodyOffset = info.actual_offset;
	
	gamehelpers->FindSendPropInfo("CBaseAnimating", "m_hLightingOrigin", &info);
	m_pStudioHdrOffset += info.actual_offset;

	gamehelpers->FindSendPropInfo("CBaseEntity", "m_bIsPlayerSimulated", &info);
	m_fDataObjectTypesOffset = info.actual_offset;
	m_fDataObjectTypesOffset += 1 + sizeof(EHANDLE);

	sharesys->AddNatives(myself, g_sNativesInfo);
	
	plsys->AddPluginsListener(this);

	matproxy_handle = handlesys->CreateType("matproxy", this, 0, nullptr, nullptr, myself->GetIdentity(), nullptr);

	sharesys->RegisterLibrary(myself, "animhelpers");
	sharesys->AddInterface(myself, this);

#ifdef __HAS_PROXYSEND
	sharesys->AddDependency(myself, "proxysend.ext", false, true);
#endif

	HandleSystemHack::init();

	return true;
}

void Sample::SDK_OnAllLoaded()
{
#ifdef __HAS_PROXYSEND
	SM_GET_LATE_IFACE(PROXYSEND, proxysend);
	if(proxysend) {
		proxysend->add_listener(this);
		SV_ComputeClientPacks_detour->DisableDetour();
	} else {
		SV_ComputeClientPacks_detour->EnableDetour();
	}
#endif
}

bool Sample::QueryRunning(char *error, size_t maxlength)
{
#ifdef __HAS_PROXYSEND
	//SM_CHECK_IFACE(PROXYSEND, proxysend);
#endif
	return true;
}

bool Sample::QueryInterfaceDrop(SMInterface *pInterface)
{
#ifdef __HAS_PROXYSEND
	if(pInterface == proxysend)
		return false;
#endif

	return IExtensionInterface::QueryInterfaceDrop(pInterface);
}

void Sample::NotifyInterfaceDrop(SMInterface *pInterface)
{
#ifdef __HAS_PROXYSEND
	if(strcmp(pInterface->GetInterfaceName(), SMINTERFACE_PROXYSEND_NAME) == 0) {
		proxysend = NULL;
		SV_ComputeClientPacks_detour->EnableDetour();
	}
#endif
}

void Sample::SDK_OnUnload()
{
	handlesys->RemoveType(matproxy_handle, myself->GetIdentity());

	SV_ComputeClientPacks_detour->Destroy();

	gameconfs->CloseGameConfigFile(g_pGameConf);
}

int Sample::SelectWeightedSequence(CBaseAnimating *pEntity, int activity)
{
	return pEntity->SelectWeightedSequence(activity);
}

void Sample::StudioFrameAdvance(CBaseAnimating *pEntity)
{
	pEntity->StudioFrameAdvance();
}

void Sample::DispatchAnimEvents(CBaseAnimating *pEntity)
{
	pEntity->DispatchAnimEvents();
}

void Sample::ResetSequenceInfo(CBaseAnimating *pEntity)
{
	pEntity->ResetSequenceInfo();
}

const char *Sample::ActivityName(int activity)
{
	return ActivityList_NameForIndex(activity);
}

const char *Sample::SequenceName(CBaseAnimating *pEntity, int sequence)
{
	return pEntity->GetSequenceName(sequence);
}

Activity Sample::SequenceActivity(CBaseAnimating *pEntity, int sequence)
{
	return pEntity->GetSequenceActivity(sequence);
}