#!/usr/bin/env python3
"""Generates the expansion roster (ids 48..149) as C++ include files.

Everything creative lives in the tables below (names, concepts, stats, areas, sprite parts). Roles expand into four
balanced moves per friend; rare friends carry hand-written moves. Run from the repo root:

    python3 tools/gen_roster.py

Outputs: src/core/roster_ext.inc, src/core/sprite_lines.inc and a markdown table appended to ROSTER.md.
"""
import re, sys

FIRST_ID = 48          # ids 0..47 are written by hand in data.cpp
FIRST_LINE = 27        # sprite/habitat line index of the first generated line
AREAS = ['Meadow', 'Woods', 'Caverns', 'Shore', 'Peaks', 'Ruins']
KIND = {'wound': 'Wound', 'daze': 'Daze', 'slow': 'Slow', 'haste': 'Haste', 'armor': 'Armor', 'thorns': 'Thorns', 'regen': 'Regen',
        'focus': 'Focus', 'mark': 'Mark', 'trap': 'Trap', 'stun': 'Stun', 'taunt': 'Taunt', 'doom': 'Doom', 'seal': 'Seal',
        'shield': 'Shield', 'heal': 'Heal', 'cleanse': 'Cleanse', 'fswitch': 'ForceSwitch', 'atk': 'AtkMod', 'def': 'DefMod',
        'spd': 'SpdMod', 'evade': 'Evade', 'counter': 'Counter', 'counterhit': 'CounterHit', 'absorb': 'CounterAbsorb',
        'empower': 'Empower', 'nomiss': 'NoMiss', 'prionext': 'PrioNext', 'slowatk': 'SlowAttackers', 'refdebuff': 'ReflectDebuff',
        'refdmg': 'ReflectDamage', 'selfdmg': 'SelfDamage', 'plate': 'Plate', 'leech': 'Leech'}

def eff(kind, a=0, b=0, ch=100): return (kind, a, b, ch)

def mvx(name, tier='U', cd=0, tgt=None, fx=(), me=(), prio=False, pivot=False, charge=None, once=False, idef=0, crit=0,
        recoil=0, drain=0, hits=None, rand=False, perhit=False, ign=False, sp=None, mimic=0):
    if hits: s = f'mv("{name}", 0)'
    elif tier in ('L', 'M', 'H'): s = f'mv("{name}", {tier})'
    else: s = f'util("{name}")'
    if cd: s += f'.CD({cd})'
    if tgt: s += f'.T(Tgt::{tgt})'
    if hits: s += '.H({' + ','.join(map(str, hits)) + '})'
    if rand: s += '.Rand()'
    if perhit: s += '.PerHit()'
    if prio: s += '.Prio()'
    if pivot: s += '.Pivot()'
    if charge is not None: s += f'.Charge({"true" if charge else "false"})'
    if once: s += '.Once()'
    if idef: s += f'.IgnoreDef({idef})'
    if ign: s += '.IgnoreDefenses()'
    if crit: s += f'.Crit({crit})'
    if recoil: s += f'.Recoil({recoil})'
    if drain: s += f'.Drain({drain})'
    if mimic: s += f'.Mimic({mimic})'
    if sp: s += f'.Special(Sp::{sp})'
    for k, a, b, ch in fx: s += f'.Fx(EK::{KIND[k]}, {a}, {b}, {ch})'
    for k, a, b, ch in me: s += f'.Me(EK::{KIND[k]}, {a}, {b}, {ch})'
    return s

# ------------------------------------------------------------------ role templates
def debuff(D, st, weak=False):
    if weak:
        return [eff(D, 1 if D in ('daze', 'slow') else 2, 0, 30)] if D not in ('mark', 'seal', 'trap') else [eff(D, 2 if D == 'trap' else 0, 0, 40)]
    n = 2 if st < 3 else 3
    return {'wound': [eff('wound', n)], 'daze': [eff('daze', n)], 'slow': [eff('slow', n)], 'trap': [eff('trap', 2)],
            'mark': [eff('mark')], 'seal': [eff('seal')], 'doom': [eff('doom', 3)]}[D]

def buff(B, st):
    return {'haste': [eff('haste', 3)], 'armor': [eff('armor', 2 if st < 3 else 3)], 'focus': [eff('focus')],
            'regen': [eff('regen', 3 if st < 3 else 4)], 'shield': [eff('shield', 15 + 5 * st)], 'thorns': [eff('thorns', 3)],
            'evade': [eff('evade')], 'atk': [eff('atk', 30, 3)], 'empower': [eff('empower', 60, 1)]}[B]

def tier0(st): return 'L' if st == 1 else 'M'

