#define EVENT_SPECIFIC			0
#define EVENT_SCRIPTED			1000		// see scriptevent.h
#define EVENT_SHARED			2000
#define EVENT_WEAPON			3000
#define EVENT_CLIENT			5000

#define NPC_EVENT_BODYDROP_LIGHT	2001
#define NPC_EVENT_BODYDROP_HEAVY	2002

#define NPC_EVENT_SWISHSOUND		2010

#define NPC_EVENT_180TURN			2020

#define NPC_EVENT_ITEM_PICKUP					2040
#define NPC_EVENT_WEAPON_DROP					2041
#define NPC_EVENT_WEAPON_SET_SEQUENCE_NAME		2042
#define NPC_EVENT_WEAPON_SET_SEQUENCE_NUMBER	2043
#define NPC_EVENT_WEAPON_SET_ACTIVITY			2044

#define	NPC_EVENT_LEFTFOOT			2050
#define NPC_EVENT_RIGHTFOOT			2051

#define NPC_EVENT_OPEN_DOOR			2060

// !! DON'T CHANGE TO ORDER OF THESE.  THEY ARE HARD CODED IN THE WEAPON QC FILES (YUCK!) !!
#define EVENT_WEAPON_MELEE_HIT			3001
#define EVENT_WEAPON_SMG1				3002
#define EVENT_WEAPON_MELEE_SWISH		3003
#define EVENT_WEAPON_SHOTGUN_FIRE		3004
#define EVENT_WEAPON_THROW				3005
#define EVENT_WEAPON_AR1				3006
#define EVENT_WEAPON_AR2				3007
#define EVENT_WEAPON_HMG1				3008
#define EVENT_WEAPON_SMG2				3009
#define EVENT_WEAPON_MISSILE_FIRE		3010
#define EVENT_WEAPON_SNIPER_RIFLE_FIRE	3011
#define EVENT_WEAPON_AR2_GRENADE		3012
#define EVENT_WEAPON_THROW2				3013
#define	EVENT_WEAPON_PISTOL_FIRE		3014
#define EVENT_WEAPON_RELOAD				3015
#define EVENT_WEAPON_THROW3				3016
#define EVENT_WEAPON_RELOAD_SOUND		3017		// Use this + EVENT_WEAPON_RELOAD_FILL_CLIP to prevent shooting during the reload animation 
#define EVENT_WEAPON_RELOAD_FILL_CLIP	3018
#define EVENT_WEAPON_SMG1_BURST1		3101		// first round in a 3-round burst
#define EVENT_WEAPON_SMG1_BURSTN		3102		// 2, 3 rounds
#define EVENT_WEAPON_AR2_ALTFIRE		3103

#define EVENT_WEAPON_SEQUENCE_FINISHED	3900

// NOTE: MUST BE THE LAST WEAPON EVENT -- ONLY WEAPON EVENTS BETWEEN EVENT_WEAPON AND THIS
#define EVENT_WEAPON_LAST				3999

#define CL_EVENT_MUZZLEFLASH0		5001	// Muzzleflash on attachment 0
#define CL_EVENT_MUZZLEFLASH1		5011	// Muzzleflash on attachment 1
#define CL_EVENT_MUZZLEFLASH2		5021	// Muzzleflash on attachment 2
#define CL_EVENT_MUZZLEFLASH3		5031	// Muzzleflash on attachment 3
#define CL_EVENT_SPARK0				5002	// Spark on attachment 0
#define CL_EVENT_NPC_MUZZLEFLASH0	5003	// Muzzleflash on attachment 0 for third person views
#define CL_EVENT_NPC_MUZZLEFLASH1	5013	// Muzzleflash on attachment 1 for third person views
#define CL_EVENT_NPC_MUZZLEFLASH2	5023	// Muzzleflash on attachment 2 for third person views
#define CL_EVENT_NPC_MUZZLEFLASH3	5033	// Muzzleflash on attachment 3 for third person views
#define CL_EVENT_SOUND				5004	// Emit a sound // NOTE THIS MUST MATCH THE DEFINE AT CBaseEntity::PrecacheModel on the server!!!!!
#define CL_EVENT_EJECTBRASS1		6001	// Eject a brass shell from attachment 1

#define CL_EVENT_DISPATCHEFFECT0	9001	// Hook into a DispatchEffect on attachment 0
#define CL_EVENT_DISPATCHEFFECT1	9011	// Hook into a DispatchEffect on attachment 1
#define CL_EVENT_DISPATCHEFFECT2	9021	// Hook into a DispatchEffect on attachment 2
#define CL_EVENT_DISPATCHEFFECT3	9031	// Hook into a DispatchEffect on attachment 3
#define CL_EVENT_DISPATCHEFFECT4	9041	// Hook into a DispatchEffect on attachment 4
#define CL_EVENT_DISPATCHEFFECT5	9051	// Hook into a DispatchEffect on attachment 5
#define CL_EVENT_DISPATCHEFFECT6	9061	// Hook into a DispatchEffect on attachment 6
#define CL_EVENT_DISPATCHEFFECT7	9071	// Hook into a DispatchEffect on attachment 7
#define CL_EVENT_DISPATCHEFFECT8	9081	// Hook into a DispatchEffect on attachment 8
#define CL_EVENT_DISPATCHEFFECT9	9091	// Hook into a DispatchEffect on attachment 9

