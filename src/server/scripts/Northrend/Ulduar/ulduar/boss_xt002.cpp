/*
 * Copyright (C) 2008-2011 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ulduar.h"

enum Spells
{
    SPELL_TYMPANIC_TANTRUM                      = 62776,
    SPELL_SEARING_LIGHT_10                      = 63018,
    SPELL_SEARING_LIGHT_25                      = 65121,
    SPELL_GRAVITY_BOMB_10                       = 63024,
    SPELL_GRAVITY_BOMB_25                       = 64234,
    SPELL_HEARTBREAK                            = 65737,
    SPELL_ENRAGE                                = 26662,

    //------------------VOID ZONE--------------------
    SPELL_VOID_ZONE                             = 64203,
    SPELL_VOID_ZONE_DAMAGE                      = 46264,

    // Life Spark
    SPELL_STATIC_CHARGED                        = 64227,
    SPELL_SHOCK                                 = 64230,

    //----------------XT-002 HEART-------------------
    SPELL_EXPOSED_HEART                         = 63849,

    //---------------XM-024 PUMMELLER----------------
    SPELL_ARCING_SMASH                          = 8374,
    SPELL_TRAMPLE                               = 5568,
    SPELL_UPPERCUT                              = 10966,

    //------------------BOOMBOT-----------------------
    SPELL_BOOM                                  = 62834,
    
    //------------------SCRAPBOT-----------------------
    SPELL_REPAIR                                = 62832,
};

enum Timers
{
    TIMER_TYMPANIC_TANTRUM                      = 60000,
    TIMER_SEARING_LIGHT                         = 20000,
    TIMER_SPAWN_LIFE_SPARK                      = 9000,
    TIMER_GRAVITY_BOMB                          = 20000,
    TIMER_SPAWN_GRAVITY_BOMB                    = 9000,
    TIMER_HEART_PHASE                           = 35000,
    TIMER_ENRAGE                                = 600000,

    TIMER_VOID_ZONE                             = 2000,

    // Life Spark
    TIMER_SHOCK                                 = 12000,

    // Pummeller
    // Timers may be off
    TIMER_ARCING_SMASH                          = 27000,
    TIMER_TRAMPLE                               = 22000,
    TIMER_UPPERCUT                              = 17000,

    TIMER_SPAWN_ADD                             = 12000,
};

enum Creatures
{
    NPC_VOID_ZONE                               = 34001,
    NPC_LIFE_SPARK                              = 34004,
    NPC_XT002_HEART                             = 33329,
    NPC_XS013_SCRAPBOT                          = 33343,
    NPC_XM024_PUMMELLER                         = 33344,
    NPC_XE321_BOOMBOT                           = 33346,
};

enum Actions
{
    ACTION_ENTER_HARD_MODE                      = 0,
    ACTION_DISABLE_NERF_ACHI                    = 1,
};

enum Yells
{
    SAY_AGGRO                                   = -1603300,
    SAY_HEART_OPENED                            = -1603301,
    SAY_HEART_CLOSED                            = -1603302,
    SAY_TYMPANIC_TANTRUM                        = -1603303,
    SAY_SLAY_1                                  = -1603304,
    SAY_SLAY_2                                  = -1603305,
    SAY_BERSERK                                 = -1603306,
    SAY_DEATH                                   = -1603307,
    SAY_SUMMON                                  = -1603308,
};

#define EMOTE_TYMPANIC    "XT-002 Deconstructor begins to cause the earth to quake."
#define EMOTE_HEART       "XT-002 Deconstructor's heart is exposed and leaking energy."
#define EMOTE_REPAIR      "XT-002 Deconstructor consumes a scrap bot to repair himself!"

#define ACHIEV_TIMED_START_EVENT                21027
#define ACHIEVEMENT_HEARTBREAKER                RAID_MODE(3058, 3059)
#define ACHIEVEMENT_NERF_ENG                    RAID_MODE(2931, 2932)
/************************************************
-----------------SPAWN LOCATIONS-----------------
************************************************/
//Shared Z-level
#define SPAWN_Z                                 412
//Lower right
#define LR_X                                    796
#define LR_Y                                    -94
//Lower left
#define LL_X                                    796
#define LL_Y                                    57
//Upper right
#define UR_X                                    890
#define UR_Y                                    -82
//Upper left
#define UL_X                                    894
#define UL_Y                                    62


