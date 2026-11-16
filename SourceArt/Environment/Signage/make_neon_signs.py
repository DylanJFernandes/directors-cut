"""Blender (4.3) generator for neon tube signs.

    blender -b -P make_neon_signs.py -- <out_dir>

Each sign becomes one FBX: letter outlines swept as round neon tubes (slot BK_NeonTube),
a border tube (BK_NeonTube) and a dark backing board (BK_Ink). The sign front faces -Y in
Blender (+Y in Unreal, matching the BacklotKit facade convention), pivot at bottom centre
of the board.
"""
import os
import sys

import bpy
import bmesh

FONT = 'C:/Windows/Fonts/bahnschrift.ttf'
# name, text, letter height (m), vertical stack
SIGNS = [
    ('Hotel', 'HOTEL', 2.6, False), ('Dancing', 'DANCING', 2.2, False), ('Loans', 'LOANS', 2.4, False),
    ('Cafe', 'CAFE', 2.4, False), ('Billiards', 'BILLIARDS', 2.0, False), ('Liquors', 'LIQUORS', 2.2, False),
    ('Cigars', 'CIGARS', 2.2, False), ('Jazz', 'JAZZ', 2.8, False), ('Pawn', 'PAWN', 2.4, False),
    ('Pictures', 'PICTURES', 2.0, False), ('Radio', 'RADIO', 2.4, False), ('ChopSuey', 'CHOP SUEY', 2.0, False),
    ('Tonight', 'TONIGHT', 2.0, False), ('Garage', 'GARAGE', 2.2, False),
    ('V_Hotel', 'HOTEL', 0.85, True), ('V_Bar', 'BAR', 0.95, True), ('V_Club', 'CLUB', 0.9, True),
    ('V_Eats', 'EATS', 0.9, True), ('V_Jazz', 'JAZZ', 0.9, True), ('V_Rooms', 'ROOMS', 0.85, True),
]


def mat(name):
    m = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    return m


def clear():
    bpy.ops.wm.read_factory_settings(use_empty=True)


def text_tubes(text, size, vertical):
    font = bpy.data.fonts.load(FONT, check_existing=True)
    body = '\n'.join(text) if vertical else text
    cu = bpy.data.curves.new('txt', 'FONT')
    cu.body = body
    cu.font = font
    cu.size = size * 1.38  # cap height of the face is ~0.72 of the em size
    cu.align_x = 'CENTER'
    cu.align_y = 'BOTTOM'
    cu.space_line = 0.95
    cu.space_character = 1.12
    ob = bpy.data.objects.new('txt', cu)
    bpy.context.collection.objects.link(ob)
    bpy.context.view_layer.objects.active = ob
    ob.select_set(True)
    bpy.ops.object.convert(target='CURVE')
    cv = ob.data
    cv.fill_mode = 'NONE'
    cv.bevel_mode = 'ROUND'
    cv.bevel_depth = max(0.018, size * 0.016)
    cv.bevel_resolution = 2
    cv.resolution_u = 6
    cv.extrude = 0.0
    bpy.ops.object.convert(target='MESH')
    return ob


def bounds(ob):
    xs = [v.co.x for v in ob.data.vertices]
    ys = [v.co.y for v in ob.data.vertices]
    return min(xs), max(xs), min(ys), max(ys)


def box(name, x0, x1, z0, z1, y0, y1):
    me = bpy.data.meshes.new(name)
    bm = bmesh.new()
    bmesh.ops.create_cube(bm, size=1.0)
    for v in bm.verts:
        v.co.x = x0 if v.co.x < 0 else x1
        v.co.y = y0 if v.co.y < 0 else y1
        v.co.z = z0 if v.co.z < 0 else z1
    bm.to_mesh(me)
    bm.free()
    ob = bpy.data.objects.new(name, me)
    bpy.context.collection.objects.link(ob)
    return ob


def border_tube(x0, x1, z0, z1, r):
    cu = bpy.data.curves.new('border', 'CURVE')
    cu.dimensions = '3D'
    sp = cu.splines.new('POLY')
    pts = [(x0, 0, z0), (x1, 0, z0), (x1, 0, z1), (x0, 0, z1)]
    sp.points.add(len(pts) - 1)
    for p, c in zip(sp.points, pts):
        p.co = (c[0], c[1], c[2], 1)
    sp.use_cyclic_u = True
    cu.bevel_depth = r
    cu.bevel_resolution = 2
    ob = bpy.data.objects.new('border', cu)
    bpy.context.collection.objects.link(ob)
    bpy.context.view_layer.objects.active = ob
    for o in bpy.context.selected_objects:
        o.select_set(False)
    ob.select_set(True)
    bpy.ops.object.convert(target='MESH')
    return ob


def build(name, text, size, vertical, out):
    clear()
    tube_m, ink_m = mat('BK_NeonTube'), mat('BK_Ink')
    t = text_tubes(text, size, vertical)
    # stand the letters up: text plane XY -> XZ, front facing -Y
    t.rotation_euler = (1.5707963, 0, 0)
    bpy.context.view_layer.update()
    bpy.ops.object.select_all(action='DESELECT')
    t.select_set(True)
    bpy.context.view_layer.objects.active = t
    bpy.ops.object.transform_apply(rotation=True)
    xs = [v.co.x for v in t.data.vertices]
    zs = [v.co.z for v in t.data.vertices]
    x0, x1, z0, z1 = min(xs), max(xs), min(zs), max(zs)
    pad = size * 0.32
    bx0, bx1, bz0, bz1 = x0 - pad, x1 + pad, z0 - pad, z1 + pad
    # recentre so the board's bottom centre is the origin
    ox, oz = (bx0 + bx1) / 2, bz0
    for v in t.data.vertices:
        v.co.x -= ox
        v.co.z -= oz
        v.co.y -= 0.06
    bx0, bx1, bz1 = bx0 - ox, bx1 - ox, bz1 - oz
    bz0 = 0.0
    t.data.materials.append(tube_m)
    r = max(0.02, size * 0.014)
    b = border_tube(bx0 + pad * 0.35, bx1 - pad * 0.35, bz0 + pad * 0.35, bz1 - pad * 0.35, r)
    for v in b.data.vertices:
        v.co.y -= 0.06
    b.data.materials.append(tube_m)
    board = box('board', bx0, bx1, bz0, bz1, 0.0, 0.14)
    board.data.materials.append(ink_m)
    bpy.ops.object.select_all(action='DESELECT')
    for o in (t, b, board):
        o.select_set(True)
    bpy.context.view_layer.objects.active = board
    bpy.ops.object.join()
    ob = bpy.context.view_layer.objects.active
    ob.name = 'SM_NeonSign_' + name
    bpy.ops.object.shade_smooth()
    path = os.path.join(out, f'SM_NeonSign_{name}.fbx')
    bpy.ops.export_scene.fbx(filepath=path, use_selection=True, apply_unit_scale=True,
                             apply_scale_options='FBX_SCALE_UNITS', mesh_smooth_type='FACE',
                             add_leaf_bones=False, bake_space_transform=True)
    return (bx1 - bx0, bz1 - bz0)


if __name__ == '__main__':
    out = sys.argv[sys.argv.index('--') + 1] if '--' in sys.argv else os.path.dirname(__file__)
    os.makedirs(out, exist_ok=True)
    for name, text, size, vertical in SIGNS:
        w, h = build(name, text, size, vertical, out)
        print(f'SIGN {name} {w:.2f} x {h:.2f} m')
