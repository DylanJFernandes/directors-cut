"""Night-noir lighting pass for LVL_NewYorkSet: moonlight, volumetric fog,
black-and-white grade, wet streets and practical lights on lamps and neon.

Re-runnable: lights live under LIGHT_ROOT and are cleared first; the sky,
fog and post-process actors are reused if present.
"""
import math
import unreal
import dresslib as D

LIGHT_ROOT = 'Backlot/09_Lighting'
MAT_DIR = '/Game/Environment/BacklotKit/Materials'
STREET_MESHES = ('SM_CobblestoneRoad_4x8', 'SM_CobblestoneJunction_8x8', 'SM_TrolleyRoad_4x8',
                 'SM_SidewalkSlab_2x4', 'SM_SidewalkCurb_2x4', 'SM_SidewalkDrain_2x4', 'SM_SidewalkInfill_1x4',
                 'SM_SidewalkCorner_2x2')
# wet copies: (source MI, new MI, roughness, tint multiplier)
WET = [('MI_Backlot_Concrete', 'MI_Backlot_WetConcrete', 0.12, 0.55),
       ('MI_Backlot_Limestone', 'MI_Backlot_WetLimestone', 0.18, 0.6),
       ('MI_Backlot_Mortar', 'MI_Backlot_WetMortar', 0.08, 0.8)]

# practical lights: mesh name -> (local offset x, y, z, intensity cd, radius, shadows, volumetric)
PRACTICALS = {
    'SM_Streetlamp_DoubleGlobe': [(0, 0, 420, 320, 2000, True, 2.25)],
    'SM_NeonBlade_Bar': [(0, 110, 130, 70, 900, False, 1.5)],
    'SM_NeonBlade_Club': [(0, 110, 130, 70, 900, False, 1.5)],
    'SM_NeonBlade_Diner': [(0, 110, 130, 70, 900, False, 1.5)],
    'SM_NeonBlade_Hotel': [(0, 110, 130, 70, 900, False, 1.5)],
    'SM_NeonCabinet_Bar': [(0, 90, 45, 50, 700, False, 1.1)],
    'SM_NeonCabinet_Club': [(0, 90, 45, 50, 700, False, 1.1)],
    'SM_NeonCabinet_Diner': [(0, 90, 45, 50, 700, False, 1.1)],
    'SM_NeonCabinet_Hotel': [(0, 90, 45, 50, 700, False, 1.1)],
    'SM_MarqueeBlank_4m': [(0, 160, 120, 240, 1400, True, 1.5)],
    'SM_Moxy': [(0, 500, 380, 800, 2600, True, 2.25)],
    'SM_RustyRabbit': [(0, 520, 300, 500, 2000, True, 2.25)],
    'SM_CornerDiner': [(0, 480, 300, 500, 2000, True, 2.25)],
    'SM_BarberShop': [(0, 480, 300, 360, 1800, True, 1.9)],
    'SM_ElevatedStation': [(0, 0, 700, 300, 2000, True, 1.5)],
    'SM_SubwayStreetExit': [(0, -150, 300, 160, 1200, True, 1.5)],
}
# Glass slot overrides: randomly lit windows per instance (M_Backlot_NightWindow)
WINDOW_MI = {
    'SM_WindowWall_6m_2Bay': 'MI_Backlot_NightWindow', 'SM_WindowWall_4m_2Bay': 'MI_Backlot_NightWindow',
    'SM_WindowWall_2m_1Bay': 'MI_Backlot_NightWindow', 'SM_Storefront_4m': 'MI_Backlot_NightShopfront',
    'SM_Storefront_2m': 'MI_Backlot_NightShopfront', 'SM_Storefront_1m': 'MI_Backlot_NightShopfront',
    'SM_RecessedStorefront_4m': 'MI_Backlot_NightShopfront',
}
# neon signs from neon.py: rooftop boards throw a broad glow, blades a small one
for _w in ('Hotel', 'Dancing', 'Loans', 'Cafe', 'Billiards', 'Liquors', 'Cigars', 'Jazz', 'Pawn', 'Pictures',
           'Radio', 'ChopSuey', 'Tonight', 'Garage'):
    PRACTICALS['SM_NeonSign_' + _w] = [(0, 250, 150, 1600, 5000, False, 2.0)]
for _w in ('V_Hotel', 'V_Bar', 'V_Club', 'V_Eats', 'V_Jazz', 'V_Rooms'):
    PRACTICALS['SM_NeonSign_' + _w] = [(0, 70, 250, 60, 900, False, 1.2)]