/*-------------------------------------------------------
 *
 *        XT-002 DECONSTRUCTOR
 *
 *///----------------------------------------------------
class boss_xt002 : public CreatureScript
{
public:
    boss_xt002() : CreatureScript("boss_xt002") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new boss_xt002_AI(pCreature);
    }

    struct boss_xt002_AI : public BossAI
    {
        boss_xt002_AI(Creature *pCreature) : BossAI(pCreature, BOSS_XT002), vehicle(me->GetVehicleKit())
        {
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_GRIP, true);
        }

        Vehicle *vehicle;
    
        uint32 uiSearingLightTimer;
        uint32 uiSpawnLifeSparkTimer;
        uint32 uiGravityBombTimer;
        uint32 uiSpawnGravityBombTimer;
        uint32 uiTympanicTantrumTimer;
        uint32 uiHeartPhaseTimer;
        uint32 uiSpawnAddTimer;
        uint32 uiEnrageTimer;

        bool searing_light_active;
        bool gravity_bomb_active;
        uint64 uiSearingLightTarget;
        uint64 uiGravityBombTarget;

        uint8 phase;
        uint8 heart_exposed;
        bool enraged;

        bool enterHardMode;
        bool hardMode;
        bool achievement_nerf;

        void Reset()
        {
            _Reset();
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_DISABLE_MOVE | UNIT_FLAG_NOT_SELECTABLE);
            me->SetReactState(REACT_AGGRESSIVE);
            me->ResetLootMode();

            //Makes XT-002 to cast a light bomb 10 seconds after aggro.
            uiSearingLightTimer = TIMER_SEARING_LIGHT / 2;
            uiSpawnLifeSparkTimer = TIMER_SPAWN_LIFE_SPARK;
            uiGravityBombTimer = TIMER_GRAVITY_BOMB;
            uiSpawnGravityBombTimer = TIMER_SPAWN_GRAVITY_BOMB;
            uiHeartPhaseTimer = TIMER_HEART_PHASE;
            uiSpawnAddTimer = TIMER_SPAWN_ADD;
            uiEnrageTimer = TIMER_ENRAGE;
            uiTympanicTantrumTimer = TIMER_TYMPANIC_TANTRUM / 2;

            searing_light_active = false;
            gravity_bomb_active = false;
            enraged = false;
            hardMode = false;
            enterHardMode = false;
            achievement_nerf = true;

            phase = 1;
            heart_exposed = 0;

            vehicle->Reset();

            if (instance)
                instance->DoStopTimedAchievement(ACHIEVEMENT_TIMED_TYPE_EVENT, ACHIEV_TIMED_START_EVENT);
        }

        void EnterCombat(Unit* /*who*/)
        {
            DoScriptText(SAY_AGGRO, me);
            _EnterCombat();

            if (instance)
                instance->DoStartTimedAchievement(ACHIEVEMENT_TIMED_TYPE_EVENT, ACHIEV_TIMED_START_EVENT);
        }

        void DoAction(const int32 action)
        {
            switch (action)
            {
                case ACTION_ENTER_HARD_MODE:
                    if (!hardMode)
                    {
                        hardMode = true;
                        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_DISABLE_MOVE);
                        me->SetReactState(REACT_AGGRESSIVE);
                        me->SetStandState(UNIT_STAND_STATE_STAND);
                        DoZoneInCombat();

                        uiEnrageTimer = TIMER_ENRAGE;
                        // Add HardMode Loot
                        me->AddLootMode(LOOT_MODE_HARD_MODE_1);

                        // Enter hard mode
                        enterHardMode = true;

                        // set max health
                        me->SetFullHealth();

                        // Get his heartbreak buff
                        DoCast(me, SPELL_HEARTBREAK, true);
                    }
                    break;
                case ACTION_DISABLE_NERF_ACHI:
                    achievement_nerf = false;
                    break;
            }
        }

        void KilledUnit(Unit* /*victim*/)
        {
            DoScriptText(RAND(SAY_SLAY_1,SAY_SLAY_2), me);
        }

        void JustDied(Unit* /*victim*/)
        {
            DoScriptText(SAY_DEATH, me);
            _JustDied();

            // Needed if is killed during the Heart-phase
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_DISABLE_MOVE);

            if (instance)
            {
                // Heartbreaker
                if (hardMode)
                    instance->DoCompleteAchievement(ACHIEVEMENT_HEARTBREAKER);
                // Nerf Engineering
                if (achievement_nerf)
                    instance->DoCompleteAchievement(ACHIEVEMENT_NERF_ENG);
            }
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            if (enterHardMode)
            {
                SetPhaseOne();
                enterHardMode = false;
            }

            // Handles spell casting. These spells only occur during phase 1 and hard mode
            if (phase == 1 || hardMode)
            {
                if (uiSearingLightTimer <= diff)
                {
                    if (Unit* pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0))
                    {
                        me->AddAura(RAID_MODE(SPELL_SEARING_LIGHT_10, SPELL_SEARING_LIGHT_25), pTarget);
                        uiSearingLightTarget = pTarget->GetGUID();
                    }
                    uiSpawnLifeSparkTimer = TIMER_SPAWN_LIFE_SPARK;
                    if (hardMode)
                        searing_light_active = true;
                    uiSearingLightTimer = TIMER_SEARING_LIGHT;
                } else uiSearingLightTimer -= diff;

                if (uiGravityBombTimer <= diff)
                {
                    if (Unit* pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0))
                    {
                        me->AddAura(RAID_MODE(SPELL_GRAVITY_BOMB_10, SPELL_GRAVITY_BOMB_25), pTarget);
                        uiGravityBombTarget = pTarget->GetGUID();
                    }
                    uiGravityBombTimer = TIMER_GRAVITY_BOMB;
                    if (hardMode)
                        gravity_bomb_active = true;
                } else uiGravityBombTimer -= diff;

                if (uiTympanicTantrumTimer <= diff)
                {
                    DoScriptText(SAY_TYMPANIC_TANTRUM, me);
                    me->MonsterTextEmote(EMOTE_TYMPANIC, 0, true);
                    DoCast(SPELL_TYMPANIC_TANTRUM);
                    uiTympanicTantrumTimer = TIMER_TYMPANIC_TANTRUM;
                } else uiTympanicTantrumTimer -= diff;
            }

            if (!hardMode)
            {
                if (phase == 1)
                {
                    if (HealthBelowPct(75) && heart_exposed == 0)
                    {
                        exposeHeart();
                    }
                    else if (HealthBelowPct(50) && heart_exposed == 1)
                    {
                        exposeHeart();
                    }
                    else if (HealthBelowPct(25) && heart_exposed == 2)
                    {
                        exposeHeart();
                    }

                    DoMeleeAttackIfReady();
                }
                else
                {
                    //Start summoning adds
                    if (uiSpawnAddTimer <= diff)
                    {
                        //DoScriptText(SAY_SUMMON, me);

                        // Spawn Pummeller
                        switch (rand() % 4)
                        {
                            case 0: me->SummonCreature(NPC_XM024_PUMMELLER, LR_X, LR_Y, SPAWN_Z, 0, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 3000); break;
                            case 1: me->SummonCreature(NPC_XM024_PUMMELLER, LL_X, LL_Y, SPAWN_Z, 0, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 3000); break;
                            case 2: me->SummonCreature(NPC_XM024_PUMMELLER, UR_X, UR_Y, SPAWN_Z, 0, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 3000); break;
                            case 3: me->SummonCreature(NPC_XM024_PUMMELLER, UL_X, UL_Y, SPAWN_Z, 0, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 3000); break;
                        }

                        // Spawn 5 Scrapbots
                        switch(rand() % 4)
                        {
                            case 0: 
                                for (int8 n = 0; n < 5; n++)
                                    me->SummonCreature(NPC_XS013_SCRAPBOT, irand(LR_X - 3, LR_X + 3), irand(LR_Y - 3, LR_Y + 3), SPAWN_Z, 0, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 3000); break;
                            case 1: 
                                for (int8 n = 0; n < 5; n++)
                                    me->SummonCreature(NPC_XS013_SCRAPBOT, irand(LL_X - 3, LL_X + 3), irand(LL_Y - 3, LL_Y + 3), SPAWN_Z, 0, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 3000); break;
                            case 2: 
                                for (int8 n = 0; n < 5; n++)
                                    me->SummonCreature(NPC_XS013_SCRAPBOT, irand(UR_X - 3, UR_X + 3), irand(UR_Y - 3, UR_Y + 3), SPAWN_Z, 0, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 3000); break;
                            case 3: 
                                for (int8 n = 0; n < 5; n++)
                                    me->SummonCreature(NPC_XS013_SCRAPBOT, irand(UL_X - 3, UL_X + 3), irand(UL_Y - 3, UL_Y + 3), SPAWN_Z, 0, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 3000); break;
                        }

                        // Spawn Bombs
                        switch (rand() % 4)
                        {
                            case 0: me->SummonCreature(NPC_XE321_BOOMBOT, LR_X, LR_Y, SPAWN_Z, 0, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 3000); break;
                            case 1: me->SummonCreature(NPC_XE321_BOOMBOT, LL_X, LL_Y, SPAWN_Z, 0, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 3000); break;
                            case 2: me->SummonCreature(NPC_XE321_BOOMBOT, UR_X, UR_Y, SPAWN_Z, 0, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 3000); break;
                            case 3: me->SummonCreature(NPC_XE321_BOOMBOT, UL_X, UL_Y, SPAWN_Z, 0, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 3000); break;
                        }

                        uiSpawnAddTimer = TIMER_SPAWN_ADD;
                    } else uiSpawnAddTimer -= diff;

                    // Is the phase over?
                    if (uiHeartPhaseTimer <= diff)
                    {
                        DoScriptText(SAY_HEART_CLOSED, me);
                        me->HandleEmoteCommand(EMOTE_ONESHOT_EMERGE);
                        SetPhaseOne();
                    }
                    else uiHeartPhaseTimer -= diff;
                }
            }
            else
            {
                // Adding life sparks when searing light debuff runs out if hard mode
                if (searing_light_active)
                {
                    if (uiSpawnLifeSparkTimer <= diff)
                    {
                        if (Unit *pSearingLightTarget = me->GetUnit(*me, uiSearingLightTarget))
                            pSearingLightTarget->SummonCreature(NPC_LIFE_SPARK, pSearingLightTarget->GetPositionX(), pSearingLightTarget->GetPositionY(), pSearingLightTarget->GetPositionZ(), 0, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 60000);
                        uiSpawnLifeSparkTimer = TIMER_SPAWN_LIFE_SPARK;
                        searing_light_active = false;
                    } else uiSpawnLifeSparkTimer -= diff;
                }

                // Adding void zones when gravity bomb debuff runs out if hard mode
                if (gravity_bomb_active)
                {
                    if (uiSpawnGravityBombTimer <= diff)
                    {
                        if (Unit *pGravityBombTarget = me->GetUnit(*me, uiGravityBombTarget))
                            DoCast(pGravityBombTarget, SPELL_VOID_ZONE);
                        uiSpawnGravityBombTimer = TIMER_SPAWN_GRAVITY_BOMB;
                        gravity_bomb_active = false;
                    } else uiSpawnGravityBombTimer -= diff;
                }

                DoMeleeAttackIfReady();
            }

            // Enrage stuff
            if (!enraged)
                if (uiEnrageTimer <= diff)
                {
                    DoScriptText(SAY_BERSERK, me);
                    DoCast(me, SPELL_ENRAGE);
                    enraged = true;
                } else uiEnrageTimer -= diff;
        }

        void exposeHeart()
        {
            // Make untargetable
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_DISABLE_MOVE);
            me->SetReactState(REACT_PASSIVE);
            me->SetStandState(UNIT_STAND_STATE_SUBMERGED);
            me->AttackStop();

            // Expose the heart npc
            if (Unit *Heart = vehicle->GetPassenger(0))
                Heart->ToCreature()->AI()->DoAction(0);
             
            // Start "end of phase 2 timer"
            uiHeartPhaseTimer = TIMER_HEART_PHASE;

            // Phase 2 has offically started
            phase = 2;
            heart_exposed++;

            // Reset the add spawning timer
            uiSpawnAddTimer = TIMER_SPAWN_ADD;

            DoScriptText(SAY_HEART_OPENED, me);
            me->MonsterTextEmote(EMOTE_HEART, 0, true);
            me->HandleEmoteCommand(EMOTE_ONESHOT_SUBMERGE);
        }

        void SetPhaseOne()
        {
            uiSearingLightTimer = TIMER_SEARING_LIGHT / 2;
            uiGravityBombTimer = TIMER_GRAVITY_BOMB;
            uiTympanicTantrumTimer = TIMER_TYMPANIC_TANTRUM / 2;
            uiSpawnAddTimer = TIMER_SPAWN_ADD;

            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_DISABLE_MOVE);
            me->SetReactState(REACT_AGGRESSIVE);
            me->SetStandState(UNIT_STAND_STATE_STAND);
            DoZoneInCombat();
            phase = 1;
        }
    };
};

