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
#if defined GAME_L4D2
	FIRE_BULLETS_NO_PIERCING_SPARK = 0x16,	// do a piercing spark effect when a bullet penetrates an alien
	FIRE_BULLETS_HULL = 0x32,	// bullet trace is a hull rather than a line
	FIRE_BULLETS_ANGULAR_SPREAD = 0x64,	// bullet spread is based on uniform random change to angles rather than gaussian search
#endif
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
#if defined GAME_TF2
	bool m_bUseServerRandomSeed;
#endif
}

#define FireBulletsInfo_tVar any[]

methodmap BaseEntity
{
	public BaseEntity(int entity) { return view_as<BaseEntity>(entity); }
	public native void FireBullets(FireBulletsInfo_tVar info);
	public native void WorldSpaceCenter(float center[3]);
	public native void SetAbsOrigin(float pos[3]);
}

typeset BaseAnimatingFuncs
{
	//SetHandleAnimEvent
	function Action (BaseAnimating anim, animevent_t event, any data);
};

methodmap BaseAnimating < BaseEntity
{
	public BaseAnimating(int entity) { return view_as<BaseAnimating>(entity); }

	public native Activity LookupActivity(const char[] name);
	public native int SelectWeightedSequence(Activity activity);
	public native int SelectHeaviestSequence(Activity activity);

	public native int GetNumPoseParameters();
	public native void GetPoseParameterName(int id, char[] name, int len);
	public native int LookupPoseParameter(const char[] name);
	public native void SetPoseParameter(int index, float value);
	public native float GetPoseParameter(int index);
	public native void StudioFrameAdvance();
	public native void DispatchAnimEvents();
	public native void ResetSequenceInfo();

	public native int GetNumAttachments();
	public native void GetAttachmentName(int id, char[] name, int len);
	public native int LookupAttachment(const char[] pAttachmentName);
	public native int GetAttachmentBone(int id);
	public native bool GetAttachment(int id, float absOrigin[3] = NULL_VECTOR, float absAngles[3] = NULL_VECTOR);
	public native bool __GetAttachmentMatrix(int id, float matrix[3 * 4]);
	public bool GetAttachmentMatrix(int id, float matrix[3][4])
	{
		float __matrix[3 * 4];
		bool ret = this.__GetAttachmentMatrix(id, __matrix);

		matrix[0][0] = __matrix[0];
		matrix[0][1] = __matrix[1];
		matrix[0][2] = __matrix[2];
		matrix[0][3] = __matrix[3];

		matrix[1][0] = __matrix[4];
		matrix[1][1] = __matrix[5];
		matrix[1][2] = __matrix[6];
		matrix[1][3] = __matrix[7];

		matrix[2][0] = __matrix[8];
		matrix[2][1] = __matrix[9];
		matrix[2][2] = __matrix[10];
		matrix[2][3] = __matrix[11];

		return ret;
	}
	public native bool GetAttachmentLocal(int id, float absOrigin[3] = NULL_VECTOR, float absAngles[3] = NULL_VECTOR);

	public native int GetNumBones();
	public native void GetBoneName(int id, char[] name, int len);
	public native int LookupBone(const char[] name);
	public native void GetBonePosition(int id, float absOrigin[3] = NULL_VECTOR, float absAngles[3] = NULL_VECTOR);

	public native int GetNumBodygroups();
	public native void GetBodygroupName(int id, char[] name, int len);
	public native int FindBodygroupByName(const char[] name);
	public native void SetBodygroup(int iGroup, int iValue);

	public native int GetNumBoneControllers();
	public native float GetBoneController(int id);
	public native void SetBoneController(int id, float value);

	public native int GetNumSequences();
	public native void GetSequenceName(int id, char[] name, int len);
	public native void GetSequenceActivityName(int id, char[] name, int len);
	public native Activity GetSequenceActivity(int seq);
	public native int LookupSequence(const char[] name);
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
	public native float SequenceDuration(int sequence);

	public native void SetHandleAnimEvent(BaseAnimatingFuncs funcs, any data = 0);

	public native bool GetIntervalMovement(bool &bMoveSeqFinished = false, float pos[3] = NULL_VECTOR, float ang[3] = NULL_VECTOR);
	public native bool GetSequenceMovement(bool &bMoveSeqFinished = false, float pos[3] = NULL_VECTOR, float ang[3] = NULL_VECTOR);
};

methodmap BaseAnimatingOverlay < BaseAnimating
{
	public BaseAnimatingOverlay(int entity) { return view_as<BaseAnimatingOverlay>(entity); }

	public native int AddGestureSequence1(int sequence, bool autokill=true);
	public native int AddGestureSequence2(int sequence, float flDuration, bool autokill=true);
	public native int AddGesture1(Activity activity, bool autokill=true);
	public native int AddGesture2(Activity activity, float flDuration, bool autokill=true);
	public native bool IsPlayingGesture(Activity activity);
	public native void RestartGesture(Activity activity, bool addifmissing=true, bool autokill=true);
	public native void RemoveGesture(Activity activity);
	public native int AddLayeredSequence(int sequence, int iPriority);

	public native void RemoveAllGestures();
	public native void SetLayerPriority(int iLayer, int iPriority);
	public native bool IsValidLayer(int iLayer);
	public native void SetLayerDuration(int iLayer, float flDuration);
	public native float GetLayerDuration(int iLayer);
	public native void SetLayerCycle1(int iLayer, float flCycle);
	public native void SetLayerCycle2(int iLayer, float flCycle, float flPrevCycle);
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
	public native void SetFlexWeight(int id, float value);
	public native float GetFlexWeight(int id);
};

native Activity ActivityList_IndexForName(const char[] name);
native void ActivityList_NameForIndex(int id, char[] name, int len);

native int EventList_IndexForName(const char[] name);
native void EventList_NameForIndex(int id, char[] name, int len);

stock bool EntityIsAnimating(int entity) { return HasEntProp(entity, Prop_Send, "m_nSequence"); }
stock bool EntityIsAnimatingOverlay(int entity) { return HasEntProp(entity, Prop_Data, "m_AnimOverlay"); }
stock bool EntityIsFlex(int entity) { return HasEntProp(entity, Prop_Send, "m_flexWeight"); }

enum model_type_t
{
	model_bad, 
	model_brush, 
	model_sprite, 
	model_studio
};

methodmap Model_t
{
	property model_type_t Type
	{
		public native get();
	}

	public native void GetName(char[] name, int length);
	public native void GetBounds(float mins[3], float maxs[3]);
	public native void GetRenderBounds(float mins[3], float maxs[3]);

	property float Radius
	{
		public native get();
	}

	property int MaterialCount
	{
		public native get();
	}

	public native void GetMaterialName(int idx, char[] name, int len);
};

#define Model_Null (view_as<Model_t>(Address_Null))

methodmap ModelInfo
{
	public static native int GetModelIndex(const char[] name);
	public static native Model_t GetModel(int idx);

	public static int GetNumModels()
	{
		static int table = INVALID_STRING_TABLE;
		if(table == INVALID_STRING_TABLE) {
			table = FindStringTable("modelprecache");
		}
		return GetStringTableNumStrings(table);
	}

	public static native int GetNumWorldSubmodels();

	public static native void GetWorldSubmodelOrigin(int idx, float vec[3]);
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
