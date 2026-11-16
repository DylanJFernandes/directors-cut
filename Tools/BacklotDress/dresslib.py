"""Placement helpers for dressing LVL_NewYorkSet with the BacklotKit.

Loaded from editor Python via sys.path. Every generated actor lives under the
Outliner folder root ROOT so a pass can be cleared and re-run.
"""
import math
import unreal

ROOT = 'Backlot'
K = '/Game/Environment/BacklotKit/'

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
sds = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
_mc = {}


def mesh(path):
    if not path.startswith('/Game/'):
        path = K + path
    if path not in _mc:
        m = unreal.load_asset(path)
        if m is None:
            raise RuntimeError('missing mesh ' + path)
        _mc[path] = m
    return _mc[path]


def world():
    return unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()


def clear(folder):
    """Destroy every actor whose Outliner folder starts with folder."""
    n = 0
    for a in eas.get_all_level_actors():
        f = str(a.get_folder_path())
        if f == folder or f.startswith(folder + '/'):
            eas.destroy_actor(a)
            n += 1
    return n


def _xf(t):
    x, y, z, yaw = t[0], t[1], t[2], t[3]
    s = t[4] if len(t) > 4 else 1.0
    if not isinstance(s, (tuple, list)):
        s = (s, s, s)
    return unreal.Transform(unreal.Vector(x, y, z), unreal.Rotator(0, 0, yaw), unreal.Vector(*s))


def ism(label, folder, mesh_path, xforms, collision=True, cull=0):
    """One actor holding a HISM of mesh_path with the given world transforms.

    xforms: iterable of (x, y, z, yaw[, scale]).
    """
    xforms = list(xforms)
    if not xforms:
        return None
    cx = sum(t[0] for t in xforms) / len(xforms)
    cy = sum(t[1] for t in xforms) / len(xforms)
    a = eas.spawn_actor_from_class(unreal.Actor, unreal.Vector(cx, cy, 0))
    hs = sds.k2_gather_subobject_data_for_instance(a)
    p = unreal.AddNewSubobjectParams(parent_handle=hs[0],
                                     new_class=unreal.HierarchicalInstancedStaticMeshComponent,
                                     blueprint_context=None)
    sds.add_new_subobject(p)
    a.root_component.set_mobility(unreal.ComponentMobility.STATIC)
    c = a.get_components_by_class(unreal.HierarchicalInstancedStaticMeshComponent)[0]
    c.set_mobility(unreal.ComponentMobility.STATIC)
    c.set_static_mesh(mesh(mesh_path))
    if not collision:
        c.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    if cull:
        c.set_editor_property('instance_end_cull_distance', cull)
    c.add_instances([_xf(t) for t in xforms], False, True)
    a.set_actor_label(label)
    a.set_folder_path(folder)
    return a


def sm(label, folder, mesh_path, x, y, z, yaw=0.0, scale=1.0, collision=True):
    a = eas.spawn_actor_from_object(mesh(mesh_path), unreal.Vector(x, y, z), unreal.Rotator(0, 0, yaw))
    if not isinstance(scale, (tuple, list)):
        scale = (scale, scale, scale)
    a.set_actor_scale3d(unreal.Vector(*scale))
    a.set_actor_label(label)
    a.set_folder_path(folder)
    if not collision:
        a.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    return a


class Bucket:
    """Collects transforms per mesh, then flushes one HISM actor per mesh."""

    def __init__(self, folder, prefix):
        self.folder = folder
        self.prefix = prefix
        self.items = {}
        self.nocoll = set()

    def add(self, mesh_path, x, y, z, yaw, scale=1.0, collision=True):
        self.items.setdefault(mesh_path, []).append((x, y, z, yaw, scale))
        if not collision:
            self.nocoll.add(mesh_path)

    def count(self):
        return sum(len(v) for v in self.items.values())

    def flush(self):
        out = []
        for mp, xs in self.items.items():
            name = mp.split('/')[-1].replace('SM_', '')
            out.append(ism(f'{self.prefix}_{name}', self.folder, mp, xs, collision=mp not in self.nocoll))
        self.items = {}
        return out


class Street:
    """Frame along a street: s along direction d, p across (positive = left, n)."""

    def __init__(self, name, ox, oy, yaw):
        self.name = name
        self.ox, self.oy, self.yaw = ox, oy, yaw
        r = math.radians(yaw)
        self.d = (math.cos(r), math.sin(r))
        self.n = (-self.d[1], self.d[0])

    def pt(self, s, p):
        return (self.ox + self.d[0] * s + self.n[0] * p, self.oy + self.d[1] * s + self.n[1] * p)

    def local(self, x, y):
        dx, dy = x - self.ox, y - self.oy
        return (dx * self.d[0] + dy * self.d[1], dx * self.n[0] + dy * self.n[1])

    def face_yaw(self, side):
        """Yaw for a +Y-facing module on side (+1 left, -1 right) to face the street."""
        return self.yaw + 180 if side > 0 else self.yaw

    def curb_yaw(self, side):
        """Yaw for a sidewalk piece whose local -X edge faces the street."""
        return self.yaw + 90 if side > 0 else self.yaw - 90


def point_in_poly(x, y, poly):
    inside = False
    j = len(poly) - 1
    for i in range(len(poly)):
        xi, yi = poly[i]
        xj, yj = poly[j]
        if (yi > y) != (yj > y) and x < (xj - xi) * (y - yi) / (yj - yi + 1e-9) + xi:
            inside = not inside
        j = i
    return inside


def trace_down(x, y, top=3000.0, bottom=-3000.0, ignore=()):
    h = unreal.SystemLibrary.line_trace_single(world(), unreal.Vector(x, y, top), unreal.Vector(x, y, bottom),
                                               unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, True, list(ignore),
                                               unreal.DrawDebugTrace.NONE, True)
    if h is None:
        return None
    t = h.to_tuple()
    if not t[0]:
        return None
    return (t[4].z, t[9].get_actor_label() if t[9] else None)


def find(label):
    for a in eas.get_all_level_actors():
        if a.get_actor_label() == label:
            return a
    return None


def disable_proxy(label):
    """Hide a CubeGrid proxy and turn off its collision; the actor stays in the level."""
    a = find(label)
    if a is None:
        return False
    a.set_actor_hidden_in_game(True)
    a.set_is_temporarily_hidden_in_editor(False)
    a.static_mesh_component.set_visibility(False)
    a.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    a.set_actor_enable_collision(False)
    a.set_folder_path('CubeGridProxies/Replaced')
    return True