/*-------------------------------------------------------
 *
 *        XT-002 HEART
 *
 *///----------------------------------------------------
class mob_xt002_heart : public CreatureScript
{
public:
    mob_xt002_heart() : CreatureScript("mob_xt002_heart") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new mob_xt002_heartAI(pCreature);
    }

    struct mob_xt002_heartAI : public ScriptedAI
    {
        mob_xt002_heartAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_GRIP, true);
        }

        uint32 uiExposeTimer;
        bool Exposed;

        void JustDied(Unit* /*victim*/)
        {
            if (Unit* pXT002 = me->ToTempSummon()->GetSummoner())
                pXT002->ToCreature()->AI()->DoAction(ACTION_ENTER_HARD_MODE);

            me->DespawnOrUnsummon();
        }

        void UpdateAI(const uint32 diff)
        {
            if (Exposed)
            {
                if (!me->HasAura(SPELL_EXPOSED_HEART))
                {
                    Exposed = false;
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                    me->RemoveAllAuras();
                    me->SetFullHealth();
                    me->ChangeSeat(0);
                }
            }
            else
            {
                if (!me->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE))
                {
                    if (uiExposeTimer <= diff)
                    {
                        Exposed = true;
                        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                        DoCast(me, SPELL_EXPOSED_HEART, true);
                    }
                    else uiExposeTimer -= diff;
                }
            }
        }

        void Reset()
        {
            Exposed = false;
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
        }

        void DamageTaken(Unit *pDone, uint32 &damage)
        {
            if (Unit *pXT002 = me->ToTempSummon()->GetSummoner())
            {
                if (damage > me->GetHealth())
                    damage = me->GetHealth();
                
                if (pDone)
                    pDone->DealDamage(pXT002, damage);
            }
        }

        void DoAction(const int32 action)
        {
            if (action == 0)
            {
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                me->ChangeSeat(1);
                uiExposeTimer = 3500;
            }
        }
    };

};