def role_moves(role, N, st, D, B):
    t0 = tier0(st)
    if role == 'striker':
        return [mvx(N[0], t0, fx=debuff(D, st, True)), mvx(N[1], 'U', 3, 'Self', me=buff(B, st)), mvx(N[2], 'H', 2, fx=debuff(D, st)),
                mvx(N[3], 'U', 3, 'Self', prio=True, me=[eff('focus')]) if st < 3 else mvx(N[3], 'U', 4, 'Self', me=[eff('evade'), eff('empower', 60, 1)])]
    if role == 'tank':
        slot2 = [mvx(N[2], 'U', 3, 'Self', me=[eff('thorns', 3)]), mvx(N[2], 'U', 3, 'Self', me=[eff('counter', 15)]),
                 mvx(N[2], 'U', 3, 'Self', me=[eff('counterhit', 70, 2)])][st - 1]
        return [mvx(N[0], t0, fx=debuff(D, st, True)), mvx(N[1], 'U', 3, 'Self', me=[eff('armor', 2 if st < 3 else 3)] + ([eff('cleanse')] if st == 3 else [])), slot2,
                mvx(N[3], 'U', 4, 'Self', me=[eff('heal', 20 + 5 * st)] + (buff(B, 1) if B in ('regen', 'shield', 'armor') else []))]
    if role == 'support':
        return [mvx(N[0], t0, fx=debuff(D, st, True)), mvx(N[1], 'U', 3, 'Ally', fx=[eff('heal', 15 + 5 * st)]), mvx(N[2], 'U', 3, 'Ally', fx=buff(B, st)),
                mvx(N[3], 'U', 4, 'Ally', fx=[eff('shield', 15 + 5 * st), eff('cleanse')])]
    if role == 'controller':
        return [mvx(N[0], t0, fx=debuff(D, st, True)), mvx(N[1], 'U', 3, fx=debuff(D, st)), mvx(N[2], 'U', 4, 'Spread', fx=debuff(D, st)),
                mvx(N[3], 'U', 3, fx=[eff('trap', 2)] + debuff(D, 1, True)) if st == 1 else mvx(N[3], 'U', 5, fx=[eff('stun')])]
    if role == 'trickster':
        s3 = [mvx(N[3], 'U', 3, fx=[eff('fswitch')]), mvx(N[3], 'U', 3, sp='StealBuff'), mvx(N[3], 'U', 2, mimic=70)][st - 1]
        return [mvx(N[0], 'L' if st < 3 else 'M', prio=True), mvx(N[1], 'U', 3, 'Self', me=[eff('evade'), eff('haste', 1 + (st >= 2))]),
                mvx(N[2], 'U', 3, fx=[eff('mark')] + ([eff(D, 2 if D != 'trap' else 2)] if D not in ('mark',) else [])), s3]
    if role == 'bruiser':
        return [mvx(N[0], 'M'), mvx(N[1], 'H', 2, recoil=8), mvx(N[2], 'U', 3 if st < 3 else 4, 'Self', me=[eff('atk', 20 + 10 * st, 3)]),
                mvx(N[3], 'M', 3, drain=40, fx=debuff(D, st, True))]
    if role == 'spreader':
        return [mvx(N[0], 'L'), mvx(N[1], 'L', 2, 'Spread', fx=debuff(D, 1, True) if D not in ('wound',) else [eff('wound', 2, 0, 40)]),
                mvx(N[2], 'L' if st == 1 else 'M', 3, 'Spread', fx=debuff(D, st)), mvx(N[3], 'U', 3, 'Self', me=buff(B, st))]
    if role == 'charger':
        return [mvx(N[0], 'M'), mvx(N[1], 'M' if st == 1 else 'H', 3, charge=True), mvx(N[2], 'U', 3, 'Self', me=buff(B, st)),
                mvx(N[3], 'M', 3, fx=[eff('trap', 2)] + (debuff(D, st, True) if st == 3 and D != 'trap' else []))]
    if role == 'cursed':
        return [mvx(N[0], 'M', fx=[eff('mark')]), mvx(N[1], 'U', 3, fx=[eff('mark')] + debuff(D, 1, True)) if st == 1 else mvx(N[1], 'U', 4, fx=[eff('doom', 3)]),
                mvx(N[2], 'U', 3, fx=[eff('seal')]), mvx(N[3], 'U', 4, 'Spread', fx=[eff('daze', 2)] if D == 'daze' else debuff(D, st))]
    if role == 'regenerator':
        return [mvx(N[0], t0, fx=debuff(D, st, True)), mvx(N[1], 'U', 3, 'Self', me=[eff('regen', 3 if st < 3 else 4)] + ([eff('cleanse')] if st >= 2 else [])),
                mvx(N[2], 'U', 4, 'Ally', fx=[eff('regen', 3), eff('heal', 10)]), mvx(N[3], 'M', 3, drain=40 + 10 * (st >= 2))]
    if role == 'swarm':
        return [mvx(N[0], 'L'), mvx(N[1], 'U', 2, hits=[40, 40] if st == 1 else [40, 40, 40], rand=True, perhit=True, fx=[eff(D, 2 if D != 'daze' else 1, 0, 25)]),
                mvx(N[2], 'L', 3, 'Spread', fx=debuff(D, st)), mvx(N[3], 'U', 4, 'Ally', fx=buff(B, st))]
    if role == 'bomber':
        return [mvx(N[0], 'M'), mvx(N[1], 'H', 2, recoil=10, fx=debuff(D, st)), mvx(N[2], 'U', 4, 'Self', me=[eff('selfdmg', 20), eff('empower', 60 + 10 * st, 1)]),
                mvx(N[3], 'M', 3, 'Spread', fx=debuff(D, st))]
    if role == 'counter':
        return [mvx(N[0], t0), mvx(N[1], 'U', 3, 'Self', me=[eff('counter', 20)] if st < 3 else [eff('counterhit', 70, 2)]),
                mvx(N[2], 'U', 3, 'Self', me=[eff('armor', 2), eff('thorns', 2)]), mvx(N[3], 'M', 3, fx=debuff(D, st, True))]
    raise ValueError(role)

# base pips (hp, atk, def, spd) and which two stats gain a pip at stage 2 and stage 3
ROLE_STATS = {
    'striker': ((2, 3, 2, 4), (0, 1), (1, 3)), 'tank': ((4, 2, 3, 1), (2, 1), (0, 2)), 'support': ((3, 1, 2, 3), (0, 2), (2, 3)),
    'controller': ((3, 2, 2, 3), (0, 3), (2, 1)), 'trickster': ((2, 2, 2, 4), (1, 2), (0, 3)), 'bruiser': ((3, 4, 2, 2), (0, 1), (2, 3)),
    'spreader': ((3, 3, 2, 3), (1, 0), (2, 3)), 'charger': ((3, 3, 2, 3), (1, 0), (2, 3)), 'cursed': ((2, 3, 2, 4), (0, 1), (2, 0)),
    'regenerator': ((4, 2, 3, 2), (2, 0), (1, 3)), 'swarm': ((2, 3, 1, 5), (0, 2), (1, 0)), 'bomber': ((2, 5, 1, 3), (0, 3), (2, 0)),
    'counter': ((3, 2, 4, 2), (0, 2), (1, 0)),
}
def stats_for(role, st):
    base, g2, g3 = ROLE_STATS[role]
    v = list(base)
    for g in ([g2] if st >= 2 else []) + ([g3] if st >= 3 else []):
        for i in g: v[i] = min(5, v[i] + 1)
    return v

