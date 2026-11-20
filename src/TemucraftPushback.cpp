/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

// Spell pushback, as one of three rules:
//  0 Off:     damage never pushes a cast or a channel back (as in today's game).
//  1 Wrath:   the 3.3.5 rule, unchanged: 0.5 seconds a hit for casts, 25% of the duration for channels, twice at most.
//  2 Classic: every hit pushes back, with no limit, and a cast loses Classic.CastDelay ms a hit (1 second by default).
// Works from the damage hook, which runs before the core applies its pushback.

#include "Config.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "SpellInfo.h"
#include "WorldPacket.h"

namespace
{
    struct Settings
    {
        uint32 mode = 1;
        int32 castDelay = 1000;
    } cfg;

    // Reads protected members of Spell through member pointers taken in a derived class (no core change needed).
    struct SpellAccess : Spell
    {
        static uint8 Spell::* Count() { return &SpellAccess::m_delayAtDamageCount; }
        static int32 Spell::* Timer() { return &SpellAccess::m_timer; }
        static int32 Spell::* CastTime() { return &SpellAccess::m_casttime; }
    };

    // Pushback that the 3.3.5 rule does not give: how much of `delay` survives the caster's pushback reductions.
    int32 Reduced(Player* player, Spell* spell, int32 delay)
    {
        int32 reduce = 100;
        player->ApplySpellMod(spell->m_spellInfo->Id, SPELLMOD_NOT_LOSE_CASTING_TIME, reduce, spell);
        reduce += player->GetTotalAuraModifier(SPELL_AURA_REDUCE_PUSHBACK) - 100;
        if (reduce >= 100)
            return 0;
        AddPct(delay, -reduce);
        return delay;
    }
}

class TemucraftPushback_Unit : public UnitScript
{
public:
    TemucraftPushback_Unit() : UnitScript("TemucraftPushback_Unit") { }

    void OnDamage(Unit* attacker, Unit* victim, uint32& damage) override
    {
        if (cfg.mode == 1 || !victim || victim == attacker || !damage)
            return;
        if (!victim->IsPlayer())
            return;   // the core only pushes players back
        Spell* cast = victim->GetCurrentSpell(CURRENT_GENERIC_SPELL);
        Spell* channel = victim->GetCurrentSpell(CURRENT_CHANNELED_SPELL);
        if (cfg.mode == 0)
        {
            // the core skips pushback once a spell has been pushed back twice
            if (cast) cast->*SpellAccess::Count() = 2;
            if (channel) channel->*SpellAccess::Count() = 2;
            return;
        }
        // classic: the core stops after two pushbacks, so its count goes back to zero before every hit
        if (cast) cast->*SpellAccess::Count() = 0;
        if (channel) channel->*SpellAccess::Count() = 0;
        // and the part of a cast's delay over the core's 0.5 seconds
        if (!cast || cast->getState() != SPELL_STATE_PREPARING || cfg.castDelay <= 500)
            return;
        if (!(cast->m_spellInfo->InterruptFlags & SPELL_INTERRUPT_FLAG_PUSH_BACK) || cast->m_spellInfo->HasAttribute(SPELL_ATTR6_NO_PUSHBACK))
            return;
        int32 extra = Reduced(victim->ToPlayer(), cast, cfg.castDelay - 500);
        int32& timer = cast->*SpellAccess::Timer();
        int32 castTime = cast->*SpellAccess::CastTime();
        if (extra <= 0 || timer >= castTime)
            return;
        extra = std::min(extra, castTime - timer);
        timer += extra;
        WorldPacket data(SMSG_SPELL_DELAYED, 8 + 4);
        data << victim->GetPackGUID();
        data << uint32(extra);
        victim->SendMessageToSet(&data, true);
    }
};

class TemucraftPushback_World : public WorldScript
{
public:
    TemucraftPushback_World() : WorldScript("TemucraftPushback_World") { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        cfg.mode = std::min<uint32>(sConfigMgr->GetOption<uint32>("Temucraft.Pushback.Mode", 1), 2);
        cfg.castDelay = sConfigMgr->GetOption<int32>("Temucraft.Pushback.Classic.CastDelay", 1000);
    }
};

void AddTemucraftPushbackScripts()
{
    new TemucraftPushback_Unit();
    new TemucraftPushback_World();
}
