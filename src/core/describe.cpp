#include "describe.hpp"

namespace gf {

std::string effectText(const Effect& e) {
    const std::string a = std::to_string(e.a);
    std::string s;
    switch (e.k) {
        case EK::Wound: s = "Wound " + a; break;
        case EK::Daze: s = "Daze " + a; break;
        case EK::Slow: s = "Slow " + a; break;
        case EK::Haste: s = "Haste " + a; break;
        case EK::Armor: s = "Armor " + a; break;
        case EK::Thorns: s = "Thorns " + a; break;
        case EK::Regen: s = "Regen " + a; break;
        case EK::Focus: s = "Focus"; break;
        case EK::Mark: s = "Mark"; break;
        case EK::Trap: s = "Trap " + a; break;
        case EK::Stun: s = "Stun"; break;
        case EK::Taunt: s = "Taunt " + a; break;
        case EK::Doom: s = "Doom " + a; break;
        case EK::Seal: s = "Seal"; break;
        case EK::Shield: s = "Shield " + a + "%"; break;
        case EK::Heal: s = "Heal " + a + "%"; break;
        case EK::Cleanse: s = "Cleanse"; break;
        case EK::ForceSwitch: s = "Force switch"; break;
        case EK::AtkMod: s = "ATK " + std::string(e.a > 0 ? "+" : "") + a + "% " + std::to_string(e.b) + "t"; break;
        case EK::DefMod: s = "DEF " + std::string(e.a > 0 ? "+" : "") + a + "% " + std::to_string(e.b) + "t"; break;
        case EK::SpeMod: s = "SPE " + std::string(e.a > 0 ? "+" : "") + a + "% " + std::to_string(e.b) + "t"; break;
        case EK::Evade: s = e.chance < 100 ? "Evade (" + std::to_string(e.chance) + "%)" : "Evade"; break;
        case EK::Counter: s = "Counter x" + std::to_string(e.a / 10.0).substr(0, 3); break;
        case EK::CounterHit: s = "Counter hit"; break;
        case EK::CounterAbsorb: s = "Absorb & heal"; break;
        case EK::Empower: s = "Next " + std::to_string(e.b) + " atk +" + a + "%"; break;
        case EK::NoMiss: s = "Can't miss"; break;
        case EK::PrioNext: s = "Next move Priority"; break;
        case EK::SlowAttackers: s = "Slows attackers"; break;
        case EK::ReflectDebuff: s = "Reflect debuff"; break;
        case EK::ReflectDamage: s = "Reflect hit"; break;
        case EK::SelfDamage: s = "Lose " + a + "% HP"; break;
        case EK::Plate: s = "+1 Plate"; break;
        case EK::Leech: s = "Leech"; break;
    }
    if (e.chance < 100 && e.k != EK::Evade) s += " (" + std::to_string(e.chance) + "%)";
    if (e.self) s += " [self]";
    return s;
}

std::string specialText(Sp sp) {
    switch (sp) {
        case Sp::SlipstreamBonus: return "+30% if you move first";
        case Sp::WardenBonus: return "+50% with Armor/Shield/Thorns";
        case Sp::BackdraftBonus: return "+50% vs Wounded";
        case Sp::CannonballBonus: return "+10% per Plate";
        case Sp::ShapeshiftSlam: return "1.2x damage you last took";
        case Sp::Mimic: return "Copy foe's last move";
        case Sp::InvertBuffs: return "Foe's buffs become debuffs";
        case Sp::StealBuff: return "Steal a buff";
        case Sp::CopyMods: return "Copy foe's stat changes";
        case Sp::ResetCooldowns: return "Reset your cooldowns";
        case Sp::WildCard: return "Random effect";
        case Sp::ChaosBolt: return "Random bonus effect";
        case Sp::PrismaticShatter: return "Strip buffs, +20% each";
        case Sp::Anarchy: return "Reset all statuses & cooldowns";
        case Sp::RebirthPyre: return "Revive once when KO'd";
        default: return "";
    }
}

std::string describeMove(const Move& m) {
    std::string s;
    auto add = [&](const std::string& p) { if (!p.empty()) { if (!s.empty()) s += " · "; s += p; } };
    if (m.damaging() || m.sp == Sp::ShapeshiftSlam) {
        std::string pw;
        if (m.multiHit()) {
            bool same = true;
            for (int h : m.hits) if (h != m.hits[0]) same = false;
            if (same) pw = "Power " + std::to_string(m.hits[0]) + " x" + std::to_string(m.hits.size());
            else { pw = "Power "; for (size_t i = 0; i < m.hits.size(); ++i) pw += (i ? "/" : "") + std::to_string(m.hits[i]); }
            if (m.randomTargets) pw += " (random foes)";
        } else {
            const int p = m.sp == Sp::ShapeshiftSlam ? 70 : (m.hits.empty() ? m.power : m.hits[0]);
            pw = "Power " + std::to_string(p);
        }
        add(pw);
        add(m.special ? "Special" : "Physical");
    } else {
        add("Utility");
    }
    if (m.tgt == Tgt::Spread) add("all foes");
    if (m.tgt == Tgt::Ally) add("ally or self");
    if (m.prio) add("Priority");
    if (m.charge) add(m.chargeEvade ? "2 turns (dodges)" : "2 turns");
    if (m.pivot) add("then swap out");
    if (m.ignoreDef) add("ignores " + std::to_string(m.ignoreDef) + "% DEF");
    if (m.ignoreDefenses) add("ignores Shield/Armor");
    if (m.crit) add(std::to_string(m.crit) + "% crit");
    if (m.drain) add("drain " + std::to_string(m.drain) + "%");
    if (m.recoil) add("recoil " + std::to_string(m.recoil) + "%");
    for (const auto& e : m.fx) add(effectText(e));
    add(specialText(m.sp));
    if (m.once) add("once per battle");
    if (m.cd > 0) add("CD " + std::to_string(m.cd));
    return s;
}

std::string moveSignature(const Move& m) {
    std::string o;
    o += m.damaging() ? (m.hits.empty() ? std::to_string(m.power) : "x" + std::to_string(m.hits.size())) : "U";
    o += "|cd" + std::to_string(m.cd) + "|t" + std::to_string(static_cast<int>(m.tgt));
    o += "|f" + std::to_string(m.prio) + std::to_string(m.pivot) + std::to_string(m.charge) + std::to_string(m.chargeEvade) + std::to_string(m.once) +
         std::to_string(m.special) + std::to_string(m.randomTargets) + std::to_string(m.ignoreDefenses);
    o += "|i" + std::to_string(m.ignoreDef) + "c" + std::to_string(m.crit) + "r" + std::to_string(m.recoil) + "d" + std::to_string(m.drain) + "s" + std::to_string(static_cast<int>(m.sp));
    for (int h : m.hits) o += "h" + std::to_string(h);
    for (const auto& e : m.fx) o += "|" + std::to_string(static_cast<int>(e.k)) + ":" + std::to_string(e.a) + ":" + std::to_string(e.b) + ":" + std::to_string(e.chance) + (e.self ? "s" : "t");
    return o;
}

const char* statLabel(int index) {
    static const char* names[6] = {"HP", "ATK", "DEF", "SPA", "SPD", "SPE"};
    return names[index < 0 ? 0 : index > 5 ? 5 : index];
}

}  // namespace gf
