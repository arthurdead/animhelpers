#if defined animhelpers_included
	#endinput
#endif
#define animhelpers_included

#include <sdktools>

enum struct animevent_t
{
	int event;
	char options[64];
	float cycle;
	float eventtime;
	int type;
	int pSource;
}

#define animevent_tVar any[]

#include <animhelpers_activity>
#include <animhelpers_eventlist>

native void EntityWorldSpaceCenter(int entity, float center[3]);
native void EntitySetAbsOrigin(int entity, const float pos[3]);

typedef handleanimevent_func_t = function Action (int entity, animevent_t event);

native Activity AnimatingLookupActivity(int entity, const char[] name);
native int AnimatingSelectWeightedSequence(int entity, Activity activity);
native int AnimatingSelectHeaviestSequence(int entity, Activity activity);

native int AnimatingGetNumPoseParameters(int entity);
native int AnimatingGetPoseParameterName(int entity, int id, char[] name, int len);
native int AnimatingLookupPoseParameter(int entity, const char[] name);
native void AnimatingSetPoseParameter(int entity, int index, float value);
native float AnimatingGetPoseParameter(int entity, int index);
native void AnimatingStudioFrameAdvance(int entity);
native void AnimatingDispatchAnimEvents(int entity);
native void AnimatingResetSequenceInfo(int entity);

native int AnimatingGetNumAttachments(int entity);
native int AnimatingGetAttachmentName(int entity, int id, char[] name, int len);
native int AnimatingLookupAttachment(int entity, const char[] pAttachmentName);
native int AnimatingGetAttachmentBone(int entity, int id);
native bool AnimatingGetAttachment(int entity, int id, float absOrigin[3] = NULL_VECTOR, float absAngles[3] = NULL_VECTOR);
native bool AnimatingGetAttachmentLocal(int entity, int id, float absOrigin[3] = NULL_VECTOR, float absAngles[3] = NULL_VECTOR);

native bool AnimatingGetAttachmentMatrix(int entity, int id, float matrix[3][4]);

native int AnimatingGetNumBones(int entity);
native int AnimatingGetBoneName(int id, char[] name, int len);
native int AnimatingLookupBone(int entity, const char[] name);
native bool AnimatingGetBonePosition(int entity, int id, float absOrigin[3] = NULL_VECTOR, float absAngles[3] = NULL_VECTOR);
native bool AnimatingGetBonePositionLocal(int entity, int id, float absOrigin[3] = NULL_VECTOR, float absAngles[3] = NULL_VECTOR);

native int AnimatingGetNumBodygroups(int entity);
native int AnimatingGetBodygroupName(int entity, int id, char[] name, int len);
native int AnimatingFindBodygroupByName(int entity, const char[] name);
native void AnimatingSetBodygroup(int entity, int iGroup, int iValue);

native int AnimatingGetNumBoneControllers(int entity);
native float AnimatingGetBoneController(int entity, int id);
native void AnimatingSetBoneController(int entity, int id, float value);

native int AnimatingGetNumSequences(int entity);
native int AnimatingGetSequenceName(int entity, int id, char[] name, int len);
native int AnimatingGetSequenceActivityName(int entity, int id, char[] name, int len);
native Activity AnimatingGetSequenceActivity(int entity, int seq);
native int AnimatingLookupSequence(int entity, const char[] name);

stock void AnimatingResetSequence(int entity, int sequence, bool force = false)
{
	int old = GetEntProp(entity, Prop_Send, "m_nSequence");
	bool changed = (old != sequence);
	SetEntProp(entity, Prop_Send, "m_nSequence", sequence);
	if(force || changed) {
		SetEntPropFloat(entity, Prop_Data, "m_flAnimTime", GetGameTime());
		SetEntPropFloat(entity, Prop_Send, "m_flCycle", 0.0);
		AnimatingResetSequenceInfo(entity);
	}
}

native float AnimatingSequenceDuration(int entity, int sequence);
native int AnimatingSequenceNumFrames(int entity, int sequence);
native float AnimatingSequenceFPS(int entity, int sequence);
native float AnimatingSequenceCPS(int entity, int sequence);