# ------------------------------------------------------------------ the creative part: lines
# (names, role, debuff D, buff B, areas bitmask, palette body/belly/accent/accent2, bodies, parts (kind, colour slot, min stage), eye style, moves per stage)
# colour slots: 0 body 1 belly 2 accent 3 accent2 4 white 5 dark
THREE = [
 dict(names=('Bumblet', 'Honeybrute', 'Queenbuzz'), role='swarm', D='wound', B='haste', hab=3,
      pal=((240, 200, 50), (255, 235, 140), (60, 50, 60), (190, 220, 250)), body=('Round', 'Round', 'Pear'),
      parts=[('WingsInsect', 3, 1), ('Stripes', 2, 1), ('Antennae', 2, 1), ('Stinger', 2, 1), ('Crown', 2, 3)], eye=0,
      moves=[['Tiny Sting', 'Buzz Barrage', 'Pollen Cloud', 'Wing Hum'], ['Stinger Jab', 'Hive Flurry', 'Honey Splash', 'Drone Rally'], ['Royal Stinger', 'Swarm Volley', 'Wax Storm', "Queen's Decree"]]),
 dict(names=('Mudpup', 'Bogbark', 'Swampcrown'), role='regenerator', D='slow', B='regen', hab=10,
      pal=((140, 120, 76), (196, 176, 120), (100, 150, 80), (80, 64, 44)), body=('Wide', 'Wide', 'Wide'),
      parts=[('Warts', 3, 1), ('EarsRound', 0, 1), ('TailLong', 3, 2), ('CrestLeaf', 2, 3)], eye=0,
      moves=[['Mud Pie', 'Wallow', 'Muddy Paws', 'Slurp Silt'], ['Swamp Snap', 'Peat Blanket', 'Bog Tonic', 'Mire Drain'], ['Reed Whip', 'Lotus Bloom', 'Marsh Blessing', 'Quagmire Grasp']]),
 dict(names=('Flurrcub', 'Snowmane', 'Blizzarion'), role='controller', D='slow', B='armor', hab=16,
      pal=((210, 232, 250), (245, 250, 255), (130, 180, 230), (90, 140, 210)), body=('Round', 'Round', 'Round'),
      parts=[('EarsRound', 0, 1), ('Mane', 2, 2), ('CrestIce', 2, 2), ('Sparkles', 4, 1), ('Fangs', 4, 3)], eye=0,
      moves=[['Snowball', 'Chilly Nuzzle', 'Flurry Spray', 'Drift Pin'], ['Frost Nip', 'Numbing Roar', 'Whiteout Gust', 'Deep Freeze'], ['Glacier Mane Slash', 'Hoarfrost Howl', 'Polar Cyclone', 'Absolute Zero']]),
 dict(names=('Thornet', 'Rosewhip', 'Bramblequeen'), role='bruiser', D='wound', B='thorns', hab=3,
      pal=((100, 160, 80), (190, 220, 150), (230, 70, 100), (70, 110, 60)), body=('Round', 'Round', 'Tall'),
      parts=[('Flower', 2, 1), ('Spikes', 3, 1), ('TailLeaf', 3, 2), ('Crown', 2, 3)], eye=0,
      moves=[['Briar Scratch', 'Reckless Thorn Rush', 'Bloom Rage', 'Sap Sting'], ['Vine Lash', 'Barbed Charge', 'Petal Fury', 'Nectar Leech'], ["Queen's Lash", 'Bramble Stampede', 'Crimson Bloom', 'Thorned Embrace']]),
 dict(names=('Cometling', 'Starfall', 'Novadrake'), role='bomber', D='wound', B='empower', hab=48,
      pal=((70, 60, 140), (150, 130, 220), (255, 230, 120), (180, 200, 255)), body=('Round', 'Pear', 'Tall'),
      parts=[('CrestStar', 2, 1), ('TailFish', 3, 1), ('Sparkles', 4, 1), ('WingsFeather', 3, 2), ('Spikes', 2, 3)], eye=0,
      moves=[['Star Bonk', 'Shooting Charge', 'Stardust Surge', 'Meteor Shower'], ['Comet Tail', 'Falling Star', 'Supernova Wish', 'Asteroid Rain'], ['Nova Claw', 'Cosmic Crash', 'Stellar Overload', 'Event Horizon']]),
 dict(names=('Urchi', 'Spineurchin', 'Pricklord'), role='tank', D='wound', B='shield', hab=8,
      pal=((110, 70, 150), (190, 150, 220), (240, 120, 170), (70, 40, 100)), body=('Round', 'Round', 'Round'),
      parts=[('Spikes', 3, 1), ('Bubbles', 4, 1), ('Crown', 2, 3)], eye=0,
      moves=[['Spine Poke', 'Curl Tight', 'Prickle Coat', 'Tidepool Nap'], ['Needle Jab', 'Sea Spine Guard', 'Barbed Reprisal', 'Brine Recovery'], ['Trident Spike', 'Spiked Bulwark', 'Pincushion Retort', 'Abyssal Rest']]),
 dict(names=('Lavapup', 'Magmahound', 'Calderon'), role='bruiser', D='wound', B='atk', hab=36,
      pal=((90, 70, 70), (140, 110, 100), (255, 130, 40), (60, 40, 40)), body=('Round', 'Round', 'Wide'),
      parts=[('Cracks', 2, 1), ('EarsPoint', 0, 1), ('TailFlame', 2, 1), ('CrestFlame', 2, 2), ('HornsStraight', 2, 3)], eye=3,
      moves=[['Ember Chomp', 'Lava Lunge', 'Heat Surge', 'Scorch Gnaw'], ['Magma Maw', 'Molten Pounce', 'Core Rage', 'Cinder Drain'], ['Caldera Claw', 'Volcanic Tackle', 'Eruption Heart', 'Pyroclast Bite']]),
 dict(names=('Breezecub', 'Galelynx', 'Cycloneth'), role='trickster', D='daze', B='evade', hab=17,
      pal=((190, 225, 235), (240, 250, 250), (120, 200, 220), (90, 160, 200)), body=('Round', 'Round', 'Tall'),
      parts=[('EarsPoint', 0, 1), ('TailLong', 2, 1), ('Mane', 2, 2), ('Sparkles', 4, 2), ('WingsFeather', 4, 3)], eye=0,
      moves=[['Gust Swipe', 'Air Skip', 'Dizzy Draft', 'Shove Breeze'], ['Slipstream Slash', 'Tailwind Blur', 'Vertigo Gale', 'Pilfer Wind'], ['Cyclone Claw', 'Eye Wall Dodge', 'Whirling Daze', 'Echoing Gust']]),
 dict(names=('Chimey', 'Belltoll', 'Carillon'), role='support', D='daze', B='haste', hab=33,
      pal=((250, 210, 90), (255, 240, 170), (200, 140, 40), (255, 255, 255)), body=('Dome', 'Dome', 'Dome'),
      parts=[('Bell', 3, 1), ('Notes', 3, 1), ('Halo', 2, 3)], eye=0,
      moves=[['Ting Strike', 'Soothing Ring', 'Quickstep Chime', 'Clear Tone'], ['Clapper Bonk', 'Healing Peal', 'Tempo Toll', 'Sanctuary Note'], ['Resonant Smash', 'Choir of Mending', 'Allegro Cascade', 'Hymn of Warding']]),
 dict(names=('Shadowcub', 'Duskpanther', 'Nightstalker'), role='cursed', D='daze', B='evade', hab=34,
      pal=((60, 50, 90), (100, 90, 140), (200, 150, 255), (255, 230, 120)), body=('Round', 'Round', 'Tall'),
      parts=[('EarsPoint', 0, 1), ('Cloud', 0, 2), ('Fangs', 4, 3), ('TailLong', 2, 1)], eye=3,
      moves=[['Shade Pounce', 'Fading Whisper', 'Silence Paw', 'Gloom Mist'], ['Dusk Claw', 'Dread Countdown', 'Muzzle Shadow', 'Veil of Night'], ["Stalker's Mark", 'Doom Prowl', 'Hush of Ruin', 'Nightfall Shroud']]),
 dict(names=('Kelpie', 'Tidemane', 'Maelstrider'), role='charger', D='slow', B='evade', hab=8,
      pal=((60, 170, 190), (170, 230, 235), (40, 120, 180), (130, 230, 200)), body=('Round', 'Tall', 'Tall'),
      parts=[('EarsPoint', 3, 1), ('Mane', 3, 1), ('Fins', 2, 2), ('TailFish', 2, 1)], eye=0,
      moves=[['Wave Nip', 'Breaker Dive', 'Foam Shield', 'Rip Tether'], ['Surf Kick', 'Crest Leap', 'Sea Spray Guard', 'Undercurrent Bind'], ['Gale Hoof', 'Whirlpool Plunge', 'Brine Armor', 'Maelstrom Snare']]),
 dict(names=('Caterpix', 'Cocoonix', 'Papilion'), role='support', D='daze', B='regen', hab=3,
      pal=((150, 210, 90), (220, 245, 170), (255, 170, 200), (110, 160, 70)), body=('Serpent', 'Dome', 'Tall'),
      parts=[('Antennae', 3, 1), ('Spots', 3, 1), ('WingsInsect', 2, 3)], eye=0,
      moves=[['Silk Spit', 'Leaf Snack', 'Cozy Thread', 'Sticky Cocoon'], ['Cocoon Bump', 'Chrysalis Rest', 'Silken Blessing', 'Armored Wrap'], ['Wing Flutter', 'Scale Dust Heal', 'Rainbow Dust', 'Butterfly Barrier']]),
 dict(names=('Pricklet', 'Saguarock', 'Cactitan'), role='counter', D='wound', B='thorns', hab=5,
      pal=((100, 170, 100), (190, 225, 170), (255, 120, 160), (60, 110, 70)), body=('Round', 'Tall', 'Tall'),
      parts=[('Spikes', 3, 1), ('Flower', 2, 1), ('Arms', 0, 2)], eye=0,
      moves=[['Needle Flick', 'Spiny Stance', 'Desert Coat', 'Sting Bloom'], ['Arm Swing', 'Cactus Riposte', 'Dry Fortress', 'Flower Barb'], ['Titan Arm', 'Sunbaked Reprisal', 'Oasis Bulwark', 'Thousand Spines']]),
 dict(names=('Snaillet', 'Stoneshell', 'Mountaingast'), role='tank', D='slow', B='regen', hab=5,
      pal=((190, 160, 110), (235, 215, 170), (150, 110, 90), (120, 170, 120)), body=('Round', 'Round', 'Round'),
      parts=[('ShellBack', 2, 1), ('Antennae', 0, 1), ('Spikes', 3, 3)], eye=0,
      moves=[['Slimy Nudge', 'Withdraw', 'Slick Shell', 'Slow Recovery'], ['Shell Ram', 'Stone Retreat', 'Granite Reprisal', 'Mineral Rest'], ['Boulder Crash', 'Fortress Withdrawal', 'Avalanche Reprisal', "Mountain's Patience"]]),
]

