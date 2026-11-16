"""Street furniture, parked vehicles, subway-island edge, elevated rail and skyline airships."""
import json
import math
import random
import dresslib as D
import layout as L

P = 'Backlot/06_StreetProps'
LAMP = 'Signage/SM_Streetlamp_DoubleGlobe'


def _ok(x, y, rname, entrances, hub_pad=1700):
    if L.hub_dist(x, y) < L.R_J + hub_pad:
        return False
    if L.in_carriageway(x, y, skip=rname, pad=200):
        return False
    if L.in_tower(x, y, pad=300):
        return False
    for ex, ey, _ in entrances:
        if abs(ex - x) < 300 and abs(ey - y) < 300:
            return False
    return True


def build_furniture(entrances):
    D.clear(P)
    rng = random.Random(11)
    totals = {}
    for r in L.ROADS:
        st = L.STREETS[r['name']]
        cross = r['name'].startswith('R30') or r['name'].startswith('R33')
        for side, F in ((1, r['Fl']), (-1, r['Fr'])):
            median = F <= r['H'] + 400
            b = D.Bucket(f"{P}/{r['name']}", f"{r['name'].split('_')[0]}{'L' if side > 0 else 'R'}")
            width = F - r['H']
            lamp_step = 2400
            hub_s0 = st.local(*L.HC)[0]
            sign_s = hub_s0 - (1 if hub_s0 > 0 else -1) * (L.R_J + 1100)
            s = -r['L'] / 2 + 600
            k = 0
            while s < r['L'] / 2 - 600:
                px = side * (r['H'] + 70)
                x, y = st.pt(s, px)
                if _ok(x, y, r['name'], entrances):
                    b.add(LAMP, x, y, 3, r['yaw'])
                    if cross and k % 2 == 0:
                        ux, uy = st.pt(s + 1200, side * (r['H'] + 60))
                        if _ok(ux, uy, r['name'], entrances):
                            b.add('Street/SM_UtilityPole', ux, uy, 3, r['yaw'])
                # one item between lamps, in the furniture zone next to the curb
                sm = s + (600 if cross else 1200)
                fx, fy = st.pt(sm, side * (r['H'] + 90))
                if not median and abs(sm - sign_s) > 700 and _ok(fx, fy, r['name'], entrances):
                    roll = k % 6
                    face = r['yaw'] if side > 0 else r['yaw'] + 180  # bench back to the kerb
                    if roll == 0 and width >= 400:
                        b.add('Street/SM_ParkBench', *st.pt(sm, side * (r['H'] + 110)), 3, face)
                    elif roll == 1:
                        b.add('Street/SM_TrashCan', fx, fy, 3, rng.uniform(0, 360))
                    elif roll == 2:
                        b.add('Street/SM_FireHydrant', *st.pt(sm, side * (r['H'] + 60)), 3, face)
                    elif roll == 3:
                        b.add('Street/SM_USMailbox', fx, fy, 3, face + 180)
                    elif roll == 4 and width >= 400:
                        b.add('Street/SM_ParkBench', *st.pt(sm, side * (r['H'] + 110)), 3, face)
                    elif roll == 5:
                        b.add('Street/SM_TrashCan', fx, fy, 3, rng.uniform(0, 360))
                s += lamp_step
                k += 1
            # street-name signpost at the hub-facing end of each sidewalk
            hub_s = st.local(*L.HC)[0]
            for ds in (L.R_J + 1100, ):
                sgn = 1 if hub_s > 0 else -1
                sx, sy = st.pt(hub_s - sgn * ds, side * (r['H'] + 80))
                if not L.in_carriageway(sx, sy, skip=r['name'], pad=100):
                    b.add('Street/SM_StreetSignpost', sx, sy, 3, r['yaw'])
                    for o in (-160, 160):
                        bx, by = st.pt(hub_s - sgn * ds + o, side * (r['H'] + 40))
                        b.add('Street/SM_CastIronBollard', bx, by, 3, 0)
            totals[b.prefix] = b.count()
            b.flush()
    return totals


