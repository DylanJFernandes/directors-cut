"""Turn every CubeGrid building mass into a full kit-built block.

Street faces come from buildings.build(); this module dresses the remaining sides,
corners, roofs and rooftop clutter, then hides the proxies (collision kept so the
hollow shells stay solid).
"""
import math
import random
import unreal
import dresslib as D
import facades as FA
import buildings as B

ROOT = 'Backlot/04_BuildingRows/Blocks'
ALLEY = {600: 'Architecture/SM_AlleyUtilityWall_6m', 400: FA.BRICK[400], 200: 'Architecture/SM_AlleyServiceDoor_2m'}
ROOF_PROPS = ['Rooftop/SM_BrickChimney', 'Rooftop/SM_HVACVent', 'Rooftop/SM_GlassSkylight', 'Rooftop/SM_HVACVent']


def proxies():
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
        out.append(dict(a=a, b=lab, cx=c.x, cy=c.y, yaw=a.get_actor_rotation().yaw, hx=sx / 2, hy=sy / 2))
    return out


def sides(p):
    """Four (line, outward unit) tuples; module local X runs along each line."""
    r = math.radians(p['yaw'])
    ex, ey = (math.cos(r), math.sin(r)), (-math.sin(r), math.cos(r))
    out = []
    for nrm, half_n, half_t, fyaw in ((ey, p['hy'], p['hx'], p['yaw']), ((-ey[0], -ey[1]), p['hy'], p['hx'], p['yaw'] + 180),
                                       (ex, p['hx'], p['hy'], p['yaw'] - 90), ((-ex[0], -ex[1]), p['hx'], p['hy'], p['yaw'] + 90)):
        mx, my = p['cx'] + nrm[0] * half_n, p['cy'] + nrm[1] * half_n
        fr = math.radians(fyaw)
        u = (math.cos(fr), math.sin(fr))
        ln = FA.Line(mx - u[0] * half_t, my - u[1] * half_t, mx + u[0] * half_t, my + u[1] * half_t, fyaw)
        out.append((ln, nrm))
    return out


