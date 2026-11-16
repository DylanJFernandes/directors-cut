"""Big neon signs for LVL_NewYorkSet: rooftop signs on scaffold frames and vertical blade
signs projecting from facades. Meshes come from SourceArt/Environment/Signage/make_neon_signs.py.

Practical lights for these meshes are added by noir.practicals() (see noir.PRACTICALS).
"""
import json
import math
import random

import dresslib as D
import buildings as B

ROOT = 'Backlot/10_NeonSigns'
N = '/Game/Environment/BacklotKit/Signage/Neon/SM_NeonSign_'
ROOF = ['Hotel', 'Dancing', 'Loans', 'Cafe', 'Billiards', 'Liquors', 'Cigars', 'Jazz', 'Pawn', 'Pictures',
        'Radio', 'ChopSuey', 'Tonight', 'Garage']
# board widths (cm) from the imported meshes
WIDTH = {'Hotel': 1003, 'Dancing': 1154, 'Loans': 970, 'Cafe': 771, 'Billiards': 1220, 'Liquors': 1121,
         'Cigars': 970, 'Jazz': 870, 'Pawn': 854, 'Pictures': 1125, 'Radio': 892, 'ChopSuey': 1297,
         'Tonight': 1018, 'Garage': 1054}
BLADE = ['V_Hotel', 'V_Bar', 'V_Club', 'V_Eats', 'V_Jazz', 'V_Rooms']
SUPPORT = 'Infrastructure/SM_ScaffoldSupport_4m'
TRUSS = 'Infrastructure/SM_ScaffoldTruss_4m'


def _frame(street):
    mx, my = street['mid']
    yaw = street['yaw']
    r = math.radians(yaw)
    u = (math.cos(r), math.sin(r))      # along the facade (module local X)
    f = (-math.sin(r), math.cos(r))     # out to the street (module local +Y)

    def at(s, out=0.0):
        return mx + u[0] * s + f[0] * out, my + u[1] * s + f[1] * out
    return at, yaw


def build(street_faces, roof_count=18, blade_count=26, seed=1940, roof_scale=2.2):
    D.clear(ROOT)
    rng = random.Random(seed)
    faces = [sf for sf in street_faces if sf['len'] > 3000]
    rng.shuffle(faces)
    # rooftop signs: prefer the tall and main blocks that frame the streets, one per block
    pri = sorted(faces, key=lambda sf: (sf['b'] not in B.TALL and sf['b'] not in B.MAIN, rng.random()))
    used = set()
    roof = D.Bucket(ROOT + '/Rooftop', 'RoofSign')
    words = ROOF[:]
    rng.shuffle(words)
    n_roof = 0
    for sf in pri:
        if n_roof >= roof_count:
            break
        if sf['b'] in used:
            continue
        used.add(sf['b'])
        at, yaw = _frame(sf)
        top = 400 * B.floors(sf['b'])
        word = words[n_roof % len(words)]
        k = roof_scale
        w = WIDTH[word] * k
        s = rng.uniform(-0.25, 0.25) * sf['len']
        lift = rng.choice((260, 380, 520))
        x, y = at(s, -400)
        roof.add(N + word, x, y, top + lift, yaw, scale=k, collision=False)
        # scaffold legs behind the board and a truss under it
        for f_ in (-0.4, -0.13, 0.13, 0.4):
            lx, ly = at(s + f_ * w, -470)
            roof.add(SUPPORT, lx, ly, top, yaw, scale=(1.4, 1, (lift + 120 * k) / 400.0))
        for dz in (-40, 250 * k):
            tx, ty = at(s, -440)
            roof.add(TRUSS, tx, ty, top + lift + dz, yaw + 90, scale=(1.3, w / 406.0, 1.3))
        n_roof += 1
    roof.flush()

    blade = D.Bucket(ROOT + '/Blades', 'BladeSign')
    n_blade = 0
    main_faces = [sf for sf in faces if sf['b'] in B.MAIN or sf['b'] in B.TALL] + faces
    seen = set()
    for sf in main_faces:
        if n_blade >= blade_count:
            break
        key = (sf['b'], round(sf['mid'][0]), round(sf['mid'][1]))
        if key in seen:
            continue
        seen.add(key)
        at, yaw = _frame(sf)
        for _ in range(rng.choice((1, 1, 2))):
            if n_blade >= blade_count:
                break
            s = rng.uniform(-0.42, 0.42) * sf['len']
            word = rng.choice(BLADE)
            z = rng.choice((470, 470, 870))
            bx, by = at(s, 80)
            byaw = yaw + 90
            # back-to-back pair so the blade reads from both directions along the street
            br = math.radians(byaw)
            back = (math.sin(br) * 14, -math.cos(br) * 14)
            blade.add(N + word, bx, by, z, byaw, collision=False)
            blade.add(N + word, bx + back[0], by + back[1], z, byaw + 180, collision=False)
            n_blade += 1
    blade.flush()
    return n_roof, n_blade
