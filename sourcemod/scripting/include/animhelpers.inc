#if defined animhelpers_included
	#endinput
#endif
#define animhelpers_included

#include <animhelpers_activity>
#include <animhelpers_eventlist>

enum FireBulletsFlags_t
{
	FIRE_BULLETS_FIRST_SHOT_ACCURATE = 0x1,		// Pop the first shot with perfect accuracy
	FIRE_BULLETS_DONT_HIT_UNDERWATER = 0x2,		// If the shot hits its target underwater, don't damage it
	FIRE_BULLETS_ALLOW_WATER_SURFACE_IMPACTS = 0x4,	// If the shot hits water surface, still call DoImpactEffect
	FIRE_BULLETS_TEMPORARY_DANGER_SOUND = 0x8,		// Danger sounds added from this impact can be stomped immediately if another is queued
};

enum struct FireBulletsInfo_t
{
	int m_iShots;
	float m_vecSrc[3];
	float m_vecDirShooting[3];
	float m_vecSpread[3];
	float m_flDistance;
	int m_iAmmoType;
	int m_iTracerFreq;
	float m_flDamage;
	int m_iPlayerDamage;	// Damage to be used instead of m_flDamage if we hit a player
	int m_nFlags;			// See FireBulletsFlags_t
	float m_flDamageForceScale;
	int m_pAttacker;
	int m_pAdditionalIgnoreEnt;
	bool m_bPrimaryAttack;
	bool m_bUseServerRandomSeed;
}

#define FireBulletsInfo_tVar any[]

methodmap BaseEntity
{
	public BaseEntity(int entity) { return view_as<BaseEntity>(entity); }
	public native void FireBullets(FireBulletsInfo_tVar info);
	public native void WorldSpaceCenter(float center[3]);
	public native void SetAbsOrigin(float pos[3]);
}

methodmap BaseAnimating < BaseEntity
{
	public BaseAnimating(int entity) { return view_as<BaseAnimating>(entity); }

	public native Activity LookupActivity(const char[] name);
	public native int SelectWeightedSequenceEx(Activity activity);
	public native int SelectHeaviestSequenceEx(Activity activity);
	public int SelectWeightedSequence(const char[] name)
	{
		Activity id = this.LookupActivity(name);
		if(id != ACT_INVALID) {
			return this.SelectWeightedSequenceEx(id);
		} else {
			return -1;
		}
	}
	public int SelectHeaviestSequence(const char[] name)
	{
		Activity id = this.LookupActivity(name);
		if(id != ACT_INVALID) {
			return this.SelectHeaviestSequenceEx(id);
		} else {
			return -1;
		}
	}

	public native int GetNumPoseParameters();
	public native void GetPoseParameterName(int id, char[] name, int len);
	public native int LookupPoseParameter(const char[] name);
	public native void SetPoseParameterEx(int index, float value);
	public native float GetPoseParameterEx(int index);
	public void SetPoseParameter(const char[] name, float value)
	{
		int id = this.LookupPoseParameter(name);
		if(id != -1) {
			this.SetPoseParameterEx(id, value);
		}
	}
	public float GetPoseParameter(const char[] name)
	{
		int id = this.LookupPoseParameter(name);
		if(id != -1) {
			return this.GetPoseParameterEx(id);
		} else {
			return 0.0;
		}
	}
	public native void StudioFrameAdvance();
	public native void DispatchAnimEvents();
	public native void ResetSequenceInfo();

	public native int GetNumAttachments();
	public native void GetAttachmentName(int id, char[] name, int len);
	public native int LookupAttachment(const char[] pAttachmentName);
	public native void GetAttachmentEx(int id, float absOrigin[3] = NULL_VECTOR, float absAngles[3] = NULL_VECTOR);
	public void GetAttachment(const char[] szName, float absOrigin[3] = NULL_VECTOR, float absAngles[3] = NULL_VECTOR)
	{
		int id = this.LookupAttachment(szName);
		if(id != -1) {
			this.GetAttachmentEx(id, absOrigin, absAngles);
		}
	}

	public native int GetNumBones();
	public native void GetBoneName(int id, char[] name, int len);
	public native int LookupBone(const char[] name);
	public native void GetBonePositionEx(int id, float absOrigin[3] = NULL_VECTOR, float absAngles[3] = NULL_VECTOR);
	public void GetBonePosition(const char[] szName, float absOrigin[3] = NULL_VECTOR, float absAngles[3] = NULL_VECTOR)
	{
		int id = this.LookupBone(szName);
		if(id != -1) {
			this.GetBonePositionEx(id, absOrigin, absAngles);
		}
	}

	public native int GetNumBodygroups();
	public native void GetBodygroupName(int id, char[] name, int len);
	public native int FindBodygroupByName(const char[] name);
	public native void SetBodygroupEx(int iGroup, int iValue);
	public void SetBodygroup(const char[] name, int iValue)
	{
		int id = this.FindBodygroupByName(name);
		if(id != -1) {
			this.SetBodygroupEx(id, iValue);
		}
	}

	public native int GetNumBoneControllers();
	public native float GetBoneControllerEx(int id);
	public native void SetBoneControllerEx(int id, float value);

	public native int GetNumSequences();
	public native void GetSequenceName(int id, char[] name, int len);
	public native void GetSequenceActivityName(int id, char[] name, int len);
	public native int LookupSequence(const char[] name);
	public void ResetSequenceEx(int sequence)
	{
		int entity = view_as<int>(this);
		int old = GetEntProp(entity, Prop_Send, "m_nSequence");
		bool changed = (old != sequence);
		SetEntProp(entity, Prop_Send, "m_nSequence", sequence);
		if(changed) {
			SetEntPropFloat(entity, Prop_Data, "m_flAnimTime", GetGameTime());
			SetEntPropFloat(entity, Prop_Send, "m_flCycle", 0.0);
			this.ResetSequenceInfo();
		}
	}
	public void ResetSequence(const char[] name)
	{
		int sequence = this.LookupSequence(name);
		if(sequence != -1) {
			this.ResetSequenceEx(sequence);
		}
	}
	public native float SequenceDurationEx(int sequence);
	public float SequenceDuration(const char[] name)
	{
		int id = this.LookupSequence(name);
		if(id != -1) {
			return this.SequenceDurationEx(id);
		} else {
			return 0.0;
		}
	}
};