def build(street_faces, only=None):
    D.clear(ROOT)
    n_inst = 0
    for p in proxies():
        if only and p['b'] not in only:
            continue
        rng = random.Random(p['b'] + 'blk')
        top = 400 * B.floors(p['b'])
        name = p['b'].replace('CubeGridToolOutput', 'Bldg')
        b = D.Bucket(ROOT, name)
        for k, (ln, nrm) in enumerate(sides(p)):
            mx, my = ln.at(ln.len / 2)
            street = any(sf['b'] == p['b'] and math.hypot(sf['mid'][0] - mx, sf['mid'][1] - my) < 1500
                         for sf in street_faces)
            if not street:
                FA.run(b, ln, 0, ALLEY, pilasters=True)
                FA.band(b, ln, 400, FA.CORNICE, out=8)
                B.upper(b, ln, 400, top)
                FA.band(b, ln, top, FA.CORNICE, out=12)
                FA.band(b, ln, top + 30, FA.PARAPET, out=4)
                # fire-escape stacks
                for frac in (0.22, 0.68) if ln.len > 9000 else (0.5,):
                    tc = ln.len * frac
                    for z in range(400, top, 400):
                        x, y = ln.at(tc)
                        b.add('Infrastructure/SM_FireEscapePlatform_4m', x, y, z, ln.yaw)
                        for tt in (tc - 180, tc + 180):
                            bx, by = ln.at(tt)
                            b.add('Infrastructure/SM_FireEscapeBracket', bx, by, z - 5, ln.yaw)
                        if z + 400 < top:
                            sx, sy = ln.at(tc + (150 if (z // 400) % 2 else -150), 75)
                            b.add('Infrastructure/SM_FireEscapeStair_4m', sx, sy, z,
                                  ln.yaw + (90 if (z // 400) % 2 else -90), scale=(1, 0.9, 1))
                    lx, ly = ln.at(tc + 150, 120)
                    b.add('Infrastructure/SM_DropLadder_4m', lx, ly, 0, ln.yaw)
                # service clutter at the foot of the wall
                for _ in range(3):
                    tc = rng.uniform(600, ln.len - 600)
                    x, y = ln.at(tc, 160)
                    b.add(rng.choice(['Street/SM_MetalBarrel', 'Street/SM_WoodCrate', 'Street/SM_TrashCan']), x, y, 2,
                          rng.uniform(0, 360))
            # corner columns on both ends of every side
            for tc in (48, ln.len - 48):
                for z in range(0, top, 400):
                    x, y = ln.at(tc, 6)
                    b.add(FA.PIL_W, x, y, z, ln.yaw, scale=(1.0, 1.3, 1.0))
        # roof deck
        r = math.radians(p['yaw'])
        ex, ey = (math.cos(r), math.sin(r)), (-math.sin(r), math.cos(r))
        nx, ny = int(math.ceil(2 * p['hx'] / 2000)), int(math.ceil(2 * p['hy'] / 4000))
        stx, sty = 2 * p['hx'] / nx, 2 * p['hy'] / ny
        for i in range(nx):
            for j in range(ny):
                lx = -p['hx'] + stx * (i + 0.5)
                ly = -p['hy'] + sty * (j + 0.5)
                x = p['cx'] + ex[0] * lx + ey[0] * ly
                y = p['cy'] + ex[1] * lx + ey[1] * ly
                b.add('Street/SM_SidewalkSlab_2x4', x, y, top - 1, p['yaw'], scale=(stx / 200, sty / 400, 1))
        # rooftop clutter
        def roof_pt(margin):
            lx = rng.uniform(-p['hx'] + margin, p['hx'] - margin)
            ly = rng.uniform(-p['hy'] + margin, p['hy'] - margin)
            return p['cx'] + ex[0] * lx + ey[0] * ly, p['cy'] + ex[1] * lx + ey[1] * ly
        for _ in range(rng.randint(1, 2)):
            x, y = roof_pt(1500)
            b.add('Rooftop/SM_WoodWaterTower', x, y, top, rng.uniform(0, 90), scale=1.4)
        for _ in range(rng.randint(5, 9)):
            x, y = roof_pt(800)
            b.add(rng.choice(ROOF_PROPS), x, y, top, p['yaw'] + rng.choice((0, 90, 180, 270)))
        n_inst += b.count()
        b.flush()
    return n_inst


def hide_proxies():
    """Hide every CubeGrid actor. Building masses keep collision; everything else is fully disabled."""
    n = 0
    blocks = {p['b'] for p in proxies()}
    for a in D.eas.get_all_level_actors():
        lab = a.get_actor_label()
        if not lab.startswith('CubeGridToolOutput') or a.get_class().get_name() != 'StaticMeshActor':
            continue
        c = a.static_mesh_component
        c.set_visibility(False)
        a.set_actor_hidden_in_game(True)
        if lab not in blocks:
            c.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
            a.set_actor_enable_collision(False)
        n += 1
    return n


def build_ground(folder='Backlot/03_Streets/BaseGround', holes=None):
    """Coarse cobble ground under the whole district, replacing the CubeGrid ground slab.

    holes: world-aligned (x0, x1, y0, y1) rectangles left open, e.g. the subway stair well.
    """
    import layout as L
    if holes is None:
        x0, x1, y0, y1 = L.EXIT_CUT
        holes = [(x0 - 10, x1 + 10, y0 - 10, y1 + 10)]
    D.clear(folder)
    b = D.Bucket(folder, 'BaseGround')

    def rect(ax, bx, ay, by):
        if bx - ax > 1 and by - ay > 1:
            b.add('Street/SM_CobblestoneRoad_4x8', (ax + bx) / 2, (ay + by) / 2, -2.0, 0,
                  scale=((bx - ax) / 400, (by - ay) / 800, 1))

    for x in range(-84000, 62000, 1600):
        for y in range(-104000, 104000, 1600):
            tx0, tx1, ty0, ty1 = x, x + 1600, y, y + 1600
            hit = next((h for h in holes if h[0] < tx1 and h[1] > tx0 and h[2] < ty1 and h[3] > ty0), None)
            if hit is None:
                rect(tx0, tx1, ty0, ty1)
                continue
            hx0, hx1 = max(hit[0], tx0), min(hit[1], tx1)
            hy0, hy1 = max(hit[2], ty0), min(hit[3], ty1)
            rect(tx0, hx0, ty0, ty1)      # left strip
            rect(hx1, tx1, ty0, ty1)      # right strip
            rect(hx0, hx1, ty0, hy0)      # below the hole
            rect(hx0, hx1, hy1, ty1)      # above the hole
    n = b.count()
    b.flush()
    return n