TWO = [
 dict(names=('Dandycub', 'Puffmane'), role='spreader', D='daze', B='haste', hab=1,
      pal=((240, 205, 90), (255, 238, 170), (255, 255, 255), (230, 170, 50)), body=('Round', 'Round'),
      parts=[('Mane', 4, 1), ('EarsRound', 0, 1), ('Sparkles', 4, 2), ('TailFluffy', 4, 1)], eye=0,
      moves=[['Fluff Tackle', 'Seed Scatter', 'Wish Gust', 'Breeze Ride'], ['Mane Whip', 'Pappus Storm', 'Dandelion Blizzard', 'Wishful Sprint']]),
 dict(names=('Leapkin', 'Bounderhare'), role='trickster', D='slow', B='haste', hab=17,
      pal=((210, 190, 170), (250, 240, 230), (255, 170, 180), (150, 120, 100)), body=('Round', 'Tall'),
      parts=[('EarsLong', 0, 1), ('TailFluffy', 4, 1), ('Whiskers', 5, 1)], eye=0,
      moves=[['Hop Tap', 'Spring Skip', 'Carrot Trick', 'Bunny Bounce'], ['Kick Flurry', 'Moonlit Leap', 'Lucky Foot Hex', 'Burrow Swap']]),
 dict(names=('Spotter', 'Rubybeetle'), role='counter', D='wound', B='armor', hab=3,
      pal=((220, 60, 60), (255, 150, 140), (30, 30, 40), (255, 255, 255)), body=('Dome', 'Dome'),
      parts=[('Spots', 2, 1), ('Antennae', 2, 1), ('Plates', 3, 2)], eye=0,
      moves=[['Dot Nip', 'Shell Clasp', 'Lucky Spots', 'Aphid Snack'], ['Ruby Ram', 'Carapace Riposte', 'Polished Shell', 'Garnet Stinger']]),
 dict(names=('Cloudlamb', 'Stormram'), role='support', D='daze', B='shield', hab=17,
      pal=((245, 245, 252), (255, 255, 255), (150, 190, 240), (110, 150, 210)), body=('Round', 'Round'),
      parts=[('Cloud', 4, 1), ('HornsCurl', 2, 2), ('EarsRound', 0, 1), ('Sparkles', 2, 2)], eye=0,
      moves=[['Fleece Bump', 'Soft Mist', 'Cumulus Cushion', 'Rainy Day'], ['Thunder Ram', 'Rain Tonic', 'Static Fleece', 'Squall Shelter']]),
 dict(names=('Acornet', 'Oakguard'), role='tank', D='slow', B='armor', hab=2,
      pal=((170, 120, 70), (220, 190, 140), (90, 160, 80), (110, 80, 50)), body=('Round', 'Tall'),
      parts=[('CrestLeaf', 2, 1), ('TailFluffy', 0, 1), ('Antlers', 3, 2)], eye=0,
      moves=[['Nut Throw', 'Bark Up', 'Squirrel Away', 'Forest Snack'], ['Branch Swing', 'Ironwood Skin', 'Splinter Reprisal', 'Sapwell']]),
 dict(names=('Foxfire', 'Kitsunar'), role='trickster', D='daze', B='evade', hab=34,
      pal=((235, 150, 70), (255, 225, 180), (120, 200, 255), (255, 255, 255)), body=('Round', 'Tall'),
      parts=[('EarsPoint', 0, 1), ('TailFlame', 2, 1), ('CrestFlame', 2, 2), ('Sparkles', 2, 1)], eye=0,
      moves=[['Cinder Flick', 'Spirit Step', 'Will-o-Wisp Lure', 'Fox Trick'], ['Flame Whisker', 'Nine-Tail Blur', 'Phantom Fire Daze', 'Shape Swap']]),
 dict(names=('Fireflit', 'Lanternoth'), role='support', D='daze', B='focus', hab=2,
      pal=((200, 230, 110), (240, 250, 180), (255, 220, 90), (90, 110, 60)), body=('Round', 'Round'),
      parts=[('WingsInsect', 4, 1), ('Lantern', 2, 1), ('Antennae', 3, 1)], eye=0,
      moves=[['Spark Peck', 'Glow Mend', 'Guiding Flicker', 'Safe Light'], ['Moth Dust', 'Warm Lantern Heal', 'Beacon Focus', 'Haven Glow']]),
 dict(names=('Geodot', 'Geodrake'), role='bruiser', D='slow', B='armor', hab=4,
      pal=((130, 120, 150), (190, 180, 210), (160, 100, 220), (255, 255, 255)), body=('Round', 'Wide'),
      parts=[('Spikes', 2, 1), ('CrestGem', 2, 1), ('Claws', 3, 2)], eye=0,
      moves=[['Pebble Punch', 'Rock Roll', 'Crystal Rage', 'Mineral Munch'], ['Geode Smash', 'Landquake Dive', 'Amethyst Fury', 'Shard Siphon']]),
 dict(names=('Glowworm', 'Lumenwyrm'), role='support', D='daze', B='focus', hab=4,
      pal=((230, 240, 160), (250, 255, 210), (120, 255, 200), (60, 140, 100)), body=('Serpent', 'Serpent'),
      parts=[('Lantern', 2, 1), ('Antennae', 3, 1), ('Sparkles', 2, 2)], eye=0,
      moves=[['Wriggle Tap', 'Soft Glow Heal', 'Aim Lantern', 'Lumen Cover'], ['Coil Slam', 'Radiant Mend', 'Spotlight Focus', 'Cavern Shelter']]),
 dict(names=('Magnetoad', 'Ferrofrog'), role='controller', D='slow', B='armor', hab=36,
      pal=((120, 130, 180), (190, 200, 230), (230, 60, 60), (70, 80, 200)), body=('Wide', 'Wide'),
      parts=[('Magnet', 2, 1), ('Warts', 3, 1), ('EarsRound', 0, 2)], eye=0,
      moves=[['Static Tongue', 'Magnetize', 'Pull Field', 'Polarity Lock'], ['Iron Tongue', 'Attraction Surge', 'Flux Storm', 'Magnet Prison']]),
 dict(names=('Tadwink', 'Rainfrog'), role='regenerator', D='daze', B='regen', hab=8,
      pal=((90, 190, 150), (190, 240, 210), (255, 210, 90), (40, 120, 100)), body=('Blob', 'Round'),
      parts=[('TailFish', 3, 1), ('Bubbles', 4, 1), ('Fins', 2, 2)], eye=0,
      moves=[['Tail Slap', 'Puddle Hop', 'Lily Pad Rest', 'Wriggle Siphon'], ['Rain Tongue', 'Downpour Rest', 'Thunder Croak Bless', 'Monsoon Siphon']]),
 dict(names=('Jellybloom', 'Medusar'), role='cursed', D='daze', B='evade', hab=8,
      pal=((240, 170, 220), (255, 215, 240), (160, 200, 255), (200, 110, 190)), body=('Blob', 'Blob'),
      parts=[('Tentacles', 3, 1), ('Bubbles', 4, 1), ('Sparkles', 2, 2)], eye=0,
      moves=[['Tentacle Tap', 'Sting Countdown', 'Numbing Pulse', 'Drift Fog'], ['Venom Lash', 'Lethargic Doom', 'Paralytic Hush', 'Bioluminous Haze']]),
 dict(names=('Scuttler', 'Scarabrute'), role='bruiser', D='wound', B='atk', hab=12,
      pal=((230, 110, 80), (255, 190, 160), (250, 230, 200), (150, 60, 50)), body=('Wide', 'Wide'),
      parts=[('Claws', 3, 1), ('Plates', 1, 2), ('Spikes', 3, 2)], eye=2,
      moves=[['Claw Snip', 'Sidestep Slam', 'Pincer Pump', 'Brine Bite'], ['Pincer Crush', 'Beachhead Charge', 'Shell Rage', 'Tideline Drain']]),
 dict(names=('Yetilet', 'Frostbrute'), role='bruiser', D='slow', B='atk', hab=16,
      pal=((235, 240, 250), (255, 255, 255), (120, 170, 230), (90, 120, 170)), body=('Round', 'Tall'),
      parts=[('EarsRound', 0, 1), ('Mane', 0, 2), ('Claws', 2, 1), ('Fangs', 4, 2), ('Sparkles', 2, 1)], eye=0,
      moves=[['Snow Swat', 'Avalanche Roll', 'Chest Thump', 'Icicle Gnaw'], ['Glacier Fist', 'Whiteout Rush', 'Yeti Fury', 'Permafrost Chomp']]),
 dict(names=('Eaglet', 'Thunderhawk'), role='charger', D='daze', B='evade', hab=16,
      pal=((150, 120, 90), (240, 230, 220), (250, 200, 60), (70, 50, 40)), body=('Round', 'Round'),
      parts=[('Beak', 2, 1), ('WingsFeather', 3, 1), ('CrestFeather', 4, 2)], eye=2,
      moves=[['Talon Peck', 'Swoop Down', 'Fledgling Flutter', 'Gale Grip'], ['Lightning Talon', 'Stormdive', 'Static Plumage', 'Skyhook Snare']]),
 dict(names=('Runepup', 'Runewarden'), role='tank', D='seal', B='armor', hab=32,
      pal=((110, 120, 140), (170, 180, 200), (100, 230, 220), (70, 80, 100)), body=('Round', 'Wide'),
      parts=[('EarsPoint', 0, 1), ('Runes', 2, 1), ('Plates', 3, 2)], eye=0,
      moves=[['Glyph Bump', 'Rune Plate', 'Etched Retort', 'Sigil Heal'], ['Glyph Smash', 'Warding Script', 'Runic Reprisal', 'Sealed Recovery']]),
 dict(names=('Wrapling', 'Pharoth'), role='cursed', D='daze', B='armor', hab=32,
      pal=((225, 215, 185), (245, 238, 215), (120, 200, 170), (160, 140, 100)), body=('Wisp', 'Tall'),
      parts=[('Stripes', 3, 1), ('Crown', 2, 2), ('Sparkles', 2, 2)], eye=3,
      moves=[['Bandage Whip', 'Curse Wrap', 'Silent Tomb', 'Dust Cloud Hex'], ['Tomb Strike', "Pharaoh's Countdown", 'Seal of Ages', 'Sandstorm Curse']]),
 dict(names=('Wisplet', 'Spectrael'), role='trickster', D='slow', B='evade', hab=34,
      pal=((170, 210, 230), (220, 240, 250), (150, 255, 220), (100, 140, 190)), body=('Wisp', 'Wisp'),
      parts=[('Cloud', 4, 2), ('Halo', 2, 2), ('Sparkles', 2, 1)], eye=3,
      moves=[['Boo Poke', 'Phase Blink', 'Chill Touch', 'Haunt Shuffle'], ['Ghost Claw', 'Ethereal Veil', 'Haunting Chill', 'Spirit Echo']]),
]