// These two events are used by c_env_spritegroup.
// FIXME: Should this be local to c_env_spritegroup?
#define CL_EVENT_SPRITEGROUP_CREATE		6002
#define CL_EVENT_SPRITEGROUP_DESTROY	6003
#define CL_EVENT_FOOTSTEP_LEFT			6004
#define CL_EVENT_FOOTSTEP_RIGHT			6005
#define CL_EVENT_MFOOTSTEP_LEFT			6006 // Footstep sounds based on material underfoot.
#define CL_EVENT_MFOOTSTEP_RIGHT		6007
#define CL_EVENT_MFOOTSTEP_LEFT_LOUD	6008 // Loud material impact sounds from feet attachments
#define CL_EVENT_MFOOTSTEP_RIGHT_LOUD	6009

#if defined GAME_TF2
	#define TF_OBJ_ENABLEBODYGROUP			6000
	#define TF_OBJ_DISABLEBODYGROUP			6001
	#define TF_OBJ_ENABLEALLBODYGROUPS		6002
	#define TF_OBJ_DISABLEALLBODYGROUPS		6003
	#define TF_OBJ_PLAYBUILDSOUND			6004

	#define TF_AE_CIGARETTE_THROW			7000
	#define TF_AE_GUN_SALUTE				7001
	#define TF_AE_PICKAXE_THROW				7002
#endif

#define AE_TYPE_SERVER			( 1 << 0 )
#define AE_TYPE_SCRIPTED		( 1 << 1 )		// see scriptevent.h
#define AE_TYPE_SHARED			( 1 << 2 )
#define AE_TYPE_WEAPON			( 1 << 3 )
#define AE_TYPE_CLIENT			( 1 << 4 )
#define AE_TYPE_FACEPOSER		( 1 << 5 )

#define AE_TYPE_NEWEVENTSYSTEM  ( 1 << 10 ) //Temporary flag.

#define AE_NOT_AVAILABLE		-1

enum
{
	AE_INVALID = -1,			// So we have something more succint to check for than '-1'
	AE_EMPTY,
	AE_NPC_LEFTFOOT, 
	AE_NPC_RIGHTFOOT, 
	AE_NPC_BODYDROP_LIGHT, 
	AE_NPC_BODYDROP_HEAVY, 
	AE_NPC_SWISHSOUND, 
	AE_NPC_180TURN, 
	AE_NPC_ITEM_PICKUP, 
	AE_NPC_WEAPON_DROP, 
	AE_NPC_WEAPON_SET_SEQUENCE_NAME, 
	AE_NPC_WEAPON_SET_SEQUENCE_NUMBER, 
	AE_NPC_WEAPON_SET_ACTIVITY, 
	AE_NPC_HOLSTER,
	AE_NPC_DRAW,
	AE_NPC_WEAPON_FIRE,

	AE_CL_PLAYSOUND, 
	AE_SV_PLAYSOUND,
	AE_CL_STOPSOUND,

	AE_START_SCRIPTED_EFFECT,
	AE_STOP_SCRIPTED_EFFECT,

	AE_CLIENT_EFFECT_ATTACH,
	
	AE_MUZZLEFLASH,			// Muzzle flash from weapons held by the player
	AE_NPC_MUZZLEFLASH,		// Muzzle flash from weapons held by NPCs
	
	AE_THUMPER_THUMP,		//Thumper Thump!
	AE_AMMOCRATE_PICKUP_AMMO,	//Ammo crate pick up ammo!

	AE_NPC_RAGDOLL,

	AE_NPC_ADDGESTURE,
	AE_NPC_RESTARTGESTURE,

	AE_NPC_ATTACK_BROADCAST,

	AE_NPC_HURT_INTERACTION_PARTNER,
	AE_NPC_SET_INTERACTION_CANTDIE,

	AE_SV_DUSTTRAIL,

	AE_CL_CREATE_PARTICLE_EFFECT,
#if defined GAME_L4D2
	AE_CL_STOP_PARTICLE_EFFECT,
	AE_CL_ADD_PARTICLE_EFFECT_CP,
	AE_CL_CREATE_PARTICLE_BRASS,
#endif

	AE_RAGDOLL,

	AE_CL_ENABLE_BODYGROUP,
	AE_CL_DISABLE_BODYGROUP,
	AE_CL_BODYGROUP_SET_VALUE,
	AE_CL_BODYGROUP_SET_VALUE_CMODEL_WPN,

	AE_WPN_PRIMARYATTACK,	// Used by weapons that want their primary attack to occur during an attack anim (i.e. grenade throwing)
	AE_WPN_INCREMENTAMMO,

	AE_WPN_HIDE,		// Used to hide player weapons
	AE_WPN_UNHIDE,		// Used to unhide player weapons

	AE_WPN_PLAYWPNSOUND,	// Play a weapon sound from the weapon script file

#if defined GAME_TF2
	AE_RD_ROBOT_POP_PANELS_OFF,

	AE_TAUNT_ENABLE_MOVE,
	AE_TAUNT_DISABLE_MOVE,
#endif

	AE_CL_REMOVE_PARTICLE_EFFECT,

	LAST_SHARED_ANIMEVENT,
};

enum struct animevent_t
{
	int				event;
	char options[64];
	float			cycle;
	float			eventtime;
	int				type;
	int pSource;
}
