/**************************************************************************
    SolarSystem23 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    See <https://www.gnu.org/licenses/>.
*/

#include "RPU_Config.h"
#include "RPU.h"
#include "SolarSystem23.h"
#include "SelfTestAndAudit.h"
#include <EEPROM.h>

#ifndef RPU_SIMPLIFY_DISPLAY_FOR_7VOLUTION
#define USE_SCORE_OVERRIDES
#endif
#define USE_SCORE_ACHIEVEMENTS

#if defined(RPU_OS_USE_WAV_TRIGGER) || defined(RPU_OS_USE_WAV_TRIGGER_1p3)
#include "SendOnlyWavTrigger.h"
SendOnlyWavTrigger wTrig;             // Our WAV Trigger object
#endif

#define GALAXY_2021_MAJOR_VERSION  2023
#define GALAXY_2021_MINOR_VERSION  15
#define DEBUG_MESSAGES  0

// flickering GI attract (how long?)
// retry sun

/*********************************************************************

    Game specific code

*********************************************************************/

// MachineState
//  0 - Attract Mode
//  negative - self-test modes
//  positive - game play
char MachineState = 0;
boolean MachineStateChanged = true;
#define MACHINE_STATE_ATTRACT         0
#define MACHINE_STATE_INIT_GAMEPLAY   1
#define MACHINE_STATE_INIT_NEW_BALL   2
#define MACHINE_STATE_NORMAL_GAMEPLAY 4
#define MACHINE_STATE_COUNTDOWN_BONUS 99
#define MACHINE_STATE_BALL_OVER       100
#define MACHINE_STATE_MATCH_MODE      110

#define MACHINE_STATE_ADJUST_FREEPLAY           (MACHINE_STATE_TEST_DONE-1)
#define MACHINE_STATE_ADJUST_BALL_SAVE          (MACHINE_STATE_TEST_DONE-2)
#define MACHINE_STATE_ADJUST_SFX_AND_SOUNDTRACK (MACHINE_STATE_TEST_DONE-3)
#define MACHINE_STATE_ADJUST_MUSIC_VOLUME       (MACHINE_STATE_TEST_DONE-4)
#define MACHINE_STATE_ADJUST_SFX_VOLUME         (MACHINE_STATE_TEST_DONE-5)
#define MACHINE_STATE_ADJUST_CALLOUTS_VOLUME    (MACHINE_STATE_TEST_DONE-6)
#define MACHINE_STATE_ADJUST_TOURNAMENT_SCORING (MACHINE_STATE_TEST_DONE-7)
#define MACHINE_STATE_ADJUST_TILT_WARNING       (MACHINE_STATE_TEST_DONE-8)
#define MACHINE_STATE_ADJUST_AWARD_OVERRIDE     (MACHINE_STATE_TEST_DONE-9)
#define MACHINE_STATE_ADJUST_BALLS_OVERRIDE     (MACHINE_STATE_TEST_DONE-10)
#define MACHINE_STATE_ADJUST_SCROLLING_SCORES   (MACHINE_STATE_TEST_DONE-11)
#define MACHINE_STATE_ADJUST_EXTRA_BALL_AWARD   (MACHINE_STATE_TEST_DONE-12)
#define MACHINE_STATE_ADJUST_SPECIAL_AWARD      (MACHINE_STATE_TEST_DONE-13)
#define MACHINE_STATE_ADJUST_DIM_LEVEL          (MACHINE_STATE_TEST_DONE-14)
#define MACHINE_STATE_ADJUST_EXTRA_BALL_RANK    (MACHINE_STATE_TEST_DONE-15)
#define MACHINE_STATE_ADJUST_SPECIAL_RANK       (MACHINE_STATE_TEST_DONE-16)
#define MACHINE_STATE_ADJUST_SUN_MISSION_RANK   (MACHINE_STATE_TEST_DONE-17)
#define MACHINE_STATE_AJDUST_SIDE_QUEST_START   (MACHINE_STATE_TEST_DONE-18)
#define MACHINE_STATE_ADJUST_GALAXY_BALL_SAVE   (MACHINE_STATE_TEST_DONE-19)
#define MACHINE_STATE_ADJUST_SAVE_PROGRESS      (MACHINE_STATE_TEST_DONE-20)
#define MACHINE_STATE_ADJUST_DONE               (MACHINE_STATE_TEST_DONE-21)

int MachineStateToAdjustmentPrompt[] = {
  134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 0, 170, 171, 172, // Standard adjustment prompts
  150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169 // Game-specific adjustment prompts
};

// The lower 4 bits of the Game Mode are modes, the upper 4 are for frenzies
// and other flags that carry through different modes
#define GAME_MODE_SKILL_SHOT                        0
#define GAME_MODE_UNSTRUCTURED_PLAY                 1
#define GAME_MODE_MISSION_QUALIFIED                 2
#define GAME_MODE_MISSION_FUELING                   3
#define GAME_MODE_MISSION_READY_TO_LAUNCH           4
#define GAME_MODE_MISSION_LAUNCHING                 5
#define GAME_MODE_MISSION_FIRST_LEG                 6
#define GAME_MODE_MISSION_SECOND_LEG                7
#define GAME_MODE_MISSION_THIRD_LEG                 8
#define GAME_MODE_MISSION_FINISH                    9
#define GAME_MODE_WAITING_FINISH_WITH_BONUS         10
#define GAME_MODE_WIZARD_QUALIFIED                  11
#define GAME_MODE_WIZARD_START                      12
#define GAME_MODE_WIZARD                            13
#define GAME_MODE_WIZARD_FINISHED                   14
#define GAME_BASE_MODE                              0x0F
#define GAME_MODE_FLAG_CARGO_COLLECTION             0x10
#define GAME_MODE_FLAG_PASSENGER_ABOARD             0x20
#define GAME_MODE_FLAG_PIRATE_ENCOUNTER             0x40
#define GAME_MODE_FLAG_REFUEL                       0x80



#define EEPROM_BALL_SAVE_BYTE           100
#define EEPROM_FREE_PLAY_BYTE           101
#define EEPROM_MUSIC_LEVEL_BYTE         102
#define EEPROM_SKILL_SHOT_BYTE          103
#define EEPROM_TILT_WARNING_BYTE        104
#define EEPROM_AWARD_OVERRIDE_BYTE      105
#define EEPROM_BALLS_OVERRIDE_BYTE      106
#define EEPROM_TOURNAMENT_SCORING_BYTE  107
#define EEPROM_MUSIC_VOLUME_BYTE        108
#define EEPROM_SFX_VOLUME_BYTE          109
#define EEPROM_SCROLLING_SCORES_BYTE    110
#define EEPROM_CALLOUTS_VOLUME_BYTE     111
#define EEPROM_DIM_LEVEL_BYTE           113
#define EEPROM_RANK_FOR_EB_BYTE         114
#define EEPROM_RANK_FOR_SPECIAL_BYTE    115
#define EEPROM_RANK_FOR_SUN_BYTE        116
#define EEPROM_SIDE_QUEST_START_BYTE    117
#define EEPROM_GALAXY_BALLSAVE_BYTE     118
#define EEPROM_SAVE_PROGRESS_BYTE       119
#define EEPROM_EXTRA_BALL_SCORE_BYTE    140
#define EEPROM_SPECIAL_SCORE_BYTE       144


#define SOUND_EFFECT_NONE               0
#define SOUND_EFFECT_BONUS_COUNT        1
#define SOUND_EFFECT_INLANE_UNLIT       3
#define SOUND_EFFECT_OUTLANE_UNLIT      4
#define SOUND_EFFECT_INLANE_LIT         5
#define SOUND_EFFECT_OUTLANE_LIT        6
#define SOUND_EFFECT_BUMPER_HIT         7
#define SOUND_EFFECT_SKILL_SHOT         8
#define SOUND_EFFECT_ADD_CREDIT_1       10
#define SOUND_EFFECT_ADD_CREDIT_2       11
#define SOUND_EFFECT_ADD_CREDIT_3       12
#define SOUND_EFFECT_GALAXY_LETTER_AWARD  13
#define SOUND_EFFECT_DROP_TARGET_NORMAL 14
#define SOUND_EFFECT_BALL_OVER          19
#define SOUND_EFFECT_GAME_OVER          20
#define SOUND_EFFECT_EXTRA_BALL         23
#define SOUND_EFFECT_MACHINE_START      24
#define SOUND_EFFECT_TILT_WARNING       28
#define SOUND_EFFECT_MATCH_SPIN         30
#define SOUND_EFFECT_JACKPOT            31
#define SOUND_EFFECT_SLING_SHOT         34
#define SOUND_EFFECT_ROLLOVER           35
#define SOUND_EFFECT_ANIMATION_TICK     36
#define SOUND_EFFECT_REFUELING_SPINNER  37
#define SOUND_EFFECT_LAUNCH_ROCKET      38
#define SOUND_EFFECT_GALAXY_LETTER_COLLECT  39
#define SOUND_EFFECT_GALAXY_LETTER_UNLIT    40
#define SOUND_EFFECT_GALAXY_BOUNCE      41
#define SOUND_EFFECT_FEATURE_SHOT       42
#define SOUND_EFFECT_TILT               61
#define SOUND_EFFECT_SIDE_QUEST_STARTED 62
#define SOUND_EFFECT_PASSENGER_COLLECT  63
#define SOUND_EFFECT_CARGO_COLLECT      64
#define SOUND_EFFECT_PIRATE_COLLECT     65
#define SOUND_EFFECT_REFUEL_COLLECT     66
#define SOUND_EFFECT_DROP_SEQUENCE_SKILL  67
#define SOUND_EFFECT_BONUS_X_INCREASED    69
#define SOUND_EFFECT_SIDE_QUESTS_OVER   70
#define SOUND_EFFECT_MISSION_COMPLETE   71
#define SOUND_EFFECT_DROP_TARGET_CLEAR_1  72
#define SOUND_EFFECT_DROP_TARGET_CLEAR_2  (SOUND_EFFECT_DROP_TARGET_CLEAR_1+1)
#define SOUND_EFFECT_DROP_TARGET_CLEAR_3  (SOUND_EFFECT_DROP_TARGET_CLEAR_1+2)
#define SOUND_EFFECT_DROP_TARGET_CLEAR_4  (SOUND_EFFECT_DROP_TARGET_CLEAR_1+3)
#define SOUND_EFFECT_POP_BUMPER_ADV_1   76
#define SOUND_EFFECT_POP_BUMPER_ADV_3   77

#define SOUND_EFFECT_SUN_MISSION_LAUNCH 80
#define SUN_MISSION_LAUNCH_TOTAL_DURATION         16434
#define SUN_MISSION_LAUNCH_GI_NUM_TOGGLES         23
unsigned long SunMissionLaunchGIToggleTimes[] =  
    { 4000, 4200, 4300, 4450, 4800, 5000, 5700, 6500, 
      6700, 7050, 10000, 10750, 11700, 11850, 12000, 12150, 
      12300, 12450, 12550, 12700, 15000, 15200, 16200 };

#define SOUND_EFFECT_SELF_TEST_MODE_START             133
#define SOUND_EFFECT_SELF_TEST_AUDIO_OPTIONS_START    190

#define RALLY_MUSIC_WAITING_FOR_SKILLSHOT         0
#define RALLY_MUSIC_WAITING_FOR_SKILLSHOT_CALM    1
#define RALLY_MUSIC_WAITING_FOR_SKILLSHOT_UPBEAT  3
#define RALLY_MUSIC_WAITING_FOR_SKILLSHOT_MEDIUM  2

#define SOUND_EFFECT_WAITING_FOR_LAUNCH       110
#define SOUND_EFFECT_RADIO_TRANSITION         125

// General music
#define SOUND_EFFECT_BACKGROUND_SONG_1        700
#define SOUND_EFFECT_BACKGROUND_SONG_2        701
#define SOUND_EFFECT_BACKGROUND_SONG_3        702
#define SOUND_EFFECT_BACKGROUND_SONG_4        703
#define SOUND_EFFECT_BACKGROUND_SONG_18       717

// For each music set, there are five types of music
#define MUSIC_TYPE_UNSTRUCTURED_PLAY  0
#define MUSIC_TYPE_SIDE_QUEST         1
#define MUSIC_TYPE_MISSION            2
#define MUSIC_TYPE_WIZARD             3
#define MUSIC_TYPE_RALLY              4
//