stock int AnimatingSequenceFrame(int entity)
{
	int sequence = GetEntProp(entity, Prop_Send, "m_nSequence");
	int num_frames = AnimatingSequenceNumFrames(entity, sequence);

	float cycle = GetEntPropFloat(entity, Prop_Send, "m_flCycle");
	return RoundToFloor((cycle * float(num_frames)) / 1.0);
}

native void AnimatingHookHandleAnimEvent(int entity, handleanimevent_func_t func);
native void AnimatingHandleAnimEvent(int entity, animevent_tVar event);

native bool AnimatingGetIntervalMovement(int entity, bool &bMoveSeqFinished = false, float pos[3] = NULL_VECTOR, float ang[3] = NULL_VECTOR);
native bool AnimatingGetSequenceMovement(int entity, bool &bMoveSeqFinished = false, float pos[3] = NULL_VECTOR, float ang[3] = NULL_VECTOR);

native int AnimatingOverlayAddGestureSequence1(int entity, int sequence, bool autokill=true);
native int AnimatingOverlayAddGestureSequence2(int entity, int sequence, float flDuration, bool autokill=true);
native int AnimatingOverlayAddGesture1(int entity, Activity activity, bool autokill=true);
native int AnimatingOverlayAddGesture2(int entity, Activity activity, float flDuration, bool autokill=true);
native bool AnimatingOverlayIsPlayingGesture(int entity, Activity activity);
native void AnimatingOverlayRestartGesture(int entity, Activity activity, bool addifmissing=true, bool autokill=true);
native void AnimatingOverlayRemoveGesture(int entity, Activity activity);
native int AnimatingOverlayAddLayeredSequence(int entity, int sequence, int iPriority);

native void AnimatingOverlayRemoveAllGestures(int entity);
native void AnimatingOverlaySetLayerPriority(int entity, int iLayer, int iPriority);
native bool AnimatingOverlayIsValidLayer(int entity, int iLayer);
native void AnimatingOverlaySetLayerDuration(int entity, int iLayer, float flDuration);
native float AnimatingOverlayGetLayerDuration(int entity, int iLayer);
native void AnimatingOverlaySetLayerCycle1(int entity, int iLayer, float flCycle);
native void AnimatingOverlaySetLayerCycle2(int entity, int iLayer, float flCycle, float flPrevCycle);
native float AnimatingOverlayGetLayerCycle(int entity, int iLayer);
native void AnimatingOverlaySetLayerPlaybackRate(int entity, int iLayer, float flPlaybackRate);
native void AnimatingOverlaySetLayerWeight(int entity, int iLayer, float flWeight);
native float AnimatingOverlayGetLayerWeight(int entity, int iLayer);
native void AnimatingOverlaySetLayerBlendIn(int entity, int iLayer, float flBlendIn);
native void AnimatingOverlaySetLayerBlendOut(int entity, int iLayer, float flBlendOut);
native void AnimatingOverlaySetLayerAutokill(int entity, int iLayer, bool bAutokill);
native void AnimatingOverlaySetLayerLooping(int entity, int iLayer, bool bLooping);
native void AnimatingOverlaySetLayerNoRestore(int entity, int iLayer, bool bNoRestore);
native Activity AnimatingOverlayGetLayerActivity(int entity, int iLayer);
native int AnimatingOverlayGetLayerSequence(int entity, int iLayer);
native int AnimatingOverlayFindGestureLayer(int entity, Activity activity);
native void AnimatingOverlayRemoveLayer(int entity, int iLayer, float flKillRate=0.2, float flKillDelay=0.0);
native void AnimatingOverlayFastRemoveLayer(int entity, int iLayer);
native int AnimatingOverlayGetNumAnimOverlays(int entity);
native void AnimatingOverlaySetNumAnimOverlays(int entity, int num);
native void AnimatingOverlayVerifyOrder(int entity);
native bool AnimatingOverlayHasActiveLayer(int entity);
native int AnimatingOverlayAllocateLayer(int entity, int iPriority=0);
native int AnimatingOverlayMaxOverlays();

