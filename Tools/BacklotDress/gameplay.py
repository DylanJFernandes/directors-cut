"""Gameplay pass for LVL_NewYorkSet: street cover, guards, checkpoints, the tower
grapple climb, and the rooftop upload finale.

Everything lives under the Outliner folder G so the pass can be cleared and re-run.
Tower numbers come from line traces of SM_MetropolisTower (scale 5, sink -2400):
each tier is (front face X, roof Z, half extent Y about TY).
"""
import math
import random
import unreal
import dresslib as D
import layout as L

G = 'Backlot/10_Gameplay'
TY = L.TOWER['y']
TX = L.TOWER['x']

# front face X, roof Z, half Y, back edge X of the ring (next face)
PODIUM_X, PODIUM_TOP = -7950.0, 938.0
T16 = dict(x=-7290.0, z=1600.0, hy=5250.0, nx=-6570.0)
T55 = dict(x=-6570.0, z=5599.0, hy=4150.0, nx=-5790.0)
T95 = dict(x=-5790.0, z=9500.0, hy=3340.0, nx=-5240.0)
T220 = dict(x=-5240.0, z=22049.0, hy=2560.0, nx=-4910.0)
T248 = dict(x=-4910.0, z=24849.0, hy=2140.0, nx=-4370.0)
T266 = dict(x=-4370.0, z=26649.0, hy=1480.0, nx=-1720.0)

LANES = (TY - 1600.0, TY + 1600.0)
GUARD_Z = 92.0      # capsule half height above the floor
HOOK_LIFT = 200.0   # hook height above the ledge it lands on
CREATED = {'guards': 0, 'hooks': 0, 'perches': 0, 'cover': 0}


def _cls(name):
    return unreal.load_class(None, '/Script/DirectorsCut.' + name)


def _spawn(cls_name, label, folder, x, y, z, yaw=0.0):
    a = D.eas.spawn_actor_from_class(_cls(cls_name), unreal.Vector(x, y, z), unreal.Rotator(0, 0, yaw))
    a.set_actor_label(label)
    a.set_folder_path(folder)
    return a


def _local(dx, dy, dz, yaw):
    r = math.radians(-yaw)
    return unreal.Vector(dx * math.cos(r) - dy * math.sin(r), dx * math.sin(r) + dy * math.cos(r), dz)


def floor_z(x, y, top=3000.0, bottom=-3000.0):
    hit = D.trace_down(x, y, top, bottom)
    return hit[0] if hit else 0.0


# ---------------------------------------------------------------- actors
def hook(label, x, y, z):
    _spawn('GrappleHookPoint', label, G + '/Grapples', x, y, z)
    CREATED['hooks'] += 1


def _capsule_free(x, y, z):
    c = unreal.Vector(x, y, z)
    h = unreal.SystemLibrary.capsule_trace_single(D.world(), c, c + unreal.Vector(0, 0, 1), 40, 88,
                                                  unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, [],
                                                  unreal.DrawDebugTrace.NONE, True)
    return not (h and h.to_tuple()[0])


