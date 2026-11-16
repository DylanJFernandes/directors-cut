"""Roads, junction, sidewalks and paved fill."""
import math
import dresslib as D
import layout as L

Z_FILL = 1.0
Z_JUNC = 1.5
Z_ROAD = 2.0
Z_WALK = 2.2

ROAD = 'Street/SM_CobblestoneRoad_4x8'
TROLLEY = 'Street/SM_TrolleyRoad_4x8'
JUNC = 'Street/SM_CobblestoneJunction_8x8'
CURB = 'Street/SM_SidewalkCurb_2x4'
DRAIN = 'Street/SM_SidewalkDrain_2x4'
SLAB = 'Street/SM_SidewalkSlab_2x4'
INFILL = 'Street/SM_SidewalkInfill_1x4'
CORNER = 'Street/SM_SidewalkCorner_2x2'


def build_roads(folder='Backlot/03_Streets/Roads'):
    D.clear(folder)
    n = 0
    for r in L.ROADS:
        st = L.STREETS[r['name']]
        b = D.Bucket(folder, r['name'])
        cols = int(round(2 * r['H'] / 400))
        mid = cols // 2
        rows = int(r['L'] // 800)
        s0 = -rows * 800 / 2
        for i in range(rows):
            s = s0 + 400 + 800 * i
            for c in range(cols):
                p = -r['H'] + 200 + 400 * c
                x, y = st.pt(s, p)
                if L.hub_dist(x, y) < L.R_J - 250:
                    continue
                m = TROLLEY if (r['trolley'] and c in (mid - 1, mid)) else ROAD
                b.add(m, x, y, Z_ROAD, r['yaw'] - 90)
        n += b.count()
        b.flush()
    return n


def build_junction(folder='Backlot/03_Streets/Junction'):
    D.clear(folder)
    b = D.Bucket(folder, 'HubJunction')
    R = int(L.R_J // 800) + 2
    for i in range(-R, R + 1):
        for j in range(-R, R + 1):
            x, y = L.HC[0] + 800 * i, L.HC[1] + 800 * j
            if L.hub_dist(x, y) < L.R_J + 150:
                b.add(JUNC, x, y, Z_JUNC, 0)
    n = b.count()
    b.flush()
    return n


def build_sidewalks(folder='Backlot/03_Streets/Sidewalks'):
    D.clear(folder)
    n = 0
    for k, r in enumerate(L.ROADS):
        st = L.STREETS[r['name']]
        z = Z_WALK + 0.15 * k
        for side, F in ((1, r['Fl']), (-1, r['Fr'])):
            b = D.Bucket(folder, f"{r['name']}_{'L' if side > 0 else 'R'}")
            rows = int(r['L'] // 400)
            s0 = -rows * 400 / 2
            yaw = st.curb_yaw(side)
            for i in range(rows):
                s = s0 + 200 + 400 * i
                edge = r['H']
                c = 0
                while edge < F:
                    p = side * (edge + 100)
                    x, y = st.pt(s, p)
                    edge += 200
                    if L.hub_dist(x, y) < L.R_J + 300:
                        c += 1
                        continue
                    if L.in_carriageway(x, y, skip=r['name'], pad=150):
                        c += 1
                        continue
                    if L.in_tower(x, y):
                        c += 1
                        continue
                    if c == 0:
                        m = DRAIN if i % 9 == 4 else CURB
                    else:
                        m = SLAB
                    b.add(m, x, y, z, yaw)
                    c += 1
            n += b.count()
            b.flush()
    return n


def _fill_tiles():
    """World-aligned 4m(X) x 2m(Y) grid anchored on the subway exit."""
    X0, Y0 = -41166.0, -1100.0
    tiles = set()

    def add_poly(poly, pad):
        xs = [p[0] for p in poly]
        ys = [p[1] for p in poly]
        for i in range(int((min(xs) - X0) // 400) - 1, int((max(xs) - X0) // 400) + 2):
            for j in range(int((min(ys) - Y0) // 200) - 1, int((max(ys) - Y0) // 200) + 2):
                cx, cy = X0 + 200 + 400 * i, Y0 + 100 + 200 * j
                hit = any(D.point_in_poly(cx + dx, cy + dy, poly)
                          for dx in (-pad, 0, pad) for dy in (-pad / 2, 0, pad / 2))
                if hit:
                    tiles.add((i, j))

    add_poly(L.MEDIAN, 200)
    add_poly(L.WEDGE, 200)
    # hub ring
    R = 9500
    ring = [(L.HC[0] + R * math.cos(math.radians(a)), L.HC[1] + R * math.sin(math.radians(a))) for a in range(0, 360, 10)]
    add_poly(ring, 200)
    out = []
    ex0, ex1, ey0, ey1 = L.EXIT_CUT
    for i, j in tiles:
        cx, cy = X0 + 200 + 400 * i, Y0 + 100 + 200 * j
        if ex0 < cx < ex1 and ey0 < cy < ey1:
            continue
        if L.hub_dist(cx, cy) < L.R_J - 400:
            continue
        if L.in_tower(cx, cy, pad=-300):
            continue
        out.append((cx, cy))
    return out


def build_fill(folder='Backlot/03_Streets/PlazaFill'):
    D.clear(folder)
    b = D.Bucket(folder, 'PlazaPaving')
    for cx, cy in _fill_tiles():
        b.add(SLAB, cx, cy, Z_FILL, 90)
    n = b.count()
    b.flush()
    return n