/*-------------------------------------------------------
 *
 *        XS-013 SCRAPBOT
 *
 *///----------------------------------------------------
class mob_scrapbot : public CreatureScript
{
public:
    mob_scrapbot() : CreatureScript("mob_scrapbot") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new mob_scrapbotAI(pCreature);
    }

    struct mob_scrapbotAI : public ScriptedAI
    {
        mob_scrapbotAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            m_pInstance = me->GetInstanceScript();
        }

        InstanceScript* m_pInstance;
        bool repaired;

        void Reset()
        {
            me->SetReactState(REACT_PASSIVE);
            repaired = false;

            if (Creature* pXT002 = me->GetCreature(*me, m_pInstance->GetData64(DATA_XT002)))
                me->AI()->AttackStart(pXT002);
        }

        void UpdateAI(const uint32 /*diff*/)
        {
            if (Creature* pXT002 = me->GetCreature(*me, m_pInstance->GetData64(DATA_XT002)))
            {
                if (!repaired && me->GetDistance2d(pXT002) <= 0.5)
                {
                    me->MonsterTextEmote(EMOTE_REPAIR, 0, true);

                    // Increase health with 1 percent
                    pXT002->CastSpell(me, SPELL_REPAIR, true);
                    repaired = true;

                    // Disable Nerf Engineering Achievement
                    if (pXT002->AI())
                        pXT002->AI()->DoAction(ACTION_DISABLE_NERF_ACHI);

                    // Despawns the scrapbot
                    me->ForcedDespawn(500);
                }
            }
        }
    };

};