def _free_spot(x, y, z):
    """Nearest spot (same floor height) where a guard capsule overlaps nothing."""
    for rad in (0, 150, 300, 450, 600):
        for k in range(max(1, rad // 50)):
            a = 2 * math.pi * k / max(1, rad // 50)
            px, py = x + rad * math.cos(a), y + rad * math.sin(a)
            if _capsule_free(px, py, z + GUARD_Z + 2):
                return px, py
    return x, y


def guard(label, folder, x, y, z, yaw=0.0, patrol=()):
    """patrol: world-space points the guard walks through after its start."""
    x, y = _free_spot(x, y, z)
    a = _spawn('GuardCharacter', label, folder, x, y, z + GUARD_Z, yaw)
    pts = [_local(px - x, py - y, 0.0, yaw) for px, py in patrol]
    a.set_editor_property('PatrolPoints', pts)
    CREATED['guards'] += 1
    return a


def checkpoint(label, zone, x, y, z, ext, starts=False, help_=False, yaw=0.0):
    a = _spawn('ZoneCheckpoint', label, G + '/Checkpoints', x, y, z, yaw)
    a.set_editor_property('ZoneName', zone)
    a.set_editor_property('bStartsRun', starts)
    a.set_editor_property('bShowsControlsHelp', help_)
    a.get_editor_property('TriggerBox').set_box_extent(unreal.Vector(*ext), True)
    return a


# ---------------------------------------------------------------- cover
def _cover_cluster(b, kind, x, y, yaw, rng):
    z = floor_z(x, y)
    r = math.radians(yaw)
    ux, uy = math.cos(r), math.sin(r)        # along
    vx, vy = -uy, ux                         # across
    if kind == 'sedan':
        b.add('Street/SM_Sedan_1940s', x, y, z, yaw)
        b.add('Street/SM_WoodCrate', x + vx * 200 + ux * 120, y + vy * 200 + uy * 120, z, yaw + 12)
    elif kind == 'truck':
        b.add('Street/SM_DeliveryTruck_1940s', x, y, z, yaw)
        for k in range(2):
            b.add('Street/SM_MetalBarrel', x + vx * 190 + ux * (k * 70 - 200), y + vy * 190 + uy * (k * 70 - 200), z, 0)
    elif kind == 'crates':
        for k in range(3):
            b.add('Street/SM_WoodCrate', x + ux * (k - 1) * 120, y + uy * (k - 1) * 120, z, yaw + rng.uniform(-6, 6))
        b.add('Street/SM_WoodCrate', x, y, z + 89, yaw + 20)
        b.add('Street/SM_MetalBarrel', x + vx * 110, y + vy * 110, z, 0)
    else:  # barriers
        for k in range(3):
            b.add('Street/SM_ConstructionBarrier_2m', x + ux * (k - 1) * 205, y + uy * (k - 1) * 205, z, yaw)
        b.add('Street/SM_MetalBarrel', x - vx * 90 + ux * 330, y - vy * 90 + uy * 330, z, 0)
    CREATED['cover'] += 1


def _clear_spot(x, y):
    """True when nothing taller than the kerb already stands here."""
    for dx, dy in ((0, 0), (250, 0), (-250, 0), (0, 250), (0, -250)):
        if floor_z(x + dx, y + dy) > 60:
            return False
    ex0, ex1, ey0, ey1 = L.EXIT_CUT
    return not (ex0 - 500 < x < ex1 + 500 and ey0 - 500 < y < ey1 + 500)


def _corridor(x):
    """Usable street half-width (y0, y1) at world X on the route to the tower."""
    if x < -26000:
        return (-3600.0, 2800.0)
    if x < -20000:
        return (-4000.0, 3200.0)
    lo = L.road_at('R8_TowerAveS', x) + 1200
    hi = L.road_at('R9_TowerAveN', x) - 1200
    return (max(lo, -6000.0), min(hi, 6500.0))


def build_street():
    """Cover clusters every ~18 m from the subway to the podium, and the street guards."""
    rng = random.Random(7)
    b = D.Bucket(G + '/Cover/Street', 'Cover')
    kinds = ['sedan', 'crates', 'truck', 'barriers', 'crates', 'sedan']
    x = -39600.0
    i = 0
    stations = []
    while x < -9200:
        y0, y1 = _corridor(x)
        lanes = [y0 + (y1 - y0) * f for f in ((0.2, 0.62), (0.4, 0.85), (0.12, 0.5))[i % 3]]
        for j, ly in enumerate(lanes):
            cx, cy = x + rng.uniform(-300, 300), ly + rng.uniform(-200, 200)
            if L.hub_dist(cx, cy) < 900 or not _clear_spot(cx, cy):
                continue
            _cover_cluster(b, kinds[(i + j) % len(kinds)], cx, cy, rng.choice((0, 90, 25, -30, 70)), rng)
        stations.append((x, y0, y1))
        x += 1800.0
        i += 1
    b.flush()

    # street guards: alternate cross-street sweeps and along-street walks
    F = G + '/Guards/Street'
    n = 0
    for k, (sx, y0, y1) in enumerate(stations[1:], 1):
        mid = (y0 + y1) / 2
        z = floor_z(sx + 900, mid)
        if k % 3 == 1:
            guard(f'Guard_Street_{n:02d}', F, sx + 900, y0 + 600, z, 90,
                  [(sx + 900, y1 - 600)])
        elif k % 3 == 2:
            gy = mid + (y1 - mid) * 0.5
            guard(f'Guard_Street_{n:02d}', F, sx + 400, gy, floor_z(sx + 400, gy), 180,
                  [(sx + 400, gy - 1600)])
        else:
            guard(f'Guard_Street_{n:02d}', F, sx, mid + 800, z, 0,
                  [(sx + 2600, mid + 800), (sx + 2600, mid - 1200), (sx, mid - 1200)])
        n += 1
    # plaza sentries in front of the podium
    for k, ly in enumerate((TY - 3200, TY, TY + 3200)):
        guard(f'Guard_Plaza_{k}', F, -9300, ly, floor_z(-9300, ly), 180,
              [(-9300, ly + (1400 if k != 1 else -1400))])
    return stations


# ---------------------------------------------------------------- tower climb
def perch(label, face_x, y, z, wide=False):
    """Scaffold balcony bolted to a -X facing wall, deck top at z. Returns the stand point."""
    b = D.Bucket(G + '/Perches/' + label, label)
    depth = 4 if wide else 2
    spans = (-203, 203)
    for k in range(depth):
        cx = face_x - 78 - 156 * k
        for sy in spans:
            b.add('Infrastructure/SM_ScaffoldDeck_4m', cx, y + sy, z, 0)
    outer = face_x - 156 * depth
    for sy in spans:
        b.add('Infrastructure/SM_ScaffoldTruss_4m', outer + 38, y + sy, z - 75, 0)
        b.add('Infrastructure/SM_ScaffoldTruss_4m', face_x - 38, y + sy, z - 75, 0)
    half = 203 * len(spans)
    b.add('Infrastructure/SM_Handrail_2m', outer + 3, y - half + 103, z, 90)
    b.add('Infrastructure/SM_Handrail_2m', outer + 3, y + half - 103, z, 90)
    b.add('Signage/SM_CagedWorkLight', face_x - 20, y, z + 260, 180, collision=False)
    b.flush()
    CREATED['perches'] += 1
    return (face_x - 78 * depth, y, z)


def ledge_hook(label, tier, y):
    """Hook just outside the lip of a tier roof; the exit hop lands on the roof."""
    hook(label, tier['x'] - 15, y, tier['z'] + HOOK_LIFT)


def perch_hook(label, stand):
    hook(label, stand[0], stand[1], stand[2] + 150)


def roof_cover(prefix, tier, ys, rng):
    """Vents, crates and chimneys along the front ring of a tier for hiding."""
    b = D.Bucket(G + '/Cover/' + prefix, prefix)
    mid = (tier['x'] + tier['nx']) / 2
    width = tier['nx'] - tier['x']
    pieces = ['Rooftop/SM_HVACVent', 'Street/SM_WoodCrate', 'Rooftop/SM_BrickChimney', 'Rooftop/SM_HVACVent']
    for k, y in enumerate(ys):
        x = mid + rng.uniform(-0.15, 0.15) * width
        p = pieces[k % len(pieces)]
        b.add(p, x, y, tier['z'], rng.choice((0, 90)), scale=1.6 if 'HVAC' in p else 1.0)
        if p == 'Street/SM_WoodCrate':
            b.add(p, x + 120, y + 20, tier['z'], 8)
            b.add(p, x + 60, y, tier['z'] + 89, 30)
        CREATED['cover'] += 1
    b.flush()


def ring_guard(label, tier, y_from, y_to, x_frac=0.5):
    x = tier['x'] + (tier['nx'] - tier['x']) * x_frac
    guard(label, G + '/Guards/Tower', x, y_from, tier['z'], 90 if y_to > y_from else -90, [(x, y_to)])


def build_tower():
    rng = random.Random(23)
    for li, ly in enumerate(LANES):
        tag = 'S' if li == 0 else 'N'
        # podium wall top: scaffold landing bridged over the wall
        pb = D.Bucket(G + '/Perches/Podium' + tag, 'Podium' + tag)
        for sy in (-80, 80):
            pb.add('Infrastructure/SM_ScaffoldDeck_4m', PODIUM_X, ly + sy, PODIUM_TOP + 10, 90)
        pb.add('Infrastructure/SM_Handrail_2m', PODIUM_X + 200, ly, PODIUM_TOP + 10, 90)
        pb.flush()
        hook(f'Hook_{tag}0_Podium', PODIUM_X - 60, ly, PODIUM_TOP + 10 + 160)
        ledge_hook(f'Hook_{tag}1_T16', T16, ly)
        st = perch(f'Perch_{tag}_T55', T55['x'], ly, T16['z'] + 2000)
        perch_hook(f'Hook_{tag}2_T55Perch', st)
        ledge_hook(f'Hook_{tag}3_T55', T55, ly)
        st = perch(f'Perch_{tag}_T95', T95['x'], ly, T55['z'] + 2000)
        perch_hook(f'Hook_{tag}4_T95Perch', st)
        ledge_hook(f'Hook_{tag}5_T95', T95, ly)

    # shaft: zig-zag of perches up the 125 m face between the 95 and 220 roofs
    z = T95['z'] + 1800
    k = 0
    while z < T220['z'] - 1500:
        side = 1 if k % 2 == 0 else -1
        y = TY + 650 * side
        st = perch(f'Perch_Shaft{k}', T220['x'], y, z, wide=(k == 3))
        # hook just past the inner deck edge so the deck never blocks the view from below
        hook(f'Hook_Shaft{k}', T220['x'] - 160, y - 420 * side, z + 150)
        if k == 3:
            guard('Guard_ShaftBalcony', G + '/Guards/Tower', st[0] - 60, y - 250, z, 180)
        z += 1800
        k += 1
    ledge_hook('Hook_T220', T220, TY)
    # offset sideways so the deck is never overhead on the narrow 220 ring; hook at its inner edge
    perch('Perch_T248', T248['x'], TY + 900, T220['z'] + 1500)
    hook('Hook_T248Perch', T248['x'] - 160, TY + 480, T220['z'] + 1650)
    ledge_hook('Hook_T248', T248, TY)
    ledge_hook('Hook_T266', T266, TY + 600)
    ledge_hook('Hook_T266b', T266, TY - 600)

    # guards on every setback, cover to slip between them
    ring_guard('Guard_T16_Front', T16, TY - 4300, TY + 4300, 0.55)
    ring_guard('Guard_T16_S', T16, TY - 600, TY - 4300, 0.3)
    ring_guard('Guard_T16_N', T16, TY + 600, TY + 4300, 0.7)
    roof_cover('T16', T16, [TY + d for d in (-3800, -2300, -800, 800, 2300, 3800)], rng)
    ring_guard('Guard_T55_A', T55, TY - 3500, TY + 3500, 0.5)
    ring_guard('Guard_T55_B', T55, TY + 2500, TY - 2500, 0.65)
    roof_cover('T55', T55, [TY + d for d in (-3000, -1000, 1000, 3000)], rng)
    ring_guard('Guard_T95_A', T95, TY - 2900, TY + 2900, 0.5)
    ring_guard('Guard_T95_B', T95, TY + 2000, TY - 2000, 0.5)
    roof_cover('T95', T95, [TY + d for d in (-2400, -400, 1400)], rng)
    ring_guard('Guard_T220', T220, TY - 2200, TY + 2200, 0.5)
    roof_cover('T220', T220, [TY + d for d in (-1600, 1600)], rng)
    ring_guard('Guard_T248_A', T248, TY - 1800, TY + 1800, 0.5)
    ring_guard('Guard_T248_B', T248, TY + 1200, TY - 1200, 0.5)
    roof_cover('T248', T248, [TY + d for d in (-1300, 1300)], rng)


def build_roof():
    """Upload terminal, tommy gun, wave doors and defensive cover on the 266 m roof."""
    z = T266['z']
    F = G + '/Roof'
    term = _spawn('UploadTerminal', 'UploadTerminal_Roof', F, TX + 200, TY, z, 180)
    # wave doors at the back corners and sides of the roof (local to the terminal, yaw 180)
    doors_world = [(TX + 1100, TY - 1250), (TX + 1100, TY + 1250), (TX - 300, TY - 1350), (TX - 300, TY + 1350)]
    term.set_editor_property('GuardSpawnPoints',
                             [_local(dx - (TX + 200), dy - TY, 100, 180) for dx, dy in doors_world])
    _spawn('TommyGunPickup', 'TommyGun_Roof', F, TX - 700, TY + 350, z + 110)
    b = D.Bucket(F + '/Dressing', 'Roof')
    b.add('Rooftop/SM_WoodWaterTower', TX + 900, TY, z, 0)
    for dx, dy, p, yaw in ((-400, -700, 'Rooftop/SM_HVACVent', 0), (-400, 900, 'Rooftop/SM_HVACVent', 90),
                           (300, -900, 'Rooftop/SM_BrickChimney', 0), (300, 900, 'Rooftop/SM_BrickChimney', 0),
                           (-1000, -300, 'Street/SM_WoodCrate', 10), (-1000, 400, 'Street/SM_WoodCrate', -15),
                           (600, 300, 'Street/SM_ConstructionBarrier_2m', 90),
                           (600, -400, 'Street/SM_ConstructionBarrier_2m', 90)):
        b.add(p, TX + dx, TY + dy, z, yaw, scale=1.6 if 'HVAC' in p else 1.0)
    b.add('Signage/SM_CagedWorkLight', TX + 200, TY, z + 320, 0, collision=False)
    b.flush()
    guard('Guard_Roof_A', G + '/Guards/Roof', TX - 900, TY - 1100, z, 0, [(TX + 600, TY - 1100)])
    guard('Guard_Roof_B', G + '/Guards/Roof', TX + 600, TY + 1100, z, 180, [(TX - 900, TY + 1100)])
    guard('Guard_Roof_C', G + '/Guards/Roof', TX + 1000, TY - 400, z, 90, [(TX + 1000, TY + 500)])


def build_checkpoints(stations):
    ps = next(a for a in D.eas.get_all_level_actors() if a.get_class().get_name() == 'PlayerStart')
    p = ps.get_actor_location()
    checkpoint('CP0_SubwayExit', 'The Subway Exit', p.x, p.y, p.z, (300, 600, 300), True, True,
               ps.get_actor_rotation().yaw)
    for label, zone, x in (('CP1_WestApproach', 'West Approach', -38800),
                           ('CP2_Crossing', 'The Crossing', -24000),
                           ('CP3_TowerAvenue', 'Tower Avenue', -15000)):
        y0, y1 = _corridor(x)
        checkpoint(label, zone, x, (y0 + y1) / 2, floor_z(x, (y0 + y1) / 2) + 100,
                   (300, (y1 - y0) / 2 + 1500, 600))
    for label, zone, t, hy in (('CP4_Podium', 'The Podium', T16, 4800),
                               ('CP5_FirstSetback', 'First Setback', T55, 3800),
                               ('CP6_SecondSetback', 'Second Setback', T95, 3000),
                               ('CP7_Spire', 'The Spire', T220, 2300),
                               ('CP8_Crown', 'The Crown', T248, 1900)):
        cx = (t['x'] + t['nx']) / 2
        checkpoint(label, zone, cx, TY, t['z'] + 110, ((t['nx'] - t['x']) / 2 - 20, hy, 250))
    checkpoint('CP9_Roof', 'The Roof - Upload', T266['x'] + 400, TY, T266['z'] + 110, (300, 1300, 250))


def build_nav():
    F = G + '/Navigation'
    for label, c, e in (('Nav_Street', (-25000, 0, 600), (17500, 9000, 1600)),
                        ('Nav_Tower', (TX - 1000, TY, 14500), (6500, 6200, 13000))):
        a = D.eas.spawn_actor_from_class(unreal.NavMeshBoundsVolume, unreal.Vector(*c))
        a.set_actor_scale3d(unreal.Vector(e[0] / 100, e[1] / 100, e[2] / 100))
        a.set_actor_label(label)
        a.set_folder_path(F)


def build():
    for k in CREATED:
        CREATED[k] = 0
    D.clear(G)
    stations = build_street()
    build_tower()
    build_roof()
    build_checkpoints(stations)
    build_nav()
    return dict(CREATED)
