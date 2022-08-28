#define ACTIVITY_NOT_AVAILABLE -1

#if defined GAME_TF2
	#include "animhelpers_activity_tf2.inc"
#elseif defined GAME_L4D2
	#include "animhelpers_activity_l4d2.inc"
#endif