def build_vehicles(avoid=()):
    F = f'{P}/ParkedVehicles'
    rng = random.Random(3)
    b = D.Bucket(F, 'Parked')
    el = next(r for r in L.ROADS if r['name'] == 'R33_CrossAveS')
    for r in L.ROADS:
        st = L.STREETS[r['name']]
        for side, Fv in ((1, r['Fl']), (-1, r['Fr'])):
            if Fv <= r['H'] + 400:
                continue  # keep the median kerbs (player route) clear
            s = -r['L'] / 2 + 1500
            while s < r['L'] / 2 - 1500:
                x, y = st.pt(s, side * (r['H'] - 160))
                clear = all(math.hypot(x - ax, y - ay) > 1000 for ax, ay in avoid)
                under_el = r is el and side < 0  # trestle legs stand in this lane
                if clear and not under_el and L.hub_dist(x, y) > L.R_J + 2500 and rng.random() < 0.45:
                    yaw = r['yaw'] - 90 if side < 0 else r['yaw'] + 90
                    b.add('Street/SM_Sedan_1940s', x, y, 3, yaw + rng.uniform(-1.5, 1.5))
                s += rng.choice((900, 1400, 2100))
    n = b.count()
    b.flush()
    return n


def build_alley_trucks():
    """A delivery truck at the kerb beside every third alley gate."""
    F = f'{P}/ParkedVehicles'
    b = D.Bucket(F, 'Delivery')
    n = 0
    trucks = []
    for a in D.eas.get_all_level_actors():
        if not a.get_actor_label().endswith('_Gate_OrnateIronGate_4m'):
            continue
        comp = a.get_components_by_class(D.unreal.HierarchicalInstancedStaticMeshComponent)[0]
        t = comp.get_instance_transform(0, True)
        gx, gy = t.translation.x, t.translation.y
        best = None
        for r in L.ROADS:
            s, p = L.STREETS[r['name']].local(gx, gy)
            if abs(s) < r['L'] / 2 and (best is None or abs(abs(p) - r['H']) < best[0]):
                best = (abs(abs(p) - r['H']), r, s, 1 if p > 0 else -1)
        n += 1
        if n % 3 != 1 or best is None:
            continue
        _, r, s, side = best
        st = L.STREETS[r['name']]
        x, y = st.pt(s + 900, side * (r['H'] - 170))
        yaw = r['yaw'] - 90 if side < 0 else r['yaw'] + 90
        b.add('Street/SM_DeliveryTruck_1940s', x, y, 3, yaw)
        trucks.append((x, y))
        # crates stacked against the facade beside the gate, clear of the kerb furniture
        face = abs(L.STREETS[r['name']].local(gx, gy)[1]) - 30
        cx, cy = st.pt(s + 300, side * (face - 115))
        b.add('Street/SM_WoodCrate', cx, cy, 3, r['yaw'] + 8)
        b.add('Street/SM_WoodCrate', cx, cy, 92, r['yaw'] - 5)
    c = b.count()
    b.flush()
    return c, trucks


def build_subway_island():
    """Close the void edge behind the subway exit and dress the exit head."""
    F = 'Backlot/01_SubwayExit/Island'
    D.clear(F)
    b = D.Bucket(F, 'Island')
    x_edge = -43980
    y0 = L.road_at('R1_WestApproachL', x_edge) + 1600 + 100
    y1 = L.road_at('R3_WestApproachR', x_edge) - 1600 - 100
    n = int(math.ceil((y1 - y0) / 420))
    step = (y1 - y0) / n
    for i in range(n):
        b.add('Street/SM_PerimeterFence_4m', x_edge, y0 + step * (i + 0.5), 3, 90, scale=(step / 422 + 0.01, 1, 1))
    # bollards flanking the exit mouth, lamp and sign on the approach side
    for yy in (-1250, -550):
        for xx in (-41000, -40800):
            b.add('Street/SM_CastIronBollard', xx, yy, 3, 0)
    b.add(LAMP, -40850, -1700, 3, 0)
    b.add(LAMP, -40850, -100, 3, 0)
    b.add('Street/SM_StreetSignpost', -40700, -1850, 3, 0)
    b.add('Street/SM_TrashCan', -40800, 100, 3, 20)
    b.add('Street/SM_ParkBench', -42800, -2200, 3, 90)
    b.add('Street/SM_ParkBench', -42800, 400, 3, 90)
    c = b.count()
    b.flush()
    return c