LAMP_COLOR = unreal.LinearColor(1.0, 0.82, 0.62, 1.0)


def _actor(cls):
    for a in D.eas.get_all_level_actors():
        if a.get_class() == cls.static_class():
            return a
    return D.eas.spawn_actor_from_class(cls, unreal.Vector(-9200, 0, 900))


def sky():
    moon = _actor(unreal.DirectionalLight)
    moon.set_actor_rotation(unreal.Rotator(roll=0, pitch=-38, yaw=-125), False)
    lc = moon.light_component
    lc.set_intensity(3.0)
    lc.set_light_color(unreal.LinearColor(0.72, 0.8, 1.0, 1.0))
    lc.set_editor_property('light_source_angle', 0.6)
    lc.set_editor_property('volumetric_scattering_intensity', 2.0)
    lc.set_editor_property('cast_shadows', True)
    lc.set_editor_property('atmosphere_sun_light', True)

    sl = _actor(unreal.SkyLight).light_component
    sl.set_intensity(2.5)
    sl.set_light_color(unreal.LinearColor(0.55, 0.62, 0.8, 1.0))
    sl.set_editor_property('volumetric_scattering_intensity', 0.5)
    sl.recapture_sky()

    fog = _actor(unreal.ExponentialHeightFog).component
    fog.set_editor_property('fog_density', 0.05)
    fog.set_editor_property('fog_height_falloff', 0.08)
    fog.set_editor_property('fog_inscattering_luminance', unreal.LinearColor(0.05, 0.055, 0.065, 1.0))
    fog.set_editor_property('directional_inscattering_luminance', unreal.LinearColor(0.08, 0.09, 0.11, 1.0))
    fog.set_editor_property('directional_inscattering_exponent', 6.0)
    fog.set_editor_property('start_distance', 0.0)
    fog.set_editor_property('fog_max_opacity', 0.97)
    fog.set_editor_property('enable_volumetric_fog', True)
    fog.set_editor_property('volumetric_fog_scattering_distribution', 0.75)
    fog.set_editor_property('volumetric_fog_albedo', unreal.Color(r=210, g=210, b=215, a=255))
    fog.set_editor_property('volumetric_fog_extinction_scale', 0.7)
    fog.set_editor_property('volumetric_fog_distance', 9000.0)
    fog.set_editor_property('volumetric_fog_start_distance', 0.0)
    # faint ambient glow so the fog reads away from lamps (sky capture is near black at night)
    fog.set_editor_property('volumetric_fog_emissive', unreal.LinearColor(0.0012, 0.0013, 0.0015, 1.0))
    fog.get_owner().set_actor_location(unreal.Vector(-9200, 0, 0), False, False)


def grade():
    ppv = None
    for a in D.eas.get_all_level_actors():
        if isinstance(a, unreal.PostProcessVolume):
            ppv = a
    if ppv is None:
        ppv = D.eas.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(-9200, 0, 1200))
    ppv.set_actor_label('PPV_NoirGrade')
    ppv.set_folder_path(LIGHT_ROOT.rsplit('/', 1)[0] + '/09_Grade')
    ppv.set_editor_property('unbound', True)
    ppv.set_editor_property('priority', 10.0)
    s = ppv.settings
    def put(name, value):
        s.set_editor_property('override_' + name, True)
        s.set_editor_property(name, value)
    # black and white with a hard, inky film curve
    put('color_saturation', unreal.Vector4(1, 1, 1, 0))
    put('color_contrast', unreal.Vector4(1, 1, 1, 1.18))
    put('color_gamma', unreal.Vector4(1, 1, 1, 0.95))
    put('color_gain_shadows', unreal.Vector4(1, 1, 1, 0.8))
    put('film_toe', 0.62)
    put('film_shoulder', 0.3)
    put('film_black_clip', 0.0)
    put('film_grain_intensity', 0.35)
    put('film_grain_intensity_shadows', 0.6)
    put('vignette_intensity', 0.65)
    put('bloom_intensity', 0.9)
    put('bloom_threshold', -1.0)
    put('lens_flare_intensity', 0.0)
    # fixed night exposure so lamps and neon read as the brightest things on screen
    put('auto_exposure_method', unreal.AutoExposureMethod.AEM_HISTOGRAM)
    put('auto_exposure_min_brightness', 0.3)
    put('auto_exposure_max_brightness', 0.3)
    put('auto_exposure_bias', 0.0)
    put('local_exposure_highlight_contrast_scale', 0.85)
    put('local_exposure_shadow_contrast_scale', 0.6)
    put('scene_fringe_intensity', 0.25)
    ppv.set_editor_property('settings', s)
    return ppv


