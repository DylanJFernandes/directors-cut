"""Facades, alleys and landmark storefronts on the CubeGrid building rows."""
import math
import random
import unreal
import dresslib as D
import layout as L
import facades as FA

ROOT = 'Backlot/04_BuildingRows'
LM_ROOT = 'Backlot/05_Landmarks'

MAIN = {'CubeGridToolOutput12', 'CubeGridToolOutput13', 'CubeGridToolOutput14', 'CubeGridToolOutput15',
        'CubeGridToolOutput21', 'CubeGridToolOutput22', 'CubeGridToolOutput23', 'CubeGridToolOutput24',
        'CubeGridToolOutput19', 'CubeGridToolOutput29', 'CubeGridToolOutput35', 'CubeGridToolOutput45',
        'CubeGridToolOutput32', 'CubeGridToolOutput34'}

# (building, road): (mesh, half width, front depth, height, where) ; where = 'mid' | 'hub'
LANDMARKS = {
    # depth = measured front-wall plane (local +Y) minus 5cm, so the shopfront sits just proud of the facade line
    ('CubeGridToolOutput12', 'R1_WestApproachL'): ('Storefronts/SM_BarberShop', 404, 339, 485, 'mid', 'BarberShop'),
    ('CubeGridToolOutput13', 'R1_WestApproachL'): ('/Game/Environment/Moxy/SM_Moxy', 687, 375, 693, 'hub', 'Moxy'),
    ('CubeGridToolOutput21', 'R3_WestApproachR'): ('Storefronts/SM_CornerDiner', 478, 369, 470, 'hub', 'CornerDiner'),
    ('CubeGridToolOutput34', 'R30_CrossAveN'): ('Storefronts/SM_RustyRabbit', 481, 364, 529, 'mid', 'RustyRabbit'),
}

NEON = ['Signage/SM_NeonBlade_Bar', 'Signage/SM_NeonBlade_Club', 'Signage/SM_NeonBlade_Diner',
        'Signage/SM_NeonBlade_Hotel']
CAB = ['Signage/SM_NeonCabinet_Bar', 'Signage/SM_NeonCabinet_Club', 'Signage/SM_NeonCabinet_Diner',
       'Signage/SM_NeonCabinet_Hotel']


# blocks that frame the tower run tallest; the approach rows sit mid-height
TALL = {'CubeGridToolOutput19', 'CubeGridToolOutput29', 'CubeGridToolOutput35', 'CubeGridToolOutput45',
        'CubeGridToolOutput18', 'CubeGridToolOutput39', 'CubeGridToolOutput28', 'CubeGridToolOutput46'}


def floors(label):
    """Storey count (ground floor included) per block, deterministic."""
    rng = random.Random(label)
    if label in TALL:
        return rng.randint(13, 17)
    if label in ('CubeGridToolOutput13', 'CubeGridToolOutput21'):
        return 11
    if label in MAIN:
        return rng.randint(7, 10)
    return rng.randint(5, 12)