native int FlexGetNumFlexControllers(int entity);
native int FlexGetFlexControllerName(int entity, int id, char[] name, int len);
native int FlexFindFlexController(int entity, const char[] name);
native void FlexSetFlexWeight(int entity, int id, float value);
native float FlexGetFlexWeight(int entity, int id);

native Activity ActivityList_IndexForName(const char[] name);
native int ActivityList_NameForIndex(int id, char[] name, int len);
native Activity ActivityList_RegisterPrivateActivity(const char[] name);

native int EventList_IndexForName(const char[] name);
native int EventList_NameForIndex(int id, char[] name, int len);
native int EventList_RegisterPrivateEvent(const char[] name);

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
	MarkNativeAsOptional("AnimatingSelectWeightedSequence");
	MarkNativeAsOptional("AnimatingLookupSequence");
	MarkNativeAsOptional("AnimatingLookupActivity");
	MarkNativeAsOptional("AnimatingLookupPoseParameter");
	MarkNativeAsOptional("AnimatingSetPoseParameter");
	MarkNativeAsOptional("AnimatingGetPoseParameter");

	MarkNativeAsOptional("AnimatingOverlayAddGestureSequence");
	MarkNativeAsOptional("AnimatingOverlayAddGestureSequenceEx");
	MarkNativeAsOptional("AnimatingOverlayAddGesture");
	MarkNativeAsOptional("AnimatingOverlayAddGestureEx");
	MarkNativeAsOptional("AnimatingOverlayIsPlayingGesture");
	MarkNativeAsOptional("AnimatingOverlayRestartGesture");
	MarkNativeAsOptional("AnimatingOverlayRemoveGesture");
	MarkNativeAsOptional("AnimatingOverlayRemoveAllGestures");
	MarkNativeAsOptional("AnimatingOverlayAddLayeredSequence");
	MarkNativeAsOptional("AnimatingOverlaySetLayerPriority");
	MarkNativeAsOptional("AnimatingOverlayIsValidLayer");
	MarkNativeAsOptional("AnimatingOverlaySetLayerDuration");
	MarkNativeAsOptional("AnimatingOverlaySetLayerCycle");
	MarkNativeAsOptional("AnimatingOverlaySetLayerCycleEx");
	MarkNativeAsOptional("AnimatingOverlayGetLayerCycle");
	MarkNativeAsOptional("AnimatingOverlaySetLayerPlaybackRate");
	MarkNativeAsOptional("AnimatingOverlaySetLayerWeight");
	MarkNativeAsOptional("AnimatingOverlayGetLayerWeight");
	MarkNativeAsOptional("AnimatingOverlaySetLayerBlendIn");
	MarkNativeAsOptional("AnimatingOverlaySetLayerBlendOut");
	MarkNativeAsOptional("AnimatingOverlaySetLayerAutokill");
	MarkNativeAsOptional("AnimatingOverlaySetLayerLooping");
	MarkNativeAsOptional("AnimatingOverlaySetLayerNoRestore");
	MarkNativeAsOptional("AnimatingOverlayGetLayerActivity");
	MarkNativeAsOptional("AnimatingOverlayGetLayerSequence");
	MarkNativeAsOptional("AnimatingOverlayFindGestureLayer");
	MarkNativeAsOptional("AnimatingOverlayRemoveLayer");
	MarkNativeAsOptional("AnimatingOverlayFastRemoveLayer");
	MarkNativeAsOptional("AnimatingOverlayGetNumAnimOverlays");
	MarkNativeAsOptional("AnimatingOverlaySetNumAnimOverlays");
	MarkNativeAsOptional("AnimatingOverlayVerifyOrder");
	MarkNativeAsOptional("AnimatingOverlayHasActiveLayer");
	MarkNativeAsOptional("AnimatingOverlayAllocateLayer");
	MarkNativeAsOptional("AnimatingOverlayMaxOverlays");
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
