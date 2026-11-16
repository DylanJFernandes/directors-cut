"""Metropolis Tower with a street-scale Art Deco podium."""
import math
import dresslib as D
import layout as L
import facades as FA

F = 'Backlot/02_MetropolisTower'


def face_line(cx, cy, length, face_yaw):
    r = math.radians(face_yaw)
    u = (math.cos(r), math.sin(r))
    return FA.Line(cx - u[0] * length / 2, cy - u[1] * length / 2, cx + u[0] * length / 2, cy + u[1] * length / 2,
                   face_yaw)


def build():
    D.clear(F)
    T = L.TOWER
    D.sm('SM_MetropolisTower', F, 'Landmarks/SM_MetropolisTower', T['x'], T['y'], T['sink'], T['yaw'], T['scale'])
    x0, x1 = T['x'] - L.TOWER_HX, T['x'] + L.TOWER_HX
    y0, y1 = T['y'] - L.TOWER_HY, T['y'] + L.TOWER_HY
    b = D.Bucket(F + '/Podium', 'Podium')
    # front (faces -X, toward the subway), back (+X), sides
    sides = [
        ('front', face_line(x0, T['y'], y1 - y0, 90)),
        ('back', face_line(x1, T['y'], y1 - y0, -90)),
        ('south', face_line(T['x'], y0, x1 - x0, 180)),
        ('north', face_line(T['x'], y1, x1 - x0, 0)),
    ]
    for name, ln in sides:
        if name == 'front':
            mid = ln.len / 2
            ent0, ent1 = mid - 800, mid + 800
            FA.run(b, ln, 0, FA.DECO, 0, ent0, pil_every=1)
            FA.run(b, ln, 0, FA.DECO, ent1, ln.len, pil_every=1)
            for k in range(4):
                x, y = ln.at(ent0 + 200 + 400 * k)
                b.add(FA.RECESSED, x, y, 0, ln.yaw)
            for tc in (ent0, ent1):
                x, y = ln.at(tc, 4)
                b.add(FA.PIL_W, x, y, 0, ln.yaw, scale=(1.4, 1.2, 1.0))
                b.add(FA.PIL_W, x, y, 400, ln.yaw, scale=(1.4, 1.2, 1.0))
            # marquee canopy over the doors
            for k in range(4):
                x, y = ln.at(ent0 + 200 + 400 * k, 40)
                b.add('Signage/SM_MarqueeBlank_4m', x, y, 330, ln.yaw)
        else:
            FA.run(b, ln, 0, FA.DECO, pil_every=1)
        FA.run(b, ln, 400, FA.WIN, pil_every=2)
        FA.band(b, ln, 400, FA.CORNICE, out=10)
        FA.band(b, ln, 800, FA.CORNICE, out=10)
        FA.band(b, ln, 830, FA.STEP_PARAPET, out=0)
    b.flush()
    for p in ('CubeGridToolOutput2', 'CubeGridToolOutput31'):
        D.disable_proxy(p)
    return (x0, x1, y0, y1)
