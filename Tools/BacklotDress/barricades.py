"""Car pile-ups that close every street except the route to the Metropolis Tower."""
import math
import random
import unreal
import dresslib as D
import layout as L

ROOT = 'Backlot/09_Blockades'
SEDAN = 'Street/SM_Sedan_1940s'
TRUCK = 'Street/SM_DeliveryTruck_1940s'
BARRIER = 'Street/SM_ConstructionBarrier_2m'


def _road(name):
    return next(r for r in L.ROADS if r['name'] == name)


def plans():
    """(road, s of barricade line, direction away from the route)."""
    out = []
    for nm in ('R1_WestApproachL', 'R3_WestApproachR'):
        r = _road(nm)
        st = L.STREETS[nm]
        s, _ = st.local(-45200, L.road_at(nm, -45200))
        out.append((r, s, -1))  # jam runs west, behind the spawn
    for nm in ('R30_CrossAveN', 'R33_CrossAveS'):
        r = _road(nm)
        st = L.STREETS[nm]
        hub_s = st.local(*L.HC)[0]
        away = -1 if hub_s > 0 else 1
        out.append((r, hub_s + away * (L.R_J + 2600), away))
    for nm in ('R8_TowerAveS', 'R9_TowerAveN'):
        r = _road(nm)
        st = L.STREETS[nm]
        s, _ = st.local(-2500, L.road_at(nm, -2500))
        out.append((r, s, 1))  # beyond the tower's front half
    return out


def blocker(label, folder, x, y, yaw, length, height=1000, thick=200):
    """Invisible collision wall (engine cube, hidden in editor and game)."""
    a = D.sm(label, folder, '/Engine/BasicShapes/Cube', x, y, height / 2, yaw,
             (thick / 100.0, length / 100.0, height / 100.0))
    c = a.static_mesh_component
    c.set_visibility(False)
    c.set_collision_profile_name('BlockAll')
    a.set_actor_hidden_in_game(True)
    return a


def clear_conflicts():
    """Remove parked cars inside each jam and street furniture on the pile-up line."""
    removed = 0
    zones = []
    for r, s0, away in plans():
        zones.append((r, s0, away))
    for a in D.eas.get_all_level_actors():
        f = str(a.get_folder_path())
        if not f.startswith('Backlot/06_StreetProps'):
            continue
        for c in a.get_components_by_class(unreal.HierarchicalInstancedStaticMeshComponent) or []:
            kill = []
            for i in range(c.get_instance_count()):
                t = c.get_instance_transform(i, True).translation
                for r, s0, away in zones:
                    s, p = L.STREETS[r['name']].local(t.x, t.y)
                    F = max(r['Fl'], r['Fr']) + 200
                    d = (s - s0) * away
                    if abs(p) < F and -900 < d < 9500:
                        kill.append(i)
                        break
            if kill:
                c.remove_instances(kill)
                removed += len(kill)
    return removed


def build():
    D.clear(ROOT)
    total = 0
    for r, s0, away in plans():
        st = L.STREETS[r['name']]
        rng = random.Random(r['name'])
        folder = f"{ROOT}/{r['name']}"
        b = D.Bucket(folder, r['name'].split('_')[0] + '_Pileup')
        F = max(r['Fl'], r['Fr'])
        # front line of the pile-up: crashed cars nose-to-tail across the street, slewed at angles
        p = -F + 150
        while p < F - 100:
            truck = rng.random() < 0.25
            m = TRUCK if truck else SEDAN
            ang = rng.uniform(25, 70) * rng.choice((-1, 1))
            x, y = st.pt(s0 + rng.uniform(-150, 150), p)
            b.add(m, x, y, 3, r['yaw'] + ang)
            p += rng.uniform(260, 340) if not truck else 380
        # second row offset behind it, plus barriers / barrels plugging the gaps
        p = -F + 300
        while p < F - 150:
            x, y = st.pt(s0 + away * rng.uniform(550, 750), p)
            b.add(SEDAN, x, y, 3, r['yaw'] + rng.uniform(-35, 35) + rng.choice((0, 180)))
            p += rng.uniform(320, 420)
        p = -F + 100
        while p < F:
            x, y = st.pt(s0 - away * 260, p)
            b.add(BARRIER, x, y, 3, r['yaw'] + 90 + rng.uniform(-12, 12))
            if rng.random() < 0.4:
                bx, by = st.pt(s0 - away * 380, p + rng.uniform(-60, 60))
                b.add('Street/SM_MetalBarrel', bx, by, 3, rng.uniform(0, 360))
            p += 210
        # abandoned jam filling the street beyond
        lane_w = 800
        lanes = int(2 * r['H'] // lane_w)
        for k in range(lanes):
            pl = -r['H'] + lane_w * (k + 0.5)
            if r['name'].startswith('R33') and k == 0:
                continue  # El trestle legs stand in this lane
            s = s0 + away * 1500
            while abs(s - s0) < 9000:
                if rng.random() < 0.85:
                    m = TRUCK if rng.random() < 0.18 else SEDAN
                    head = 0 if pl > 0 else 180
                    x, y = st.pt(s, pl + rng.uniform(-60, 60))
                    b.add(m, x, y, 3, r['yaw'] - 90 + head + rng.uniform(-14, 14))
                s += away * rng.uniform(620, 820)
        total += b.count()
        b.flush()
        # invisible wall along the front line, wall to wall plus overlap into the buildings
        cx, cy = st.pt(s0 + away * 300, 0)
        blocker(f"Blocker_{r['name'].split('_')[0]}", folder, cx, cy, r['yaw'], 2 * F + 1200)
    return total