def upper(b, line, z0, top, t0=0.0, t1=None):
    """Window storeys from z0 up to top, with a cornice band every fourth storey."""
    z = z0
    while z < top:
        FA.run(b, line, z, FA.WIN, t0, t1, pil_every=2)
        if z > z0 and (z // 400) % 4 == 0:
            FA.band(b, line, z, FA.CORNICE, t0, t1, out=8)
        z += 400


def faces():
    """Every (building, road, side, s_centre, face_perp, half_along) street face."""
    out = []
    for a in D.eas.get_all_level_actors():
        lab = a.get_actor_label()
        if not lab.startswith('CubeGrid') or a.get_class().get_name() != 'StaticMeshActor':
            continue
        bb = a.static_mesh_component.static_mesh.get_bounding_box()
        s = a.get_actor_scale3d()
        sx, sy = (bb.max.x - bb.min.x) * s.x, (bb.max.y - bb.min.y) * s.y
        if sx < 11000 or sy < 11000 or sx > 12000:
            continue
        c = a.get_actor_transform().transform_location(
            unreal.Vector((bb.min.x + bb.max.x) / 2, (bb.min.y + bb.max.y) / 2, 0))
        yaw = a.get_actor_rotation().yaw
        for r in L.ROADS:
            st = L.STREETS[r['name']]
            ss, pp = st.local(c.x, c.y)
            rel = abs((yaw - r['yaw']) % 180)
            par = rel < 45 or rel > 135
            hp = sy / 2 if par else sx / 2
            ha = sx / 2 if par else sy / 2
            face = abs(pp) - hp
            F = r['Fl'] if pp > 0 else r['Fr']
            if abs(ss) < r['L'] / 2 + ha and r['H'] < face < F + 600:
                out.append(dict(b=lab, r=r, side=1 if pp > 0 else -1, s=ss, face=face, ha=ha, hp=hp))
    return out


def face_line(st, side, s_lo, s_hi, perp):
    yaw = st.face_yaw(side)
    if side > 0:  # module local X = -d
        a, b = st.pt(s_hi, side * perp), st.pt(s_lo, side * perp)
    else:
        a, b = st.pt(s_lo, side * perp), st.pt(s_hi, side * perp)
    return FA.Line(a[0], a[1], b[0], b[1], yaw)


def t_of(line, st, s):
    """Distance along line for street coordinate s."""
    x, y = line.at(0)
    s0, _ = st.local(x, y)
    return abs(s - s0)


def build(only=None):
    D.clear(ROOT)
    D.clear(LM_ROOT)
    rng = random.Random(7)
    fs = faces()
    # neighbours along each road side for alley gaps
    by_side = {}
    for f in fs:
        by_side.setdefault((f['r']['name'], f['side']), []).append(f)
    for k in by_side:
        by_side[k].sort(key=lambda f: f['s'])
    entrances = []
    street_faces = []
    done = 0
    for f in fs:
        if only and f['b'] not in only:
            continue
        r, side = f['r'], f['side']
        st = L.STREETS[r['name']]
        main = f['b'] in MAIN
        s_lo, s_hi = f['s'] - f['ha'], f['s'] + f['ha']
        perp = f['face']
        line = face_line(st, side, s_lo, s_hi, perp)
        folder = f"{ROOT}/{r['name']}"
        b = D.Bucket(folder, f"{f['b'].replace('CubeGridToolOutput', 'Bldg')}_{r['name'].split('_')[0]}")
        top = 400 * floors(f['b'])
        street_faces.append(dict(b=f['b'], mid=line.at(line.len / 2), yaw=line.yaw, len=line.len))
        # landmark slot
        holes = []
        lm = LANDMARKS.get((f['b'], r['name']))
        if lm:
            mp, hw, dep, h, where, nm = lm
            w = 2 * hw + 30
            if where == 'hub':
                hub_end_s = s_hi if st.local(*L.HC)[0] > f['s'] else s_lo
                t_hub = t_of(line, st, hub_end_s)
                t0 = max(0, t_hub - w - 300) if t_hub > line.len / 2 else t_hub + 300
            else:
                t0 = line.len / 2 - w / 2
            t1 = t0 + w
            holes.append((t0, t1, h))
            tc = (t0 + t1) / 2
            x, y = line.at(tc, -dep)
            D.sm(nm, LM_ROOT, mp, x, y, 0.5, line.yaw)
            ex, ey = line.at(tc, 250)
            entrances.append((ex, ey, nm))
            # brick infill above the landmark to the band top
            hz = math.ceil(h / 100.0) * 100
            sc = (800 - hz) / 400.0
            n = int(math.ceil((t1 - t0) / 400))
            step = (t1 - t0) / n
            if sc > 0.05:
                for i in range(n):
                    x, y = line.at(t0 + step * (i + 0.5))
                    b.add(FA.BRICK[400], x, y, hz, line.yaw, scale=(step / 400, 1, sc))
            upper(b, line, 800, top, t0, t1)
        segs = []
        cur = 0.0
        for t0, t1, h in sorted(holes):
            segs.append((cur, t0))
            cur = t1
        segs.append((cur, line.len))
        # ground floor: storefront rhythm with recessed entrances
        def pat(i, w, m):
            if w == 400 and i % 3 == 1:
                return FA.RECESSED
            return None

        for t0, t1 in segs:
            if t1 - t0 < 100:
                continue
            placed = FA.run(b, line, 0, FA.STORE, t0, t1, pattern=pat, pil_every=2)
            for i, (tc, w) in enumerate(placed):
                if w == 400 and i % 3 == 1:
                    ex, ey = line.at(tc, 250)
                    entrances.append((ex, ey, f['b']))
                    if main and rng.random() < 0.45:
                        x, y = line.at(tc, 2)
                        b.add(rng.choice(CAB), x, y, 405, line.yaw, collision=False)
                if main and i % 6 == 3:
                    x, y = line.at(tc - w / 2, 4)
                    b.add(rng.choice(NEON), x, y, 430, line.yaw, collision=False)
            FA.band(b, line, 400, FA.CORNICE, t0, t1, out=8)
            upper(b, line, 400, top, t0, t1)
        FA.band(b, line, top, FA.CORNICE, 0, line.len, out=12)
        FA.band(b, line, top + 30, FA.PARAPET if not main else FA.STEP_PARAPET, 0, line.len, out=4)
        # alleys: side returns, gate, fire escape
        sib = by_side[(r['name'], side)]
        idx = sib.index(f)
        for end, nb in ((s_hi, sib[idx + 1] if idx + 1 < len(sib) else None),
                        (s_lo, sib[idx - 1] if idx > 0 else None)):
            if nb is None:
                continue
            gap = (nb['s'] - nb['ha']) - end if end == s_hi else end - (nb['s'] + nb['ha'])
            if gap < 500 or gap > 3000:
                continue
            outward = 1 if end == s_hi else -1
            if outward > 0:
                # gate across the alley mouth, set 30cm behind the facade line
                gb = D.Bucket(f"{folder}/Alleys", f"{f['b'].replace('CubeGridToolOutput', 'Bldg')}_Gate")
                mid = end + gap / 2
                gx, gy = st.pt(mid, side * (perp + 30))
                gyaw = st.face_yaw(side)
                gb.add('Street/SM_OrnateIronGate_4m', gx, gy, 0, gyaw)
                rest = (gap - 418) / 2
                if rest > 20:
                    for sgn in (-1, 1):
                        fx, fy = st.pt(mid + sgn * (209 + rest / 2), side * (perp + 30))
                        gb.add('Street/SM_PerimeterFence_4m', fx, fy, 0, gyaw, scale=(rest / 422 + 0.02, 1, 1))
                gb.flush()
        b.flush()
        done += 1
    return done, entrances, street_faces