methodmap BaseAnimatingOverlay < BaseAnimating
{
	public BaseAnimatingOverlay(int entity) { return view_as<BaseAnimatingOverlay>(entity); }

	public native int AddGestureSequenceEx1(int sequence, bool autokill=true);
	public native int AddGestureSequenceEx2(int sequence, float flDuration, bool autokill=true);
	public native int AddGestureEx1(Activity activity, bool autokill=true);
	public native int AddGestureEx2(Activity activity, float flDuration, bool autokill=true);
	public native bool IsPlayingGestureEx(Activity activity);
	public native void RestartGestureEx(Activity activity, bool addifmissing=true, bool autokill=true);
	public native void RemoveGestureEx(Activity activity);
	public native int AddLayeredSequenceEx(int sequence, int iPriority);

	public int AddGestureSequence1(const char[] name, bool autokill=true)
	{
		int id = this.LookupSequence(name);
		if(id != -1) {
			return this.AddGestureSequenceEx1(id, autokill);
		} else {
			return -1;
		}
	}
	public int AddGestureSequence2(const char[] name, float flDuration, bool autokill=true)
	{
		int id = this.LookupSequence(name);
		if(id != -1) {
			return this.AddGestureSequenceEx2(id, flDuration, autokill);
		} else {
			return -1;
		}
	}
	public int AddLayeredSequence(const char[] name, int iPriority)
	{
		int id = this.LookupSequence(name);
		if(id != -1) {
			return this.AddLayeredSequenceEx(id, iPriority);
		} else {
			return -1;
		}
	}

	public int AddGesture1(const char[] name, bool autokill=true)
	{
		Activity id = this.LookupActivity(name);
		if(id != ACT_INVALID) {
			return this.AddGestureEx1(id, autokill);
		} else {
			return -1;
		}
	}
	public int AddGesture2(const char[] name, float flDuration, bool autokill=true)
	{
		Activity id = this.LookupActivity(name);
		if(id != ACT_INVALID) {
			return this.AddGestureEx2(id, flDuration, autokill);
		} else {
			return -1;
		}
	}
	public bool IsPlayingGesture(const char[] name)
	{
		Activity id = this.LookupActivity(name);
		if(id != ACT_INVALID) {
			return this.IsPlayingGestureEx(id);
		} else {
			return false;
		}
	}
	public void RemoveGesture(const char[] name)
	{
		Activity id = this.LookupActivity(name);
		if(id != ACT_INVALID) {
			this.RemoveGestureEx(id);
		}
	}
	public void RestartGesture(const char[] name, bool addifmissing=true, bool autokill=true)
	{
		Activity id = this.LookupActivity(name);
		if(id != ACT_INVALID) {
			this.RestartGestureEx(id, addifmissing, autokill);
		}
	}

	public native void RemoveAllGestures();
	public native void SetLayerPriority(int iLayer, int iPriority);
	public native bool IsValidLayer(int iLayer);
	public native void SetLayerDuration(int iLayer, float flDuration);
	public native float GetLayerDuration(int iLayer);
	public native void SetLayerCycle(int iLayer, float flCycle);
	public native void SetLayerCycleEx(int iLayer, float flCycle, float flPrevCycle);
	public native float GetLayerCycle(int iLayer);
	public native void SetLayerPlaybackRate(int iLayer, float flPlaybackRate);
	public native void SetLayerWeight(int iLayer, float flWeight);
	public native float GetLayerWeight(int iLayer);
	public native void SetLayerBlendIn(int iLayer, float flBlendIn);
	public native void SetLayerBlendOut(int iLayer, float flBlendOut);
	public native void SetLayerAutokill(int iLayer, bool bAutokill);
	public native void SetLayerLooping(int iLayer, bool bLooping);
	public native void SetLayerNoRestore(int iLayer, bool bNoRestore);
	public native Activity GetLayerActivity(int iLayer);
	public native int GetLayerSequence(int iLayer);
	public native int FindGestureLayer(Activity activity);
	public native void RemoveLayer(int iLayer, float flKillRate=0.2, float flKillDelay=0.0);
	public native void FastRemoveLayer(int iLayer);
	public native int GetNumAnimOverlays();
	public native void SetNumAnimOverlays(int num);
	public native void VerifyOrder();
	public native bool HasActiveLayer();
	public native int AllocateLayer(int iPriority=0);
	public static native int MaxOverlays()
};

