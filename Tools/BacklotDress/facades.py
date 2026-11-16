"""Facade runs: lay +Y-facing wall modules along a line with pilaster caps."""
import math
import random
import dresslib as D

A = 'Architecture/'
STORE = {400: A + 'SM_Storefront_4m', 200: A + 'SM_Storefront_2m', 100: A + 'SM_Storefront_1m'}
WIN = {600: A + 'SM_WindowWall_6m_2Bay', 400: A + 'SM_WindowWall_4m_2Bay', 200: A + 'SM_WindowWall_2m_1Bay'}
BRICK = {400: A + 'SM_BlankBrickWall_4m', 200: A + 'SM_BlankBrickWall_2m'}
DECO = {600: A + 'SM_ArtDecoWall_6m'}
PIL_W = A + 'SM_ArtDecoPilaster_Wide'
PIL_N = A + 'SM_ArtDecoPilaster_Narrow'
CORNICE = A + 'SM_DentilCornice_4m'
PARAPET = A + 'SM_Parapet_4m'
STEP_PARAPET = A + 'SM_SteppedParapet_4m'
RECESSED = A + 'SM_RecessedStorefront_4m'


class Line:
    """Wall line from a to b; modules face to the right of a->b when face_yaw is derived."""

    def __init__(self, ax, ay, bx, by, face_yaw):
        self.ax, self.ay, self.bx, self.by = ax, ay, bx, by
        self.len = math.hypot(bx - ax, by - ay)
        self.u = ((bx - ax) / self.len, (by - ay) / self.len)
        self.yaw = face_yaw
        r = math.radians(face_yaw)
        self.f = (-math.sin(r), math.cos(r))  # local +Y in world = facing direction

    def at(self, t, out=0.0):
        return (self.ax + self.u[0] * t + self.f[0] * out, self.ay + self.u[1] * t + self.f[1] * out)


def plan(length, sizes):
    """Greedy split of length into module widths (largest first), centred; returns (offset, [widths])."""
    sizes = sorted(sizes, reverse=True)
    widths = []
    rem = length
    for w in sizes:
        while rem >= w:
            widths.append(w)
            rem -= w
    return rem / 2.0, widths


def run(b, line, z, family, t0=0.0, t1=None, out=0.0, pattern=None, pilasters=True, pil_every=0, rng=None):
    """Fill [t0, t1] of line with modules from family {width: mesh}. pattern(i, w) may return an override mesh.

    Returns the list of (t_centre, width) placed.
    """
    if t1 is None:
        t1 = line.len
    off, widths = plan(t1 - t0, family.keys())
    # put the big modules in the middle and small ones at the ends
    big = [w for w in widths if w == max(widths)] if widths else []
    small = [w for w in widths if w != max(widths)]
    order = small[: len(small) // 2] + big + small[len(small) // 2:]
    t = t0 + off
    placed = []
    for i, w in enumerate(order):
        c = t + w / 2.0
        m = family[w]
        if pattern:
            m = pattern(i, w, m) or m
        x, y = line.at(c, out)
        b.add(m, x, y, z, line.yaw)
        placed.append((c, w))
        if pilasters and pil_every and i % pil_every == 0 and i > 0:
            px, py = line.at(t, out + 2)
            b.add(PIL_N, px, py, z, line.yaw)
        t += w
    if pilasters and off > 10:
        for tc in (t0 + off / 2.0, t1 - off / 2.0):
            x, y = line.at(tc, out)
            b.add(PIL_W, x, y, z, line.yaw, scale=(max(off / 96.0, 0.5), 1.0, 1.0))
    return placed


def band(b, line, z, mesh, t0=0.0, t1=None, out=0.0, w=400):
    """Continuous band of a 4m trim (cornice/parapet), overlapping ends to avoid gaps."""
    if t1 is None:
        t1 = line.len
    n = max(1, int(math.ceil((t1 - t0) / w)))
    step = (t1 - t0) / n
    sc = step / w
    for i in range(n):
        x, y = line.at(t0 + step * (i + 0.5), out)
        b.add(mesh, x, y, z, line.yaw, scale=(sc, 1.0, 1.0))
