#if defined animhelpers_included
	#endinput
#endif
#define animhelpers_included

#include <animhelpers_activity>

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
	public native int SelectWeightedSequenceEx(Activity activity, int sequence);
	public int SelectWeightedSequence(Activity activity)
	{
		int sequence = GetEntProp(view_as<int>(this), Prop_Send, "m_nSequence");
		return this.SelectWeightedSequenceEx(activity, sequence);
	}
	public native int LookupSequence(const char[] name);
	public native Activity LookupActivity(const char[] name);
	public native int LookupPoseParameter(const char[] name);
	public native void SetPoseParameter(int index, float value);
	public native float GetPoseParameter(int index);
	public native void StudioFrameAdvance();
	public native void DispatchAnimEvents();
	public native void ResetSequenceInfo();

	public native int LookupAttachment(const char[] pAttachmentName);
	public native void GetAttachmentEx(int id, float absOrigin[3] = NULL_VECTOR, float absAngles[3] = NULL_VECTOR);
	public void GetAttachment(const char[] szName, float absOrigin[3] = NULL_VECTOR, float absAngles[3] = NULL_VECTOR)
	{
		int id = this.LookupAttachment(szName);
		if(id != -1) {
			this.GetAttachmentEx(id, absOrigin, absAngles);
		}
	}

	public native int LookupBone(const char[] name);
	public native void GetBonePositionEx(int id, float absOrigin[3] = NULL_VECTOR, float absAngles[3] = NULL_VECTOR);
	public void GetBonePosition(const char[] szName, float absOrigin[3] = NULL_VECTOR, float absAngles[3] = NULL_VECTOR)
	{
		int id = this.LookupBone(szName);
		if(id != -1) {
			this.GetBonePositionEx(id, absOrigin, absAngles);
		}
	}
	public native float SequenceDuration(int sequence);
	public native int FindBodygroupByName(const char[] name);
	public native void SetBodygroup(int iGroup, int iValue);

	public void ResetSequence(int sequence)
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
};

methodmap BaseAnimatingOverlay < BaseAnimating
{
	public BaseAnimatingOverlay(int entity) { return view_as<BaseAnimatingOverlay>(entity); }
	public native int AddGestureSequence(int sequence, bool autokill=true);
	public native int AddGestureSequenceEx(int sequence, float flDuration, bool autokill=true);
	public native int AddGesture(Activity activity, bool autokill=true);
	public native int AddGestureEx(Activity activity, float flDuration, bool autokill=true);
	public native bool IsPlayingGesture(Activity activity);
	public native void RestartGesture(Activity activity, bool addifmissing=true, bool autokill=true);
	public native void RemoveGesture(Activity activity);
	public native void RemoveAllGestures();
	public native int AddLayeredSequence(int sequence, int iPriority);
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