def build_plazas():
    """Lamps and benches on the tower forecourt and the median approach, route kept clear."""
    F = f'{P}/Plazas'
    b = D.Bucket(F, 'Forecourt')
    T = L.TOWER
    fx = T['x'] - L.TOWER_HX
    for yy in range(-4400, 5600, 1200):
        b.add('Street/SM_CastIronBollard', fx - 900, T['y'] + yy, 3, 0)
    for yy in (-3600, 4400):
        b.add(LAMP, fx - 1100, T['y'] + yy, 3, 90)
        b.add('Street/SM_ParkBench', fx - 1500, T['y'] + yy, 3, 90)
    # median: twin lamp rows 6m either side of the centreline leading to the hub
    for x in range(-39000, -30000, 3000):
        cy = (L.road_at('R1_WestApproachL', x) + L.road_at('R3_WestApproachR', x)) / 2
        half = (L.road_at('R3_WestApproachR', x) - L.road_at('R1_WestApproachL', x)) / 2 - 1600
        if half > 900:
            for sgn in (-1, 1):
                b.add(LAMP, x, cy + sgn * min(600, half - 300), 3, 0)
    c = b.count()
    b.flush()
    return c


def build_el():
    F = 'Backlot/07_ElevatedRail'
    D.clear(F)
    r = next(r for r in L.ROADS if r['name'] == 'R33_CrossAveS')
    st = L.STREETS[r['name']]
    hub_s = st.local(*L.HC)[0]
    sgn = -1 if hub_s > 0 else 1  # walk away from the hub
    p_t = -(r['H'] - 260)  # over the outer lane on the right side
    b = D.Bucket(F, 'El')
    yaw = r['yaw'] - 90
    z_top = 579
    s_start = hub_s + sgn * (L.R_J + 3500)
    length = 44000
    nseg = int(length // 800)
    for i in range(nseg):
        s = s_start + sgn * (400 + 800 * i)
        x, y = st.pt(s, p_t)
        b.add('Transit/SM_ElTrack_8m', x, y, z_top + 92, yaw)
        if i % 2 == 0:
            tx, ty = st.pt(s - sgn * 400, p_t)
            b.add('Transit/SM_ElTrestle_4m', tx, ty, 3, yaw)
    # station a third of the way down, platform alongside the track on the kerb side
    s_st = s_start + sgn * 12000
    sx, sy = st.pt(s_st, p_t - 420)
    D.sm('ElevatedStation', F, 'Transit/SM_ElevatedStation', sx, sy, 3, yaw if sgn > 0 else yaw + 180)
    cx, cy = st.pt(s_st + sgn * 400, p_t)
    D.sm('ElevatedTrainCar', F, 'Transit/SM_ElevatedTrainCar', cx, cy, z_top + 92 + 10, yaw)
    c = b.count()
    b.flush()
    return c, (sx, sy)


def build_airships():
    F = 'Backlot/08_Skyline'
    D.clear(F)
    ships = [(-9000, -11000, 33000, 20, 3.0), (12000, 16000, 26000, 205, 3.4),
             (-30000, 21000, 40000, 120, 2.6), (24000, -14000, 45000, 300, 3.8)]
    for i, (x, y, z, yaw, sc) in enumerate(ships):
        D.sm(f'Airship_{i + 1}', F, 'Landmarks/SM_Airship', x, y, z, yaw, sc, collision=False)
    return len(ships)