/*-------------------------------------------------------
 *
 *        XM-024 PUMMELLER
 *
 *///----------------------------------------------------
class mob_pummeller : public CreatureScript
{
public:
    mob_pummeller() : CreatureScript("mob_pummeller") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new mob_pummellerAI(pCreature);
    }

    struct mob_pummellerAI : public ScriptedAI
    {
        mob_pummellerAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            m_pInstance = pCreature->GetInstanceScript();
        }

        InstanceScript* m_pInstance;
        uint32 uiArcingSmashTimer;
        uint32 uiTrampleTimer;
        uint32 uiUppercutTimer;

        void Reset()
        {
            uiArcingSmashTimer = TIMER_ARCING_SMASH;
            uiTrampleTimer = TIMER_TRAMPLE;
            uiUppercutTimer = TIMER_UPPERCUT;
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            if (me->IsWithinMeleeRange(me->getVictim()))
            {
                if (uiArcingSmashTimer <= diff)
                {
                    DoCast(me->getVictim(), SPELL_ARCING_SMASH);
                    uiArcingSmashTimer = TIMER_ARCING_SMASH;
                } else uiArcingSmashTimer -= diff;

                if (uiTrampleTimer <= diff)
                {
                    DoCast(me->getVictim(), SPELL_TRAMPLE);
                    uiTrampleTimer = TIMER_TRAMPLE;
                } else uiTrampleTimer -= diff;

                if (uiUppercutTimer <= diff)
                {
                    DoCast(me->getVictim(), SPELL_UPPERCUT);
                    uiUppercutTimer = TIMER_UPPERCUT;
                } else uiUppercutTimer -= diff;
            }

            DoMeleeAttackIfReady();
        }
    };

};