# rares: hand-written moves
def R(name, stats, hab, pal, body, parts, eye, moves, role):
    return dict(name=name, stats=stats, hab=hab, pal=pal, body=body, parts=parts, eye=eye, moves=moves, role=role)

RARES = [
 R('Tessellate', (3, 3, 4, 3), 36, ((120, 200, 230), (200, 240, 250), (240, 120, 200), (255, 230, 120)), 'Tall',
   [('CrestGem', 2, 1), ('Plates', 3, 1), ('Sparkles', 4, 1), ('Spikes', 2, 1)], 0,
   [mvx('Prism Shard', 'M', fx=[eff('mark')]), mvx('Mosaic Ward', 'U', 4, 'Self', me=[eff('shield', 25), eff('armor', 2)]),
    mvx('Kaleidoscope Beam', 'M', 3, 'Spread', ign=True), mvx('Fractal Cleanse', 'U', 3, 'Ally', fx=[eff('cleanse'), eff('regen', 2)])], 'warder'),
 R('Paradoxel', (3, 3, 2, 4), 32, ((110, 100, 170), (180, 170, 220), (255, 220, 100), (100, 230, 255)), 'Round',
   [('Halo', 2, 1), ('Sparkles', 3, 1), ('CrestGem', 3, 1)], 3,
   [mvx('Echo Strike', 'M'), mvx('Rewind', 'U', 5, 'Self', me=[eff('heal', 15)], sp='ResetCooldowns'), mvx('Temporal Mirror', 'U', 4, 'Self', me=[eff('refdmg')]), mvx('Paradox Swap', 'U', 4, sp='InvertBuffs')], 'time'),
 R('Nimbusaur', (4, 4, 3, 2), 17, ((200, 210, 230), (240, 245, 255), (255, 230, 100), (120, 140, 190)), 'Wide',
   [('Cloud', 4, 1), ('Spikes', 2, 1), ('HornsStraight', 2, 1), ('TailLong', 0, 1)], 0,
   [mvx('Thunderclap Stomp', 'M', 2, 'Spread', fx=[eff('daze', 1)]), mvx('Cloud Cover', 'U', 3, 'Ally', fx=[eff('evade')]), mvx('Storm Surge', 'U', 4, 'Ally', fx=[eff('haste', 3), eff('focus')]), mvx('Downpour', 'L', 3, 'Spread', fx=[eff('slow', 2)])], 'storm'),
 R('Rustwraith', (3, 4, 3, 3), 36, ((150, 100, 70), (200, 150, 110), (90, 200, 170), (70, 50, 40)), 'Wisp',
   [('Plates', 3, 1), ('Spikes', 3, 1), ('Cracks', 2, 1)], 3,
   [mvx('Corroding Touch', 'M', fx=[eff('def', -30, 3)]), mvx('Spectral Plate', 'U', 4, 'Self', me=[eff('armor', 3)]), mvx('Oxidize', 'U', 4, 'Spread', fx=[eff('def', -20, 3)]), mvx('Flake Reprisal', 'U', 3, 'Self', me=[eff('counterhit', 70, 0)])], 'rust'),
 R('Voidmaw', (4, 5, 2, 3), 34, ((40, 30, 60), (70, 50, 100), (200, 60, 220), (255, 255, 255)), 'Round',
   [('Fangs', 4, 1), ('Mane', 2, 1), ('Sparkles', 2, 1)], 3,
   [mvx('Hungry Bite', 'M', drain=50), mvx('Devour Buff', 'U', 3, sp='StealBuff', me=[eff('heal', 10)]), mvx('Void Shatter', 'M', 3, sp='PrismaticShatter'), mvx('Singularity', 'U', 5, fx=[eff('doom', 2), eff('slow', 1)])], 'void'),
 R('Dawnbringer', (4, 3, 3, 4), 17, ((255, 200, 120), (255, 235, 190), (255, 120, 90), (255, 255, 220)), 'Round',
   [('Halo', 4, 1), ('WingsFeather', 3, 1), ('CrestStar', 2, 1), ('Mane', 3, 1)], 0,
   [mvx('Sunbeam', 'M', ign=True), mvx('Daybreak Mend', 'U', 4, 'Ally', fx=[eff('heal', 30), eff('regen', 3)]), mvx('Golden Hour', 'U', 4, 'Ally', fx=[eff('haste', 3), eff('focus')]), mvx('Solar Flare', 'H', 2, fx=[eff('wound', 2)])], 'dawn'),
 R('Gloamreaper', (3, 5, 2, 4), 34, ((50, 45, 70), (90, 80, 120), (190, 190, 230), (120, 255, 180)), 'Tall',
   [('Scythe', 4, 1), ('Cloud', 0, 1)], 3,
   [mvx('Harvest Swing', 'M', crit=25), mvx('Soul Tally', 'U', 4, fx=[eff('doom', 2), eff('mark')]), mvx('Grim Dash', 'U', 3, 'Self', me=[eff('haste', 2), eff('evade')]), mvx('Final Toll', 'H', 3, idef=30)], 'reaper'),
 R('Abyssopod', (4, 3, 3, 2), 8, ((40, 80, 110), (90, 140, 170), (255, 230, 120), (255, 255, 255)), 'Blob',
   [('Lantern', 2, 1), ('Fangs', 4, 1), ('Fins', 3, 1)], 0,
   [mvx('Lure Glow', 'U', 3, fx=[eff('mark'), eff('daze', 2)]), mvx('Gulp', 'M', drain=40), mvx('Pressure Trap', 'U', 4, fx=[eff('trap', 3), eff('slow', 1)]), mvx('Deep Camouflage', 'U', 4, 'Self', me=[eff('evade'), eff('armor', 2)])], 'lure'),
 R('Pangolem', (5, 3, 5, 1), 36, ((150, 120, 90), (210, 185, 150), (180, 170, 160), (110, 90, 70)), 'Wide',
   [('Plates', 2, 1), ('Claws', 3, 1), ('TailLong', 3, 1)], 0,
   [mvx('Scale Slam', 'M'), mvx('Roll Up', 'U', 3, 'Self', me=[eff('plate'), eff('armor', 2)]), mvx('Plate Stack', 'U', 0, 'Self', me=[eff('plate')]), mvx('Ricochet Roll', 'H', 3, sp='CannonballBonus')], 'plates'),
 R('Zephyra', (3, 3, 2, 5), 17, ((170, 240, 230), (235, 255, 250), (255, 255, 255), (110, 200, 200)), 'Wisp',
   [('WingsFeather', 4, 1), ('Mane', 2, 1), ('Sparkles', 4, 1), ('Halo', 2, 1)], 0,
   [mvx('Cutting Wind', 'L', prio=True), mvx('Updraft Blessing', 'U', 3, 'Ally', fx=[eff('evade'), eff('haste', 2)]), mvx('Zephyr Veil', 'U', 4, 'Ally', fx=[eff('evade'), eff('shield', 15)]), mvx('Gale Force', 'M', 3, 'Spread', fx=[eff('slow', 1)])], 'wind'),
 R('Cinderqueen', (3, 5, 2, 3), 32, ((90, 50, 60), (140, 90, 90), (255, 140, 40), (255, 230, 120)), 'Tall',
   [('Crown', 2, 1), ('WingsFlame', 3, 1), ('Cracks', 3, 1)], 3,
   [mvx('Ember Scepter', 'M', fx=[eff('wound', 2, 0, 30)]), mvx('Ashfall', 'L', 2, 'Spread', fx=[eff('wound', 2)]), mvx('Royal Blaze', 'H', 3, fx=[eff('wound', 3)]), mvx('Crown of Cinders', 'U', 3, 'Self', me=[eff('counterhit', 70, 2)])], 'fire'),
 R('Fennecho', (3, 3, 2, 4), 5, ((240, 210, 150), (255, 240, 210), (250, 160, 120), (190, 140, 90)), 'Round',
   [('EarsLong', 0, 1), ('TailFluffy', 4, 1), ('Notes', 3, 1), ('Whiskers', 5, 1)], 0,
   [mvx('Sandy Nip', 'L'), mvx('Echo Bark', 'U', 3, mimic=100), mvx('Big Ear Hush', 'U', 3, fx=[eff('seal'), eff('daze', 1)]), mvx('Burrow Dodge', 'U', 3, 'Self', me=[eff('evade'), eff('prionext')])], 'echo'),
 R('Bastionwyrm', (5, 3, 5, 1), 36, ((130, 110, 100), (190, 170, 150), (220, 80, 60), (80, 70, 70)), 'Wide',
   [('Plates', 2, 1), ('HornsStraight', 3, 1), ('Spikes', 2, 1), ('TailLong', 3, 1)], 0,
   [mvx('Rampart Bite', 'M'), mvx('Fortress Roar', 'U', 4, 'Ally', fx=[eff('shield', 30), eff('armor', 2)]), mvx('Stone Scales', 'U', 4, 'Self', me=[eff('armor', 3), eff('shield', 20)]), mvx('Siege Breath', 'H', 3, fx=[eff('slow', 1)])], 'fort'),
 R('Lullabyss', (4, 2, 3, 3), 10, ((130, 150, 230), (200, 215, 255), (255, 200, 240), (90, 100, 180)), 'Wisp',
   [('Moon', 2, 1), ('Bubbles', 4, 1), ('Notes', 2, 1), ('Sparkles', 4, 1)], 0,
   [mvx('Soft Note', 'L', fx=[eff('daze', 1)]), mvx('Sleep Song', 'U', 5, fx=[eff('stun')]), mvx('Lullaby Tide', 'U', 4, 'Spread', fx=[eff('slow', 2), eff('daze', 1)]), mvx('Dreamy Dive', 'U', 4, 'Self', me=[eff('evade'), eff('heal', 20)])], 'sleep'),
 R('Harlequill', (3, 4, 2, 4), 33, ((230, 230, 240), (255, 255, 255), (230, 60, 90), (60, 90, 230)), 'Tall',
   [('HornsCurl', 2, 1), ('Bell', 3, 1), ('Spots', 3, 1), ('Stripes', 2, 1)], 0,
   [mvx('Pratfall', 'M', sp='ChaosBolt'), mvx('Juggling Act', 'U', 2, hits=[40, 40, 40], rand=True), mvx("Joker's Gambit", 'U', 3, 'Self', sp='WildCard'), mvx('Mime Show', 'U', 3, mimic=100)], 'chaos'),
 R('Thornmother', (5, 3, 4, 1), 3, ((60, 120, 70), (130, 190, 110), (250, 120, 160), (110, 70, 50)), 'Wide',
   [('Flower', 2, 1), ('Spikes', 3, 1), ('TailLeaf', 1, 1), ('Antlers', 3, 1)], 0,
   [mvx('Briar Smack', 'M'), mvx('Nursery Bloom', 'U', 3, 'Ally', fx=[eff('regen', 4), eff('heal', 10)]), mvx('Thorn Garden', 'U', 4, 'Self', me=[eff('thorns', 3), eff('armor', 2)]), mvx('Lullaby Roots', 'U', 4, fx=[eff('trap', 2), eff('slow', 2)])], 'garden'),
 R('Prismoth', (3, 3, 2, 5), 34, ((200, 190, 250), (240, 235, 255), (255, 160, 210), (120, 230, 255)), 'Round',
   [('WingsInsect', 3, 1), ('Antennae', 2, 1), ('Sparkles', 4, 1)], 0,
   [mvx('Rainbow Powder', 'L', 2, 'Spread', fx=[eff('daze', 2)]), mvx('Spectrum Wing', 'M', ign=True), mvx('Prism Veil', 'U', 4, 'Ally', fx=[eff('evade'), eff('shield', 15)]), mvx('Glimmer Scales', 'U', 4, 'Self', me=[eff('atk', 30, 3), eff('spd', 30, 3)])], 'prism'),
 R('Epochite', (3, 3, 3, 4), 32, ((170, 150, 90), (225, 210, 160), (90, 200, 230), (60, 50, 30)), 'Round',
   [('Halo', 3, 1), ('Sparkles', 3, 1), ('Plates', 2, 1)], 0,
   [mvx('Chrono Nick', 'L', prio=True), mvx('Time Pocket', 'U', 5, 'Self', me=[eff('shield', 20)], sp='ResetCooldowns'), mvx('Slow Time', 'U', 4, 'Spread', fx=[eff('slow', 2)]), mvx('Frozen Instant', 'U', 4, fx=[eff('trap', 2), eff('seal')])], 'time'),
 R('Magnaclaw', (4, 4, 4, 2), 12, ((150, 70, 70), (210, 130, 120), (70, 100, 230), (230, 230, 240)), 'Wide',
   [('Claws', 3, 1), ('Magnet', 2, 1), ('Plates', 4, 1)], 2,
   [mvx('Pincer Pull', 'M', fx=[eff('trap', 1)]), mvx('Magnetic Yank', 'U', 3, fx=[eff('fswitch')]), mvx('Iron Grip', 'H', 2, idef=20), mvx('Field Polarize', 'U', 4, 'Spread', fx=[eff('def', -20, 2)])], 'magnet'),
 R('Rimewyrm', (4, 4, 3, 3), 16, ((150, 200, 240), (225, 240, 255), (100, 150, 230), (255, 255, 255)), 'Serpent',
   [('CrestIce', 4, 1), ('Spikes', 4, 1), ('Fangs', 4, 1), ('Sparkles', 2, 1)], 0,
   [mvx('Rime Fang', 'M', fx=[eff('slow', 1)]), mvx('Flash Frostbreath', 'M', 3, 'Spread', fx=[eff('slow', 2)]), mvx('Deepfreeze Stare', 'U', 5, fx=[eff('stun')]), mvx('Crystalline Coil', 'U', 4, 'Self', me=[eff('armor', 3), eff('slowatk', 3)])], 'frost'),
 R('Mirage', (3, 3, 2, 4), 37, ((240, 190, 110), (255, 230, 170), (120, 220, 230), (200, 130, 60)), 'Wisp',
   [('Crown', 2, 1), ('Mane', 2, 1), ('Sparkles', 4, 1)], 0,
   [mvx('Heat Haze', 'L', fx=[eff('daze', 1)]), mvx('Vanish', 'U', 3, 'Self', me=[eff('evade'), eff('haste', 2)]), mvx('Phantom Oasis', 'U', 4, 'Ally', fx=[eff('heal', 25), eff('evade')]), mvx('Dust Devil Veil', 'U', 4, 'Spread', fx=[eff('daze', 2), eff('slow', 1)])], 'mirage'),
 R('Marrowmancer', (3, 5, 2, 3), 34, ((230, 225, 200), (250, 248, 230), (120, 255, 160), (90, 80, 70)), 'Tall',
   [('HornsCurl', 4, 1), ('Plates', 3, 1), ('Fangs', 4, 1)], 3,
   [mvx('Bone Spear', 'M', fx=[eff('wound', 2)]), mvx('Bone Wall', 'U', 4, 'Self', me=[eff('armor', 3), eff('thorns', 2)]), mvx('Rattle Hex', 'U', 4, fx=[eff('doom', 3)]), mvx('Skeletal Grasp', 'U', 4, fx=[eff('trap', 2), eff('wound', 3)])], 'bone'),
 R('Lumenwing', (4, 2, 3, 4), 48, ((255, 250, 230), (255, 255, 245), (255, 220, 120), (180, 220, 255)), 'Round',
   [('Halo', 2, 1), ('WingsFeather', 4, 1), ('Sparkles', 2, 1), ('CrestStar', 2, 1)], 0,
   [mvx('Halo Strike', 'M'), mvx('Radiant Rebirth', 'U', 4, 'Ally', fx=[eff('heal', 35), eff('cleanse')]), mvx('Wings of Warding', 'U', 4, 'Ally', fx=[eff('shield', 30), eff('haste', 1)]), mvx('Dawn Beam', 'H', 3, ign=True)], 'light'),
 R('Gambit', (3, 4, 3, 4), 32, ((60, 60, 80), (110, 110, 140), (230, 230, 250), (230, 190, 90)), 'Tall',
   [('EarsPoint', 0, 1), ('Mane', 2, 1), ('Plates', 3, 1)], 0,
   [mvx("Knight's Hop", 'M', pivot=True), mvx('Sacrifice Play', 'U', 5, 'Self', me=[eff('selfdmg', 15), eff('empower', 80, 2)]), mvx('Fork Strike', 'H', 3, fx=[eff('mark')]), mvx('Castling', 'U', 4, 'Self', me=[eff('shield', 25), eff('evade')])], 'chess'),
]
assert len(RARES) == 24