byte CurrentMusicType = 0; // value can be 0-3
unsigned short MusicIndices[2][5] = {
  {700, 725, 750, 775, 790},
  {800, 825, 850, 875, 890}
};
unsigned short MusicNumEntries[2][4] = {
  {6, 5, 6, 3},
  {13, 5, 5, 3}
};
unsigned short MusicLengths[2][4][15] = {
  {
    {210, 185, 315, 270, 210, 230, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {225, 180, 195, 230, 190, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {265, 240, 280, 210, 175, 200, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {220, 320, 160, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
  },
  {
    {400, 280, 300, 165, 115, 200, 170, 230, 195, 250, 190, 195, 190, 0, 0},
    {185, 250, 210, 130, 125, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {190, 95, 110, 150, 52, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {220, 320, 160, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}     
  }
};


#define SOUND_EFFECT_ASCENDING_SPINNER        200
#define SOUND_EFFECT_ASCENDING_SPINNER_END    249
#define SOUND_EFFECT_SPINNER_COMPLETION       250

unsigned long BackgroundSongStartTime = 0; 
unsigned long BackgroundSongEndTime = 0; 

// Game play status callouts
#define NUM_VOICE_NOTIFICATIONS                 74
byte VoiceNotificationDurations[NUM_VOICE_NOTIFICATIONS] = {
  4, 3, 6, 4, 3, 2, 3, 2, 5, 4, 
  4, 3, 3, 3, 3, 4, 3, 3, 4, 3,
  3, 3, 4, 4, 4, 2, 3, 4, 3, 5,
  4, 6, 2, 4, 3, 5, 4, 6, 5, 7,
  3, 4, 2, 2, 2, 2, 2, 2, 2, 2, 
  3, 5, 4, 4, 4, 4, 4, 4, 3, 3,
  4, 4, 4, 4, 3, 3, 3, 3, 3, 5,
  3, 4, 2, 2
  };
unsigned long NextVoiceNotificationPlayTime;

#define SOUND_EFFECT_VP_VOICE_NOTIFICATIONS_START  500
#define SOUND_EFFECT_VP_PLUNGE_PROMPT_1            500
#define SOUND_EFFECT_VP_PLUNGE_PROMPT_2            501
#define SOUND_EFFECT_VP_PLUNGE_PROMPT_3            502
#define SOUND_EFFECT_VP_PLUNGE_PROMPT_4            503
#define SOUND_EFFECT_VP_SKILL_SHOT_1               504
#define SOUND_EFFECT_VP_SKILL_SHOT_2               505
#define SOUND_EFFECT_VP_SKILL_SHOT_3               506
#define SOUND_EFFECT_VP_SKILL_SHOT_4               507
#define SOUND_EFFECT_VP_SIDE_QUEST_REMINDER        508
#define SOUND_EFFECT_VP_REFULING_DONE              509
#define SOUND_EFFECT_VP_MERCURY_MISSION_START      510

#define SOUND_EFFECT_VP_COMING_IN_FOR_LANDING       519
#define SOUND_EFFECT_VP_CARGO_ADDED                 520
#define SOUND_EFFECT_VP_PIRATE_ENCOUNTER            521
#define SOUND_EFFECT_VP_REFUEL_STARTED              522
#define SOUND_EFFECT_VP_PASSEGER_ADDED              523
#define SOUND_EFFECT_VP_MORE_FUEL_REQUIRED          524
#define SOUND_EFFECT_VP_SIDE_QUEST_1                525
#define SOUND_EFFECT_VP_SIDE_QUEST_15               539
#define SOUND_EFFECT_VP_ACQUIRE_FUEL                540
#define SOUND_EFFECT_VP_POPS_FOR_PLAYFIELD_MULTI    541

#define SOUND_EFFECT_VP_ADD_PLAYER_1        542
#define SOUND_EFFECT_VP_ADD_PLAYER_2        (SOUND_EFFECT_ADD_PLAYER_1+1)
#define SOUND_EFFECT_VP_ADD_PLAYER_3        (SOUND_EFFECT_ADD_PLAYER_1+2)
#define SOUND_EFFECT_VP_ADD_PLAYER_4        (SOUND_EFFECT_ADD_PLAYER_1+3)
#define SOUND_EFFECT_VP_PLAYER_1_UP         546
#define SOUND_EFFECT_VP_PLAYER_2_UP         (SOUND_EFFECT_PLAYER_1_UP+1)
#define SOUND_EFFECT_VP_PLAYER_3_UP         (SOUND_EFFECT_PLAYER_1_UP+2)
#define SOUND_EFFECT_VP_PLAYER_4_UP         (SOUND_EFFECT_PLAYER_1_UP+3)
#define SOUND_EFFECT_VP_SHOOT_AGAIN         550

// new callouts
#define SOUND_EFFECT_VP_ENTERING_CRYO_SLEEP                     551
#define SOUND_EFFECT_VP_BUILD_MISSION_BONUS_1                   552
#define SOUND_EFFECT_VP_BUILD_MISSION_BONUS_2                   553
#define SOUND_EFFECT_VP_BUILD_MISSION_BONUS_3                   554
#define SOUND_EFFECT_VP_BUILD_MISSION_BONUS_4                   555
#define SOUND_EFFECT_VP_BUILD_MISSION_BONUS_5                   556
#define SOUND_EFFECT_VP_BUILD_MISSION_BONUS_6                   557
#define SOUND_EFFECT_VP_BUILD_MISSION_BONUS_7                   558
#define SOUND_EFFECT_VP_BUILD_MISSION_BONUS_8                   559
#define SOUND_EFFECT_VP_GALXAXY_TURNAROUND_TO_COMPLETE_MISSION  560
#define SOUND_EFFECT_VP_TOP_POP_TO_COMPLETE_MISSION             561
#define SOUND_EFFECT_VP_STANDUP_TARGET_TO_COMPLETE_MISSION      562
#define SOUND_EFFECT_VP_1X_PLAYFIELD                            563
#define SOUND_EFFECT_VP_2X_PLAYFIELD                            564
#define SOUND_EFFECT_VP_3X_PLAYFIELD                            565
#define SOUND_EFFECT_VP_4X_PLAYFIELD                            566
#define SOUND_EFFECT_VP_5X_PLAYFIELD                            567
#define SOUND_EFFECT_VP_6X_PLAYFIELD                            568
#define SOUND_EFFECT_VP_WAITING_FOR_LAUNCH_MISSION_TO_SUN       569
#define SOUND_EFFECT_VP_MISSION_WINDOW_CLOSED                   570
#define SOUND_EFFECT_VP_BUILD_WIZARD_JACKPOT_WITH_DROPS         571
#define SOUND_EFFECT_VP_JACKPOT                                 572
#define SOUND_EFFECT_VP_SUPER_JACKPOT                           573


#define MAX_DISPLAY_BONUS     120
#define TILT_WARNING_DEBOUNCE_TIME      1000


/*********************************************************************

    Machine state and options

*********************************************************************/
byte SoundtrackSelection = 0;
byte Credits = 0;
byte ChuteCoinsInProgress[3] = {0, 0, 0};
byte MusicLevel = 3;
byte BallSaveNumSeconds = 0;
byte CurrentBallSaveNumSeconds = 0;
byte MaximumCredits = 40;
byte BallsPerGame = 3;
byte DimLevel = 2;
byte ScoreAwardReplay = 0;
byte MusicVolume = 10;
byte SoundEffectsVolume = 10;
byte CalloutsVolume = 10;
boolean HighScoreReplay = true;
boolean MatchFeature = true;
boolean TournamentScoring = false;
boolean FreePlayMode = false;
unsigned long HighScore = 0;
unsigned long AwardScores[3];
unsigned long ExtraBallValue = 0;
unsigned long SpecialValue = 0;
unsigned long CurrentTime = 0;
unsigned long SoundSettingTimeout = 0;

#ifdef RPU_SIMPLIFY_DISPLAY_FOR_7VOLUTION
boolean ScrollingScores = false;
#else
boolean ScrollingScores = true;
#endif


/*********************************************************************

    Game State

*********************************************************************/
byte CurrentPlayer = 0;
byte CurrentBallInPlay = 1;
byte CurrentNumPlayers = 0;
byte Bonus[4];
byte BonusX[4];
byte GameMode = GAME_MODE_SKILL_SHOT;
byte MaxTiltWarnings = 2;
byte NumTiltWarnings = 0;
byte CurrentAchievements[4];

boolean SamePlayerShootsAgain = false;
boolean BallSaveUsed = false;
boolean ExtraBallCollected = false;
boolean SpecialCollected = false;
boolean ShowingModeStats = false;
boolean PlayScoreAnimationTick = true;

#define NUMBER_OF_SONGS_REMEMBERED    20
unsigned int LastSongsPlayed[NUMBER_OF_SONGS_REMEMBERED];

unsigned long CurrentScores[4];
unsigned long BallFirstSwitchHitTime = 0;
unsigned long BallTimeInTrough = 0;
unsigned long GameModeStartTime = 0;
unsigned long GameModeEndTime = 0;
unsigned long LastTiltWarningTime = 0;
unsigned long ScoreAdditionAnimation;
unsigned long ScoreAdditionAnimationStartTime;
unsigned long LastRemainingAnimatedScoreShown;
unsigned long ScoreMultiplier;
unsigned long BallSaveEndTime;

#define BALL_SAVE_GRACE_PERIOD  1500


/*********************************************************************

    Game Specific State Variables

*********************************************************************/
#define MISSION_FINISH_SHOT_GALAXY_TURNAROUND 0
#define MISSION_FINISH_SHOT_TOP_POP           1
#define MISSION_FINISH_SHOT_STANDUPS          2

#define MISSION_FEATURE_SHOT_DROPS            0
#define MISSION_FEATURE_SHOT_LOWER_POPS       1
#define MISSION_FEATURE_SHOT_STANDUPS         2
#define MISSION_FEATURE_SHOT_TOP_LANES        3
#define MISSION_FEATURE_SHOT_TOP_POP          4
#define MISSION_FEATURE_SHOT_SLING_SHOTS      5
#define MISSION_FEATURE_SHOT_SPINNER          6
#define MISSION_FEATURE_SHOT_SAUCER           7

byte NextMission;
byte RankForSunMission = 7;
byte RankForExtraBallLight = 3;
byte RankForSpecialLight = 5;
byte ExtraBallLit[4];
byte SpecialLit[4];
byte GalaxyTurnaroundLights[4];
byte GalaxyStatus[4];
byte ResumeGameMode[4];
byte ResumeFuel[4];
byte ResumeMissionNum[4];
byte FuelRequirements[] = {40, 20, 20, 32, 40, 72, 120, 136, 48};
byte MissionStartStage[] = {GAME_MODE_MISSION_THIRD_LEG, GAME_MODE_MISSION_THIRD_LEG, 
                            GAME_MODE_MISSION_SECOND_LEG, GAME_MODE_MISSION_SECOND_LEG, 
                            GAME_MODE_MISSION_FIRST_LEG, GAME_MODE_MISSION_FIRST_LEG, 
                            GAME_MODE_MISSION_FIRST_LEG, GAME_MODE_MISSION_FIRST_LEG,
                            GAME_MODE_MISSION_FIRST_LEG };
byte MissionFinishShot[] = {  MISSION_FINISH_SHOT_GALAXY_TURNAROUND, MISSION_FINISH_SHOT_TOP_POP,
                              MISSION_FINISH_SHOT_GALAXY_TURNAROUND, MISSION_FINISH_SHOT_STANDUPS,
                              MISSION_FINISH_SHOT_GALAXY_TURNAROUND, MISSION_FINISH_SHOT_GALAXY_TURNAROUND,
                              MISSION_FINISH_SHOT_TOP_POP, MISSION_FINISH_SHOT_STANDUPS,
                              MISSION_FINISH_SHOT_GALAXY_TURNAROUND };
byte MissionFeatureShot[] = { MISSION_FEATURE_SHOT_DROPS, MISSION_FEATURE_SHOT_LOWER_POPS,
                              MISSION_FEATURE_SHOT_STANDUPS, MISSION_FEATURE_SHOT_TOP_LANES,
                              MISSION_FEATURE_SHOT_TOP_POP, MISSION_FEATURE_SHOT_SLING_SHOTS, 
                              MISSION_FEATURE_SHOT_LOWER_POPS, MISSION_FEATURE_SHOT_SPINNER,
                              MISSION_FEATURE_SHOT_SAUCER };

byte CurrentSaucerValue;
byte SaucerHitLampNum;
byte CurrentFuel;
byte FuelRequired;
byte DropTargetStatus;
byte DTBankHitOrder;
byte SpinnerAscension;
byte PlayerRank[4];
byte PopsForMultiplier;
byte MidlaneStatus;
byte DropTargetClears[4];
byte StandupsHitThisBall;
byte SpinnerHits[4];
byte SideQuestsQualified[4];
byte OutlaneStatus;
byte WizardLastDropTargetUp;
byte SideQuestStartSwitches;
byte SideQuestNumTopPops;
byte GalaxyKickerBallSave = 5;

boolean GeneralIlluminationOn = true;
boolean TimersPaused = true;
boolean SideQuestQualifiedReminderPlayed;
boolean ShowGalaxyTurnaroundDraw;
boolean SaveMissionProgress = true;

unsigned int MissionsCompleted[4];

unsigned long LastSpinnerHit;
unsigned long BonusXAnimationStart;
unsigned long GalaxyLetterAnimationEnd[6];
unsigned long SaucerHitAnimationEnd;
unsigned long SaucerValueDecreaseTime;
unsigned long ResetDropTargetStatusTime;
unsigned long LastSwitchHitTime;
unsigned long SpinnerGoalReachedTime;
unsigned long GalaxyStatusResetTime;
unsigned long DropTargetGoalReachedTime;
unsigned long StandupGoalReachedTime;
unsigned long LastGameplayPrompt;
unsigned long SideQuestQualifiedEndTime;
unsigned long SideQuestEndTime;
unsigned long LastSaucerHitTime;
unsigned long GalaxyTurnaroundEjectTime = 0;
unsigned long GalaxyBounceAnimationStart = 0;
unsigned long MissionBonus;
unsigned long FeatureShotLastHit;
unsigned long AnnounceFinishShotTime;
unsigned long PeriodicModeCheck;
unsigned long WizardLastDropTargetSet;
unsigned long WizardJackpotValue;
unsigned long WizardJackpotLastChanged;

#define SIDE_QUEST_START_3_TOP_POPS 0x01
#define SIDE_QUEST_START_RIGHT_A    0x02

#define MISSION_TO_MERCURY      0
#define MISSION_TO_VENUS        1
#define MISSION_TO_MARS         2
#define MISSION_TO_JUPITER      3
#define MISSION_TO_SATURN       4
#define MISSION_TO_URANUS       5
#define MISSION_TO_NEPTUNE      6
#define MISSION_TO_PLUTO        7
#define MISSION_TO_SUN          8
#define MAX_NUMBER_OF_MISSIONS  9

#define GALAXY_STATUS_LETTER_G    0x01
#define GALAXY_STATUS_LETTER_A1   0x02
#define GALAXY_STATUS_LETTER_L    0x04
#define GALAXY_STATUS_LETTER_A2   0x08
#define GALAXY_STATUS_LETTER_X    0x10
#define GALAXY_STATUS_LETTER_Y    0x20

#define GALAXY_LETTER_ANIMATION_DURATION  5000

#define DROP_TARGET_CLEARS_FOR_CARGO  4
#define SPINS_FOR_REFUEL              250
#define SIDE_QUEST_QUALIFY_AWARD    20000
#define SIDE_QUEST_QUALIFY_TIME     60000
#define AWARD_FOR_CARGO_COLLECTION    3000
#define AWARD_FOR_PASSENGER_ABOARD    3000
#define AWARD_FOR_PIRATE_ENCOUNTER    3000
#define AWARD_FOR_REFUEL_SPIN         500

#define MISSION_FIRST_LEG_TIME      15000
#define MISSION_SECOND_LEG_TIME     15000
#define MISSION_THIRD_LEG_TIME      60000


void ReadStoredParameters() {
  HighScore = RPU_ReadULFromEEProm(RPU_HIGHSCORE_EEPROM_START_BYTE, 10000);
  Credits = RPU_ReadByteFromEEProm(RPU_CREDITS_EEPROM_BYTE);
  if (Credits > MaximumCredits) Credits = MaximumCredits;

  ReadSetting(EEPROM_FREE_PLAY_BYTE, 0);
  FreePlayMode = (EEPROM.read(EEPROM_FREE_PLAY_BYTE)) ? true : false;

  BallSaveNumSeconds = ReadSetting(EEPROM_BALL_SAVE_BYTE, 15);
  if (BallSaveNumSeconds > 20) BallSaveNumSeconds = 20;
  CurrentBallSaveNumSeconds = BallSaveNumSeconds;

  MusicLevel = ReadSetting(EEPROM_MUSIC_LEVEL_BYTE, 3);
  if (MusicLevel > 4) MusicLevel = 4;
  if (MusicLevel>=3) SoundtrackSelection = MusicLevel-3;

  MusicVolume = ReadSetting(EEPROM_MUSIC_VOLUME_BYTE, 10);
  if (MusicVolume==0 || MusicVolume>10) MusicVolume = 10;

  SoundEffectsVolume = ReadSetting(EEPROM_SFX_VOLUME_BYTE, 10);
  if (SoundEffectsVolume==0 || SoundEffectsVolume>10) SoundEffectsVolume = 10;

  CalloutsVolume = ReadSetting(EEPROM_CALLOUTS_VOLUME_BYTE, 10);
  if (CalloutsVolume==0 || CalloutsVolume>10) CalloutsVolume = 10;

  RankForExtraBallLight = ReadSetting(EEPROM_RANK_FOR_EB_BYTE, 3);
  if (RankForExtraBallLight==0 || RankForExtraBallLight>10) RankForExtraBallLight = 3;

  RankForSpecialLight = ReadSetting(EEPROM_RANK_FOR_SPECIAL_BYTE, 5);
  if (RankForSpecialLight==0 || RankForSpecialLight>10) RankForSpecialLight = 5;

  RankForSunMission = ReadSetting(EEPROM_RANK_FOR_SUN_BYTE, 7);
  if (RankForSunMission==0 || RankForSunMission>10) RankForSunMission = 7;

  TournamentScoring = (ReadSetting(EEPROM_TOURNAMENT_SCORING_BYTE, 0)) ? true : false;

  MaxTiltWarnings = ReadSetting(EEPROM_TILT_WARNING_BYTE, 2);
  if (MaxTiltWarnings > 2) MaxTiltWarnings = 2;

  SideQuestStartSwitches = ReadSetting(EEPROM_SIDE_QUEST_START_BYTE, 0);
  if (SideQuestStartSwitches > 0x03) SideQuestStartSwitches = 0;

  GalaxyKickerBallSave = ReadSetting(EEPROM_GALAXY_BALLSAVE_BYTE, 5);
  if (GalaxyKickerBallSave>10) GalaxyKickerBallSave = 5;

  SaveMissionProgress = ReadSetting(EEPROM_SAVE_PROGRESS_BYTE, 1) ? true : false;

  byte awardOverride = ReadSetting(EEPROM_AWARD_OVERRIDE_BYTE, 99);
  if (awardOverride != 99) {
    ScoreAwardReplay = awardOverride;
  }

  byte ballsOverride = ReadSetting(EEPROM_BALLS_OVERRIDE_BYTE, 99);
  if (ballsOverride == 3 || ballsOverride == 5) {
    BallsPerGame = ballsOverride;
  } else {
    if (ballsOverride != 99) EEPROM.write(EEPROM_BALLS_OVERRIDE_BYTE, 99);
  }

  ScrollingScores = (ReadSetting(EEPROM_SCROLLING_SCORES_BYTE, 1)) ? true : false;
#ifdef RPU_SIMPLIFY_DISPLAY_FOR_7VOLUTION
  ScrollingScores = false;
#endif

  ExtraBallValue = RPU_ReadULFromEEProm(EEPROM_EXTRA_BALL_SCORE_BYTE);
  if (ExtraBallValue % 1000 || ExtraBallValue > 100000) ExtraBallValue = 20000;

  SpecialValue = RPU_ReadULFromEEProm(EEPROM_SPECIAL_SCORE_BYTE);
  if (SpecialValue % 1000 || SpecialValue > 100000) SpecialValue = 40000;

  DimLevel = ReadSetting(EEPROM_DIM_LEVEL_BYTE, 2);
  if (DimLevel < 2 || DimLevel > 3) DimLevel = 2;
  RPU_SetDimDivisor(1, DimLevel);

  AwardScores[0] = RPU_ReadULFromEEProm(RPU_AWARD_SCORE_1_EEPROM_START_BYTE);
  AwardScores[1] = RPU_ReadULFromEEProm(RPU_AWARD_SCORE_2_EEPROM_START_BYTE);
  AwardScores[2] = RPU_ReadULFromEEProm(RPU_AWARD_SCORE_3_EEPROM_START_BYTE);

}


#ifdef RPU_OS_USE_SB300
void InitSB300Registers() {
  RPU_PlaySB300SquareWave(1, 0x00); // Write 0x00 to CR2 (Timer 2 off, continuous mode, 16-bit, C2 clock, CR3 set)
  RPU_PlaySB300SquareWave(0, 0x00); // Write 0x00 to CR3 (Timer 3 off, continuous mode, 16-bit, C3 clock, not prescaled)
  RPU_PlaySB300SquareWave(1, 0x01); // Write 0x00 to CR2 (Timer 2 off, continuous mode, 16-bit, C2 clock, CR1 set)
  RPU_PlaySB300SquareWave(0, 0x00); // Write 0x00 to CR1 (Timer 1 off, continuous mode, 16-bit, C1 clock, timers allowed)
}


void PlaySB300StartupBeep() {
  RPU_PlaySB300SquareWave(1, 0x92); // Write 0x92 to CR2 (Timer 2 on, continuous mode, 16-bit, E clock, CR3 set)
  RPU_PlaySB300SquareWave(0, 0x92); // Write 0x92 to CR3 (Timer 3 on, continuous mode, 16-bit, E clock, not prescaled)
  RPU_PlaySB300SquareWave(4, 0x02); // Set Timer 2 to 0x0200
  RPU_PlaySB300SquareWave(5, 0x00); 
  RPU_PlaySB300SquareWave(6, 0x80); // Set Timer 3 to 0x8000
  RPU_PlaySB300SquareWave(7, 0x00);
  RPU_PlaySB300Analog(0, 0x02);
}
#endif


void setup() {
  if (DEBUG_MESSAGES) {
    Serial.begin(57600);
  }

  // Tell the OS about game-specific lights and switches
  RPU_SetupGameSwitches(NUM_SWITCHES_WITH_TRIGGERS, NUM_PRIORITY_SWITCHES_WITH_TRIGGERS, SolenoidAssociatedSwitches);

  // Set up the chips and interrupts
  RPU_InitializeMPU(RPU_CMD_BOOT_ORIGINAL_IF_CREDIT_RESET | RPU_CMD_BOOT_ORIGINAL_IF_NOT_SWITCH_CLOSED, SW_CREDIT_RESET);
  RPU_DisableSolenoidStack();
  RPU_SetDisableFlippers(true);

  // Read parameters from EEProm
  ReadStoredParameters();
  RPU_SetCoinLockout((Credits >= MaximumCredits) ? true : false);

  CurrentScores[0] = GALAXY_2021_MAJOR_VERSION;
  CurrentScores[1] = GALAXY_2021_MINOR_VERSION;
  CurrentScores[2] = RPU_OS_MAJOR_VERSION;
  CurrentScores[3] = RPU_OS_MINOR_VERSION;

#if defined(RPU_OS_USE_WAV_TRIGGER) || defined(RPU_OS_USE_WAV_TRIGGER_1p3)
  // WAV Trigger startup at 57600
  wTrig.start();
  wTrig.stopAllTracks();
  delayMicroseconds(10000);
#endif

#ifdef RPU_OS_USE_SB300
  ClearSoundQueue();
  InitSB300Registers();
  PlaySB300StartupBeep();
#endif

  StopAudio();
  CurrentTime = millis();
  PlaySoundEffect(SOUND_EFFECT_MACHINE_START, ConvertVolumeSettingToGain(SoundEffectsVolume));
  for (byte count=0; count<NUMBER_OF_SONGS_REMEMBERED; count++) LastSongsPlayed[count] = 0;
}

byte ReadSetting(byte setting, byte defaultValue) {
  byte value = EEPROM.read(setting);
  if (value == 0xFF) {
    EEPROM.write(setting, defaultValue);
    return defaultValue;
  }
  return value;
}


// This function is useful for checking the status of drop target switches
byte CheckSequentialSwitches(byte startingSwitch, byte numSwitches) {
  byte returnSwitches = 0;
  for (byte count = 0; count < numSwitches; count++) {
    returnSwitches |= (RPU_ReadSingleSwitchState(startingSwitch + count) << count);
  }
  return returnSwitches;
}


void SetDropTargets(byte newTargetStatus) {

  unsigned long nextDropTime = CurrentTime;

  // See if any targets need to be raised
  if (~newTargetStatus & DropTargetStatus) {
    RPU_PushToSolenoidStack(SOL_DROP_TARGET_RESET, 12);
    nextDropTime += 125;
  }

  for (int count=0; count<4; count++) {
    if (newTargetStatus & (1<<count)) {
      byte targetToDrop = 8 + count + (count/2)*2;
      RPU_PushToTimedSolenoidStack(targetToDrop, 8, nextDropTime);
      nextDropTime += 100;
    }
  }
      
  if (nextDropTime!=CurrentTime) ResetDropTargetStatusTime = nextDropTime;    
}


byte GetDropTargetStatus() {
  byte returnSwitches = 0;
  returnSwitches |= (RPU_ReadSingleSwitchState(SW_RED_DROP_TARGET));
  returnSwitches |= (RPU_ReadSingleSwitchState(SW_BLUE_DROP_TARGET)<<1);
  returnSwitches |= (RPU_ReadSingleSwitchState(SW_YELLOW_DROP_TARGET)<<2);
  returnSwitches |= (RPU_ReadSingleSwitchState(SW_BLACK_DROP_TARGET)<<3);

  return returnSwitches;
}

void SpotNextDropTarget() {
  if (ResetDropTargetStatusTime) return;
  byte solToDrop = 0;

  for (byte count=0; count<4; count++) {
    if ((DropTargetStatus&(0x01<<count))==0x00) {
      switch (count) {
        case 0: solToDrop = SOL_RED_DROP_TARGET; break;
        case 1: solToDrop = SOL_BLUE_DROP_TARGET; break;
        case 2: solToDrop = SOL_YELLOW_DROP_TARGET; break;
        case 3: solToDrop = SOL_BLACK_DROP_TARGET; break;
      }
      break;
    }
  }

  if (solToDrop!=0) {
    RPU_PushToSolenoidStack(solToDrop, 3);
  }
  
}

/*
byte GetNextMission() {
  byte nextMission = 0;
  for (nextMission=0; nextMission<MAX_NUMBER_OF_MISSIONS; nextMission++) {
    unsigned int missionBit = (unsigned int)1 << (unsigned int)nextMission;
    if ((MissionsCompleted[CurrentPlayer]&missionBit)==0) break;
  }
  return (nextMission);
}
*/

void SetMissionComplete(byte missionNum) {
  unsigned int missionBit = (unsigned int)1<<((unsigned int)missionNum);
  MissionsCompleted[CurrentPlayer] |= missionBit;
}

boolean CheckIfMissionComplete(byte missionNum) {
  unsigned int missionBit = (unsigned int)1<<((unsigned int)missionNum);
  if (MissionsCompleted[CurrentPlayer]&missionBit) return true;
  return false;
}

byte GetNextAvailableMission(byte curMission) {

  byte checkMission;
  for (byte count=0; count<(MAX_NUMBER_OF_MISSIONS-1); count++) {
    checkMission = (count+curMission)%(MAX_NUMBER_OF_MISSIONS-1);
    unsigned int missionBit = (unsigned int)1 << (unsigned int)checkMission;
    if ((MissionsCompleted[CurrentPlayer]&missionBit)==0) break;
  }
  curMission = checkMission;
  
  return curMission;
}



////////////////////////////////////////////////////////////////////////////
//
//  Lamp Management functions
//
////////////////////////////////////////////////////////////////////////////
void ShowLampAnimation(byte animationNum, unsigned long divisor, unsigned long baseTime, byte subOffset, boolean dim, boolean reverse = false, byte keepLampOn = 99) {
  byte currentStep = (baseTime / divisor) % LAMP_ANIMATION_STEPS;
  if (reverse) currentStep = (LAMP_ANIMATION_STEPS - 1) - currentStep;

  byte lampNum = 0;
  for (int byteNum = 0; byteNum < 8; byteNum++) {
    for (byte bitNum = 0; bitNum < 8; bitNum++) {

      // if there's a subOffset, turn off lights at that offset
      if (subOffset) {
        byte lampOff = true;
        lampOff = LampAnimations[animationNum][(currentStep + subOffset) % LAMP_ANIMATION_STEPS][byteNum] & (1 << bitNum);
        if (lampOff && lampNum != keepLampOn) RPU_SetLampState(lampNum, 0);
      }

      byte lampOn = false;
      lampOn = LampAnimations[animationNum][currentStep][byteNum] & (1 << bitNum);
      if (lampOn) RPU_SetLampState(lampNum, 1, dim);

      lampNum += 1;
    }
#if not defined (RPU_OS_SOFTWARE_DISPLAY_INTERRUPT)
    if (byteNum % 2) RPU_DataRead(0);
#endif
  }
}


void SetGeneralIllumination(boolean generalIlluminationOn) {
  
  if (generalIlluminationOn!=GeneralIlluminationOn) {
    GeneralIlluminationOn = generalIlluminationOn;
    RPU_SetContinuousSolenoidBit(GeneralIlluminationOn, SOLCONT_GENERAL_ILLUMINATION);
  }
  
}


byte BonusLampValues[] = {40, 30, 20, 10, 8, 6, 4, 2};
byte BonusLampScanAnimation[] = {7, 6, 2, 1, 0, 5, 4, 5, 0, 1, 2, 6};

void ShowBonusLamps() {
  if ((GameMode & GAME_BASE_MODE) == GAME_MODE_WIZARD_QUALIFIED) {
    for (int count=0; count<8; count++) RPU_SetLampState(LAMP_BONUS_40+count, 0);
  } else if ((GameMode & GAME_BASE_MODE) == GAME_MODE_SKILL_SHOT) {
    byte bonusPhase = ((CurrentTime-GameModeStartTime)/100)%12;    
    for (int count=0; count<8; count++) {
      if ((LAMP_BONUS_40+count)!=LAMP_BONUS_10) RPU_SetLampState(LAMP_BONUS_40+count, (BonusLampScanAnimation[bonusPhase])==count);
      else RPU_SetLampState(LAMP_BONUS_10, 1);
    }
  } else {
    if (Bonus[CurrentPlayer]>1) {
      byte curBonus = Bonus[CurrentPlayer];
      for (int count=0; count<8; count++) {
        if (curBonus>BonusLampValues[count]) {
          RPU_SetLampState(LAMP_BONUS_40+count, 1);
          curBonus -= BonusLampValues[count];
        } else {
          RPU_SetLampState(LAMP_BONUS_40+count, 0);
        }
      }
    } else {
      for (int count=0; count<7; count++) RPU_SetLampState(LAMP_BONUS_40+count, 0);
      RPU_SetLampState(LAMP_BONUS_2, 1, 0, 200);
    }
  }
}

void ShowSaucerLamps() {
  if ((GameMode & GAME_BASE_MODE) == GAME_MODE_SKILL_SHOT) {
    byte lampPhase = ((CurrentTime-GameModeStartTime)/300)%14;
    byte lampNum = 255;
    if (lampPhase<7) {
      if ((lampPhase%2)==0) lampNum = 0;
      CurrentSaucerValue = 2;
    } else if (lampPhase<11) {
      lampNum = lampPhase-6;
      CurrentSaucerValue = 2 + lampNum*2;
    } else {
      lampNum = 14-lampPhase;  
      CurrentSaucerValue = 2 + lampNum*2;
    }
    for (int count=0; count<5; count++) {
      RPU_SetLampState(LAMP_SAUCER_2K-count, (count==lampNum));
    }
  } else if (SideQuestQualifiedEndTime!=0 || ((GameMode & GAME_BASE_MODE)==GAME_MODE_WIZARD_QUALIFIED)) { 
    byte lampPhase = (CurrentTime/100)%6;
    for (int count=0; count<5; count++) {
      RPU_SetLampState(LAMP_SAUCER_2K-count, (count==lampPhase)||count==(lampPhase-1));
    }
  } else {
    if (SaucerHitAnimationEnd) {
      for (int count=0; count<5; count++) {
        RPU_SetLampState(LAMP_SAUCER_2K-count, (count==SaucerHitLampNum), 0, 120);
      }
      if (CurrentTime>SaucerHitAnimationEnd) SaucerHitAnimationEnd = 0;
    } else {
      int flashing = 0;
      if (SaucerValueDecreaseTime) { 
        if (CurrentTime<SaucerValueDecreaseTime) {
          flashing = 500;
          if ((SaucerValueDecreaseTime-CurrentTime)<5000) flashing = 125;
        } else {
          if (CurrentSaucerValue>2) {
            CurrentSaucerValue -= 2;
            SaucerValueDecreaseTime = CurrentTime + 10000;
          } else {
            SaucerValueDecreaseTime = 0;
          }
        }
      }
      for (int count=0; count<5; count++) {
        RPU_SetLampState(LAMP_SAUCER_2K-count, (count+1)==(CurrentSaucerValue/2), 0, flashing);
      }
    }
  }
}


void ShowBonusXLamps() {
  if ((GameMode & GAME_BASE_MODE) == GAME_MODE_SKILL_SHOT || (GameMode & GAME_BASE_MODE) == GAME_MODE_WIZARD_QUALIFIED) {
    for (int count=0; count<4; count++) RPU_SetLampState(LAMP_BONUS_5X+count, 0);
  } else {
    if (BonusX[CurrentPlayer]==1) {
      for (int count=0; count<4; count++) RPU_SetLampState(LAMP_BONUS_5X+count, 0);
    } else if (BonusX[CurrentPlayer]<6) {
      for (int count=0; count<4; count++) RPU_SetLampState(LAMP_BONUS_5X+count, BonusX[CurrentPlayer]==(5-count));
    } else {
      RPU_SetLampState(LAMP_BONUS_5X, BonusX[CurrentPlayer]>6, 0, BonusX[CurrentPlayer]>9?200:0);
      RPU_SetLampState(LAMP_BONUS_4X, BonusX[CurrentPlayer]==6 || BonusX[CurrentPlayer]==9);
      RPU_SetLampState(LAMP_BONUS_3X, BonusX[CurrentPlayer]==8);
      RPU_SetLampState(LAMP_BONUS_2X, BonusX[CurrentPlayer]<8);
    }
  }
}


void ShowShootAgainLamp() {

  if (!BallSaveUsed && CurrentBallSaveNumSeconds > 0 && (CurrentTime<BallSaveEndTime)) {
    unsigned long msRemaining = 5000;
    if (BallSaveEndTime!=0) msRemaining = BallSaveEndTime - CurrentTime;
    RPU_SetLampState(LAMP_SHOOT_AGAIN, 1, 0, (msRemaining < 5000) ? 100 : 500);
  } else {
    RPU_SetLampState(LAMP_SHOOT_AGAIN, SamePlayerShootsAgain);
  }
}


void ShowSpinnerLamp() {
  if ((GameMode & GAME_BASE_MODE) == GAME_MODE_WIZARD_QUALIFIED) {
    RPU_SetLampState(LAMP_SPINNER_1000, 0);
  } else if ((GameMode & GAME_BASE_MODE) == GAME_MODE_MISSION_FUELING) {
    if (CurrentFuel<FuelRequired) RPU_SetLampState(LAMP_SPINNER_1000, 1, 0, 400);
    else RPU_SetLampState(LAMP_SPINNER_1000, 0);
  } else if (SpinnerGoalReachedTime || (GameMode&GAME_MODE_FLAG_REFUEL)) {
    RPU_SetLampState(LAMP_SPINNER_1000, 1, 0, 125);
    if ( SpinnerGoalReachedTime && (CurrentTime-SpinnerGoalReachedTime) > 3000 ) {
      SpinnerGoalReachedTime = 0;
    }
  } else {
    RPU_SetLampState(LAMP_SPINNER_1000, 0);
  }
}


void ShowGalaxyLamps() {
  if ((GameMode & GAME_BASE_MODE) == GAME_MODE_WIZARD_QUALIFIED) {
    byte lampPhase = ((CurrentTime-GameModeStartTime)/200)%6;
    for (int count=0; count<6; count++) {
      RPU_SetLampState(LAMP_GALAXY_TURNAROUND_G-count, count==(5-lampPhase));
      RPU_SetLampState(LAMP_TOPLANE_G-count, 0);
    }
  } else if (GalaxyBounceAnimationStart) {
    byte lampPhase = ((CurrentTime-GalaxyBounceAnimationStart)/60);
    if (lampPhase>5) lampPhase = 10-lampPhase;
    for (int count=0; count<6; count++) {
      RPU_SetLampState(LAMP_GALAXY_TURNAROUND_G-count, count==lampPhase);
      RPU_SetLampState(LAMP_TOPLANE_G-count, 0);
    }
    if (CurrentTime>(GalaxyBounceAnimationStart+660)) GalaxyBounceAnimationStart = 0;
  } else if ((GameMode & GAME_BASE_MODE) == GAME_MODE_SKILL_SHOT) {
    for (int count=0; count<6; count++) {
      RPU_SetLampState(LAMP_GALAXY_TURNAROUND_G-count, 0);
      RPU_SetLampState(LAMP_TOPLANE_G-count, 0);
    }
  } else if ((GameMode & GAME_BASE_MODE)==GAME_MODE_MISSION_QUALIFIED) {
    byte lampBit = GALAXY_STATUS_LETTER_G;
    for (int count=0; count<6; count++) {
      if (CurrentTime>GalaxyLetterAnimationEnd[count]) {
        GalaxyLetterAnimationEnd[count] = 0;
      }

      if (GalaxyLetterAnimationEnd[count]) {        
        RPU_SetLampState(LAMP_GALAXY_TURNAROUND_G-count, 1, 1, 150);
      } else {
        RPU_SetLampState(LAMP_GALAXY_TURNAROUND_G-count, GalaxyTurnaroundLights[CurrentPlayer]&lampBit);
      }
      
      RPU_SetLampState(LAMP_TOPLANE_G-count, (GalaxyStatus[CurrentPlayer]&lampBit)==0);
      lampBit *= 2;  
    }
  } else if (GalaxyStatusResetTime || (GameMode&GAME_MODE_FLAG_PASSENGER_ABOARD)) {
    byte lampBit = GALAXY_STATUS_LETTER_G;
    for (int count=0; count<6; count++) {
      RPU_SetLampState(LAMP_GALAXY_TURNAROUND_G-count, GalaxyTurnaroundLights[CurrentPlayer]&lampBit);
      RPU_SetLampState(LAMP_TOPLANE_G-count, 1, 0, 125);
      lampBit *= 2;
    }

    if ( GalaxyStatusResetTime && (CurrentTime-GalaxyStatusResetTime) > 3000 ) {
      GalaxyStatusResetTime = 0;
    }
  } else {

    byte lampBit = GALAXY_STATUS_LETTER_G;
    
    for (int count=0; count<6; count++) {
      if (GalaxyLetterAnimationEnd[count]) {
        if (CurrentTime>GalaxyLetterAnimationEnd[count]) {
          GalaxyLetterAnimationEnd[count] = 0;
        } else {
          unsigned long msLeftUntilChange = GalaxyLetterAnimationEnd[count] - CurrentTime;
          unsigned long msSinceStart = GALAXY_LETTER_ANIMATION_DURATION - msLeftUntilChange;
          byte lampPhase = ((msSinceStart*msSinceStart)/200000)%2;
          if (!ShowGalaxyTurnaroundDraw) RPU_SetLampState(LAMP_GALAXY_TURNAROUND_G-count, lampPhase);
          RPU_SetLampState(LAMP_TOPLANE_G-count, (lampPhase==0));
        }
      } else {
        if (!ShowGalaxyTurnaroundDraw) RPU_SetLampState(LAMP_GALAXY_TURNAROUND_G-count, GalaxyTurnaroundLights[CurrentPlayer]&lampBit);
        RPU_SetLampState(LAMP_TOPLANE_G-count, (GalaxyStatus[CurrentPlayer]&lampBit)==0);
      }
      lampBit *= 2;  
      byte lampPhase = ((CurrentTime-GameModeStartTime)/100)%6;
      if (ShowGalaxyTurnaroundDraw) {
        RPU_SetLampState(LAMP_GALAXY_TURNAROUND_G-count, count==(5-lampPhase));
      }
    }
  }
}


void ShowMissionLamps() {

  if ((GameMode & GAME_BASE_MODE) == GAME_MODE_SKILL_SHOT || (GameMode & GAME_BASE_MODE) == GAME_MODE_WIZARD_QUALIFIED) {
    for (int count=0; count<MISSION_TO_SUN; count++) {
      RPU_SetLampState(LAMP_MERCURY-count, 0);
    }
    RPU_SetLampState(LAMP_SUN_SPECIAL, (GameMode & GAME_BASE_MODE) == GAME_MODE_WIZARD_QUALIFIED, 0, 125);
  } else if (DropTargetGoalReachedTime || StandupGoalReachedTime) {
    byte dtLampPhase = 7 - ((CurrentTime-DropTargetGoalReachedTime)/125)%8;
    byte suLampPhase = ((CurrentTime-StandupGoalReachedTime)/125)%8;
    if (DropTargetGoalReachedTime==0) dtLampPhase = 0xFF;
    if (StandupGoalReachedTime==0) suLampPhase = 0xFF;
    for (byte count=0; count<8; count++) {
      RPU_SetLampState(LAMP_MERCURY-count, (count==dtLampPhase || count==suLampPhase));
    }
    if ( (CurrentTime-DropTargetGoalReachedTime)>5000 ) {
      DropTargetGoalReachedTime = 0;
    }
    if ( (CurrentTime-StandupGoalReachedTime)>5000 ) {
      StandupGoalReachedTime = 0;
    }
    RPU_SetLampState(LAMP_SUN_SPECIAL, 0);
  } else {
    // Show all the complete/undone missions
    for (int count=0; count<MISSION_TO_SUN; count++) {
      if (count!=NextMission) RPU_SetLampState(LAMP_MERCURY-count, CheckIfMissionComplete(count));
    }

    if ((GameMode & GAME_BASE_MODE)!=GAME_MODE_UNSTRUCTURED_PLAY) {
      // To show the NextMission, we have to know what mode we're in
      int missionFlash = 1000;
      switch(GameMode&GAME_BASE_MODE) {
        case GAME_MODE_MISSION_QUALIFIED:
          missionFlash = 750;
          break;
        case GAME_MODE_MISSION_FUELING:
          missionFlash = 500;
          break;
        case GAME_MODE_MISSION_READY_TO_LAUNCH:
        case GAME_MODE_MISSION_LAUNCHING:
        case GAME_MODE_MISSION_FIRST_LEG:
        case GAME_MODE_MISSION_SECOND_LEG:
        case GAME_MODE_MISSION_THIRD_LEG:
        case GAME_MODE_MISSION_FINISH:
        case GAME_MODE_WIZARD_QUALIFIED:
        case GAME_MODE_WIZARD_START:
        case GAME_MODE_WIZARD:
        case GAME_MODE_WIZARD_FINISHED:
          missionFlash = 250;
          break;    
      }
    
      RPU_SetLampState(LAMP_MERCURY-NextMission, 1, 0, missionFlash);
    }
    RPU_SetLampState(LAMP_SUN_SPECIAL, 0);
  }
 
}


void ShowLaneLamps() {
  if ((GameMode & GAME_BASE_MODE) == GAME_MODE_SKILL_SHOT || (GameMode & GAME_BASE_MODE) == GAME_MODE_WIZARD_QUALIFIED) {
    RPU_SetLampState(LAMP_LEFT_OUTLANE, 0);
    RPU_SetLampState(LAMP_RIGHT_OUTLANE, 0);
    RPU_SetLampState(LAMP_LEFT_EXTRABALL, 0);
    RPU_SetLampState(LAMP_RIGHT_EXTRABALL, 0);
    RPU_SetLampState(LAMP_LEFT_SPECIAL, 0);
    RPU_SetLampState(LAMP_RIGHT_SPECIAL, 0);
    RPU_SetLampState(LAMP_SPOT_TARGET_INLANES, 0);
    RPU_SetLampState(LAMP_10X_SCORE_WHEN_LIT, 1, 0, 100);
  } else {
    RPU_SetLampState(LAMP_LEFT_OUTLANE, OutlaneStatus==1);
    RPU_SetLampState(LAMP_RIGHT_OUTLANE, OutlaneStatus==2);
    RPU_SetLampState(LAMP_LEFT_EXTRABALL, ExtraBallLit[CurrentPlayer]==1);
    RPU_SetLampState(LAMP_RIGHT_EXTRABALL, ExtraBallLit[CurrentPlayer]==2);
    RPU_SetLampState(LAMP_LEFT_SPECIAL, SpecialLit[CurrentPlayer]==1);
    RPU_SetLampState(LAMP_RIGHT_SPECIAL, SpecialLit[CurrentPlayer]==2);
    RPU_SetLampState(LAMP_SPOT_TARGET_INLANES, MidlaneStatus, 0, (MidlaneStatus==1)?250:0);
    RPU_SetLampState(LAMP_10X_SCORE_WHEN_LIT, 0);
  }
  
}


////////////////////////////////////////////////////////////////////////////
//
//  Display Management functions
//
////////////////////////////////////////////////////////////////////////////
unsigned long LastTimeScoreChanged = 0;
unsigned long LastTimeOverrideAnimated = 0;
unsigned long LastFlashOrDash = 0;
#define DISPLAY_OVERRIDE_BLANK_SCORE 0xFFFFFFFF
#ifdef USE_SCORE_OVERRIDES
unsigned long ScoreOverrideValue[4] = {0, 0, 0, 0};
byte ScoreOverrideStatus = 0;
#endif
byte LastScrollPhase = 0;

byte MagnitudeOfScore(unsigned long score) {
  if (score == 0) return 0;

  byte retval = 0;
  while (score > 0) {
    score = score / 10;
    retval += 1;
  }
  return retval;
}

#ifdef USE_SCORE_OVERRIDES
void OverrideScoreDisplay(byte displayNum, unsigned long value, boolean animate) {
  if (displayNum > 3) return;
  ScoreOverrideStatus |= (0x10 << displayNum);
  if (animate) ScoreOverrideStatus |= (0x01 << displayNum);
  else ScoreOverrideStatus &= ~(0x01 << displayNum);
  ScoreOverrideValue[displayNum] = value;
}
#else
void OverrideScoreDisplay(byte displayNum, unsigned long value, boolean animate) {
  (void)displayNum;
  (void)value;
  (void)animate;
}
#endif

byte GetDisplayMask(byte numDigits) {
  byte displayMask = 0;
  for (byte digitCount = 0; digitCount < numDigits; digitCount++) {
#ifdef RPU_OS_USE_7_DIGIT_DISPLAYS    
    displayMask |= (0x40 >> digitCount);
#else
    displayMask |= (0x20 >> digitCount);
#endif
  }
  return displayMask;
}


void ShowPlayerScores(byte displayToUpdate, boolean flashCurrent, boolean dashCurrent, unsigned long allScoresShowValue = 0) {

#ifdef USE_SCORE_OVERRIDES
  if (displayToUpdate == 0xFF) ScoreOverrideStatus = 0;
#endif

  byte displayMask = 0x3F;
  unsigned long displayScore = 0;
  unsigned long overrideAnimationSeed = CurrentTime / 250;
  byte scrollPhaseChanged = false;

  byte scrollPhase = ((CurrentTime - LastTimeScoreChanged) / 250) % 16;
  if (scrollPhase != LastScrollPhase) {
    LastScrollPhase = scrollPhase;
    scrollPhaseChanged = true;
  }

  boolean updateLastTimeAnimated = false;

  for (byte scoreCount = 0; scoreCount < 4; scoreCount++) {

#ifdef USE_SCORE_OVERRIDES
    // If this display is currently being overriden, then we should update it
    if (allScoresShowValue == 0 && (ScoreOverrideStatus & (0x10 << scoreCount))) {
      displayScore = ScoreOverrideValue[scoreCount];
      if (displayScore != DISPLAY_OVERRIDE_BLANK_SCORE) {
        byte numDigits = MagnitudeOfScore(displayScore);
        if (numDigits == 0) numDigits = 1;
        if (numDigits < (RPU_OS_NUM_DIGITS - 1) && (ScoreOverrideStatus & (0x01 << scoreCount))) {
          // This score is going to be animated (back and forth)
          if (overrideAnimationSeed != LastTimeOverrideAnimated) {
            updateLastTimeAnimated = true;
            byte shiftDigits = (overrideAnimationSeed) % (((RPU_OS_NUM_DIGITS + 1) - numDigits) + ((RPU_OS_NUM_DIGITS - 1) - numDigits));
            if (shiftDigits >= ((RPU_OS_NUM_DIGITS + 1) - numDigits)) shiftDigits = (RPU_OS_NUM_DIGITS - numDigits) * 2 - shiftDigits;
            byte digitCount;
            displayMask = GetDisplayMask(numDigits);
            for (digitCount = 0; digitCount < shiftDigits; digitCount++) {
              displayScore *= 10;
              displayMask = displayMask >> 1;
            }
            RPU_SetDisplayBlank(scoreCount, 0x00);
            RPU_SetDisplay(scoreCount, displayScore, false);
            RPU_SetDisplayBlank(scoreCount, displayMask);
          }
        } else {
          RPU_SetDisplay(scoreCount, displayScore, true, 1);
        }
      } else {
        RPU_SetDisplayBlank(scoreCount, 0);
      }

    } else {
#endif
#ifdef USE_SCORE_ACHIEVEMENTS
      boolean showingCurrentAchievement = false;
#endif      
      // No override, update scores designated by displayToUpdate
      if (allScoresShowValue == 0) {
        displayScore = CurrentScores[scoreCount];
#ifdef USE_SCORE_ACHIEVEMENTS
        displayScore += (CurrentAchievements[scoreCount]%10);
        if (CurrentAchievements[scoreCount]) showingCurrentAchievement = true;
#endif 
      }
      else displayScore = allScoresShowValue;

      // If we're updating all displays, or the one currently matching the loop, or if we have to scroll
      if (displayToUpdate == 0xFF || displayToUpdate == scoreCount || displayScore > RPU_OS_MAX_DISPLAY_SCORE || showingCurrentAchievement) {

        // Don't show this score if it's not a current player score (even if it's scrollable)
        if (displayToUpdate == 0xFF && (scoreCount >= CurrentNumPlayers && CurrentNumPlayers != 0) && allScoresShowValue == 0) {
          RPU_SetDisplayBlank(scoreCount, 0x00);
          continue;
        }

        if (displayScore > RPU_OS_MAX_DISPLAY_SCORE) {
          // Score needs to be scrolled 
          if ((CurrentTime - LastTimeScoreChanged) < 4000) {
            // show score for four seconds after change
            RPU_SetDisplay(scoreCount, displayScore % (RPU_OS_MAX_DISPLAY_SCORE + 1), false);
            byte blank = RPU_OS_ALL_DIGITS_MASK;
#ifdef USE_SCORE_ACHIEVEMENTS
            if (showingCurrentAchievement && (CurrentTime/200)%2) {
              blank &= ~(0x01<<(RPU_OS_NUM_DIGITS-1));
            }
#endif            
            RPU_SetDisplayBlank(scoreCount, blank);
          } else {   
            // Scores are scrolled 10 digits and then we wait for 6
            if (scrollPhase < 11 && scrollPhaseChanged) {
              byte numDigits = MagnitudeOfScore(displayScore);

              // Figure out top part of score
              unsigned long tempScore = displayScore;
              if (scrollPhase < RPU_OS_NUM_DIGITS) {
                displayMask = RPU_OS_ALL_DIGITS_MASK;
                for (byte scrollCount = 0; scrollCount < scrollPhase; scrollCount++) {
                  displayScore = (displayScore % (RPU_OS_MAX_DISPLAY_SCORE + 1)) * 10;
                  displayMask = displayMask >> 1;
                }
              } else {
                displayScore = 0;
                displayMask = 0x00;
              }

              // Add in lower part of score
              if ((numDigits + scrollPhase) > 10) {
                byte numDigitsNeeded = (numDigits + scrollPhase) - 10;
                for (byte scrollCount = 0; scrollCount < (numDigits - numDigitsNeeded); scrollCount++) {
                  tempScore /= 10;
                }
                displayMask |= GetDisplayMask(MagnitudeOfScore(tempScore));
                displayScore += tempScore;
              }
              RPU_SetDisplayBlank(scoreCount, displayMask);
              RPU_SetDisplay(scoreCount, displayScore);
            }
          }
        } else {
          if (flashCurrent && displayToUpdate == scoreCount) {
            unsigned long flashSeed = CurrentTime / 250;
            if (flashSeed != LastFlashOrDash) {
              LastFlashOrDash = flashSeed;
              if (((CurrentTime / 250) % 2) == 0) RPU_SetDisplayBlank(scoreCount, 0x00);
              else RPU_SetDisplay(scoreCount, displayScore, true, 2);
            }
          } else if (dashCurrent && displayToUpdate == scoreCount) {
#ifdef RPU_SIMPLIFY_DISPLAY_FOR_7VOLUTION
            unsigned long dashSeed = (CurrentTime / 250)%4;
            if (dashSeed != LastFlashOrDash) {
              LastFlashOrDash = dashSeed;
              if (dashSeed) {
                RPU_SetDisplay(scoreCount, displayScore, true, 2);
              } else {
                RPU_SetDisplayBlank(scoreCount, 0x00);
              }
            }
#else            
            unsigned long dashSeed = CurrentTime / 50;
            if (dashSeed != LastFlashOrDash) {
              LastFlashOrDash = dashSeed;
              byte dashPhase = (CurrentTime / 60) % 36;
              byte numDigits = MagnitudeOfScore(displayScore);
              if (dashPhase < 12) {
                displayMask = GetDisplayMask((numDigits == 0) ? 2 : numDigits);
                if (dashPhase < 7) {
                  for (byte maskCount = 0; maskCount < dashPhase; maskCount++) {
                    displayMask &= ~(0x01 << maskCount);
                  }
                } else {
                  for (byte maskCount = 12; maskCount > dashPhase; maskCount--) {
                    displayMask &= ~(0x20 >> (maskCount - dashPhase - 1));
                  }
                }
                RPU_SetDisplay(scoreCount, displayScore);
                RPU_SetDisplayBlank(scoreCount, displayMask);
              } else {
                RPU_SetDisplay(scoreCount, displayScore, true, 2);
              }
            }
#endif            
          } else {
#ifdef USE_SCORE_ACHIEVEMENTS
            byte blank;
            blank = RPU_SetDisplay(scoreCount, displayScore, false, 2);
            if (showingCurrentAchievement && (CurrentTime/200)%2) {
              blank &= ~(0x01<<(RPU_OS_NUM_DIGITS-1));
            }
            RPU_SetDisplayBlank(scoreCount, blank);
#else
            RPU_SetDisplay(scoreCount, displayScore, true, 2);
#endif
          }
        }
      } // End if this display should be updated
#ifdef USE_SCORE_OVERRIDES
    } // End on non-overridden
#endif
  } // End loop on scores

  if (updateLastTimeAnimated) {
    LastTimeOverrideAnimated = overrideAnimationSeed;
  }

}

void ShowFlybyValue(byte numToShow, unsigned long timeBase) {
  byte shiftDigits = (CurrentTime - timeBase) / 120;
  byte rightSideBlank = 0;

  unsigned long bigVersionOfNum = (unsigned long)numToShow;
  for (byte count = 0; count < shiftDigits; count++) {
    bigVersionOfNum *= 10;
    rightSideBlank /= 2;
    if (count > 2) rightSideBlank |= 0x20;
  }
  bigVersionOfNum /= 1000;

  byte curMask = RPU_SetDisplay(CurrentPlayer, bigVersionOfNum, false, 0);
  if (bigVersionOfNum == 0) curMask = 0;
  RPU_SetDisplayBlank(CurrentPlayer, ~(~curMask | rightSideBlank));
}

/*

  XXdddddd---
           10
          100
         1000
        10000
       10x000
      10xx000
     10xxx000
    10xxxx000
   10xxxxx000
  10xxxxxx000
*/

////////////////////////////////////////////////////////////////////////////
//
//  Machine State Helper functions
//
////////////////////////////////////////////////////////////////////////////
boolean AddPlayer(boolean resetNumPlayers = false) {

  if (Credits < 1 && !FreePlayMode) return false;
  if (resetNumPlayers) CurrentNumPlayers = 0;
  if (CurrentNumPlayers >= 4) return false;

  CurrentNumPlayers += 1;
  RPU_SetDisplay(CurrentNumPlayers - 1, 0, true, 2);
//  RPU_SetDisplayBlank(CurrentNumPlayers - 1, 0x30);

  if (!FreePlayMode) {
    Credits -= 1;
    RPU_WriteByteToEEProm(RPU_CREDITS_EEPROM_BYTE, Credits);
    RPU_SetDisplayCredits(Credits, !FreePlayMode);
    RPU_SetCoinLockout(false);
  }
  QueueNotification(SOUND_EFFECT_VP_ADD_PLAYER_1 + (CurrentNumPlayers - 1), 0);

  RPU_WriteULToEEProm(RPU_TOTAL_PLAYS_EEPROM_START_BYTE, RPU_ReadULFromEEProm(RPU_TOTAL_PLAYS_EEPROM_START_BYTE) + 1);

  return true;
}

byte SwitchToChuteNum(byte switchHit) {
  byte chuteNum = 0;
  if (switchHit==SW_COIN_2) chuteNum = 1;
  else if (switchHit==SW_COIN_3) chuteNum = 2;
  return chuteNum;   
}

unsigned short ChuteAuditByte[] = {RPU_CHUTE_1_COINS_START_BYTE, RPU_CHUTE_2_COINS_START_BYTE, RPU_CHUTE_3_COINS_START_BYTE};
void AddCoinToAudit(byte chuteNum) {
  if (chuteNum>2) return;
  unsigned short coinAuditStartByte = ChuteAuditByte[chuteNum];
  RPU_WriteULToEEProm(coinAuditStartByte, RPU_ReadULFromEEProm(coinAuditStartByte) + 1);
}


void AddCredit(boolean playSound = false, byte numToAdd = 1) {
  if (Credits < MaximumCredits) {
    Credits += numToAdd;
    if (Credits > MaximumCredits) Credits = MaximumCredits;
    RPU_WriteByteToEEProm(RPU_CREDITS_EEPROM_BYTE, Credits);
    if (playSound) {
      PlaySoundEffect(SOUND_EFFECT_ADD_CREDIT_1, ConvertVolumeSettingToGain(SoundEffectsVolume));
    }
    RPU_SetDisplayCredits(Credits, !FreePlayMode);
    RPU_SetCoinLockout(false);
  } else {
    RPU_SetDisplayCredits(Credits, !FreePlayMode);
    RPU_SetCoinLockout(true);
  }

}


boolean AddCoin(byte chuteNum) {
  boolean creditAdded = false;
  if (chuteNum>2) return false;

  byte cpcSelection = GetCPCSelection(chuteNum);

  // Find the lowest chute num with the same ratio selection
  // and use that ChuteCoinsInProgress counter
  byte chuteNumToUse;
  for (chuteNumToUse=0; chuteNumToUse<=chuteNum; chuteNumToUse++) {
    if (GetCPCSelection(chuteNumToUse)==cpcSelection) break;
  }

  PlaySoundEffect(SOUND_EFFECT_ADD_CREDIT_1+CurrentTime%3, ConvertVolumeSettingToGain(SoundEffectsVolume));

  byte cpcCoins = GetCPCCoins(cpcSelection);
  byte cpcCredits = GetCPCCredits(cpcSelection);
  byte coinProgressBefore = ChuteCoinsInProgress[chuteNumToUse];
  ChuteCoinsInProgress[chuteNumToUse] += 1;

  if (ChuteCoinsInProgress[chuteNumToUse]==cpcCoins) {
    if (cpcCredits>cpcCoins) AddCredit(false, cpcCredits - (coinProgressBefore));
    else AddCredit(false, cpcCredits);
    ChuteCoinsInProgress[chuteNumToUse] = 0;
    creditAdded = true;
  } else {
    if (cpcCredits>cpcCoins) {
      AddCredit(false, 1);
      creditAdded = true;
    } else {
    }
  }

  return creditAdded;
}


/*
void AddCredit(boolean playSound = false, byte numToAdd = 1) {
  if (Credits < MaximumCredits) {
    Credits += numToAdd;
    if (Credits > MaximumCredits) Credits = MaximumCredits;
    RPU_WriteByteToEEProm(RPU_CREDITS_EEPROM_BYTE, Credits);
    if (playSound) PlaySoundEffect(SOUND_EFFECT_ADD_CREDIT_1 + CurrentTime%3, ConvertVolumeSettingToGain(SoundEffectsVolume));
    RPU_SetDisplayCredits(Credits, !FreePlayMode);
    RPU_SetCoinLockout(false);
  } else {
    RPU_SetDisplayCredits(Credits, !FreePlayMode);
    RPU_SetCoinLockout(true);
  }

}
*/


void AddSpecialCredit() {
  AddCredit(false, 1);
  RPU_PushToTimedSolenoidStack(SOL_KNOCKER, 3, CurrentTime, true);
  RPU_WriteULToEEProm(RPU_TOTAL_REPLAYS_EEPROM_START_BYTE, RPU_ReadULFromEEProm(RPU_TOTAL_REPLAYS_EEPROM_START_BYTE) + 1);
}

void AwardSpecial() {
  if (SpecialCollected) return;
  SpecialCollected = true;
  if (TournamentScoring) {
    CurrentScores[CurrentPlayer] += ScoreMultiplier * SpecialValue;
  } else {
    AddSpecialCredit();
  }
}

void AwardExtraBall() {
  if (ExtraBallCollected) return;
  ExtraBallCollected = true;
  if (TournamentScoring) {
    CurrentScores[CurrentPlayer] += ScoreMultiplier * ExtraBallValue;
  } else {
    SamePlayerShootsAgain = true;
    RPU_SetLampState(LAMP_SHOOT_AGAIN, SamePlayerShootsAgain);
//    StopAudio();
    PlaySoundEffect(SOUND_EFFECT_EXTRA_BALL, ConvertVolumeSettingToGain(SoundEffectsVolume));
  }
}




////////////////////////////////////////////////////////////////////////////
//
//  Audio Output functions
//
////////////////////////////////////////////////////////////////////////////
int VolumeToGainConversion[] = {-70, -18, -16, -14, -12, -10, -8, -6, -4, -2, 0};
int ConvertVolumeSettingToGain(byte volumeSetting) {
  if (volumeSetting==0) return -70;
  if (volumeSetting>10) return 0;
  return VolumeToGainConversion[volumeSetting];
}

#define VOICE_NOTIFICATION_STACK_SIZE   10
#define VOICE_NOTIFICATION_STACK_EMPTY  0xFFFF
byte VoiceNotificationStackFirst;
byte VoiceNotificationStackLast;
unsigned int VoiceNotificationNumStack[VOICE_NOTIFICATION_STACK_SIZE];


#if defined(RPU_OS_USE_WAV_TRIGGER) || defined(RPU_OS_USE_WAV_TRIGGER_1p3)
unsigned short CurrentBackgroundSong = SOUND_EFFECT_NONE;
#endif

void StopAudio() {
#if defined(RPU_OS_USE_WAV_TRIGGER) || defined(RPU_OS_USE_WAV_TRIGGER_1p3)
  wTrig.stopAllTracks();
  CurrentBackgroundSong = SOUND_EFFECT_NONE;
#endif
  VoiceNotificationStackFirst = 0;
  VoiceNotificationStackLast = 0;

}


void PlayBackgroundSong(unsigned int songNum, unsigned long backgroundSongNumSeconds = 0) {

#if defined(RPU_OS_USE_WAV_TRIGGER) || defined(RPU_OS_USE_WAV_TRIGGER_1p3)
  if (MusicLevel > 2) {
    if (CurrentBackgroundSong != songNum) {
      if (CurrentBackgroundSong != SOUND_EFFECT_NONE) wTrig.trackStop(CurrentBackgroundSong);

      if (DEBUG_MESSAGES) {
        char buf[128];
        sprintf(buf, "Stopping song %d to play song %d\n", CurrentBackgroundSong, songNum);
        Serial.write(buf);
      }
      
      if (songNum != SOUND_EFFECT_NONE) {
#ifdef RPU_OS_USE_WAV_TRIGGER_1p3
        wTrig.trackPlayPoly(songNum, true);
#else
        wTrig.trackPlayPoly(songNum);
#endif
        wTrig.trackLoop(songNum, true);
        wTrig.trackGain(songNum, ConvertVolumeSettingToGain(MusicVolume));
      }
      CurrentBackgroundSong = songNum;

      if (backgroundSongNumSeconds!=0) {
        BackgroundSongEndTime = CurrentTime + (unsigned long)(backgroundSongNumSeconds)*1000;
      } else {
        BackgroundSongEndTime = 0;
      }

    }
  }
#else
  byte test = songNum;
  songNum = test;
#endif

  if (DEBUG_MESSAGES) {
    char buf[129];
    sprintf(buf, "Song # %d\n", songNum);
    Serial.write(buf);
  }

}


#ifdef RPU_OS_USE_SB300
#define SOUND_QUEUE_SIZE 100
#define SB300_SOUND_FUNCTION_SQUARE_WAVE  0
#define SB300_SOUND_FUNCTION_ANALOG       1
struct SoundQueueEntry {
  byte soundFunction;
  byte soundRegister;
  byte soundByte;
  unsigned long playTime;
};
SoundQueueEntry SoundQueue[SOUND_QUEUE_SIZE];

void ClearSoundQueue() {
  for (int count=0; count<SOUND_QUEUE_SIZE; count++) {
    SoundQueue[count].playTime = 0;
  }
}

boolean AddToSoundQueue(byte soundFunction, byte soundRegister, byte soundByte, unsigned long playTime) {
  for (int count=0; count<SOUND_QUEUE_SIZE; count++) {
    if (SoundQueue[count].playTime==0) {
      SoundQueue[count].soundFunction = soundFunction;
      SoundQueue[count].soundRegister = soundRegister;
      SoundQueue[count].soundByte = soundByte;
      SoundQueue[count].playTime = playTime;
      return true;
    }
  }
  return false;
}

byte GetFromSoundQueue(unsigned long pullTime) {
  for (int count=0; count<SOUND_QUEUE_SIZE; count++) {
    if (SoundQueue[count].playTime!=0 && SoundQueue[count].playTime<pullTime) {
      byte retSound = SoundQueue[count].soundByte;
      SoundQueue[count].playTime = 0;
      return retSound;
    }
  }

  return 0xFF;
}


boolean ServiceSoundQueue(unsigned long pullTime) {
  boolean soundCommandSent = false;
  for (int count=0; count<SOUND_QUEUE_SIZE; count++) {
    if (SoundQueue[count].playTime!=0 && SoundQueue[count].playTime<pullTime) {
      if (SoundQueue[count].soundFunction==SB300_SOUND_FUNCTION_SQUARE_WAVE) {
        RPU_PlaySB300SquareWave(SoundQueue[count].soundRegister, SoundQueue[count].soundByte);   
      } else if (SoundQueue[count].soundFunction==SB300_SOUND_FUNCTION_ANALOG) {
        RPU_PlaySB300Analog(SoundQueue[count].soundRegister, SoundQueue[count].soundByte);   
      }
      SoundQueue[count].playTime = 0;
      soundCommandSent = true;
    }
  }

  return soundCommandSent;
}

#endif

int SpaceLeftOnNotificationStack() {
  if (VoiceNotificationStackFirst>=VOICE_NOTIFICATION_STACK_SIZE || VoiceNotificationStackLast>=VOICE_NOTIFICATION_STACK_SIZE) return 0;
  if (VoiceNotificationStackLast>=VoiceNotificationStackFirst) return ((VOICE_NOTIFICATION_STACK_SIZE-1) - (VoiceNotificationStackLast-VoiceNotificationStackFirst));
  return (VoiceNotificationStackFirst - VoiceNotificationStackLast) - 1;
}


void PushToNotificationStack(unsigned int notification) {
  // If the switch stack last index is out of range, then it's an error - return
  if (SpaceLeftOnNotificationStack()==0) return;

  VoiceNotificationNumStack[VoiceNotificationStackLast] = notification;
  
  VoiceNotificationStackLast += 1;
  if (VoiceNotificationStackLast==VOICE_NOTIFICATION_STACK_SIZE) {
    // If the end index is off the end, then wrap
    VoiceNotificationStackLast = 0;
  }
}


unsigned int PullFirstFromVoiceNotificationStack() {
  // If first and last are equal, there's nothing on the stack
  if (VoiceNotificationStackFirst==VoiceNotificationStackLast) return VOICE_NOTIFICATION_STACK_EMPTY;

  unsigned int retVal = VoiceNotificationNumStack[VoiceNotificationStackFirst];

  VoiceNotificationStackFirst += 1;
  if (VoiceNotificationStackFirst>=VOICE_NOTIFICATION_STACK_SIZE) VoiceNotificationStackFirst = 0;

  return retVal;
}



void QueueNotification(unsigned int soundEffectNum, byte priority) {

  if (soundEffectNum<SOUND_EFFECT_VP_VOICE_NOTIFICATIONS_START || soundEffectNum>=(SOUND_EFFECT_VP_VOICE_NOTIFICATIONS_START+NUM_VOICE_NOTIFICATIONS)) return;

  // If there's nothing playing, we can play it now
  if (NextVoiceNotificationPlayTime==0) {
    if (CurrentBackgroundSong!=SOUND_EFFECT_NONE) {
      wTrig.trackFade(CurrentBackgroundSong, ConvertVolumeSettingToGain(MusicVolume)-20, 500, 0);
    }
    NextVoiceNotificationPlayTime = CurrentTime + (unsigned long)(VoiceNotificationDurations[soundEffectNum-SOUND_EFFECT_VP_VOICE_NOTIFICATIONS_START])*1000;
    PlaySoundEffect(soundEffectNum, ConvertVolumeSettingToGain(CalloutsVolume));
  } else {
    if (priority==0) {
      PushToNotificationStack(soundEffectNum);
    }
  }
}


void ServiceNotificationQueue() {

  if (NextVoiceNotificationPlayTime!=0 && CurrentTime>NextVoiceNotificationPlayTime) {
    // Current notification done, see if there's another
    unsigned int nextNotification = PullFirstFromVoiceNotificationStack();
    if (nextNotification!=VOICE_NOTIFICATION_STACK_EMPTY) {
      if (CurrentBackgroundSong!=SOUND_EFFECT_NONE) {
        wTrig.trackFade(CurrentBackgroundSong, ConvertVolumeSettingToGain(MusicVolume)-20, 500, 0);
      }
      NextVoiceNotificationPlayTime = CurrentTime + (unsigned long)(VoiceNotificationDurations[nextNotification-SOUND_EFFECT_VP_VOICE_NOTIFICATIONS_START])*1000;
      PlaySoundEffect(nextNotification, ConvertVolumeSettingToGain(CalloutsVolume));
    } else {
      // No more notifications -- set the volume back up and clear the variable
      if (CurrentBackgroundSong!=SOUND_EFFECT_NONE) {
          wTrig.trackFade(CurrentBackgroundSong, ConvertVolumeSettingToGain(MusicVolume), 1500, 0);
      }
      NextVoiceNotificationPlayTime = 0;
    }    
  }
  
}




unsigned long NextSoundEffectTime = 0;

void PlaySoundEffect(unsigned int soundEffectNum, int gain) {

  if (MusicLevel == 0) return;

#if defined(RPU_OS_USE_WAV_TRIGGER) || defined(RPU_OS_USE_WAV_TRIGGER_1p3)

#ifndef RPU_OS_USE_WAV_TRIGGER_1p3
  if (  soundEffectNum == SOUND_EFFECT_BUMPER_HIT ||
        soundEffectNum == SOUND_EFFECT_REFUELING_SPINNER ) wTrig.trackStop(soundEffectNum);
#endif
  wTrig.trackPlayPoly(soundEffectNum);
  wTrig.trackGain(soundEffectNum, gain);
  //  char buf[128];
  //  sprintf(buf, "s=%d\n", soundEffectNum);
  //  Serial.write(buf);
#endif

#ifdef RPU_OS_USE_SB300
  switch (soundEffectNum) {
    case SOUND_EFFECT_VP_PLAYER_1_UP:
      AddToSoundQueue(SB300_SOUND_FUNCTION_SQUARE_WAVE, 1, 0x12, CurrentTime);
      AddToSoundQueue(SB300_SOUND_FUNCTION_SQUARE_WAVE, 0, 0x92, CurrentTime);
      AddToSoundQueue(SB300_SOUND_FUNCTION_SQUARE_WAVE, 6, 0x80, CurrentTime);
      AddToSoundQueue(SB300_SOUND_FUNCTION_SQUARE_WAVE, 7, 0x00, CurrentTime);
      AddToSoundQueue(SB300_SOUND_FUNCTION_ANALOG, 0, 0xAA, CurrentTime);
      AddToSoundQueue(SB300_SOUND_FUNCTION_ANALOG, 0, 0x8A, CurrentTime+50);
      AddToSoundQueue(SB300_SOUND_FUNCTION_ANALOG, 0, 0x6A, CurrentTime+100);
      AddToSoundQueue(SB300_SOUND_FUNCTION_ANALOG, 0, 0x4A, CurrentTime+150);
      AddToSoundQueue(SB300_SOUND_FUNCTION_ANALOG, 0, 0x2A, CurrentTime+250);
      AddToSoundQueue(SB300_SOUND_FUNCTION_ANALOG, 0, 0x0A, CurrentTime+300);
      break;
  }
#endif

  if (DEBUG_MESSAGES) {
    char buf[129];
    sprintf(buf, "Sound # %d\n", soundEffectNum);
    Serial.write(buf);
  }

}

inline void StopSoundEffect(byte soundEffectNum) {
#if defined(RPU_OS_USE_WAV_TRIGGER) || defined(RPU_OS_USE_WAV_TRIGGER_1p3)
  wTrig.trackStop(soundEffectNum);
#else
  if (DEBUG_MESSAGES) {
    char buf[129];
    sprintf(buf, "Sound # %d\n", soundEffectNum);
    Serial.write(buf);
  }
#endif
}


////////////////////////////////////////////////////////////////////////////
//
//  Self test, audit, adjustments mode
//
////////////////////////////////////////////////////////////////////////////

#define ADJ_TYPE_LIST                 1
#define ADJ_TYPE_MIN_MAX              2
#define ADJ_TYPE_MIN_MAX_DEFAULT      3
#define ADJ_TYPE_SCORE                4
#define ADJ_TYPE_SCORE_WITH_DEFAULT   5
#define ADJ_TYPE_SCORE_NO_DEFAULT     6
byte AdjustmentType = 0;
byte NumAdjustmentValues = 0;
byte AdjustmentValues[8];
unsigned long AdjustmentScore;
byte *CurrentAdjustmentByte = NULL;
unsigned long *CurrentAdjustmentUL = NULL;
byte CurrentAdjustmentStorageByte = 0;
byte TempValue = 0;


int RunSelfTest(int curState, boolean curStateChanged) {
  int returnState = curState;
  CurrentNumPlayers = 0;

  if (curStateChanged) {
    // Send a stop-all command and reset the sample-rate offset, in case we have
    //  reset while the WAV Trigger was already playing.
    StopAudio();
    PlaySoundEffect(MachineStateToAdjustmentPrompt[0-(curState+1)], 0);
    SoundSettingTimeout = 0;
  } else {
    if (SoundSettingTimeout && CurrentTime>SoundSettingTimeout) {
      SoundSettingTimeout = 0;
      StopAudio();
    }
  }

  // Any state that's greater than CHUTE_3 is handled by the Base Self-test code
  // Any that's less, is machine specific, so we handle it here.
  if (curState >= MACHINE_STATE_TEST_DONE) {
//    byte cpcSelection = 0xFF;
//    byte chuteNum = 0xFF;
//    if (curState==MACHINE_STATE_ADJUST_CPC_CHUTE_1) chuteNum = 0;
//    if (curState==MACHINE_STATE_ADJUST_CPC_CHUTE_2) chuteNum = 1;
//    if (curState==MACHINE_STATE_ADJUST_CPC_CHUTE_3) chuteNum = 2;
//    if (chuteNum!=0xFF) cpcSelection = GetCPCSelection(chuteNum);
    returnState = RunBaseSelfTest(returnState, curStateChanged, CurrentTime, SW_CREDIT_RESET, SW_SLAM);
//    if (chuteNum!=0xFF) {
//      if (cpcSelection != GetCPCSelection(chuteNum)) {
//        byte newCPC = GetCPCSelection(chuteNum);
//        Audio.StopAllAudio();
//        Audio.PlaySound(SOUND_EFFECT_SELF_TEST_CPC_START+newCPC, AUDIO_PLAY_TYPE_WAV_TRIGGER, 10);
//      }
//    }  
  } else {    
    byte curSwitch = RPU_PullFirstFromSwitchStack();

    if (curSwitch == SW_SELF_TEST_SWITCH && (CurrentTime - GetLastSelfTestChangedTime()) > 250) {
      SetLastSelfTestChangedTime(CurrentTime);
      returnState -= 1;
    }

    if (curSwitch == SW_SLAM) {
      returnState = MACHINE_STATE_ATTRACT;
    }

    if (curStateChanged) {
      for (int count = 0; count < 4; count++) {
        RPU_SetDisplay(count, 0);
        RPU_SetDisplayBlank(count, 0x00);
      }
      RPU_SetDisplayCredits(MACHINE_STATE_TEST_SOUNDS - curState);
      RPU_SetDisplayBallInPlay(0, false);
      CurrentAdjustmentByte = NULL;
      CurrentAdjustmentUL = NULL;
      CurrentAdjustmentStorageByte = 0;

      AdjustmentType = ADJ_TYPE_MIN_MAX;
      AdjustmentValues[0] = 0;
      AdjustmentValues[1] = 1;
      TempValue = 0;

      switch (curState) {
        case MACHINE_STATE_ADJUST_FREEPLAY:
          CurrentAdjustmentByte = (byte *)&FreePlayMode;
          CurrentAdjustmentStorageByte = EEPROM_FREE_PLAY_BYTE;
          break;
        case MACHINE_STATE_ADJUST_BALL_SAVE:
          AdjustmentType = ADJ_TYPE_LIST;
          NumAdjustmentValues = 5;
          AdjustmentValues[1] = 5;
          AdjustmentValues[2] = 10;
          AdjustmentValues[3] = 15;
          AdjustmentValues[4] = 20;
          CurrentAdjustmentByte = &BallSaveNumSeconds;
          CurrentAdjustmentStorageByte = EEPROM_BALL_SAVE_BYTE;
          break;
        case MACHINE_STATE_ADJUST_SFX_AND_SOUNDTRACK:
          AdjustmentType = ADJ_TYPE_MIN_MAX;
          AdjustmentValues[1] = 4;
          CurrentAdjustmentByte = &MusicLevel;
          CurrentAdjustmentStorageByte = EEPROM_MUSIC_LEVEL_BYTE;
          break;
        
        case MACHINE_STATE_ADJUST_MUSIC_VOLUME:
          AdjustmentType = ADJ_TYPE_MIN_MAX;
          AdjustmentValues[0] = 1;
          AdjustmentValues[1] = 10;
          CurrentAdjustmentByte = &MusicVolume;
          CurrentAdjustmentStorageByte = EEPROM_MUSIC_VOLUME_BYTE;
          break;
        case MACHINE_STATE_ADJUST_SFX_VOLUME:
          AdjustmentType = ADJ_TYPE_MIN_MAX;
          AdjustmentValues[0] = 1;
          AdjustmentValues[1] = 10;
          CurrentAdjustmentByte = &SoundEffectsVolume;
          CurrentAdjustmentStorageByte = EEPROM_SFX_VOLUME_BYTE;
          break;
        case MACHINE_STATE_ADJUST_CALLOUTS_VOLUME:
          AdjustmentType = ADJ_TYPE_MIN_MAX;
          AdjustmentValues[0] = 1;
          AdjustmentValues[1] = 10;
          CurrentAdjustmentByte = &CalloutsVolume;
          CurrentAdjustmentStorageByte = EEPROM_CALLOUTS_VOLUME_BYTE;
          break;
        
        case MACHINE_STATE_ADJUST_TOURNAMENT_SCORING:
          CurrentAdjustmentByte = (byte *)&TournamentScoring;
          CurrentAdjustmentStorageByte = EEPROM_TOURNAMENT_SCORING_BYTE;
          break;
        case MACHINE_STATE_ADJUST_TILT_WARNING:
          AdjustmentValues[1] = 2;
          CurrentAdjustmentByte = &MaxTiltWarnings;
          CurrentAdjustmentStorageByte = EEPROM_TILT_WARNING_BYTE;
          break;
        case MACHINE_STATE_ADJUST_AWARD_OVERRIDE:
          AdjustmentType = ADJ_TYPE_MIN_MAX_DEFAULT;
          AdjustmentValues[1] = 7;
          CurrentAdjustmentByte = &ScoreAwardReplay;
          CurrentAdjustmentStorageByte = EEPROM_AWARD_OVERRIDE_BYTE;
          break;
        case MACHINE_STATE_ADJUST_BALLS_OVERRIDE:
          AdjustmentType = ADJ_TYPE_LIST;
          NumAdjustmentValues = 2;
          AdjustmentValues[0] = 3;
          AdjustmentValues[1] = 5;
          CurrentAdjustmentByte = &BallsPerGame;
          CurrentAdjustmentStorageByte = EEPROM_BALLS_OVERRIDE_BYTE;
          break;
        case MACHINE_STATE_ADJUST_SCROLLING_SCORES:
          CurrentAdjustmentByte = (byte *)&ScrollingScores;
          CurrentAdjustmentStorageByte = EEPROM_SCROLLING_SCORES_BYTE;
          break;

        case MACHINE_STATE_ADJUST_EXTRA_BALL_AWARD:
          AdjustmentType = ADJ_TYPE_SCORE_WITH_DEFAULT;
          CurrentAdjustmentUL = &ExtraBallValue;
          CurrentAdjustmentStorageByte = EEPROM_EXTRA_BALL_SCORE_BYTE;
          break;

        case MACHINE_STATE_ADJUST_SPECIAL_AWARD:
          AdjustmentType = ADJ_TYPE_SCORE_WITH_DEFAULT;
          CurrentAdjustmentUL = &SpecialValue;
          CurrentAdjustmentStorageByte = EEPROM_SPECIAL_SCORE_BYTE;
          break;

        case MACHINE_STATE_ADJUST_DIM_LEVEL:
          AdjustmentType = ADJ_TYPE_LIST;
          NumAdjustmentValues = 2;
          AdjustmentValues[0] = 2;
          AdjustmentValues[1] = 3;
          CurrentAdjustmentByte = &DimLevel;
          CurrentAdjustmentStorageByte = EEPROM_DIM_LEVEL_BYTE;
          //          for (int count = 0; count < 7; count++) RPU_SetLampState(MIDDLE_ROCKET_7K + count, 1, 1);
          break;

        case MACHINE_STATE_ADJUST_EXTRA_BALL_RANK:
          AdjustmentType = ADJ_TYPE_MIN_MAX;
          AdjustmentValues[0] = 1;
          AdjustmentValues[1] = 10;
          CurrentAdjustmentByte = &RankForExtraBallLight;
          CurrentAdjustmentStorageByte = EEPROM_RANK_FOR_EB_BYTE;
          break;

        case MACHINE_STATE_ADJUST_SPECIAL_RANK:
          AdjustmentType = ADJ_TYPE_MIN_MAX;
          AdjustmentValues[0] = 1;
          AdjustmentValues[1] = 10;
          CurrentAdjustmentByte = &RankForSpecialLight;
          CurrentAdjustmentStorageByte = EEPROM_RANK_FOR_SPECIAL_BYTE;
          break;

        case MACHINE_STATE_ADJUST_SUN_MISSION_RANK:
          AdjustmentType = ADJ_TYPE_MIN_MAX;
          AdjustmentValues[0] = 1;
          AdjustmentValues[1] = 10;
          CurrentAdjustmentByte = &RankForSunMission;
          CurrentAdjustmentStorageByte = EEPROM_RANK_FOR_SUN_BYTE;
          break;

        case MACHINE_STATE_AJDUST_SIDE_QUEST_START:
          AdjustmentType = ADJ_TYPE_MIN_MAX;
          AdjustmentValues[0] = 0;
          AdjustmentValues[1] = 3;
          CurrentAdjustmentByte = &SideQuestStartSwitches;
          CurrentAdjustmentStorageByte = EEPROM_SIDE_QUEST_START_BYTE;
          break;

        case MACHINE_STATE_ADJUST_GALAXY_BALL_SAVE:
          AdjustmentValues[1] = 10;
          CurrentAdjustmentByte = &GalaxyKickerBallSave;
          CurrentAdjustmentStorageByte = EEPROM_GALAXY_BALLSAVE_BYTE;
          break;          

        case MACHINE_STATE_ADJUST_SAVE_PROGRESS:
          AdjustmentValues[1] = 1;
          CurrentAdjustmentByte = (byte *)&SaveMissionProgress;
          CurrentAdjustmentStorageByte = EEPROM_SAVE_PROGRESS_BYTE;
          break;          

        case MACHINE_STATE_ADJUST_DONE:
          returnState = MACHINE_STATE_ATTRACT;
          break;
      }

    }

    // Change value, if the switch is hit
    if (curSwitch == SW_CREDIT_RESET) {

      if (CurrentAdjustmentByte && (AdjustmentType == ADJ_TYPE_MIN_MAX || AdjustmentType == ADJ_TYPE_MIN_MAX_DEFAULT)) {
        byte curVal = *CurrentAdjustmentByte;
        curVal += 1;
        if (curVal > AdjustmentValues[1]) {
          if (AdjustmentType == ADJ_TYPE_MIN_MAX) curVal = AdjustmentValues[0];
          else {
            if (curVal > 99) curVal = AdjustmentValues[0];
            else curVal = 99;
          }
        }
        *CurrentAdjustmentByte = curVal;
        if (CurrentAdjustmentStorageByte) EEPROM.write(CurrentAdjustmentStorageByte, curVal);
        
        if (curState==MACHINE_STATE_ADJUST_SFX_AND_SOUNDTRACK) {
          StopAudio();
          PlaySoundEffect(SOUND_EFFECT_SELF_TEST_AUDIO_OPTIONS_START+curVal, 0);
          if (curVal>=3) SoundtrackSelection = curVal-3;
        } else if (curState==MACHINE_STATE_ADJUST_MUSIC_VOLUME) {
          if (SoundSettingTimeout) StopAudio();
          PlayBackgroundSong(MusicIndices[SoundtrackSelection][4] + RALLY_MUSIC_WAITING_FOR_SKILLSHOT);
          SoundSettingTimeout = CurrentTime+5000;
        } else if (curState==MACHINE_STATE_ADJUST_SFX_VOLUME) {
          if (SoundSettingTimeout) StopAudio();
          PlaySoundEffect(SOUND_EFFECT_GALAXY_LETTER_AWARD, ConvertVolumeSettingToGain(SoundEffectsVolume));
          SoundSettingTimeout = CurrentTime+5000;
        } else if (curState==MACHINE_STATE_ADJUST_CALLOUTS_VOLUME) {
          if (SoundSettingTimeout) StopAudio();
          PlaySoundEffect(SOUND_EFFECT_VP_SKILL_SHOT_1, ConvertVolumeSettingToGain(CalloutsVolume));
          SoundSettingTimeout = CurrentTime+3000;
        }
      } else if (CurrentAdjustmentByte && AdjustmentType == ADJ_TYPE_LIST) {
        byte valCount = 0;
        byte curVal = *CurrentAdjustmentByte;
        byte newIndex = 0;
        for (valCount = 0; valCount < (NumAdjustmentValues - 1); valCount++) {
          if (curVal == AdjustmentValues[valCount]) newIndex = valCount + 1;
        }
        *CurrentAdjustmentByte = AdjustmentValues[newIndex];
        if (CurrentAdjustmentStorageByte) EEPROM.write(CurrentAdjustmentStorageByte, AdjustmentValues[newIndex]);
      } else if (CurrentAdjustmentUL && (AdjustmentType == ADJ_TYPE_SCORE_WITH_DEFAULT || AdjustmentType == ADJ_TYPE_SCORE_NO_DEFAULT)) {
        unsigned long curVal = *CurrentAdjustmentUL;
        curVal += 5000;
        if (curVal > 100000) curVal = 0;
        if (AdjustmentType == ADJ_TYPE_SCORE_NO_DEFAULT && curVal == 0) curVal = 5000;
        *CurrentAdjustmentUL = curVal;
        if (CurrentAdjustmentStorageByte) RPU_WriteULToEEProm(CurrentAdjustmentStorageByte, curVal);
      }

      if (curState == MACHINE_STATE_ADJUST_DIM_LEVEL) {
        RPU_SetDimDivisor(1, DimLevel);
      }
    }

    // Show current value
    if (CurrentAdjustmentByte != NULL) {
      RPU_SetDisplay(0, (unsigned long)(*CurrentAdjustmentByte), true);
    } else if (CurrentAdjustmentUL != NULL) {
      RPU_SetDisplay(0, (*CurrentAdjustmentUL), true);
    }

  }

  if (curState == MACHINE_STATE_ADJUST_DIM_LEVEL) {
    //    for (int count = 0; count < 7; count++) RPU_SetLampState(MIDDLE_ROCKET_7K + count, 1, (CurrentTime / 1000) % 2);
  }

  if (returnState == MACHINE_STATE_ATTRACT) {
    // If any variables have been set to non-override (99), return
    // them to dip switch settings
    // Balls Per Game, Player Loses On Ties, Novelty Scoring, Award Score
    //    DecodeDIPSwitchParameters();
    RPU_SetDisplayCredits(Credits, !FreePlayMode);
    ReadStoredParameters();
  }

  return returnState;
}



////////////////////////////////////////////////////////////////////////////
//
//  Attract Mode
//
////////////////////////////////////////////////////////////////////////////

unsigned long AttractLastLadderTime = 0;
byte AttractLastLadderBonus = 0;
unsigned long AttractDisplayRampStart = 0;
byte AttractLastHeadMode = 255;
byte AttractLastPlayfieldMode = 255;
byte InAttractMode = false;


int RunAttractMode(int curState, boolean curStateChanged) {

  int returnState = curState;

  if (curStateChanged) {
    RPU_DisableSolenoidStack();
    RPU_TurnOffAllLamps();
    RPU_SetDisableFlippers(true);
    if (DEBUG_MESSAGES) {
      Serial.write("Entering Attract Mode\n\r");
    }
    AttractLastHeadMode = 0;
    AttractLastPlayfieldMode = 0;
    RPU_SetDisplayCredits(Credits, !FreePlayMode);
  }

  // Alternate displays between high score and blank
  if (CurrentTime < 16000) {
    if (AttractLastHeadMode != 1) {
      ShowPlayerScores(0xFF, false, false);
      RPU_SetDisplayCredits(Credits, !FreePlayMode);
      RPU_SetDisplayBallInPlay(0, true);
    }
  } else if ((CurrentTime / 8000) % 2 == 0) {

    if (AttractLastHeadMode != 2) {
      RPU_SetLampState(LAMP_HEAD_HIGH_SCORE, 1, 0, 250);
      RPU_SetLampState(LAMP_HEAD_GAME_OVER, 0);
      LastTimeScoreChanged = CurrentTime;
    }
    AttractLastHeadMode = 2;
    ShowPlayerScores(0xFF, false, false, HighScore);
  } else {
    if (AttractLastHeadMode != 3) {
      if (CurrentTime < 32000) {
        for (int count = 0; count < 4; count++) {
          CurrentScores[count] = 0;
        }
        CurrentNumPlayers = 0;
      }
      RPU_SetLampState(LAMP_HEAD_HIGH_SCORE, 0);
      RPU_SetLampState(LAMP_HEAD_GAME_OVER, 1);
      LastTimeScoreChanged = CurrentTime;
    }
    ShowPlayerScores(0xFF, false, false);

    AttractLastHeadMode = 3;
  }

  byte attractPlayfieldPhase = ((CurrentTime / 5000) % 5);

  if (attractPlayfieldPhase != AttractLastPlayfieldMode) {
    RPU_TurnOffAllLamps();
    AttractLastPlayfieldMode = attractPlayfieldPhase;
    if (attractPlayfieldPhase == 2) GameMode = GAME_MODE_SKILL_SHOT;
    else GameMode = GAME_MODE_UNSTRUCTURED_PLAY;
    AttractLastLadderBonus = 1;
    AttractLastLadderTime = CurrentTime;
  }

  if (attractPlayfieldPhase == 0) {
    ShowLampAnimation(3, 40, CurrentTime, 14, false, false);
  } else if (attractPlayfieldPhase == 1) {
    ShowLampAnimation(4, 40, CurrentTime, 11, false, false);
  } else if (attractPlayfieldPhase == 3) {
    ShowLampAnimation(0, 40, CurrentTime, 11, false, false, 27);
  } else if (attractPlayfieldPhase == 2) {
    ShowLampAnimation(1, 40, CurrentTime, 11, false, false);
  } else {
    ShowLampAnimation(2, 40, CurrentTime, 14, false, false);
  }

  byte switchHit;
  while ( (switchHit = RPU_PullFirstFromSwitchStack()) != SWITCH_STACK_EMPTY ) {
    if (switchHit == SW_CREDIT_RESET) {
      if (AddPlayer(true)) returnState = MACHINE_STATE_INIT_GAMEPLAY;
    }
    if (switchHit == SW_COIN_1 || switchHit == SW_COIN_2 || switchHit == SW_COIN_3) {
      AddCoinToAudit(SwitchToChuteNum(switchHit));
      AddCoin(SwitchToChuteNum(switchHit));
    }
    if (switchHit == SW_SELF_TEST_SWITCH && (CurrentTime - GetLastSelfTestChangedTime()) > 250) {
      returnState = MACHINE_STATE_TEST_LAMPS;
      SetLastSelfTestChangedTime(CurrentTime);
    }
  }

  return returnState;
}





////////////////////////////////////////////////////////////////////////////
//
//  Game Play functions
//
////////////////////////////////////////////////////////////////////////////
/*byte CountBits(byte byteToBeCounted) {
  byte numBits = 0;

  for (byte count = 0; count < 8; count++) {
    numBits += (byteToBeCounted & 0x01);
    byteToBeCounted = byteToBeCounted >> 1;
  }

  return numBits;
}
*/

byte CountBits(unsigned int intToBeCounted) {
  byte numBits = 0;

  for (byte count = 0; count < 16; count++) {
    numBits += (intToBeCounted & 0x01);
    intToBeCounted = intToBeCounted >> 1;
  }

  return numBits;
}



void AddToBonus(byte amountToAdd = 1) {
  Bonus[CurrentPlayer] += amountToAdd;
  if (Bonus[CurrentPlayer] >= MAX_DISPLAY_BONUS) {
    Bonus[CurrentPlayer] = MAX_DISPLAY_BONUS;
  }
}


void SetGameMode(byte newGameMode) {
  GameMode = newGameMode | (GameMode & ~GAME_BASE_MODE);
  GameModeStartTime = 0;
  GameModeEndTime = 0;
  if (DEBUG_MESSAGES) {
    char buf[129];
    sprintf(buf, "Game mode set to %d\n", newGameMode);
    Serial.write(buf);
  }
}

void StartScoreAnimation(unsigned long scoreToAnimate, boolean playTick=true) {
  if (ScoreAdditionAnimation != 0) {
    CurrentScores[CurrentPlayer] += ScoreAdditionAnimation;
  }
  ScoreAdditionAnimation = scoreToAnimate;
  ScoreAdditionAnimationStartTime = CurrentTime;
  LastRemainingAnimatedScoreShown = 0;
  PlayScoreAnimationTick = playTick;
}






void IncreaseBonusX(byte numLevelsIncrease = 1) {
  boolean soundPlayed = false;
  if (BonusX[CurrentPlayer] < 9) {
    BonusX[CurrentPlayer] += numLevelsIncrease;
    BonusXAnimationStart = CurrentTime;
  }

  if (!soundPlayed) {
//    PlaySoundEffect(SOUND_EFFECT_BONUS_X_INCREASED, ConvertVolumeSettingToGain(SoundEffectsVolume));
  }

}

byte IncreasePlayerRank(byte numRanks=1) {  
  PlayerRank[CurrentPlayer] += numRanks;
  if (PlayerRank[CurrentPlayer]>9) PlayerRank[CurrentPlayer] = 9;
  CurrentAchievements[CurrentPlayer] += numRanks;
  if (CurrentAchievements[CurrentPlayer]>9) CurrentAchievements[CurrentPlayer] = 9;

  if (PlayerRank[CurrentPlayer]==RankForExtraBallLight) ExtraBallLit[CurrentPlayer] = 1 + CurrentTime%2;
  if (PlayerRank[CurrentPlayer]==RankForSpecialLight) SpecialLit[CurrentPlayer] = 1 + CurrentTime%2;
  if (PlayerRank[CurrentPlayer]==RankForSunMission) {
    return GAME_MODE_WIZARD_QUALIFIED;
  }
  else return GAME_MODE_UNSTRUCTURED_PLAY;
}


boolean WaitForBallToReturn;

int InitGamePlay(boolean curStateChanged) {

  if (DEBUG_MESSAGES) {
    Serial.write("Starting game\n\r");
  }

  if (curStateChanged) {
    WaitForBallToReturn = false;
    // The start button has been hit only once to get
    // us into this mode, so we assume a 1-player game
    // at the moment
    RPU_EnableSolenoidStack();
    RPU_SetCoinLockout((Credits >= MaximumCredits) ? true : false);
    RPU_TurnOffAllLamps();
    StopAudio();
  
    // Reset displays & game state variables
    for (int count = 0; count < 4; count++) {
      // Initialize game-specific variables
      BonusX[count] = 1;
      Bonus[count] = 1;
      MissionsCompleted[count] = 0;
      GalaxyTurnaroundLights[count] = 0;
      GalaxyStatus[count] = 0;
      PlayerRank[count] = 0; 
      CurrentAchievements[count] = 0;
      SpinnerHits[count] = 0;
      DropTargetClears[count] = 0;
      SideQuestsQualified[count] = 0;
      ExtraBallLit[count] = 0;
      SpecialLit[count] = 0;
      ResumeGameMode[count] = GAME_MODE_UNSTRUCTURED_PLAY;
      ResumeFuel[count] = 0;
      ResumeMissionNum[count] = 0;
    }
    memset(CurrentScores, 0, 4 * sizeof(unsigned long));

    LastSaucerHitTime = CurrentTime;
    FeatureShotLastHit = 0;
    SamePlayerShootsAgain = false;
    CurrentBallInPlay = 1;
    CurrentNumPlayers = 1;
    CurrentPlayer = 0;
    ShowPlayerScores(0xFF, false, false);
    if (RPU_ReadSingleSwitchState(SW_GALAXY_TURNAROUND)) {
      WaitForBallToReturn = true;
      RPU_PushToSolenoidStack(SOL_GALAXY_KICKER, 5);      
    }
    if (RPU_ReadSingleSwitchState(SW_SAUCER)) {
      WaitForBallToReturn = true;
      RPU_PushToSolenoidStack(SOL_SAUCER, 5);      
    }
    SetGeneralIllumination(true);
  }

  if (WaitForBallToReturn) {
    if (RPU_ReadSingleSwitchState(SW_OUTHOLE)==0) return MACHINE_STATE_INIT_GAMEPLAY;
    WaitForBallToReturn = false;
  }

  return MACHINE_STATE_INIT_NEW_BALL;
}


int InitNewBall(bool curStateChanged, byte playerNum, int ballNum) {

  // If we're coming into this mode for the first time
  // then we have to do everything to set up the new ball
  if (curStateChanged) {
    RPU_TurnOffAllLamps();
    SetGeneralIllumination(false);
    BallFirstSwitchHitTime = 0;
    BallSaveEndTime = 0;

    RPU_SetDisableFlippers(false);
    RPU_EnableSolenoidStack();
    RPU_SetDisplayCredits(Credits, !FreePlayMode);
    if (CurrentNumPlayers > 1 && (ballNum != 1 || playerNum != 0) && !SamePlayerShootsAgain) QueueNotification(SOUND_EFFECT_VP_PLAYER_1_UP + playerNum, 0);
    SamePlayerShootsAgain = false;

    RPU_SetDisplayBallInPlay(ballNum);
    RPU_SetLampState(LAMP_HEAD_TILT, 0);

    CurrentBallSaveNumSeconds = BallSaveNumSeconds;
    if (BallSaveNumSeconds > 0) {
      RPU_SetLampState(LAMP_SHOOT_AGAIN, 1, 0, 500);
    }

    BallSaveUsed = false;
    BallTimeInTrough = 0;
    NumTiltWarnings = 0;
    LastTiltWarningTime = 0;

    // Initialize game-specific start-of-ball lights & variables
    GameModeStartTime = 0;
    GameModeEndTime = 0;
    GameMode = GAME_MODE_SKILL_SHOT;

    ExtraBallCollected = false;
    SpecialCollected = false;
    MidlaneStatus = 0;

    // Start appropriate mode music
    if (RPU_ReadSingleSwitchState(SW_OUTHOLE)) {
      RPU_PushToTimedSolenoidStack(SOL_OUTHOLE, 4, CurrentTime + 600);
    }

    // Reset progress unless holdover awards
    Bonus[CurrentPlayer] = 1;
    BonusXAnimationStart = 0;

    OutlaneStatus = 0;
    ScoreMultiplier = 1;
    ScoreAdditionAnimation = 0;
    ScoreAdditionAnimationStartTime = 0;
    LastSpinnerHit = 0;
    SpinnerAscension = 0;
    NextMission = GetNextAvailableMission(0);
    CurrentSaucerValue = 2;
    SaucerHitAnimationEnd = 0;
    SaucerHitLampNum = 0;

    StandupsHitThisBall = 0;
    SpinnerGoalReachedTime = 0;
    GalaxyStatusResetTime = 0;
    DropTargetGoalReachedTime = 0;
    StandupGoalReachedTime = 0;
    SideQuestQualifiedEndTime = 0;
    SideQuestEndTime = 0;
    SideQuestQualifiedReminderPlayed = false;
    SaucerValueDecreaseTime = 0;
    SideQuestNumTopPops = 0;
//    ResumeGameMode[CurrentPlayer] = GAME_MODE_UNSTRUCTURED_PLAY;
    
    for (int count=0; count<6; count++) {
      GalaxyLetterAnimationEnd[count] = 0;
    }

    RPU_PushToTimedSolenoidStack(SOL_DROP_TARGET_RESET, 12, CurrentTime + 200);
    ResetDropTargetStatusTime = CurrentTime + 300;    

#ifdef RPU_OS_USE_SB300
    ClearSoundQueue();
#endif
    BackgroundSongStartTime = 0;
    BackgroundSongEndTime = 0;
    if (CurrentBallInPlay==1) {
      PlayBackgroundSong(MusicIndices[SoundtrackSelection][4] + RALLY_MUSIC_WAITING_FOR_SKILLSHOT);
    } else {
      if (CurrentNumPlayers==1) {
        // shot song is based on score
        if (CurrentScores[CurrentPlayer]>200000) PlayBackgroundSong(MusicIndices[SoundtrackSelection][4] + RALLY_MUSIC_WAITING_FOR_SKILLSHOT_UPBEAT);
        else if (CurrentScores[CurrentPlayer]>100000) PlayBackgroundSong(MusicIndices[SoundtrackSelection][4] + RALLY_MUSIC_WAITING_FOR_SKILLSHOT_MEDIUM);
        else PlayBackgroundSong(MusicIndices[SoundtrackSelection][4] + RALLY_MUSIC_WAITING_FOR_SKILLSHOT_CALM);
      } else {
        unsigned long bestScore = 0, worstScore=0x7FFFFFFF;
        byte bestPlayer = 0, worstPlayer = 0;
        for (byte count=0; count<4; count++) {
          if (CurrentScores[count]>bestScore) {
            bestScore = CurrentScores[count];
            bestPlayer = count;
          }
          if (CurrentScores[count]<worstScore) {
            worstScore = CurrentScores[count];
            worstPlayer = count;
          }
        }
        if (bestPlayer==CurrentPlayer) PlayBackgroundSong(MusicIndices[SoundtrackSelection][4] + RALLY_MUSIC_WAITING_FOR_SKILLSHOT_UPBEAT);
        else if (worstPlayer==CurrentPlayer) PlayBackgroundSong(MusicIndices[SoundtrackSelection][4] + RALLY_MUSIC_WAITING_FOR_SKILLSHOT_CALM);
        else PlayBackgroundSong(MusicIndices[SoundtrackSelection][4] + RALLY_MUSIC_WAITING_FOR_SKILLSHOT_MEDIUM);
      }
    }
    CurrentMusicType = 0;
    NextVoiceNotificationPlayTime = 0;
  }

  // We should only consider the ball initialized when
  // the ball is no longer triggering the SW_OUTHOLE
  if (RPU_ReadSingleSwitchState(SW_OUTHOLE)) {
    return MACHINE_STATE_INIT_NEW_BALL;
  } else {
    return MACHINE_STATE_NORMAL_GAMEPLAY;
  }

}



void StartBackgroundMusic(int musicType) {
  PlayBackgroundSong(SOUND_EFFECT_RADIO_TRANSITION + (CurrentTime%5));
  BackgroundSongStartTime = CurrentTime + ((CurrentTime%3)*1000 + 2000);
  CurrentMusicType = musicType;
}


void StartNextSong(byte musicType) {
  unsigned int baseSong = MusicIndices[SoundtrackSelection][musicType];
  unsigned int numSongs = MusicNumEntries[SoundtrackSelection][musicType];
  unsigned int retSong = baseSong + (CurrentTime%numSongs);
  boolean songRecentlyPlayed = false;
  if (DEBUG_MESSAGES) {
    char buf[129];
    sprintf(buf, "Want to play song %d\n", retSong);
    Serial.write(buf);
  }

  unsigned int songCount = 0;
  for (songCount=0; songCount<numSongs; songCount++) {
    for (byte count=0; count<NUMBER_OF_SONGS_REMEMBERED; count++) {
      if (LastSongsPlayed[count]==retSong) {
        songRecentlyPlayed = true;
        if (DEBUG_MESSAGES) {
          char buf[129];
          sprintf(buf, "Song # %d has been recently played\n", retSong);
          Serial.write(buf);
        }
        
        break;
      }
    }
    if (!songRecentlyPlayed) break;
    retSong = (retSong+1);
    songRecentlyPlayed = false;
    if (retSong>=(baseSong+numSongs)) retSong = baseSong;
    if (DEBUG_MESSAGES) {
      char buf[129];
      sprintf(buf, "Incrementing song to %d\n", retSong);
      Serial.write(buf);
    }
  }

//  if (songCount==numSongs) retSong = baseSong;

  if (DEBUG_MESSAGES) {
    char buf[129];
    sprintf(buf, "Chose song %d in the end\n", retSong);
    Serial.write(buf);
  }

  // Record this song in the array
  for (byte count=(NUMBER_OF_SONGS_REMEMBERED-1); count>0; count--) LastSongsPlayed[count] = LastSongsPlayed[count-1];
  LastSongsPlayed[0] = retSong;

  if (DEBUG_MESSAGES) {
    char buf[129];
    for (byte count=0; count<NUMBER_OF_SONGS_REMEMBERED; count++) {
      sprintf(buf, "Last song array [%d] now is %d\n", count, LastSongsPlayed[count]);
      Serial.write(buf);
    }
  }

  unsigned int songIndex = retSong - baseSong;
  BackgroundSongStartTime = 0;

  if (DEBUG_MESSAGES) {
    char buf[128];
    sprintf(buf, "Playing song %d for %d seconds\n", retSong, MusicLengths[SoundtrackSelection][musicType][songIndex]);
    Serial.write(buf);
  }
  PlayBackgroundSong(retSong, (unsigned long)MusicLengths[SoundtrackSelection][musicType][songIndex]);
}

void ManageBackgroundSong() {
  if (BackgroundSongStartTime!=0 && CurrentTime>BackgroundSongStartTime) {
    StartNextSong(CurrentMusicType);
  } else if (BackgroundSongEndTime!=0 && CurrentTime>BackgroundSongEndTime) {
    StartBackgroundMusic(CurrentMusicType);
  }
}

void SetBallSave(unsigned long numberOfMilliseconds) {
  BallSaveEndTime = CurrentTime + numberOfMilliseconds;
  BallSaveUsed = false;
  if (CurrentBallSaveNumSeconds==0)  CurrentBallSaveNumSeconds = 2;
}



byte GalaxyLampCount = 0;
boolean DisplaysNeedResetting;
unsigned long LastGalaxyLightShownTime = 0;
unsigned long LastTimeThroughLoop = 0;
unsigned long TimerTicks = 0;

// This function manages all timers, flags, and lights
int ManageGameMode() {
  int returnState = MACHINE_STATE_NORMAL_GAMEPLAY;

  if (ResetDropTargetStatusTime && CurrentTime>ResetDropTargetStatusTime) {
    DropTargetStatus = GetDropTargetStatus();
    DTBankHitOrder = 0;
    ResetDropTargetStatusTime = 0;
  }

  boolean specialAnimationRunning = false;
  ShowGalaxyTurnaroundDraw = false;

  if (LastSpinnerHit!=0 && (CurrentTime-LastSpinnerHit)>2000) {
    SpinnerAscension = 0;
  }

  // If there's no side quest running, player can qualify one
  if (SideQuestEndTime==0) {
    // Cargo collection side quest
    if (DropTargetClears[CurrentPlayer]>=DROP_TARGET_CLEARS_FOR_CARGO) {
      DropTargetClears[CurrentPlayer] = 0;
      SideQuestsQualified[CurrentPlayer] |= GAME_MODE_FLAG_CARGO_COLLECTION;
      DropTargetGoalReachedTime = CurrentTime;
      QueueNotification(SOUND_EFFECT_VP_CARGO_ADDED, 0);
      StartScoreAnimation(SIDE_QUEST_QUALIFY_AWARD * ScoreMultiplier * ((unsigned long)CountBits(GameMode & 0xF0)));
      if (SideQuestQualifiedEndTime) SideQuestQualifiedEndTime += SIDE_QUEST_QUALIFY_TIME;
      SideQuestQualifiedReminderPlayed = false;
    }
  
    // Pirate encounter side quest
    if (StandupsHitThisBall==0x0E) {
      StandupsHitThisBall = 0;
      SideQuestsQualified[CurrentPlayer] |= GAME_MODE_FLAG_PIRATE_ENCOUNTER;
      StandupGoalReachedTime = CurrentTime;
      QueueNotification(SOUND_EFFECT_VP_PIRATE_ENCOUNTER, 0);
      StartScoreAnimation(SIDE_QUEST_QUALIFY_AWARD * ScoreMultiplier * ((unsigned long)CountBits(GameMode & 0xF0)));
      if (SideQuestQualifiedEndTime) SideQuestQualifiedEndTime += SIDE_QUEST_QUALIFY_TIME;
    }
  
    if (SpinnerHits[CurrentPlayer]>SPINS_FOR_REFUEL) {
      SpinnerHits[CurrentPlayer] = 0;
      SideQuestsQualified[CurrentPlayer] |= GAME_MODE_FLAG_REFUEL;
      SpinnerGoalReachedTime = CurrentTime;
      QueueNotification(SOUND_EFFECT_VP_REFUEL_STARTED, 0);
      StartScoreAnimation(SIDE_QUEST_QUALIFY_AWARD * ScoreMultiplier * ((unsigned long)CountBits(GameMode & 0xF0)));
      if (SideQuestQualifiedEndTime) SideQuestQualifiedEndTime += SIDE_QUEST_QUALIFY_TIME;
    }
  
    if (GalaxyStatus[CurrentPlayer]==0x3F) {
      GalaxyStatus[CurrentPlayer] = 0;    
      SideQuestsQualified[CurrentPlayer] |= GAME_MODE_FLAG_PASSENGER_ABOARD;
      GalaxyStatusResetTime = CurrentTime;
      QueueNotification(SOUND_EFFECT_VP_PASSEGER_ADDED, 0);
      StartScoreAnimation(SIDE_QUEST_QUALIFY_AWARD * ScoreMultiplier * ((unsigned long)CountBits(GameMode & 0xF0)));
      if (SideQuestQualifiedEndTime) SideQuestQualifiedEndTime += SIDE_QUEST_QUALIFY_TIME;
    }
  }

  if ((CurrentTime-LastSwitchHitTime)>3000) TimersPaused = true;
  else TimersPaused = false;

  // Manage the background song
  ManageBackgroundSong();

  // Manage notifications
  ServiceNotificationQueue();

  //  byte currentWizardTimer;

  unsigned long currentTimer;
  int currentCount;
  boolean currentFlag;

  switch ( (GameMode & GAME_BASE_MODE) ) {
    case GAME_MODE_SKILL_SHOT:
      if (GameModeStartTime == 0) {
        GameModeStartTime = CurrentTime;
        GameModeEndTime = 0;
        LastGameplayPrompt = 0;
      }

      if (LastGameplayPrompt==0) {
        // Wait a couple of seconds before first prompt
        if ((CurrentTime-GameModeStartTime)>2000) {
          LastGameplayPrompt = CurrentTime;
          QueueNotification(SOUND_EFFECT_VP_PLUNGE_PROMPT_1+CurrentTime%4, 0);
        }
      } else if ((CurrentTime-LastGameplayPrompt)>15000) {
        LastGameplayPrompt = CurrentTime;
        QueueNotification(SOUND_EFFECT_VP_PLUNGE_PROMPT_1+CurrentTime%4, 0);
      }

      if (BallFirstSwitchHitTime != 0) {
        if (SaveMissionProgress) {
          SetGameMode(ResumeGameMode[CurrentPlayer]);
          if (ResumeGameMode[CurrentPlayer]!=GAME_MODE_UNSTRUCTURED_PLAY) {
            NextMission = ResumeMissionNum[CurrentPlayer];
            QueueNotification(SOUND_EFFECT_VP_MERCURY_MISSION_START+NextMission, 0);
            if (ResumeGameMode[CurrentPlayer]==GAME_MODE_MISSION_FUELING) {
              CurrentFuel = ResumeFuel[CurrentPlayer];
            }
          }
        } else {
          SetGameMode(GAME_MODE_UNSTRUCTURED_PLAY);
        }
        CurrentSaucerValue = 2;
      }

      if (GameModeEndTime != 0 && CurrentTime > GameModeEndTime) {
        ShowPlayerScores(0xFF, false, false);
      }
      break;


    case GAME_MODE_UNSTRUCTURED_PLAY:
      // If this is the first time in this mode
      if (GameModeStartTime == 0) {
        SetGeneralIllumination(true);
        GameModeStartTime = CurrentTime;
        CurrentFuel = 0;
        ResumeFuel[CurrentPlayer] = 0;
        StartBackgroundMusic(MUSIC_TYPE_UNSTRUCTURED_PLAY);
        OutlaneStatus = 0;
        ShowGalaxyTurnaroundDraw = false;
        DisplaysNeedResetting = false;
        ResumeGameMode[CurrentPlayer] = GAME_MODE_UNSTRUCTURED_PLAY;
      }

      if (SideQuestQualifiedEndTime==0 && SideQuestsQualified[CurrentPlayer]) {
        SideQuestQualifiedEndTime = CurrentTime + SIDE_QUEST_QUALIFY_TIME;
        SideQuestQualifiedReminderPlayed = false;
        SideQuestNumTopPops = 0;
      } else if (SideQuestQualifiedEndTime!=0) {
        if (CurrentTime>SideQuestQualifiedEndTime) {
          SideQuestsQualified[CurrentPlayer] = 0;
          SideQuestQualifiedEndTime = 0; 
        } else if ( !SideQuestQualifiedReminderPlayed && (SideQuestQualifiedEndTime-CurrentTime)<(SIDE_QUEST_QUALIFY_TIME/2) ) {
          SideQuestQualifiedReminderPlayed = true;
          QueueNotification(SOUND_EFFECT_VP_SIDE_QUEST_REMINDER, 0);
        }
      } 

      if (!(SideQuestsQualified[CurrentPlayer]&GAME_MODE_FLAG_REFUEL) && !(GameMode&GAME_MODE_FLAG_REFUEL) && LastSpinnerHit!=0 && (CurrentTime-LastSpinnerHit)<5000) {
        for (byte count = 0; count < 4; count++) {
          if (count != CurrentPlayer) OverrideScoreDisplay(count, SPINS_FOR_REFUEL-SpinnerHits[CurrentPlayer], false);
        }
        DisplaysNeedResetting = true;
      } else {
        if (DisplaysNeedResetting) {
          DisplaysNeedResetting = false;
          ShowPlayerScores(0xFF, false, false);
        }
      }


      if (SideQuestEndTime) {
        for (byte count = 0; count < 4; count++) {
          if (count != CurrentPlayer) OverrideScoreDisplay(count, (SideQuestEndTime-CurrentTime)/1000, false);
        }

        if (CurrentTime>SideQuestEndTime) {
          unsigned long lastScoreMultiplier = ScoreMultiplier;
          ScoreMultiplier = 1;
          if (ScoreMultiplier!=lastScoreMultiplier) QueueNotification(SOUND_EFFECT_VP_1X_PLAYFIELD, 0);
          ShowPlayerScores(0xFF, false, false);
//          IncreasePlayerRank(CountBits(GameMode/16));
          IncreaseBonusX(CountBits(GameMode/16));
          SideQuestEndTime = 0;
          GameMode &= 0x0F;          
          PlaySoundEffect(SOUND_EFFECT_SIDE_QUESTS_OVER, ConvertVolumeSettingToGain(SoundEffectsVolume));
          StartBackgroundMusic(MUSIC_TYPE_UNSTRUCTURED_PLAY);
        }
      }
      
      break;

    case GAME_MODE_MISSION_QUALIFIED:
      if (GameModeStartTime==0) {
        GameModeStartTime = CurrentTime;
        GameModeEndTime = 0; 
        LastGalaxyLightShownTime = 0;
        GalaxyLampCount = 0;
        SetGeneralIllumination(false);

        // If a side quest was qualified but not started, end it now
        SideQuestQualifiedEndTime = 0;
        OutlaneStatus = 1;
        if ((MissionsCompleted[CurrentPlayer]&0x00FF)==0x00FF) {
          MissionsCompleted[CurrentPlayer] = 0x0000;
          PlayerRank[CurrentPlayer] = 0;
          CurrentAchievements[CurrentPlayer] = 0;
        }
      }

      if (GalaxyTurnaroundLights[CurrentPlayer]) {
        if (CurrentTime>(LastGalaxyLightShownTime+500)) {
          byte letterCount = 0;
          for (letterCount=0; letterCount<6; letterCount++) {
            if (GalaxyTurnaroundLights[CurrentPlayer]&(1<<letterCount)) {
              break;
            }
          }
          if (letterCount<6) {
            if (LastGalaxyLightShownTime!=0) NextMission = GetNextAvailableMission(NextMission+1);
            GalaxyTurnaroundLights[CurrentPlayer] &= ~(1<<letterCount);
            GalaxyLetterAnimationEnd[letterCount] = CurrentTime + 1000;
            LastGalaxyLightShownTime = CurrentTime + 1000;
            CurrentScores[CurrentPlayer] += 2000 * ((unsigned long)ScoreMultiplier);
            PlaySoundEffect(SOUND_EFFECT_GALAXY_LETTER_AWARD, ConvertVolumeSettingToGain(SoundEffectsVolume));
          } else {
            GameModeEndTime = CurrentTime;
          }
          
        } else if (CurrentTime>(LastGalaxyLightShownTime+100)) {
//          specialAnimationRunning = true;
//          ShowLampAnimation(0, 80, CurrentTime, 14, false, false);
        }
      } else {
        if (GameModeEndTime==0) GameModeEndTime = CurrentTime + 1000;
      }

      if (GameModeEndTime!=0 && CurrentTime>GameModeEndTime) {
        StartScoreAnimation(3000 * ScoreMultiplier);
        RPU_PushToSolenoidStack(SOL_GALAXY_KICKER, 5, true);
        if (GalaxyKickerBallSave) SetBallSave((unsigned long)GalaxyKickerBallSave * 1000);
        GalaxyTurnaroundEjectTime = CurrentTime;
        SetGameMode(GAME_MODE_MISSION_FUELING);
        QueueNotification(SOUND_EFFECT_VP_MERCURY_MISSION_START+NextMission, 0);
        ResumeMissionNum[CurrentPlayer] = NextMission;
      }
      break;


    case GAME_MODE_MISSION_FUELING:
      if (GameModeStartTime==0) {
        GameModeStartTime = CurrentTime;
        GameModeEndTime = 0;
        FuelRequired = FuelRequirements[NextMission];
        DisplaysNeedResetting = false;
        SetGeneralIllumination(true);
        if (CurrentFuel) {
          QueueNotification(SOUND_EFFECT_VP_MORE_FUEL_REQUIRED, 0);
        } else {
          QueueNotification(SOUND_EFFECT_VP_ACQUIRE_FUEL, 0);
        }
        ResumeGameMode[CurrentPlayer] = GAME_MODE_MISSION_FUELING;
      }

      currentTimer = (CurrentTime - GameModeStartTime)/2000;
      if (CurrentFuel < FuelRequired) {        
        if ((currentTimer%2)==0 || (LastSpinnerHit!=0 && (CurrentTime-LastSpinnerHit)<5000)) {
          for (byte count = 0; count < 4; count++) {
            if (count != CurrentPlayer) OverrideScoreDisplay(count, FuelRequired-CurrentFuel, false);
          }
          DisplaysNeedResetting = true;
        } else {
          if (DisplaysNeedResetting) {
            DisplaysNeedResetting = false;
            ShowPlayerScores(0xFF, false, false);
          }
        }
      } else {
        CurrentFuel = FuelRequired;
        if (DisplaysNeedResetting) {
          DisplaysNeedResetting = false;
          ShowPlayerScores(0xFF, false, false);
        }
        SetGameMode(GAME_MODE_MISSION_READY_TO_LAUNCH);  
      }
      ResumeFuel[CurrentPlayer] = CurrentFuel;
      
      break;
    case GAME_MODE_MISSION_READY_TO_LAUNCH:
      if (GameModeStartTime==0) {
        GameModeStartTime = CurrentTime;
        GameModeEndTime = 0;
        // startTargets should be based on rank
        byte startTargets = 0x01<<(CurrentTime%4);
        SetDropTargets(0x0F & ~startTargets);

        // Play the background sound of a rocket
        PlayBackgroundSong(SOUND_EFFECT_WAITING_FOR_LAUNCH);
        QueueNotification(SOUND_EFFECT_VP_REFULING_DONE, 0);
        PeriodicModeCheck = CurrentTime + 1000;
        ResumeGameMode[CurrentPlayer] = GAME_MODE_MISSION_READY_TO_LAUNCH;
      }

      currentTimer = ((CurrentTime - GameModeStartTime)/250)%40;
      if (currentTimer==3 || currentTimer==13) SetGeneralIllumination(false);
      else SetGeneralIllumination(true);

      // Periodically check to see of all DTs are down
      if (CurrentTime>PeriodicModeCheck) {
        byte currentStatus = GetDropTargetStatus();
        if (currentStatus==0x0F && ResetDropTargetStatusTime==0) {
          byte startTargets = 0x01<<(CurrentTime%4);
          SetDropTargets(0x0F & ~startTargets);
        }
        PeriodicModeCheck = CurrentTime + 1000;
      }
  
      // Mode will end when the drop target / stand - up target
      // goal has been reached
      specialAnimationRunning = true;
      ShowLampAnimation(3, 40, CurrentTime, 1, false, true);      
      break;

    case GAME_MODE_MISSION_LAUNCHING:
      if (GameModeStartTime==0) {
        SetGeneralIllumination(true);
        GameModeStartTime = CurrentTime;
        GameModeEndTime = CurrentTime + 3000;
        StartBackgroundMusic(MUSIC_TYPE_MISSION);
        PlaySoundEffect(SOUND_EFFECT_LAUNCH_ROCKET, ConvertVolumeSettingToGain(SoundEffectsVolume));
        ResumeGameMode[CurrentPlayer] = GAME_MODE_MISSION_LAUNCHING;
      }

      specialAnimationRunning = true;
      ShowLampAnimation(2, 63, CurrentTime, 12, false, false);

      if (CurrentTime>GameModeEndTime) {
        PopsForMultiplier = 0;
        MissionBonus = 0;

        // Set leg of mission based on mission
        SetGameMode(MissionStartStage[NextMission]);
      }
      break;

    case GAME_MODE_MISSION_FIRST_LEG:
      if (GameModeStartTime==0) {
        GameModeStartTime = CurrentTime;
        GameModeEndTime = CurrentTime + MISSION_FIRST_LEG_TIME;
        QueueNotification(SOUND_EFFECT_VP_POPS_FOR_PLAYFIELD_MULTI, 0);
//        ResumeGameMode[CurrentPlayer] = GAME_MODE_MISSION_FIRST_LEG;
      }
      
      for (byte count = 0; count < 4; count++) {
        if (count != CurrentPlayer) OverrideScoreDisplay(count, (GameModeEndTime-CurrentTime)/1000, false);
        else OverrideScoreDisplay(count, PopsForMultiplier, false);
      }

      specialAnimationRunning = true;
      ShowLampAnimation(5, 40, CurrentTime, 1, false, true);      

      if (CurrentTime>GameModeEndTime) {
        SetGameMode(GAME_MODE_MISSION_SECOND_LEG);
      }

      break;
      
    case GAME_MODE_MISSION_SECOND_LEG:
      if (GameModeStartTime==0) {
        ShowPlayerScores(0xFF, false, false);
        GameModeStartTime = CurrentTime;
        GameModeEndTime = 0;
        unsigned long lastScoreMultiplier = ScoreMultiplier;
        ScoreMultiplier = (PopsForMultiplier/10) + 1 + (unsigned long)CountBits(GameMode/16);
        if (ScoreMultiplier>6) ScoreMultiplier = 6;
        if (lastScoreMultiplier!=ScoreMultiplier) QueueNotification((SOUND_EFFECT_VP_1X_PLAYFIELD-1) + ScoreMultiplier, 0);
        TimerTicks = 0;
        MidlaneStatus = 1;
        DisplaysNeedResetting = false;
        QueueNotification(SOUND_EFFECT_VP_ENTERING_CRYO_SLEEP, 0);
//        ResumeGameMode[CurrentPlayer] = GAME_MODE_MISSION_SECOND_LEG;
      }

      if (!TimersPaused) {
        TimerTicks += (CurrentTime - LastTimeThroughLoop);
      }

      if (TimersPaused) {
        if (ScoreMultiplier>1) {
          for (byte count = 0; count < 4; count++) {
            if (count != CurrentPlayer) OverrideScoreDisplay(count, ScoreMultiplier, true);
          }
        }
        DisplaysNeedResetting = true;          
      } else {
        if (DisplaysNeedResetting) {
          DisplaysNeedResetting = false;          
          ShowPlayerScores(0xFF, false, false);
        }
        for (byte count = 0; count < 4; count++) {
          if (count != CurrentPlayer) OverrideScoreDisplay(count, (MISSION_SECOND_LEG_TIME-TimerTicks)/1000, false);
        }
      }

      if (TimerTicks>MISSION_SECOND_LEG_TIME) {
        ShowPlayerScores(0xFF, false, false);
        SetGameMode(GAME_MODE_MISSION_THIRD_LEG);
      }
      break;
    case GAME_MODE_MISSION_THIRD_LEG:
      if (GameModeStartTime==0) {
        ShowPlayerScores(0xFF, false, false);
        GameModeStartTime = CurrentTime;
        CurrentSaucerValue = 10;
        GameModeEndTime = CurrentTime + MISSION_THIRD_LEG_TIME;
        QueueNotification(SOUND_EFFECT_VP_BUILD_MISSION_BONUS_1 + MissionFeatureShot[NextMission], 0);
        AnnounceFinishShotTime = CurrentTime + 5000;
//        ResumeGameMode[CurrentPlayer] = GAME_MODE_MISSION_THIRD_LEG;
      }

      for (byte count = 0; count < 4; count++) {
        if (count != CurrentPlayer) {
          OverrideScoreDisplay(count, (GameModeEndTime-CurrentTime)/1000, false);
        } else {
          if (CurrentTime<(FeatureShotLastHit+2000)) {
            OverrideScoreDisplay(count, MissionBonus, false);
          }
        }
      }

      if (CurrentTime>AnnounceFinishShotTime) {
        QueueNotification(SOUND_EFFECT_VP_GALXAXY_TURNAROUND_TO_COMPLETE_MISSION + MissionFinishShot[NextMission], 0);        
        AnnounceFinishShotTime += 14000;
      }

      // For the last 5 seconds of Third leg, prompt player to 
      // finish mission
      if (CurrentTime>(GameModeEndTime-10000)) {
        switch(MissionFinishShot[NextMission]) {
          case MISSION_FINISH_SHOT_GALAXY_TURNAROUND:
            ShowGalaxyTurnaroundDraw = true;
            break;
          case MISSION_FINISH_SHOT_TOP_POP:
            specialAnimationRunning = true;
            ShowLampAnimation(4, 40, CurrentTime, 1, false, true);      
            break;
          case MISSION_FINISH_SHOT_STANDUPS:
            specialAnimationRunning = true;
            ShowLampAnimation(6, 40, CurrentTime, 1, false, true);      
            break;
        }
      }
      
      if (CurrentTime>GameModeEndTime) {
        AnnounceFinishShotTime = 0;
        CurrentSaucerValue = 2;
        unsigned long lastScoreMultiplier = ScoreMultiplier;
        ScoreMultiplier = (unsigned long)CountBits(GameMode/16);
        if (lastScoreMultiplier!=ScoreMultiplier) QueueNotification((SOUND_EFFECT_VP_1X_PLAYFIELD-1) + ScoreMultiplier, 0);
        ShowPlayerScores(0xFF, false, false);
        SetGameMode(GAME_MODE_MISSION_FINISH);
      }
      break;
    case GAME_MODE_MISSION_FINISH:
      if (GameModeStartTime==0) {
        ShowPlayerScores(0xFF, false, false);
        GameModeStartTime = CurrentTime;
        CurrentSaucerValue = 2;
        GameModeEndTime = CurrentTime + 4000;
        PlayBackgroundSong(SOUND_EFFECT_WAITING_FOR_LAUNCH);
        QueueNotification(SOUND_EFFECT_VP_COMING_IN_FOR_LANDING, 0);
      }
      specialAnimationRunning = true;
      ShowLampAnimation(2, 125, CurrentTime, 3, false, true);

      if (CurrentTime>GameModeEndTime) {
        StartScoreAnimation(((unsigned long)NextMission+1) * 5000 * ScoreMultiplier);
        unsigned long lastScoreMultiplier = ScoreMultiplier;
        ScoreMultiplier = 1 + (unsigned long)CountBits(GameMode/16);
        if (lastScoreMultiplier!=ScoreMultiplier) QueueNotification((SOUND_EFFECT_VP_1X_PLAYFIELD-1) + ScoreMultiplier, 0);
        SetGameMode(IncreasePlayerRank(1));
        IncreaseBonusX(1);
        if ((GameMode&GAME_BASE_MODE)!=GAME_MODE_WIZARD_QUALIFIED) PlaySoundEffect(SOUND_EFFECT_MISSION_COMPLETE, ConvertVolumeSettingToGain(SoundEffectsVolume));
        SetMissionComplete(NextMission);
        NextMission = GetNextAvailableMission(0);
      }
      break;
    case GAME_MODE_WAITING_FINISH_WITH_BONUS:
      if (GameModeStartTime==0) {
        ShowPlayerScores(0xFF, false, false);
        GameModeStartTime = CurrentTime;
        CurrentSaucerValue = 2;
        GameModeEndTime = CurrentTime + 2000;
        PlayBackgroundSong(SOUND_EFFECT_WAITING_FOR_LAUNCH);
        QueueNotification(SOUND_EFFECT_VP_SKILL_SHOT_2, 0);
      }
      specialAnimationRunning = true;
      ShowLampAnimation(2, 125, CurrentTime, 3, false, true);

      if (CurrentTime>GameModeEndTime) {
        StartScoreAnimation((((unsigned long)NextMission+1) * 5000 * ScoreMultiplier) + MissionBonus);
        unsigned long lastScoreMultiplier = ScoreMultiplier;
        ScoreMultiplier = 1 + (unsigned long)CountBits(GameMode/16);
        if (lastScoreMultiplier!=ScoreMultiplier) QueueNotification((SOUND_EFFECT_VP_1X_PLAYFIELD-1) + ScoreMultiplier, 0);
        SetGameMode(IncreasePlayerRank(1));
        IncreaseBonusX(1);
        if ((GameMode&GAME_BASE_MODE)!=GAME_MODE_WIZARD_QUALIFIED) PlaySoundEffect(SOUND_EFFECT_MISSION_COMPLETE, ConvertVolumeSettingToGain(SoundEffectsVolume));
        SetMissionComplete(NextMission);
        NextMission = GetNextAvailableMission(NextMission);
      }      
      break;
    case GAME_MODE_WIZARD_QUALIFIED:
      if (GameModeStartTime==0) {
        ShowPlayerScores(0xFF, false, false);
        GameModeStartTime = CurrentTime;
        GameModeEndTime = CurrentTime + 30000;
        PlayBackgroundSong(MusicIndices[SoundtrackSelection][4] + RALLY_MUSIC_WAITING_FOR_SKILLSHOT_UPBEAT);        
        QueueNotification(SOUND_EFFECT_VP_WAITING_FOR_LAUNCH_MISSION_TO_SUN, 0);
        AnnounceFinishShotTime = CurrentTime + 15000;
        NextMission = MISSION_TO_SUN;
        ResumeGameMode[CurrentPlayer] = GAME_MODE_WIZARD_QUALIFIED;
      }

      if (AnnounceFinishShotTime && CurrentTime>AnnounceFinishShotTime) {
        QueueNotification(SOUND_EFFECT_VP_WAITING_FOR_LAUNCH_MISSION_TO_SUN, 0);
        AnnounceFinishShotTime = 0;
      }

      if (CurrentTime>(GameModeEndTime-10000)) {
        for (byte count = 0; count < 4; count++) {
          if (count != CurrentPlayer) {
            OverrideScoreDisplay(count, (GameModeEndTime-CurrentTime)/1000, false);
          }
        }
      }
      
      if (CurrentTime>GameModeEndTime) {
        ShowPlayerScores(0xFF, false, false);
        AnnounceFinishShotTime = 0;
        QueueNotification(SOUND_EFFECT_VP_MISSION_WINDOW_CLOSED, 0);
        SetGameMode(GAME_MODE_UNSTRUCTURED_PLAY);
        NextMission = GetNextAvailableMission(0);
      }
      break;

    case GAME_MODE_WIZARD_START:
      if (GameModeStartTime==0) {
        ShowPlayerScores(0xFF, false, false);
        GameModeStartTime = CurrentTime;
        GameModeEndTime = CurrentTime + SUN_MISSION_LAUNCH_TOTAL_DURATION;
        StopAudio();
        PlaySoundEffect(SOUND_EFFECT_SUN_MISSION_LAUNCH, ConvertVolumeSettingToGain(SoundEffectsVolume));
      }

      currentFlag = false;
      for (currentCount=0; currentCount<SUN_MISSION_LAUNCH_GI_NUM_TOGGLES; currentCount++) {
        if (CurrentTime>(SunMissionLaunchGIToggleTimes[currentCount]+GameModeStartTime)) currentFlag = !currentFlag;
        else break;
      }
      SetGeneralIllumination(currentFlag);

      specialAnimationRunning = true;
      if (CurrentTime<(GameModeStartTime+3700)) {
        RPU_TurnOffAllLamps();
      } else if (CurrentTime<(GameModeStartTime+10700)) {
        byte sweepNum = ((CurrentTime-(GameModeStartTime+3700))/1000)%7;
        ShowLampAnimation(sweepNum, 63, CurrentTime-(GameModeStartTime+3700), 12, false, false);
      } else {
        byte lampPhase = ((CurrentTime-(GameModeStartTime+10700))/250)%4;
        if (lampPhase==0) RPU_TurnOffAllLamps();
        else ShowLampAnimation(2, 120, CurrentTime-(GameModeStartTime+10700), 12, false, false);
      }

      if (CurrentTime>(GameModeEndTime-10000)) {
        for (byte count = 0; count < 4; count++) {
          if (count != CurrentPlayer) {
            OverrideScoreDisplay(count, 1+((GameModeEndTime-CurrentTime)/1000), false);
          }
        }
      }
      
      if (CurrentTime>GameModeEndTime) {
        ShowPlayerScores(0xFF, false, false);
        SetGameMode(GAME_MODE_WIZARD);

        // Kick out the ball
        if (RPU_ReadSingleSwitchState(SW_GALAXY_TURNAROUND)) {
          RPU_PushToSolenoidStack(SOL_GALAXY_KICKER, 5);      
          if (GalaxyKickerBallSave) SetBallSave((unsigned long)GalaxyKickerBallSave * 1000);
        }
        if (RPU_ReadSingleSwitchState(SW_SAUCER)) {
          RPU_PushToSolenoidStack(SOL_SAUCER, 5);      
        }
      }
      break;
    case GAME_MODE_WIZARD:
      if (GameModeStartTime==0) {
        ShowPlayerScores(0xFF, false, false);
        StartBackgroundMusic(MUSIC_TYPE_WIZARD);
        GameModeStartTime = CurrentTime;
        GameModeEndTime = CurrentTime + 60000;
        StopAudio();
        PlaySoundEffect(SOUND_EFFECT_LAUNCH_ROCKET, ConvertVolumeSettingToGain(SoundEffectsVolume));
        WizardLastDropTargetSet = CurrentTime;
        WizardLastDropTargetUp = 0x01;
        SetDropTargets((~WizardLastDropTargetUp)&0x0F);
        WizardJackpotValue = 10000;
        WizardJackpotLastChanged = 0;
        AnnounceFinishShotTime = CurrentTime + 3000;
        DisplaysNeedResetting = false;
      }

      if (AnnounceFinishShotTime!=0 && CurrentTime>AnnounceFinishShotTime) {
        QueueNotification(SOUND_EFFECT_VP_BUILD_WIZARD_JACKPOT_WITH_DROPS, 0);
        AnnounceFinishShotTime = 0;
      }

      if (CurrentTime>(WizardLastDropTargetSet+3000)) {
        WizardLastDropTargetUp *= 2;
        if (WizardLastDropTargetUp>0x08) WizardLastDropTargetUp = 0x01;
        SetDropTargets((~WizardLastDropTargetUp)&0x0F);
        WizardLastDropTargetSet = CurrentTime;
      }

      if (CurrentTime>(GameModeEndTime-15000)) {
        for (byte count = 0; count < 4; count++) {
          if (count != CurrentPlayer) {
            OverrideScoreDisplay(count, 1+((GameModeEndTime-CurrentTime)/1000), false);
          }
        }
      }

      if (WizardJackpotLastChanged && CurrentTime<(WizardJackpotLastChanged+4000)) {
        OverrideScoreDisplay(CurrentPlayer, WizardJackpotValue, false);
        DisplaysNeedResetting = true;
      } else {
        if (DisplaysNeedResetting) {
          DisplaysNeedResetting = false;
          ShowPlayerScores(0xFF, false, false);
        }
      }      

      if (((CurrentTime-GameModeStartTime)/3000)%2) {
        specialAnimationRunning = true;
        ShowLampAnimation(3, 40, CurrentTime, 1, false, true);      
      }

      if (CurrentTime>GameModeEndTime) {
        ShowPlayerScores(0xFF, false, false);
        SetGameMode(GAME_MODE_WIZARD_FINISHED);
      }
      
      break;

     case GAME_MODE_WIZARD_FINISHED:
      if (GameModeStartTime==0) {
        ShowPlayerScores(0xFF, false, false);
        GameModeStartTime = CurrentTime;
        GameModeEndTime = CurrentTime + 2000;
        PlayBackgroundSong(SOUND_EFFECT_WAITING_FOR_LAUNCH);
      }

      specialAnimationRunning = true;
      ShowLampAnimation(2, 125, CurrentTime, 3, false, true);

      if (CurrentTime>GameModeEndTime) {
        unsigned long lastScoreMultiplier = ScoreMultiplier;
        ScoreMultiplier = 1 + (unsigned long)CountBits(GameMode/16);
        if (lastScoreMultiplier!=ScoreMultiplier) QueueNotification((SOUND_EFFECT_VP_1X_PLAYFIELD-1) + ScoreMultiplier, 0);
        SetGameMode(IncreasePlayerRank(1));
        IncreaseBonusX(1);
        PlaySoundEffect(SOUND_EFFECT_MISSION_COMPLETE, ConvertVolumeSettingToGain(SoundEffectsVolume));
        SetMissionComplete(NextMission);
        MissionsCompleted[CurrentPlayer] = 0x0000;
        NextMission = GetNextAvailableMission(0);
      }
      break;

  }

  if ( !specialAnimationRunning && NumTiltWarnings <= MaxTiltWarnings ) {
    ShowBonusLamps();
    ShowBonusXLamps();
#if not defined (RPU_OS_SOFTWARE_DISPLAY_INTERRUPT)
    RPU_DataRead(0);
#endif
    ShowGalaxyLamps();
    ShowShootAgainLamp();
    ShowSaucerLamps();
    ShowMissionLamps();
    ShowSpinnerLamp();
    ShowLaneLamps();
  }


  // Three types of display modes are shown here:
  // 1) score animation
  // 2) fly-bys
  // 3) normal scores
  if (ScoreAdditionAnimationStartTime != 0) {
    // Score animation
    if ((CurrentTime - ScoreAdditionAnimationStartTime) < 2000) {
      byte displayPhase = (CurrentTime - ScoreAdditionAnimationStartTime) / 60;
      byte digitsToShow = 1 + displayPhase / 6;
      if (digitsToShow > 6) digitsToShow = 6;
      unsigned long scoreToShow = ScoreAdditionAnimation;
      for (byte count = 0; count < (6 - digitsToShow); count++) {
        scoreToShow = scoreToShow / 10;
      }
      if (scoreToShow == 0 || displayPhase % 2) scoreToShow = DISPLAY_OVERRIDE_BLANK_SCORE;
      byte countdownDisplay = (1 + CurrentPlayer) % 4;

      for (byte count = 0; count < 4; count++) {
        if (count == countdownDisplay) OverrideScoreDisplay(count, scoreToShow, false);
        else if (count != CurrentPlayer) OverrideScoreDisplay(count, DISPLAY_OVERRIDE_BLANK_SCORE, false);
      }
    } else {
      byte countdownDisplay = (1 + CurrentPlayer) % 4;
      unsigned long remainingScore = 0;
      if ( (CurrentTime - ScoreAdditionAnimationStartTime) < 5000 ) {
        remainingScore = (((CurrentTime - ScoreAdditionAnimationStartTime) - 2000) * ScoreAdditionAnimation) / 3000;
        if ((remainingScore / 1000) != (LastRemainingAnimatedScoreShown / 1000)) {
          LastRemainingAnimatedScoreShown = remainingScore;
          if (PlayScoreAnimationTick) PlaySoundEffect(SOUND_EFFECT_SLING_SHOT, ConvertVolumeSettingToGain(SoundEffectsVolume));
        }
      } else {
        CurrentScores[CurrentPlayer] += ScoreAdditionAnimation;
        remainingScore = 0;
        ScoreAdditionAnimationStartTime = 0;
        ScoreAdditionAnimation = 0;
      }

      for (byte count = 0; count < 4; count++) {
        if (count == countdownDisplay) OverrideScoreDisplay(count, ScoreAdditionAnimation - remainingScore, false);
        else if (count != CurrentPlayer) OverrideScoreDisplay(count, DISPLAY_OVERRIDE_BLANK_SCORE, false);
        else OverrideScoreDisplay(count, CurrentScores[CurrentPlayer] + remainingScore, false);
      }
    }
    if (ScoreAdditionAnimationStartTime) ShowPlayerScores(CurrentPlayer, false, false);
    else ShowPlayerScores(0xFF, false, false);
  } else {
    ShowPlayerScores(CurrentPlayer, (BallFirstSwitchHitTime == 0) ? true : false, (BallFirstSwitchHitTime > 0 && ((CurrentTime - LastTimeScoreChanged) > 2000)) ? true : false);
  }

  // Check to see if ball is in the outhole
  if (RPU_ReadSingleSwitchState(SW_OUTHOLE)) {
    if (BallTimeInTrough == 0) {
      BallTimeInTrough = CurrentTime;
    } else {
      // Make sure the ball stays on the sensor for at least
      // 0.5 seconds to be sure that it's not bouncing
      if ((CurrentTime - BallTimeInTrough) > 500) {

        if (BallFirstSwitchHitTime == 0 && NumTiltWarnings <= MaxTiltWarnings) {
          // Nothing hit yet, so return the ball to the player
          RPU_PushToTimedSolenoidStack(SOL_OUTHOLE, 4, CurrentTime);
          BallTimeInTrough = 0;
          returnState = MACHINE_STATE_NORMAL_GAMEPLAY;
        } else {
          CurrentScores[CurrentPlayer] += ScoreAdditionAnimation;
          ScoreAdditionAnimationStartTime = 0;
          ScoreAdditionAnimation = 0;
          ShowPlayerScores(0xFF, false, false);
          // if we haven't used the ball save, and we're under the time limit, then save the ball
          if (!BallSaveUsed && CurrentTime<(BallSaveEndTime+BALL_SAVE_GRACE_PERIOD)) {
            RPU_PushToTimedSolenoidStack(SOL_OUTHOLE, 4, CurrentTime + 100);
            BallSaveUsed = true;
            QueueNotification(SOUND_EFFECT_VP_SHOOT_AGAIN, 0);
            RPU_SetLampState(LAMP_SHOOT_AGAIN, 0);
            BallTimeInTrough = CurrentTime;
            returnState = MACHINE_STATE_NORMAL_GAMEPLAY;
          } else {
            ShowPlayerScores(0xFF, false, false);
            PlayBackgroundSong(SOUND_EFFECT_NONE);
            StopAudio();

            if (CurrentBallInPlay < BallsPerGame) PlaySoundEffect(SOUND_EFFECT_BALL_OVER, ConvertVolumeSettingToGain(SoundEffectsVolume));
            returnState = MACHINE_STATE_COUNTDOWN_BONUS;
          }
        }
      }
    }
  } else {
    BallTimeInTrough = 0;
  }

  LastTimeThroughLoop = CurrentTime;
  return returnState;
}



unsigned long CountdownStartTime = 0;
unsigned long LastCountdownReportTime = 0;
unsigned long BonusCountDownEndTime = 0;

int CountdownBonus(boolean curStateChanged) {

  // If this is the first time through the countdown loop
  if (curStateChanged) {

    CountdownStartTime = CurrentTime;
    ShowBonusLamps();

    LastCountdownReportTime = CountdownStartTime;
    BonusCountDownEndTime = 0xFFFFFFFF;
  }

  unsigned long countdownDelayTime = 250 - (Bonus[CurrentPlayer] * 2);

  if ((CurrentTime - LastCountdownReportTime) > countdownDelayTime) {

    if (Bonus[CurrentPlayer]) {

      // Only give sound & score if this isn't a tilt
      if (NumTiltWarnings <= MaxTiltWarnings) {
        PlaySoundEffect(SOUND_EFFECT_BONUS_COUNT, ConvertVolumeSettingToGain(SoundEffectsVolume));
        CurrentScores[CurrentPlayer] += 1000 * ((unsigned long)BonusX[CurrentPlayer]);
      }

      Bonus[CurrentPlayer] -= 1;

      ShowBonusLamps();
    } else if (BonusCountDownEndTime == 0xFFFFFFFF) {
      BonusCountDownEndTime = CurrentTime + 1000;
    }
    LastCountdownReportTime = CurrentTime;
  }

  if (CurrentTime > BonusCountDownEndTime) {

    // Reset any lights & variables of goals that weren't completed
    BonusCountDownEndTime = 0xFFFFFFFF;
    return MACHINE_STATE_BALL_OVER;
  }

  return MACHINE_STATE_COUNTDOWN_BONUS;
}



void CheckHighScores() {
  unsigned long highestScore = 0;
  int highScorePlayerNum = 0;
  for (int count = 0; count < CurrentNumPlayers; count++) {
    if (CurrentScores[count] > highestScore) highestScore = CurrentScores[count];
    highScorePlayerNum = count;
  }

  if (highestScore > HighScore) {
    HighScore = highestScore;
    if (HighScoreReplay) {
      AddCredit(false, 3);
      RPU_WriteULToEEProm(RPU_TOTAL_REPLAYS_EEPROM_START_BYTE, RPU_ReadULFromEEProm(RPU_TOTAL_REPLAYS_EEPROM_START_BYTE) + 3);
    }
    RPU_WriteULToEEProm(RPU_HIGHSCORE_EEPROM_START_BYTE, highestScore);
    RPU_WriteULToEEProm(RPU_TOTAL_HISCORE_BEATEN_START_BYTE, RPU_ReadULFromEEProm(RPU_TOTAL_HISCORE_BEATEN_START_BYTE) + 1);

    for (int count = 0; count < 4; count++) {
      if (count == highScorePlayerNum) {
        RPU_SetDisplay(count, CurrentScores[count], true, 2);
      } else {
        RPU_SetDisplayBlank(count, 0x00);
      }
    }

    RPU_PushToTimedSolenoidStack(SOL_KNOCKER, 3, CurrentTime, true);
    RPU_PushToTimedSolenoidStack(SOL_KNOCKER, 3, CurrentTime + 300, true);
    RPU_PushToTimedSolenoidStack(SOL_KNOCKER, 3, CurrentTime + 600, true);
  }
}


unsigned long MatchSequenceStartTime = 0;
unsigned long MatchDelay = 150;
byte MatchDigit = 0;
byte NumMatchSpins = 0;
byte ScoreMatches = 0;

int ShowMatchSequence(boolean curStateChanged) {
  if (!MatchFeature) return MACHINE_STATE_ATTRACT;

  if (curStateChanged) {
    MatchSequenceStartTime = CurrentTime;
    MatchDelay = 1500;
    MatchDigit = CurrentTime % 10;
    NumMatchSpins = 0;
    RPU_SetLampState(LAMP_HEAD_MATCH, 1, 0);
    RPU_SetDisableFlippers();
    ScoreMatches = 0;
  }

  if (NumMatchSpins < 40) {
    if (CurrentTime > (MatchSequenceStartTime + MatchDelay)) {
      MatchDigit += 1;
      if (MatchDigit > 9) MatchDigit = 0;
      PlaySoundEffect(SOUND_EFFECT_MATCH_SPIN, ConvertVolumeSettingToGain(SoundEffectsVolume));
      RPU_SetDisplayBallInPlay((int)MatchDigit * 10);
      MatchDelay += 50 + 4 * NumMatchSpins;
      NumMatchSpins += 1;
      RPU_SetLampState(LAMP_HEAD_MATCH, NumMatchSpins % 2, 0);

      if (NumMatchSpins == 40) {
        RPU_SetLampState(LAMP_HEAD_MATCH, 0);
        MatchDelay = CurrentTime - MatchSequenceStartTime;
      }
    }
  }

  if (NumMatchSpins >= 40 && NumMatchSpins <= 43) {
    if (CurrentTime > (MatchSequenceStartTime + MatchDelay)) {
      if ( (CurrentNumPlayers > (NumMatchSpins - 40)) && ((CurrentScores[NumMatchSpins - 40] / 10) % 10) == MatchDigit) {
        ScoreMatches |= (1 << (NumMatchSpins - 40));
        AddSpecialCredit();
        MatchDelay += 1000;
        NumMatchSpins += 1;
        RPU_SetLampState(LAMP_HEAD_MATCH, 1);
      } else {
        NumMatchSpins += 1;
      }
      if (NumMatchSpins == 44) {
        MatchDelay += 5000;
      }
    }
  }

  if (NumMatchSpins > 43) {
    if (CurrentTime > (MatchSequenceStartTime + MatchDelay)) {
      return MACHINE_STATE_ATTRACT;
    }
  }

  for (int count = 0; count < 4; count++) {
    if ((ScoreMatches >> count) & 0x01) {
      // If this score matches, we're going to flash the last two digits
      if ( (CurrentTime / 200) % 2 ) {
        RPU_SetDisplayBlank(count, RPU_GetDisplayBlank(count) & 0x0F);
      } else {
        RPU_SetDisplayBlank(count, RPU_GetDisplayBlank(count) | 0x30);
      }
    }
  }

  return MACHINE_STATE_MATCH_MODE;
}


void HandleGalaxyTurnaround() {

  if (CurrentTime<GalaxyTurnaroundEjectTime) return;
  
  if ((GameMode&GAME_BASE_MODE) == GAME_MODE_UNSTRUCTURED_PLAY) {
    GalaxyTurnaroundEjectTime = CurrentTime + 10000;
    SetGameMode(GAME_MODE_MISSION_QUALIFIED);
  } else if ((GameMode&GAME_BASE_MODE) == GAME_MODE_MISSION_FUELING && GalaxyTurnaroundLights[CurrentPlayer]) {
    GalaxyTurnaroundEjectTime = CurrentTime + 10000;
    SetGameMode(GAME_MODE_MISSION_QUALIFIED);
  } else {
    GalaxyTurnaroundEjectTime = CurrentTime + 600;
    GalaxyBounceAnimationStart = CurrentTime;
    PlaySoundEffect(SOUND_EFFECT_GALAXY_BOUNCE, ConvertVolumeSettingToGain(SoundEffectsVolume));
    RPU_PushToTimedSolenoidStack(SOL_GALAXY_KICKER, 5, CurrentTime+600);    
    if (GalaxyKickerBallSave) SetBallSave(600 + (unsigned long)GalaxyKickerBallSave * 1000);
  }
}


void HandleSpinner() {
  LastSpinnerHit = CurrentTime;

  CheckForFeatureShot(MISSION_FEATURE_SHOT_SPINNER, 250*ScoreMultiplier);

  if ((GameMode&GAME_BASE_MODE) == GAME_MODE_MISSION_FUELING) {
    CurrentFuel += 1;
    CurrentScores[CurrentPlayer] += ScoreMultiplier * 1000;
    PlaySoundEffect(SOUND_EFFECT_REFUELING_SPINNER, ConvertVolumeSettingToGain(SoundEffectsVolume));
  } else {
    if (SpinnerAscension<50) {
      CurrentScores[CurrentPlayer] += ScoreMultiplier * 100;
      PlaySoundEffect(SOUND_EFFECT_ASCENDING_SPINNER+SpinnerAscension, ConvertVolumeSettingToGain(SoundEffectsVolume));
    } else {
      StartScoreAnimation(5000);
      PlaySoundEffect(SOUND_EFFECT_SPINNER_COMPLETION, ConvertVolumeSettingToGain(SoundEffectsVolume));
      SpinnerAscension = 0;
    }
    if ( (GameMode&GAME_MODE_FLAG_REFUEL) ) {
      SpinnerAscension += 3;
      CurrentScores[CurrentPlayer] += ScoreMultiplier * AWARD_FOR_REFUEL_SPIN;
    } else {
      SpinnerAscension += 1;
    }
    SpinnerHits[CurrentPlayer] += 1;
  }
  
}


void HandleGalaxySwitch(byte galaxyLetterIndex) {

  byte galaxyLetterBit = (1<<galaxyLetterIndex);

  if (GameMode&GAME_MODE_FLAG_PASSENGER_ABOARD) {
    GalaxyStatus[CurrentPlayer] |= galaxyLetterBit;
    GalaxyTurnaroundLights[CurrentPlayer] |= galaxyLetterBit;
    StartScoreAnimation(AWARD_FOR_PASSENGER_ABOARD * ScoreMultiplier);
    PlaySoundEffect(SOUND_EFFECT_PASSENGER_COLLECT, ConvertVolumeSettingToGain(SoundEffectsVolume));
    AddToBonus(2);    
  } else if ((GalaxyStatus[CurrentPlayer]&galaxyLetterBit)==0) {
    GalaxyStatus[CurrentPlayer] |= galaxyLetterBit;
    GalaxyTurnaroundLights[CurrentPlayer] |= galaxyLetterBit;
    GalaxyLetterAnimationEnd[galaxyLetterIndex] = CurrentTime + GALAXY_LETTER_ANIMATION_DURATION;
    CurrentScores[CurrentPlayer] += ScoreMultiplier * 500;
    if ((GameMode & GAME_BASE_MODE)==GAME_MODE_SKILL_SHOT) CurrentScores[CurrentPlayer] += ScoreMultiplier * 4500;
    PlaySoundEffect(SOUND_EFFECT_GALAXY_LETTER_COLLECT, ConvertVolumeSettingToGain(SoundEffectsVolume));
    AddToBonus(1);
  } else {
    CurrentScores[CurrentPlayer] += ScoreMultiplier * 100;
    if ((GameMode & GAME_BASE_MODE)==GAME_MODE_SKILL_SHOT) CurrentScores[CurrentPlayer] += ScoreMultiplier * 4900;
    PlaySoundEffect(SOUND_EFFECT_GALAXY_LETTER_UNLIT, ConvertVolumeSettingToGain(SoundEffectsVolume));
    AddToBonus(1);
  }
}



void CheckForMissionEnd(byte missionFinishShotHit) {
  if ((GameMode&GAME_BASE_MODE)==GAME_MODE_MISSION_THIRD_LEG) {
    if (MissionFinishShot[NextMission]==missionFinishShotHit) {
      SetGameMode(GAME_MODE_WAITING_FINISH_WITH_BONUS);
    }
  }
}

void CheckForFeatureShot(byte featureShotHit, unsigned long featureShotScore) {

  if ((GameMode&GAME_BASE_MODE)==GAME_MODE_MISSION_SECOND_LEG) {
    if (MissionFeatureShot[NextMission]==featureShotHit) {
      CurrentScores[CurrentPlayer] += ScoreMultiplier * featureShotScore;
    }
  } else if ((GameMode&GAME_BASE_MODE)==GAME_MODE_MISSION_THIRD_LEG) {
    if (MissionFeatureShot[NextMission]==featureShotHit) {
      FeatureShotLastHit = CurrentTime;
      CurrentScores[CurrentPlayer] += ScoreMultiplier * featureShotScore;
      MissionBonus += ScoreMultiplier * featureShotScore;
      PlaySoundEffect(SOUND_EFFECT_FEATURE_SHOT, ConvertVolumeSettingToGain(SoundEffectsVolume));
    }
  }
}




void HandleDropTargetHit(byte switchHit) {

  // Don't accept any target drops that happen during a reset?
  if (ResetDropTargetStatusTime) return;

  byte currentStatus = GetDropTargetStatus();
  boolean awardGiven = false;
  boolean soundPlayed = false;
  byte targetBit = 0;

  switch (switchHit) {
    case SW_RED_DROP_TARGET: targetBit = 1; break;
    case SW_BLUE_DROP_TARGET: targetBit = 2; break;
    case SW_YELLOW_DROP_TARGET: targetBit = 4; break;
    case SW_BLACK_DROP_TARGET: targetBit = 8; break;
  }

  // If this is a legit switch hit (not a repeat)
  if ( (targetBit & DropTargetStatus)==0 ) {

    if (DTBankHitOrder==0 && ((currentStatus&0x0F)==0x01)) {
      DTBankHitOrder = 1;
    } else if (DTBankHitOrder==1) {
      if ((currentStatus&0x0F)==0x03) DTBankHitOrder = 2;
      else DTBankHitOrder = 0;
    } else if (DTBankHitOrder==2) {
      if ((currentStatus&0x0F)==0x07) DTBankHitOrder = 3;
      else DTBankHitOrder = 0;
    } else if (DTBankHitOrder==3 && switchHit==SW_BLACK_DROP_TARGET) {
      StartScoreAnimation(15000);
      PlaySoundEffect(SOUND_EFFECT_DROP_SEQUENCE_SKILL, ConvertVolumeSettingToGain(SoundEffectsVolume));
      soundPlayed = true;
      awardGiven = true;
      DTBankHitOrder = 0;
      // In order automatically starts cargo
      DropTargetClears[CurrentPlayer] = DROP_TARGET_CLEARS_FOR_CARGO-1;
    }

    // Add points for cargo mode
    if ( (GameMode & GAME_MODE_FLAG_CARGO_COLLECTION) ) {
      CurrentScores[CurrentPlayer] += ScoreMultiplier * AWARD_FOR_CARGO_COLLECTION;
    }

    // Default scoring for a drop target
    if (!awardGiven) {
      CurrentScores[CurrentPlayer] += ScoreMultiplier * 500;
    }

    DropTargetStatus |= targetBit;
  }

  // If targets need to be reset
  if (currentStatus==0x0F) {
    if (ResetDropTargetStatusTime==0 && ((GameMode&GAME_BASE_MODE)!=GAME_MODE_WIZARD)) {
      AddToBonus(2);
      RPU_PushToTimedSolenoidStack(SOL_DROP_TARGET_RESET, 12, CurrentTime + 750);
      ResetDropTargetStatusTime = CurrentTime + 850;
    }

    // All targets are down - update mode if we're ready to launch
    if ((GameMode&GAME_BASE_MODE)==GAME_MODE_MISSION_READY_TO_LAUNCH) {
      SetGameMode(GAME_MODE_MISSION_LAUNCHING);
    } else if ((GameMode&GAME_BASE_MODE)==GAME_MODE_UNSTRUCTURED_PLAY) {
      PlaySoundEffect(SOUND_EFFECT_DROP_TARGET_CLEAR_1 + DropTargetClears[CurrentPlayer], ConvertVolumeSettingToGain(SoundEffectsVolume));
      DropTargetClears[CurrentPlayer] += 1;
      soundPlayed = true;
    } else if ((GameMode&GAME_BASE_MODE)==GAME_MODE_WIZARD) {
      PlaySoundEffect(SOUND_EFFECT_FEATURE_SHOT, ConvertVolumeSettingToGain(SoundEffectsVolume));
      soundPlayed = true;
      WizardJackpotValue += 25000;
      WizardJackpotLastChanged = CurrentTime;
      WizardLastDropTargetUp *= 2;
      if (WizardLastDropTargetUp>0x08) WizardLastDropTargetUp = 0x01;
      SetDropTargets((~WizardLastDropTargetUp)&0x0F);
      WizardLastDropTargetSet = CurrentTime;
    }
    
  }


  if (!soundPlayed) {
    PlaySoundEffect(SOUND_EFFECT_DROP_TARGET_NORMAL, ConvertVolumeSettingToGain(SoundEffectsVolume));
  }

  CheckForFeatureShot(MISSION_FEATURE_SHOT_DROPS, 1000*ScoreMultiplier);
}



void StartSideQuest() {
  GameMode |= SideQuestsQualified[CurrentPlayer];
  PlaySoundEffect(SOUND_EFFECT_SIDE_QUEST_STARTED, ConvertVolumeSettingToGain(SoundEffectsVolume));
  QueueNotification((SOUND_EFFECT_VP_SIDE_QUEST_1-1)+(unsigned int)(SideQuestsQualified[CurrentPlayer]/16), 0);
  SideQuestEndTime = CurrentTime + ((unsigned long)CountBits(SideQuestsQualified[CurrentPlayer]) * 10000) + 20000;
  SideQuestsQualified[CurrentPlayer] = 0;
  SideQuestQualifiedEndTime = 0;
  ScoreMultiplier = 1 + (unsigned long)CountBits(GameMode/16);
  QueueNotification((SOUND_EFFECT_VP_1X_PLAYFIELD-1) + ScoreMultiplier, 0);
  StartBackgroundMusic(MUSIC_TYPE_SIDE_QUEST);              
}



int RunGamePlayMode(int curState, boolean curStateChanged) {
  int returnState = curState;
  unsigned long scoreAtTop = CurrentScores[CurrentPlayer];

  // Very first time into gameplay loop
  if (curState == MACHINE_STATE_INIT_GAMEPLAY) {
    returnState = InitGamePlay(curStateChanged);
  } else if (curState == MACHINE_STATE_INIT_NEW_BALL) {
    returnState = InitNewBall(curStateChanged, CurrentPlayer, CurrentBallInPlay);
  } else if (curState == MACHINE_STATE_NORMAL_GAMEPLAY) {
    returnState = ManageGameMode();
  } else if (curState == MACHINE_STATE_COUNTDOWN_BONUS) {
    // Reset lamps to reasonable state (in case they were in an animation)
    ShowBonusLamps();
    ShowBonusXLamps();
    ShowGalaxyLamps();
    ShowShootAgainLamp();
    ShowSaucerLamps();
    ShowMissionLamps();
    ShowSpinnerLamp();
    ShowLaneLamps();
        
    // in case GI was off, turn it on
    SetGeneralIllumination(true);
    returnState = CountdownBonus(curStateChanged);
    ShowPlayerScores(0xFF, false, false);
  } else if (curState == MACHINE_STATE_BALL_OVER) {
    RPU_SetDisplayCredits(Credits, !FreePlayMode);

    if (SamePlayerShootsAgain) {
      QueueNotification(SOUND_EFFECT_VP_SHOOT_AGAIN, 0);
      returnState = MACHINE_STATE_INIT_NEW_BALL;
    } else {

      CurrentPlayer += 1;
      if (CurrentPlayer >= CurrentNumPlayers) {
        CurrentPlayer = 0;
        CurrentBallInPlay += 1;
      }

      scoreAtTop = CurrentScores[CurrentPlayer];

      if (CurrentBallInPlay > BallsPerGame) {
        CheckHighScores();
        PlaySoundEffect(SOUND_EFFECT_GAME_OVER, ConvertVolumeSettingToGain(SoundEffectsVolume));
        for (int count = 0; count < CurrentNumPlayers; count++) {
          RPU_SetDisplay(count, CurrentScores[count], true, 2);
        }

        returnState = MACHINE_STATE_MATCH_MODE;
      }
      else returnState = MACHINE_STATE_INIT_NEW_BALL;
    }
  } else if (curState == MACHINE_STATE_MATCH_MODE) {
    returnState = ShowMatchSequence(curStateChanged);
  }

  unsigned long lastBallFirstSwitchHitTime = BallFirstSwitchHitTime;
  byte switchHit;

  if (NumTiltWarnings <= MaxTiltWarnings) {
    while ( (switchHit = RPU_PullFirstFromSwitchStack()) != SWITCH_STACK_EMPTY ) {

      if (DEBUG_MESSAGES) {
        char buf[128];
        sprintf(buf, "Switch Hit = %d\n", switchHit);
        Serial.write(buf);
      }

      switch (switchHit) {
        case SW_SLAM:
          //          RPU_DisableSolenoidStack();
          //          RPU_SetDisableFlippers(true);
          //          RPU_TurnOffAllLamps();
          //          RPU_SetLampState(GAME_OVER, 1);
          //          delay(1000);
          //          return MACHINE_STATE_ATTRACT;
          break;
        case SW_TILT:
          // Tilt only counts if the playfield has been qualified
          if (BallFirstSwitchHitTime) {
            if ((CurrentTime - LastTiltWarningTime) > TILT_WARNING_DEBOUNCE_TIME) {
              LastTiltWarningTime = CurrentTime;
              NumTiltWarnings += 1;
              if (NumTiltWarnings > MaxTiltWarnings) {
                RPU_DisableSolenoidStack();
                RPU_SetDisableFlippers(true);
                RPU_TurnOffAllLamps();
                StopAudio();
                PlaySoundEffect(SOUND_EFFECT_TILT, ConvertVolumeSettingToGain(SoundEffectsVolume));
                RPU_SetLampState(LAMP_HEAD_TILT, 1);
                SetGeneralIllumination(false);
              }
              PlaySoundEffect(SOUND_EFFECT_TILT_WARNING, ConvertVolumeSettingToGain(SoundEffectsVolume));
            }
          } else {
            // Tilt before ball is plunged -- show a timer in ManageGameMode if desired
            if ( CurrentTime > (LastTiltWarningTime + TILT_WARNING_DEBOUNCE_TIME) ) {
              PlaySoundEffect(SOUND_EFFECT_TILT_WARNING, ConvertVolumeSettingToGain(SoundEffectsVolume));
            }
            LastTiltWarningTime = CurrentTime;
          }
          break;
        case SW_SELF_TEST_SWITCH:
          returnState = MACHINE_STATE_TEST_LAMPS;
          SetLastSelfTestChangedTime(CurrentTime);
          break;
        case SW_LEFT_SLING:
        case SW_RIGHT_SLING:
          CheckForFeatureShot(MISSION_FEATURE_SHOT_SLING_SHOTS, 100);
          CurrentScores[CurrentPlayer] += ScoreMultiplier * 10;
          PlaySoundEffect(SOUND_EFFECT_SLING_SHOT, ConvertVolumeSettingToGain(SoundEffectsVolume));
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          LastSwitchHitTime = CurrentTime;
          if (ExtraBallLit[CurrentPlayer]==1) ExtraBallLit[CurrentPlayer] = 2;
          else if (ExtraBallLit[CurrentPlayer]==2) ExtraBallLit[CurrentPlayer] = 1;
          break;
        case SW_UPPER_POP:
          CheckForFeatureShot(MISSION_FEATURE_SHOT_TOP_POP, 500);
          CheckForMissionEnd(MISSION_FINISH_SHOT_TOP_POP);
          if ((GameMode&GAME_BASE_MODE)==GAME_MODE_MISSION_FIRST_LEG) {
            PopsForMultiplier += 3;
            PlaySoundEffect(SOUND_EFFECT_POP_BUMPER_ADV_3, ConvertVolumeSettingToGain(SoundEffectsVolume));
          } else {
            PlaySoundEffect(SOUND_EFFECT_BUMPER_HIT, ConvertVolumeSettingToGain(SoundEffectsVolume));
          }

          if (SideQuestQualifiedEndTime) {
            SideQuestNumTopPops += 1;
            if (SideQuestNumTopPops==3 && (SideQuestStartSwitches & SIDE_QUEST_START_3_TOP_POPS)) {
              StartSideQuest();
            }
          }
          CurrentScores[CurrentPlayer] += ScoreMultiplier * 100;
          LastSwitchHitTime = CurrentTime;
          break;
        case SW_LOWER_RIGHT_POP:
        case SW_LOWER_LEFT_POP:
          CheckForFeatureShot(MISSION_FEATURE_SHOT_LOWER_POPS, 200*ScoreMultiplier);

          if ((GameMode&GAME_BASE_MODE)==GAME_MODE_MISSION_FIRST_LEG) {
            PopsForMultiplier += 1;
            PlaySoundEffect(SOUND_EFFECT_POP_BUMPER_ADV_1, ConvertVolumeSettingToGain(SoundEffectsVolume));
          } else {
            PlaySoundEffect(SOUND_EFFECT_BUMPER_HIT, ConvertVolumeSettingToGain(SoundEffectsVolume));
          }
          CurrentScores[CurrentPlayer] += ScoreMultiplier * 100;
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          LastSwitchHitTime = CurrentTime;
          if (OutlaneStatus) {
            OutlaneStatus = (OutlaneStatus==1)?2:1;
          }
          if (SpecialLit[CurrentPlayer]==1) SpecialLit[CurrentPlayer] = 2;
          else if (SpecialLit[CurrentPlayer]==2) SpecialLit[CurrentPlayer] = 1;
          break;          
        case SW_COIN_1:
        case SW_COIN_2:
        case SW_COIN_3:
          AddCoinToAudit(SwitchToChuteNum(switchHit));
          AddCoin(SwitchToChuteNum(switchHit));
          break;
        case SW_RED_DROP_TARGET:
        case SW_BLUE_DROP_TARGET:
        case SW_YELLOW_DROP_TARGET:
        case SW_BLACK_DROP_TARGET:
          // Don't accept target drops in the first second of skill shot
          // (in case one of the targets falls accidentally)
          if (BallFirstSwitchHitTime!=0 || CurrentTime>(GameModeStartTime+1000)) {
            HandleDropTargetHit(switchHit);
            if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
            LastSwitchHitTime = CurrentTime;
          }
          break;
        case SW_GALAXY_TURNAROUND:
          if ((GameMode & GAME_BASE_MODE)==GAME_MODE_SKILL_SHOT) {
            byte bonusPhase = ((CurrentTime-GameModeStartTime)/100)%12;
            unsigned long skillShotScore = 10000 + 1000*(unsigned long)(BonusLampValues[BonusLampScanAnimation[bonusPhase]]);
            StartScoreAnimation(skillShotScore);
            PlaySoundEffect(SOUND_EFFECT_SKILL_SHOT, ConvertVolumeSettingToGain(SoundEffectsVolume));
            QueueNotification(SOUND_EFFECT_VP_SKILL_SHOT_1+CurrentTime%4, 0);
            RPU_PushToTimedSolenoidStack(SOL_GALAXY_KICKER, 5, CurrentTime+2000);
            if (GalaxyKickerBallSave) SetBallSave(2000 + (unsigned long)GalaxyKickerBallSave * 1000);
          } else if ((GameMode & GAME_BASE_MODE)==GAME_MODE_WIZARD_QUALIFIED) {
            SetGameMode(GAME_MODE_WIZARD_START);
            StartScoreAnimation((unsigned long)50000, false);
          } else if ((GameMode & GAME_BASE_MODE)==GAME_MODE_WIZARD && WizardJackpotValue) {
            QueueNotification(SOUND_EFFECT_VP_JACKPOT, 0);
            StartScoreAnimation((unsigned long)WizardJackpotValue, false);
            WizardJackpotValue = 0;
            WizardJackpotLastChanged = 0;
            RPU_PushToTimedSolenoidStack(SOL_GALAXY_KICKER, 5, CurrentTime+1000);
            if (GalaxyKickerBallSave) SetBallSave(1000 + (unsigned long)GalaxyKickerBallSave * 1000);
          } else {
            HandleGalaxyTurnaround();
            CheckForMissionEnd(MISSION_FINISH_SHOT_GALAXY_TURNAROUND);
          }
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          LastSwitchHitTime = CurrentTime;
          break;
        case SW_SPINNER:
          HandleSpinner();
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          LastSwitchHitTime = CurrentTime;
          break;
        case SW_LEFT_TOPLANE:
          HandleGalaxySwitch(0);
          CheckForFeatureShot(MISSION_FEATURE_SHOT_TOP_LANES, 1000);
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          LastSwitchHitTime = CurrentTime;
          break;
        case SW_MID_TOPLANE:
          HandleGalaxySwitch(1);
          CheckForFeatureShot(MISSION_FEATURE_SHOT_TOP_LANES, 1000);
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          LastSwitchHitTime = CurrentTime;
          break;
        case SW_RIGHT_TOPLANE:
          HandleGalaxySwitch(2);
          CheckForFeatureShot(MISSION_FEATURE_SHOT_TOP_LANES, 1000);
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          LastSwitchHitTime = CurrentTime;
          break;
        case SW_A2_ROLLOVER:
          HandleGalaxySwitch(3);
          if (SideQuestQualifiedEndTime) {
            if (SideQuestStartSwitches & SIDE_QUEST_START_RIGHT_A) {
              StartSideQuest();
            }
          }
          //if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          LastSwitchHitTime = CurrentTime;
          break;
        case SW_LEFT_INLANE:
          HandleGalaxySwitch(4);
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          LastSwitchHitTime = CurrentTime;
          break;
        case SW_RIGHT_INLANE:
          HandleGalaxySwitch(5);
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          LastSwitchHitTime = CurrentTime;
          break;
        case SW_SAUCER:
          // Debounce the saucer switch 
          if ((CurrentTime-LastSaucerHitTime)>900) {
            if ((GameMode & GAME_BASE_MODE)==GAME_MODE_WIZARD_QUALIFIED) {
              SetGameMode(GAME_MODE_WIZARD_START);
              StartScoreAnimation((unsigned long)100000, false);
              SaucerHitAnimationEnd = CurrentTime + 3000;
              SaucerHitLampNum = 4;
              CurrentSaucerValue = 10;
              SaucerValueDecreaseTime = CurrentTime + 5000;
            } else if ((GameMode & GAME_BASE_MODE)==GAME_MODE_SKILL_SHOT) {
              SaucerHitAnimationEnd = CurrentTime + 3000;
              SaucerHitLampNum = (CurrentSaucerValue/2)-1;
              StartScoreAnimation(((unsigned long)CurrentSaucerValue)*(unsigned long)10000);
              if (!SideQuestsQualified[CurrentPlayer]) {
                PlaySoundEffect(SOUND_EFFECT_SKILL_SHOT, ConvertVolumeSettingToGain(SoundEffectsVolume));
                QueueNotification(SOUND_EFFECT_VP_SKILL_SHOT_1+CurrentTime%4, 0);
              }
            } else if ((GameMode & GAME_BASE_MODE)==GAME_MODE_MISSION_SECOND_LEG) {
              // Saucer skips cryo sleep
              SetGameMode(GAME_MODE_MISSION_THIRD_LEG);
              SaucerHitAnimationEnd = CurrentTime + 3000;
              SaucerHitLampNum = (CurrentSaucerValue/2)-1;
              StartScoreAnimation(((unsigned long)CurrentSaucerValue)*(unsigned long)1000);
            } else if ((GameMode & GAME_BASE_MODE)==GAME_MODE_WIZARD && WizardJackpotValue) {
              QueueNotification(SOUND_EFFECT_VP_SUPER_JACKPOT, 0);
              StartScoreAnimation((unsigned long)WizardJackpotValue*2, false);
              WizardJackpotValue = 0;
              WizardJackpotLastChanged = 0;
            } else {
              SaucerHitAnimationEnd = CurrentTime + 3000;
              SaucerHitLampNum = (CurrentSaucerValue/2)-1;
              StartScoreAnimation(((unsigned long)CurrentSaucerValue)*(unsigned long)1000);
              switch(CurrentSaucerValue) {
                case 2: CurrentSaucerValue = 4; break;
                case 4: CurrentSaucerValue = 6; break;
                case 6: CurrentSaucerValue = 8; break;
                case 8: CurrentSaucerValue = 10; break;                
              }
              SaucerValueDecreaseTime = CurrentTime + ((unsigned long)(17-CurrentSaucerValue))*(unsigned long)1000;
            }
  
            if (SideQuestQualifiedEndTime || ((GameMode & GAME_BASE_MODE)==GAME_MODE_SKILL_SHOT && SideQuestsQualified[CurrentPlayer])) {
              StartSideQuest();
            }
            
            RPU_PushToTimedSolenoidStack(SOL_SAUCER, 5, CurrentTime+1000, true);
            if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
            LastSwitchHitTime = CurrentTime;
            LastSaucerHitTime = CurrentTime;
          }
          break;
        case SW_BLACK_STANDUP:
          CheckForFeatureShot(MISSION_FEATURE_SHOT_STANDUPS, 1000);
          CheckForMissionEnd(MISSION_FINISH_SHOT_STANDUPS);
          if ((DropTargetStatus&0x08)==0) {
            // drop the Black target
            RPU_PushToSolenoidStack(SOL_BLACK_DROP_TARGET, 3);
          }
          if ( (GameMode & GAME_MODE_FLAG_PIRATE_ENCOUNTER) ) {
            CurrentScores[CurrentPlayer] += ScoreMultiplier * AWARD_FOR_PIRATE_ENCOUNTER;
            PlaySoundEffect(SOUND_EFFECT_PIRATE_COLLECT, ConvertVolumeSettingToGain(SoundEffectsVolume));
          } else {
            StandupsHitThisBall |= 0x08;
          }
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          LastSwitchHitTime = CurrentTime;
          break;
        case SW_YELLOW_STANDUP:
          CheckForFeatureShot(MISSION_FEATURE_SHOT_STANDUPS, 1000);
          CheckForMissionEnd(MISSION_FINISH_SHOT_STANDUPS);
          if ((DropTargetStatus&0x04)==0) {
            // drop the Black target
            RPU_PushToSolenoidStack(SOL_YELLOW_DROP_TARGET, 3);
          } else {
            if (DEBUG_MESSAGES) {
              char buf[256];
              sprintf(buf, "DropTargetStatus = 0x%02X\n", DropTargetStatus);
              Serial.write(buf);
            }
          }
          if ( (GameMode & GAME_MODE_FLAG_PIRATE_ENCOUNTER) ) {
            CurrentScores[CurrentPlayer] += ScoreMultiplier * AWARD_FOR_PIRATE_ENCOUNTER;
            PlaySoundEffect(SOUND_EFFECT_PIRATE_COLLECT, ConvertVolumeSettingToGain(SoundEffectsVolume));
          } else {
            StandupsHitThisBall |= 0x04;
          }
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          LastSwitchHitTime = CurrentTime;
          break;
        case SW_BLUE_STANDUP:
          CheckForFeatureShot(MISSION_FEATURE_SHOT_STANDUPS, 1000);
          CheckForMissionEnd(MISSION_FINISH_SHOT_STANDUPS);
          if ((DropTargetStatus&0x02)==0) {
            // drop the Black target
            RPU_PushToSolenoidStack(SOL_BLUE_DROP_TARGET, 3);
          }
          if ( (GameMode & GAME_MODE_FLAG_PIRATE_ENCOUNTER) ) {
            CurrentScores[CurrentPlayer] += ScoreMultiplier * AWARD_FOR_PIRATE_ENCOUNTER;
            PlaySoundEffect(SOUND_EFFECT_PIRATE_COLLECT, ConvertVolumeSettingToGain(SoundEffectsVolume));
          } else {
            StandupsHitThisBall |= 0x02;
          }
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          LastSwitchHitTime = CurrentTime;
          break;
        case SW_LEFT_MIDLANE:
          if (MidlaneStatus) {
            SpotNextDropTarget();
            MidlaneStatus = 0;
            CurrentScores[CurrentPlayer] += ScoreMultiplier * 2000;
          } else {
            PlaySoundEffect(SOUND_EFFECT_INLANE_UNLIT, ConvertVolumeSettingToGain(SoundEffectsVolume));
            CurrentScores[CurrentPlayer] += ScoreMultiplier * 100;
          }
          if (ExtraBallLit[CurrentPlayer]==1) {
            ExtraBallLit[CurrentPlayer] = 0;
            AwardExtraBall();
          }
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          LastSwitchHitTime = CurrentTime;
          break;
        case SW_RIGHT_MIDLANE:
          if (MidlaneStatus) {
            SpotNextDropTarget();
            MidlaneStatus = 0;
            CurrentScores[CurrentPlayer] += ScoreMultiplier * 2000;
          } else {
            PlaySoundEffect(SOUND_EFFECT_INLANE_UNLIT, ConvertVolumeSettingToGain(SoundEffectsVolume));
            CurrentScores[CurrentPlayer] += ScoreMultiplier * 100;
          }
          if (ExtraBallLit[CurrentPlayer]==2) {
            ExtraBallLit[CurrentPlayer] = 0;
            AwardExtraBall();
          }
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          LastSwitchHitTime = CurrentTime;
          break;
        case SW_RIGHT_OUTLANE_ROLLOVER:
          HandleGalaxySwitch(5);
          if (OutlaneStatus==2) {
            CurrentScores[CurrentPlayer] += ScoreMultiplier * 10000;
          } else {
            CurrentScores[CurrentPlayer] += ScoreMultiplier * 5000;
          }
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          LastSwitchHitTime = CurrentTime;
          if (SpecialLit[CurrentPlayer]==2) {
            SpecialLit[CurrentPlayer] = 0;
            AwardSpecial();
          }
          if (BallSaveEndTime!=0) {
            BallSaveEndTime += 3000;
          }
          break;
        case SW_LEFT_OUTLANE_ROLLOVER:
          HandleGalaxySwitch(4);
          if (OutlaneStatus==1) {
            CurrentScores[CurrentPlayer] += ScoreMultiplier * 10000;
          } else {
            CurrentScores[CurrentPlayer] += ScoreMultiplier * 5000;
          }
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          LastSwitchHitTime = CurrentTime;
          if (SpecialLit[CurrentPlayer]==1) {
            SpecialLit[CurrentPlayer] = 0;
            AwardSpecial();
          }
          if (BallSaveEndTime!=0) {
            BallSaveEndTime += 3000;
          }
          break;
        case SW_CENTER_ROLLOVER:
          AddToBonus(1);
          PlaySoundEffect(SOUND_EFFECT_ROLLOVER, ConvertVolumeSettingToGain(SoundEffectsVolume));
          LastSwitchHitTime = CurrentTime;
          break;
        case SW_CREDIT_RESET:
          if (CurrentBallInPlay < 2) {
            // If we haven't finished the first ball, we can add players
            AddPlayer();
          } else {
            // If the first ball is over, pressing start again resets the game
            if (Credits >= 1 || FreePlayMode) {
              if (!FreePlayMode) {
                Credits -= 1;
                RPU_WriteByteToEEProm(RPU_CREDITS_EEPROM_BYTE, Credits);
                RPU_SetDisplayCredits(Credits, !FreePlayMode);
              }
              returnState = MACHINE_STATE_INIT_GAMEPLAY;
            }
          }
          if (DEBUG_MESSAGES) {
            Serial.write("Start game button pressed\n\r");
          }
          break;
      }
    }
  } else {
    // We're tilted, so just wait for outhole
    while ( (switchHit = RPU_PullFirstFromSwitchStack()) != SWITCH_STACK_EMPTY ) {
      switch (switchHit) {
        case SW_SELF_TEST_SWITCH:
          returnState = MACHINE_STATE_TEST_LAMPS;
          SetLastSelfTestChangedTime(CurrentTime);
          break;
        case SW_COIN_1:
        case SW_COIN_2:
        case SW_COIN_3:
          AddCoinToAudit(SwitchToChuteNum(switchHit));
          AddCoin(SwitchToChuteNum(switchHit));
          break;
      }
    }
  }

  if (lastBallFirstSwitchHitTime==0 && BallFirstSwitchHitTime!=0) {
    BallSaveEndTime = BallFirstSwitchHitTime + ((unsigned long)CurrentBallSaveNumSeconds)*1000;
  }
  if (CurrentTime>(BallSaveEndTime+BALL_SAVE_GRACE_PERIOD)) {
    BallSaveEndTime = 0;
  }

  if (!ScrollingScores && CurrentScores[CurrentPlayer] > RPU_OS_MAX_DISPLAY_SCORE) {
    CurrentScores[CurrentPlayer] -= RPU_OS_MAX_DISPLAY_SCORE;
  }

  if (scoreAtTop != CurrentScores[CurrentPlayer]) {
    LastTimeScoreChanged = CurrentTime;
    if (!TournamentScoring) {
      for (int awardCount = 0; awardCount < 3; awardCount++) {
        if (AwardScores[awardCount] != 0 && scoreAtTop < AwardScores[awardCount] && CurrentScores[CurrentPlayer] >= AwardScores[awardCount]) {
          // Player has just passed an award score, so we need to award it
          if (((ScoreAwardReplay >> awardCount) & 0x01)) {
            AddSpecialCredit();
          } else if (!ExtraBallCollected) {
            AwardExtraBall();
          }
        }
      }
    }

  }

  return returnState;
}


void loop() {

  RPU_DataRead(0);

  CurrentTime = millis();
  int newMachineState = MachineState;

  if (MachineState < 0) {
    newMachineState = RunSelfTest(MachineState, MachineStateChanged);
  } else if (MachineState == MACHINE_STATE_ATTRACT) {
    newMachineState = RunAttractMode(MachineState, MachineStateChanged);
  } else {
    newMachineState = RunGamePlayMode(MachineState, MachineStateChanged);
  }

  if (newMachineState != MachineState) {
    MachineState = newMachineState;
    MachineStateChanged = true;
  } else {
    MachineStateChanged = false;
  }

  RPU_ApplyFlashToLamps(CurrentTime);
  RPU_UpdateTimedSolenoidStack(CurrentTime);

#ifdef RPU_OS_USE_SB300
  ServiceSoundQueue(CurrentTime);
#endif 
}