/*-------------------------------------------------------
 *
 *        XE-321 BOOMBOT
 *
 *///----------------------------------------------------
class mob_boombot : public CreatureScript
{
public:
    mob_boombot() : CreatureScript("mob_boombot") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new mob_boombotAI(pCreature);
    }

    struct mob_boombotAI : public ScriptedAI
    {
        mob_boombotAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            m_pInstance = pCreature->GetInstanceScript();
        }

        InstanceScript* m_pInstance;

        void Reset()
        {
            if (Creature* pXT002 = me->GetCreature(*me, m_pInstance->GetData64(DATA_XT002)))
                me->AI()->AttackStart(pXT002);
        }

        void UpdateAI(const uint32 diff)
        {
            if (Creature* pXT002 = me->GetCreature(*me, m_pInstance->GetData64(DATA_XT002)))
            {
                if (me->GetDistance2d(pXT002) <= 0.5 || HealthBelowPct(50))
                {
                    //Explosion
                    DoCast(me, SPELL_BOOM);
                }
            }
        }
    };

};

/*-------------------------------------------------------
 *
 *        VOID ZONE
 *
 *///----------------------------------------------------
class mob_void_zone : public CreatureScript
{
public:
    mob_void_zone() : CreatureScript("mob_void_zone") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new mob_void_zoneAI(pCreature);
    }

    struct mob_void_zoneAI : public Scripted_NoMovementAI
    {
        mob_void_zoneAI(Creature* pCreature) : Scripted_NoMovementAI(pCreature)
        {
            m_pInstance = pCreature->GetInstanceScript();
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED);
        }

        InstanceScript* m_pInstance;
        uint32 uiVoidZoneTimer;

        void Reset()
        {
            uiVoidZoneTimer = TIMER_VOID_ZONE;
        }

        void UpdateAI(const uint32 diff)
        {
            if (uiVoidZoneTimer <= diff)
            {
                DoCast(SPELL_VOID_ZONE_DAMAGE);
                uiVoidZoneTimer = TIMER_VOID_ZONE;
            } else uiVoidZoneTimer -= diff;
        }
    };

};