methodmap BaseFlex < BaseAnimatingOverlay
{
	public BaseFlex(int entity) { return view_as<BaseFlex>(entity); }

	public native int GetNumFlexControllers();
	public native void GetFlexControllerName(int id, char[] name, int len);
	public native int FindFlexController(const char[] name);
	public native void SetFlexWeightEx(int id, float value);
	public native float GetFlexWeightEx(int id);
	public void SetFlexWeight(const char[] name, float value)
	{
		int id = this.FindFlexController(name);
		if(id != -1) {
			this.SetFlexWeightEx(id, value);
		}
	}
	public float GetFlexWeight(const char[] name)
	{
		int id = this.FindFlexController(name);
		if(id != -1) {
			return this.GetFlexWeightEx(id);
		} else {
			return 0.0;
		}
	}
};

native Activity ActivityList_IndexForName(const char[] name);
native void ActivityList_NameForIndex(int id, char[] name, int len);

native int EventList_IndexForName(const char[] name);
native void EventList_NameForIndex(int id, char[] name, int len);

stock bool EntityIsAnimating(int entity) { return HasEntProp(entity, Prop_Send, "m_nSequence"); }
stock bool EntityIsAnimatingOverlay(int entity) { return HasEntProp(entity, Prop_Data, "m_AnimOverlay"); }
stock bool EntityIsFlex(int entity) { return HasEntProp(entity, Prop_Send, "m_flexWeight"); }

