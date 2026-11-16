"""Street layout for LVL_NewYorkSet, measured from the CubeGrid proxies."""
import math
import dresslib as D

HC = (-24000.0, -700.0)  # hub where the two bent avenues pinch and the cross avenue passes
R_J = 5200.0             # junction disc radius

# Tower: SM_MetropolisTower, entrance (+Y local) faces -X toward the subway.
TOWER = dict(x=-3045.0, y=449.0, scale=5.0, sink=-2400.0, yaw=90.0)
# world footprint at scale 5: X span 1794*5, Y span 2172*5
TOWER_HX = 1794 * 5 / 2 + 400   # podium half extents (X)
TOWER_HY = 2172 * 5 / 2 + 400   # (Y)

# name, proxy label, centre, yaw, length, kit half width H, F_left, F_right, trolley
ROADS = [
    dict(name='R1_WestApproachL', proxy='CubeGridToolOutput', c=(-46456, -5702), yaw=10, L=46602, H=1600,
         Fl=1600 + 400, Fr=2908, trolley=False),
    dict(name='R3_WestApproachR', proxy='CubeGridToolOutput3', c=(-46909, 4193), yaw=-10, L=46450, H=1600,
         Fl=2326, Fr=1600 + 400, trolley=False),
    dict(name='R8_TowerAveS', proxy='CubeGridToolOutput8', c=(14861, -16172), yaw=-20, L=82500, H=1600,
         Fl=2230, Fr=2254, trolley=False),
    dict(name='R9_TowerAveN', proxy='CubeGridToolOutput9', c=(12059, 18113), yaw=25, L=82500, H=1600,
         Fl=2381, Fr=2103, trolley=False),
    dict(name='R30_CrossAveN', proxy='CubeGridToolOutput30', c=(-14870, 46861), yaw=-100, L=93587, H=3600,
         Fl=3969, Fr=4198, trolley=True),
    dict(name='R33_CrossAveS', proxy='CubeGridToolOutput33', c=(-15054, -48089), yaw=-80, L=93587, H=3600,
         Fl=4105, Fr=3883, trolley=True),
]
# sidewalk-strip proxies that the kit sidewalks replace
STRIP_PROXIES = ['CubeGridToolOutput4', 'CubeGridToolOutput20', 'CubeGridToolOutput11', 'CubeGridToolOutput49',
                 'CubeGridToolOutput25', 'CubeGridToolOutput50', 'CubeGridToolOutput31']


def street(r):
    return D.Street(r['name'], r['c'][0], r['c'][1], r['yaw'])


STREETS = {r['name']: street(r) for r in ROADS}


def hub_dist(x, y):
    return math.hypot(x - HC[0], y - HC[1])


def in_carriageway(x, y, skip=None, pad=0.0):
    for r in ROADS:
        if r['name'] == skip:
            continue
        s, p = STREETS[r['name']].local(x, y)
        if abs(s) <= r['L'] / 2 and abs(p) < r['H'] + pad:
            return r['name']
    return None


def in_tower(x, y, pad=0.0):
    return abs(x - TOWER['x']) < TOWER_HX + pad and abs(y - TOWER['y']) < TOWER_HY + pad


def road_at(name, x):
    """Y of a road centreline at world X."""
    r = next(r for r in ROADS if r['name'] == name)
    return r['c'][1] + math.tan(math.radians(r['yaw'])) * (x - r['c'][0])


# fill polygons (world-aligned paving under everything else)
MEDIAN = [(-44000, road_at('R1_WestApproachL', -44000)), HC, (-44000, road_at('R3_WestApproachR', -44000))]
WEDGE = [HC, (4000, road_at('R8_TowerAveS', 4000)), (4000, road_at('R9_TowerAveN', 4000))]
EXIT_CUT = (-41966, -41166, -1100, -700)  # x0, x1, y0, y1 opening for the subway exit