def wet_materials():
    at = unreal.AssetToolsHelpers.get_asset_tools()
    mel = unreal.MaterialEditingLibrary
    out = {}
    for src, dst, rough, dark in WET:
        path = f'{MAT_DIR}/{dst}'
        mi = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
        s = unreal.load_asset(f'{MAT_DIR}/{src}')
        if mi is None:
            mi = at.create_asset(dst, MAT_DIR, unreal.MaterialInstanceConstant,
                                 unreal.MaterialInstanceConstantFactoryNew())
        mel.set_material_instance_parent(mi, s.get_editor_property('parent'))
        for p in s.scalar_parameter_values:
            mel.set_material_instance_scalar_parameter_value(mi, p.parameter_info.name, p.parameter_value)
        mel.set_material_instance_scalar_parameter_value(mi, 'Roughness', rough)
        tint = mel.get_material_instance_vector_parameter_value(s, 'Tint')
        mel.set_material_instance_vector_parameter_value(
            mi, 'Tint', unreal.LinearColor(tint.r * dark, tint.g * dark, tint.b * dark, 1.0))
        mel.update_material_instance(mi)
        unreal.EditorAssetLibrary.save_loaded_asset(mi)
        out[src] = mi
    return out


def wet_streets():
    wet = wet_materials()
    n = 0
    for a in D.eas.get_all_level_actors():
        for c in a.get_components_by_class(unreal.StaticMeshComponent):
            m = c.static_mesh
            if not m or m.get_name() not in STREET_MESHES:
                continue
            for i in range(len(m.static_materials)):
                base = m.get_material(i)
                if base and base.get_name() in wet:
                    c.set_material(i, wet[base.get_name()])
                    n += 1
    return n


def _instances():
    """(mesh name, world transform) for every practical-light source mesh in the level."""
    for a in D.eas.get_all_level_actors():
        if str(a.get_folder_path()).startswith(LIGHT_ROOT):
            continue
        for c in a.get_components_by_class(unreal.StaticMeshComponent):
            m = c.static_mesh
            if not m or m.get_name() not in PRACTICALS:
                continue
            if isinstance(c, unreal.InstancedStaticMeshComponent):
                for i in range(c.get_instance_count()):
                    yield m.get_name(), c.get_instance_transform(i, True)
            else:
                yield m.get_name(), c.get_world_transform()


def practicals():
    D.clear(LIGHT_ROOT)
    n = 0
    for name, xf in list(_instances()):
        for (ox, oy, oz, cd, rad, shadows, vol) in PRACTICALS[name]:
            loc = xf.transform_location(unreal.Vector(ox, oy, oz))
            a = D.eas.spawn_actor_from_class(unreal.PointLight, loc)
            lc = a.point_light_component
            lc.set_mobility(unreal.ComponentMobility.MOVABLE)
            lc.set_editor_property('intensity_units', unreal.LightUnits.CANDELAS)
            lc.set_intensity(cd)
            lc.set_attenuation_radius(rad)
            lc.set_light_color(LAMP_COLOR)
            lc.set_editor_property('source_radius', 12.0)
            lc.set_editor_property('cast_shadows', shadows)
            lc.set_editor_property('volumetric_scattering_intensity', vol)
            lc.set_editor_property('max_draw_distance', 7000.0)
            lc.set_editor_property('max_distance_fade_range', 2000.0)
            a.set_actor_label(f'PL_{name[3:]}_{n}')
            a.set_folder_path(f'{LIGHT_ROOT}/{name[3:]}')
            n += 1
    return n


def night_windows():
    """Swap the Glass slot on window and storefront modules for the randomly lit night glass."""
    mis = {n: unreal.load_asset(f'{MAT_DIR}/{n}') for n in set(WINDOW_MI.values())}
    n = 0
    for a in D.eas.get_all_level_actors():
        for c in a.get_components_by_class(unreal.StaticMeshComponent):
            m = c.static_mesh
            if not m or m.get_name() not in WINDOW_MI:
                continue
            for i in range(len(m.static_materials)):
                if str(m.static_materials[i].material_slot_name) == 'BK_Glass':
                    c.set_material(i, mis[WINDOW_MI[m.get_name()]])
                    n += 1
    return n


def run():
    sky()
    grade()
    return dict(wet=wet_streets(), windows=night_windows(), lights=practicals())