#if !defined REQUIRE_EXTENSIONS
public void __ext_animhelpers_SetNTVOptional()
{
	MarkNativeAsOptional("BaseAnimating.BaseAnimating");
	MarkNativeAsOptional("BaseAnimating.SelectWeightedSequence");
	MarkNativeAsOptional("BaseAnimating.LookupSequence");
	MarkNativeAsOptional("BaseAnimating.LookupActivity");
	MarkNativeAsOptional("BaseAnimating.LookupPoseParameter");
	MarkNativeAsOptional("BaseAnimating.SetPoseParameter");
	MarkNativeAsOptional("BaseAnimating.GetPoseParameter");

	MarkNativeAsOptional("BaseAnimatingOverlay.BaseAnimatingOverlay");
	MarkNativeAsOptional("BaseAnimatingOverlay.AddGestureSequence");
	MarkNativeAsOptional("BaseAnimatingOverlay.AddGestureSequenceEx");
	MarkNativeAsOptional("BaseAnimatingOverlay.AddGesture");
	MarkNativeAsOptional("BaseAnimatingOverlay.AddGestureEx");
	MarkNativeAsOptional("BaseAnimatingOverlay.IsPlayingGesture");
	MarkNativeAsOptional("BaseAnimatingOverlay.RestartGesture");
	MarkNativeAsOptional("BaseAnimatingOverlay.RemoveGesture");
	MarkNativeAsOptional("BaseAnimatingOverlay.RemoveAllGestures");
	MarkNativeAsOptional("BaseAnimatingOverlay.AddLayeredSequence");
	MarkNativeAsOptional("BaseAnimatingOverlay.SetLayerPriority");
	MarkNativeAsOptional("BaseAnimatingOverlay.IsValidLayer");
	MarkNativeAsOptional("BaseAnimatingOverlay.SetLayerDuration");
	MarkNativeAsOptional("BaseAnimatingOverlay.SetLayerCycle");
	MarkNativeAsOptional("BaseAnimatingOverlay.SetLayerCycleEx");
	MarkNativeAsOptional("BaseAnimatingOverlay.GetLayerCycle");
	MarkNativeAsOptional("BaseAnimatingOverlay.SetLayerPlaybackRate");
	MarkNativeAsOptional("BaseAnimatingOverlay.SetLayerWeight");
	MarkNativeAsOptional("BaseAnimatingOverlay.GetLayerWeight");
	MarkNativeAsOptional("BaseAnimatingOverlay.SetLayerBlendIn");
	MarkNativeAsOptional("BaseAnimatingOverlay.SetLayerBlendOut");
	MarkNativeAsOptional("BaseAnimatingOverlay.SetLayerAutokill");
	MarkNativeAsOptional("BaseAnimatingOverlay.SetLayerLooping");
	MarkNativeAsOptional("BaseAnimatingOverlay.SetLayerNoRestore");
	MarkNativeAsOptional("BaseAnimatingOverlay.GetLayerActivity");
	MarkNativeAsOptional("BaseAnimatingOverlay.GetLayerSequence");
	MarkNativeAsOptional("BaseAnimatingOverlay.FindGestureLayer");
	MarkNativeAsOptional("BaseAnimatingOverlay.RemoveLayer");
	MarkNativeAsOptional("BaseAnimatingOverlay.FastRemoveLayer");
	MarkNativeAsOptional("BaseAnimatingOverlay.GetNumAnimOverlays");
	MarkNativeAsOptional("BaseAnimatingOverlay.SetNumAnimOverlays");
	MarkNativeAsOptional("BaseAnimatingOverlay.VerifyOrder");
	MarkNativeAsOptional("BaseAnimatingOverlay.HasActiveLayer");
	MarkNativeAsOptional("BaseAnimatingOverlay.AllocateLayer");
	MarkNativeAsOptional("BaseAnimatingOverlay.MaxOverlays");
}
#endif

public Extension __ext_animhelpers =
{
	name = "animhelpers", 
	file = "animhelpers.ext", 
	
#if defined AUTOLOAD_EXTENSIONS
	autoload=1,
#else
	autoload=0,
#endif

#if defined REQUIRE_EXTENSIONS
	required=1,
#else
	required=0,
#endif
};
