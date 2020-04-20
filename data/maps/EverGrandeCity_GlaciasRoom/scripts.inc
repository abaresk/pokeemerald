EverGrandeCity_GlaciasRoom_MapScripts:: @ 8228412
	map_script MAP_SCRIPT_ON_FRAME_TABLE, EverGrandeCity_GlaciasRoom_OnFrame
	map_script MAP_SCRIPT_ON_LOAD, EverGrandeCity_GlaciasRoom_OnLoad
	map_script MAP_SCRIPT_ON_WARP_INTO_MAP_TABLE, EverGrandeCity_GlaciasRoom_OnWarp
	.byte 0

EverGrandeCity_GlaciasRoom_OnWarp: @ 8228422
	map_script_2 VAR_TEMP_1, 0, EverGrandeCity_GlaciasRoom_EventScript_PlayerTurnNorth
	.2byte 0

EverGrandeCity_GlaciasRoom_EventScript_PlayerTurnNorth:: @ 822842C
	turnobject OBJ_EVENT_ID_PLAYER, DIR_NORTH
	end

EverGrandeCity_GlaciasRoom_OnFrame: @ 8228431
	map_script_2 VAR_ELITE_4_STATE, 2, EverGrandeCity_GlaciasRoom_EventScript_WalkInCloseDoor
	.2byte 0

EverGrandeCity_GlaciasRoom_EventScript_WalkInCloseDoor:: @ 822843B
	lockall
	call PokemonLeague_EliteFour_EventScript_WalkInCloseDoor
	setvar VAR_ELITE_4_STATE, 3
	releaseall
	end

EverGrandeCity_GlaciasRoom_OnLoad: @ 8228448
	call_if_set FLAG_DEFEATED_ELITE_4_GLACIA, EverGrandeCity_GlaciasRoom_EventScript_ResetAdvanceToNextRoom
	compare VAR_ELITE_4_STATE, 3
	call_if_eq EverGrandeCity_GlaciasRoom_EventScript_CloseDoor
	end

EverGrandeCity_GlaciasRoom_EventScript_ResetAdvanceToNextRoom:: @ 822845D
	call PokemonLeague_EliteFour_EventScript_ResetAdvanceToNextRoom
	return

EverGrandeCity_GlaciasRoom_EventScript_CloseDoor:: @ 8228463
	call PokemonLeague_EliteFour_EventScript_CloseDoor
	return

EverGrandeCity_GlaciasRoom_EventScript_Glacia:: @ 8228469
	lock
	faceplayer
	goto_if_unset FLAG_DEFEATED_ELITE_4_GLACIA, EverGrandeCity_GlaciasRoom_EventScript_GlaciaEncounter
	goto_if_set TRAINER_GLACIA_2, EverGrandeCity_GlaciasRoom_EventScript_RematchPostBattleSpeech
	goto EverGrandeCity_GlaciasRoom_EventScript_PostBattleSpeech
	end

EverGrandeCity_GlaciasRoom_EventScript_GlaciaEncounter::
	playbgm MUS_SITENNOU, 0

	@ First battle
	trainerbattle_single TRAINER_GLACIA, EverGrandeCity_GlaciasRoom_Text_IntroSpeech, EverGrandeCity_GlaciasRoom_Text_Defeat, EverGrandeCity_GlaciasRoom_EventScript_Defeated

	@ Rematch
	random 2
	compare VAR_RESULT, 1
	goto_if_eq EverGrandeCity_GlaciasRoom_EventScript_GlaciaIntro_GymLeader
	msgbox EverGrandeCity_GlaciasRoom_Text_RematchIntroSpeech_Froslass, MSGBOX_DEFAULT
	goto EverGrandeCity_GlaciasRoom_EventScript_GlaciaRematchBattle
	end

EverGrandeCity_GlaciasRoom_EventScript_GlaciaIntro_GymLeader::
	msgbox EverGrandeCity_GlaciasRoom_Text_RematchIntroSpeech_GymLeader, MSGBOX_DEFAULT
	goto EverGrandeCity_GlaciasRoom_EventScript_GlaciaRematchBattle
	end