# ------------------------------------------------------------------ emit
def rgbs(c): return f'rgb({c[0]}, {c[1]}, {c[2]})'

def main():
    out, sl, md, names = [], [], [], []
    sid = FIRST_ID
    line = FIRST_LINE
    hab_ext = []
    out.append('// GENERATED by tools/gen_roster.py: do not edit by hand. Expansion roster, ids 48 to 149.')
    sl.append('// GENERATED by tools/gen_roster.py: do not edit by hand.\nconst GenLine kGenLines[] = {')

    def sprite_line(lid, label, pal, bodies, parts, eye):
        b = list(bodies) + [bodies[-1]] * (3 - len(bodies))
        ps = ', '.join(f'{{PartK::{k}, {c}, {m}}}' for k, c, m in parts)
        sl.append(f'    // line {lid}: {label}\n    {{{{{", ".join(rgbs(x) for x in pal)}}}, {{BodyK::{b[0]}, BodyK::{b[1]}, BodyK::{b[2]}}}, {{{ps}}}, {len(parts)}, {eye}}},')

    def md_row(sid_, name, stage, tot, role, hab, moves):
        areas = ', '.join(AREAS[i] for i in range(6) if hab >> i & 1)
        md.append(f'| {sid_+1} | {name} | {stage}/{tot} | {role} | {areas} | {", ".join(moves)} |')

    def move_names(exprs): return [re.match(r'(?:mv|util)\("([^"]+)"', e).group(1) for e in exprs]

    for spec in THREE + TWO:
        n = len(spec['names'])
        first = sid
        for st in range(1, n + 1):
            moves = role_moves(spec['role'], spec['moves'][st - 1], st, spec['D'], spec['B'])
            hp, atk, df, sp = stats_for(spec['role'], st)
            out.append(f'    add("{spec["names"][st-1]}", {line}, {st}, {hp}, {atk}, {df}, {sp}, {{\n        ' + ',\n        '.join(moves) + '});')
            names += move_names(moves)
            md_row(sid, spec['names'][st - 1], st, n, spec['role'], spec['hab'], move_names(moves))
            sid += 1
        if n == 3: out.append(f'    evo({first}, {first+1}, 16); evo({first+1}, {first+2}, 32);')
        else: out.append(f'    evo({first}, {first+1}, 20);')
        sprite_line(line, spec['names'][0], spec['pal'], spec['body'], spec['parts'], spec['eye'])
        hab_ext.append(spec['hab'])
        line += 1
    for r in RARES:
        hp, atk, df, sp = r['stats']
        out.append(f'    add("{r["name"]}", {line}, 1, {hp}, {atk}, {df}, {sp}, {{\n        ' + ',\n        '.join(r['moves']) + '}, true);')
        names += move_names(r['moves'])
        md_row(sid, r['name'], 1, 1, 'rare ' + r['role'], r['hab'], move_names(r['moves']))
        sprite_line(line, r['name'], r['pal'], (r['body'],), r['parts'], r['eye'])
        hab_ext.append(r['hab'])
        sid += 1; line += 1
    sl.append('};\nconst GenLine& genLine(int i) { return kGenLines[i]; }')
    out.append('    static const int habExt[] = {' + ', '.join(map(str, hab_ext)) + '};')
    out.append('    for (auto& sp : v) if (sp.line >= %d) sp.habitats = habExt[sp.line - %d];' % (FIRST_LINE, FIRST_LINE))

    # uniqueness against everything, including the hand-written 192 moves
    existing = re.findall(r'(?:mv|util)\("([^"]+)"', open('src/core/data.cpp', encoding='utf-8').read())
    allnames = existing + names
    dup = sorted({x for x in allnames if allnames.count(x) > 1})
    if dup:
        print('DUPLICATE MOVE NAMES:', dup); sys.exit(1)
    assert sid == 150, sid
    print(f'{sid - FIRST_ID} new friends, {len(names)} new moves, {len(set(allnames))} unique moves in total')

    open('src/core/roster_ext.inc', 'w', encoding='utf-8').write('\n'.join(out) + '\n')
    open('src/core/sprite_lines.inc', 'w', encoding='utf-8').write('\n'.join(sl) + '\n')
    table = '\n'.join(md)
    open('tools/roster_ext.md', 'w', encoding='utf-8').write('| # | Friend | Stage | Role | Found in | Moves |\n|---|---|---|---|---|---|\n' + table + '\n')

main()
