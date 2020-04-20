EverGrandeCity_PhoebesRoom_MapScripts:: @ 8228174
	map_script MAP_SCRIPT_ON_LOAD, EverGrandeCity_PhoebesRoom_OnLoad
	map_script MAP_SCRIPT_ON_WARP_INTO_MAP_TABLE, EverGrandeCity_PhoebesRoom_OnWarp
	map_script MAP_SCRIPT_ON_FRAME_TABLE, EverGrandeCity_PhoebesRoom_OnFrame
	.byte 0

EverGrandeCity_PhoebesRoom_OnWarp: @ 8228184
	map_script_2 VAR_TEMP_1, 0, EverGrandeCity_PhoebesRoom_EventScript_PlayerTurnNorth
	.2byte 0

EverGrandeCity_PhoebesRoom_EventScript_PlayerTurnNorth:: @ 822818E
	turnobject OBJ_EVENT_ID_PLAYER, DIR_NORTH
	end

EverGrandeCity_PhoebesRoom_OnFrame: @ 8228193
	map_script_2 VAR_ELITE_4_STATE, 1, EverGrandeCity_PhoebesRoom_EventScript_WalkInCloseDoor
	.2byte 0

EverGrandeCity_PhoebesRoom_EventScript_WalkInCloseDoor:: @ 822819D
	lockall
	call PokemonLeague_EliteFour_EventScript_WalkInCloseDoor
	setvar VAR_ELITE_4_STATE, 2
	releaseall
	end

EverGrandeCity_PhoebesRoom_OnLoad: @ 82281AA
	call_if_set FLAG_DEFEATED_ELITE_4_PHOEBE, EverGrandeCity_PhoebesRoom_EventScript_ResetAdvanceToNextRoom
	compare VAR_ELITE_4_STATE, 2
	call_if_eq EverGrandeCity_PhoebesRoom_EventScript_CloseDoor
	end

EverGrandeCity_PhoebesRoom_EventScript_ResetAdvanceToNextRoom:: @ 82281BF
	call PokemonLeague_EliteFour_EventScript_ResetAdvanceToNextRoom
	return

EverGrandeCity_PhoebesRoom_EventScript_CloseDoor:: @ 82281C5
	call PokemonLeague_EliteFour_EventScript_CloseDoor
	return

EverGrandeCity_PhoebesRoom_EventScript_Phoebe:: @ 82281CB
	lock
	faceplayer
	goto_if_unset FLAG_DEFEATED_ELITE_4_PHOEBE, EverGrandeCity_PhoebesRoom_EventScript_PhoebeEncounter
	goto_if_set TRAINER_PHOEBE_2, EverGrandeCity_PhoebesRoom_EventScript_RematchPostBattleSpeech
	goto EverGrandeCity_PhoebesRoom_EventScript_PostBattleSpeech
	end

EverGrandeCity_PhoebesRoom_EventScript_PhoebeEncounter::
	playbgm MUS_SITENNOU, 0

	@ First battle
	trainerbattle_single TRAINER_PHOEBE, EverGrandeCity_PhoebesRoom_Text_IntroSpeech, EverGrandeCity_PhoebesRoom_Text_Defeat, EverGrandeCity_PhoebesRoom_EventScript_Defeated

	@ Rematch
	msgbox EverGrandeCity_PhoebesRoom_Text_RematchIntroSpeech, MSGBOX_DEFAULT
	trainerbattle_no_intro TRAINER_PHOEBE_2, EverGrandeCity_PhoebesRoom_Text_RematchDefeat
	goto EverGrandeCity_PhoebesRoom_EventScript_RematchDefeated
	end

EverGrandeCity_PhoebesRoom_EventScript_PostBattleSpeech:: @ 82281F2
	msgbox EverGrandeCity_PhoebesRoom_Text_PostBattleSpeech, MSGBOX_DEFAULT
	release
	end

EverGrandeCity_PhoebesRoom_EventScript_RematchPostBattleSpeech:: @ 82281F2
	msgbox EverGrandeCity_PhoebesRoom_Text_RematchPostBattleSpeech, MSGBOX_DEFAULT
	release
	end

EverGrandeCity_PhoebesRoom_EventScript_Defeated:: @ 82281FC
	setflag FLAG_DEFEATED_ELITE_4_PHOEBE
	call PokemonLeague_EliteFour_SetAdvanceToNextRoomMetatiles
	msgbox EverGrandeCity_PhoebesRoom_Text_PostBattleSpeech, MSGBOX_DEFAULT
	release
	end

EverGrandeCity_PhoebesRoom_EventScript_RematchDefeated::
	setflag FLAG_DEFEATED_ELITE_4_PHOEBE
	call PokemonLeague_EliteFour_SetAdvanceToNextRoomMetatiles
	msgbox EverGrandeCity_PhoebesRoom_Text_RematchPostBattleSpeech, MSGBOX_DEFAULT
	release
	end	

EverGrandeCity_PhoebesRoom_Text_IntroSpeech: @ 822820E
	.string "Ahahaha!\p"
	.string "I'm PHOEBE of the ELITE FOUR.\n"
	.string "I did my training on MT. PYRE.\p"
	.string "While I trained, I gained the ability\n"
	.string "to commune with GHOST-type POKéMON.\p"
	.string "Yes, the bond I developed with POKéMON\n"
	.string "is extremely tight.\p"
	.string "So, come on, just try and see if you can\n"
	.string "even inflict damage on my POKéMON!$"

EverGrandeCity_PhoebesRoom_Text_Defeat: @ 8228325
	.string "Oh, darn.\n"
	.string "I've gone and lost.$"

EverGrandeCity_PhoebesRoom_Text_PostBattleSpeech: @ 8228343
	.string "There's a definite bond between you\n"
	.string "and your POKéMON, too.\p"
	.string "I didn't recognize it, so it's only\n"
	.string "natural that I lost.\p"
	.string "Yup, I'd like to see how far your bond\n"
	.string "will carry you.\p"
	.string "Go ahead, move on to the next room.$"

EverGrandeCity_PhoebesRoom_Text_RematchIntroSpeech:
	.string "Muhahaha!\p"
	.string "After I lost to you, I traveled to a\n"
	.string "distant region to train.\p"
	.string "There I encountered a shrine by a cove,\n"
	.string "where I communed with a powerful\l"
	.string "guardian POKéMON.\p"
	.string "Now, the bond between me and my\n"
	.string "POKéMON is at an all-time high.\p"
	.string "So, let's see if you can best me!$"

EverGrandeCity_PhoebesRoom_Text_RematchDefeat:
	.string "Oh, darn.\n"
	.string "You beat me again.$"

EverGrandeCity_PhoebesRoom_Text_RematchPostBattleSpeech:
	.string "I see the bond you share with your\n"
	.string "POKéMON has strengthened, too.\p"
	.string "Yes, that bond will take you far.\p"
	.string "Now, go on with your POKéMON LEAGUE\n"
	.string "challenge.$"