EverGrandeCity_GlaciasRoom_EventScript_GlaciaRematchBattle::
	trainerbattle_no_intro TRAINER_GLACIA_2, EverGrandeCity_GlaciasRoom_Text_RematchDefeat
	goto EverGrandeCity_GlaciasRoom_EventScript_RematchDefeated
	end

EverGrandeCity_GlaciasRoom_EventScript_PostBattleSpeech:: @ 8228490
	msgbox EverGrandeCity_GlaciasRoom_Text_PostBattleSpeech, MSGBOX_DEFAULT
	release
	end

EverGrandeCity_GlaciasRoom_EventScript_RematchPostBattleSpeech:: @ 8228490
	msgbox EverGrandeCity_GlaciasRoom_Text_RematchPostBattleSpeech, MSGBOX_DEFAULT
	release
	end

EverGrandeCity_GlaciasRoom_EventScript_Defeated:: @ 822849A
	setflag FLAG_DEFEATED_ELITE_4_GLACIA
	call PokemonLeague_EliteFour_SetAdvanceToNextRoomMetatiles
	msgbox EverGrandeCity_GlaciasRoom_Text_PostBattleSpeech, MSGBOX_DEFAULT
	release
	end

EverGrandeCity_GlaciasRoom_EventScript_RematchDefeated::
	setflag FLAG_DEFEATED_ELITE_4_GLACIA
	call PokemonLeague_EliteFour_SetAdvanceToNextRoomMetatiles
	msgbox EverGrandeCity_GlaciasRoom_Text_RematchPostBattleSpeech, MSGBOX_DEFAULT
	release
	end	

EverGrandeCity_GlaciasRoom_Text_IntroSpeech: @ 82284AC
	.string "Welcome, my name is GLACIA\n"
	.string "of the ELITE FOUR.\p"
	.string "I've traveled from afar to HOENN\n"
	.string "so that I may hone my ice skills.\p"
	.string "But all I have seen are challenges by\n"
	.string "weak TRAINERS and their POKéMON.\p"
	.string "What about you?\p"
	.string "It would please me to no end if I could\n"
	.string "go all out against you!$"

EverGrandeCity_GlaciasRoom_Text_Defeat: @ 82285B4
	.string "You and your POKéMON…\n"
	.string "How hot your spirits burn!\p"
	.string "The all-consuming heat overwhelms.\p"
	.string "It's no surprise that my icy skills\n"
	.string "failed to harm you.$"

EverGrandeCity_GlaciasRoom_Text_PostBattleSpeech: @ 8228640
	.string "Advance to the next room.\p"
	.string "And there, confirm the truly fearsome\n"
	.string "side of the POKéMON LEAGUE.$"

EverGrandeCity_GlaciasRoom_Text_RematchIntroSpeech_Froslass:
	.string "Hello again, young CHAMPION.\p"
	.string "Back in my home region, there are legends\n"
	.string "of a woman who froze on a snowcapped\l"
	.string "mountain and became a POKéMON.\p"
	.string "The grace of such a figure…\n"
	.string "The thought of her inflamed spirit…\p"
	.string "I'll do right by her and show you the\n"
	.string "power of an all-out ice offensive!$"

EverGrandeCity_GlaciasRoom_Text_RematchIntroSpeech_GymLeader:
	.string "Hello again, young CHAMPION.\p"
	.string "I used to be a GYM LEADER in another\n"
	.string "region, you know.\p"
	.string "You remind me of someone I once\n"
	.string "trained…\p"
	.string "A girl with fiery focus and a will of\n"
	.string "steel.\p"
	.string "She took up the GYM LEADER role when I\n"
	.string "accepted my position here.\p"
	.string "I hope you'll rise to this occasion and\n"
	.string "engage me in the full heat of battle!$"

EverGrandeCity_GlaciasRoom_Text_RematchDefeat:
	.string "Still your spirit burns red-hot!$"

EverGrandeCity_GlaciasRoom_Text_RematchPostBattleSpeech:
	.string "My icy barrage…\n"
	.string "evaporated in an instant…\p"
	.string "But your challenge is far from over.\p"
	.string "Do not let your guard down--I assure\n"
	.string "you, your next opponents will show no\l"
	.string "restraint.\p"
	.string "Go on, DRAKE awaits you.$"