/*-------------------------------------------------------
 *
 *        LIFE SPARK
 *
 *///----------------------------------------------------
class mob_life_spark : public CreatureScript
{
public:
    mob_life_spark() : CreatureScript("mob_life_spark") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new mob_life_sparkAI(pCreature);
    }

    struct mob_life_sparkAI : public ScriptedAI
    {
        mob_life_sparkAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            m_pInstance = pCreature->GetInstanceScript();
        }

        InstanceScript* m_pInstance;
        uint32 uiShockTimer;

        void Reset()
        {
            DoCast(me, SPELL_STATIC_CHARGED);
            uiShockTimer = 0; // first one is immediate.
        }

        void UpdateAI(const uint32 diff)
        {
            if (m_pInstance && m_pInstance->GetBossState(BOSS_XT002) != IN_PROGRESS)
                me->DespawnOrUnsummon();

            if (uiShockTimer <= diff)
            {
                if (me->IsWithinMeleeRange(me->getVictim()))
                {
                    DoCast(me->getVictim(), SPELL_SHOCK);
                    uiShockTimer = TIMER_SHOCK;
                }
            }
            else uiShockTimer -= diff;
        }
    };

};

class spell_xt002_gravity_bomb : public SpellScriptLoader
{
    public:
        spell_xt002_gravity_bomb() : SpellScriptLoader("spell_xt002_gravity_bomb") { }

        class spell_xt002_gravity_bomb_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_xt002_gravity_bomb_SpellScript);

            void FilterTargets(std::list<Unit*>& unitList)
            {
                unitList.remove(GetTargetUnit());
            }

            void Register()
            {
                OnUnitTargetSelect += SpellUnitTargetFn(spell_xt002_gravity_bomb_SpellScript::FilterTargets, EFFECT_0, TARGET_DST_CASTER);
            }
        };

        SpellScript *GetSpellScript() const
        {
            return new spell_xt002_gravity_bomb_SpellScript();
        }
};


void AddSC_boss_xt002()
{
    new mob_xt002_heart();
    new mob_scrapbot();
    new mob_pummeller();
    new mob_boombot();
    new mob_void_zone();
    new mob_life_spark();
    new boss_xt002();
    new spell_xt002_gravity_bomb();

    if (VehicleSeatEntry* vehSeat = const_cast<VehicleSeatEntry*>(sVehicleSeatStore.LookupEntry(3846)))
        vehSeat->m_flags |= 0x400;
}
