"""CoinRush level builder.

Runs inside the Unreal Editor commandlet:

    UnrealEditor-Cmd.exe CoinRush.uproject -run=pythonscript -script=Tools/build_level.py

It creates the five materials used by the game and an arena level, then saves
/Game/Maps/CoinRush. Everything is authored from code, so the map can always be
rebuilt from scratch: if the map already exists its actors are cleared first.
"""

import traceback

import unreal

LEVEL_PATH = "/Game/Maps/CoinRush"
MATERIAL_DIR = "/Game/Materials"

ASSET_TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
ACTORS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
LEVELS = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
MATERIAL_EDITING = unreal.MaterialEditingLibrary
EDITOR_ASSETS = unreal.EditorAssetLibrary


def log(message):
    unreal.log("[CoinRush] " + str(message))


def safe_set(obj, name, value):
    """Set an optional editor property, warning instead of failing the build."""
    try:
        obj.set_editor_property(name, value)
        return True
    except Exception as exc:  # noqa: BLE001 - engine property names vary by version
        log("skip property %s on %s (%s)" % (name, obj.get_name(), exc))
        return False


def ensure_empty_level():
    """Load the existing map and wipe it, or create a fresh map if there is none."""
    if EDITOR_ASSETS.does_asset_exist(LEVEL_PATH):
        try:
            if LEVELS.load_level(LEVEL_PATH):
                cleared = 0
                for actor in ACTORS.get_all_level_actors():
                    if ACTORS.destroy_actor(actor):
                        cleared += 1
                log("Cleared %d actors from existing map" % cleared)
                return True
        except Exception as exc:  # noqa: BLE001
            log("Could not reuse existing map: %s" % exc)

    if EDITOR_ASSETS.does_asset_exist(LEVEL_PATH):
        EDITOR_ASSETS.delete_asset(LEVEL_PATH)

    created = LEVELS.new_level(LEVEL_PATH)
    log("new_level -> %s" % created)
    return created


def make_material(name, color, emissive_strength=0.0, roughness=0.5, metallic=0.0):
    path = MATERIAL_DIR + "/" + name
    if EDITOR_ASSETS.does_asset_exist(path):
        EDITOR_ASSETS.delete_asset(path)

    material = ASSET_TOOLS.create_asset(name, MATERIAL_DIR, unreal.Material, unreal.MaterialFactoryNew())
    if material is None:
        log("ERROR: could not create material " + name)
        return None

    base = MATERIAL_EDITING.create_material_expression(
        material, unreal.MaterialExpressionConstant3Vector, -420, -120)
    base.set_editor_property(
        "constant", unreal.LinearColor(color[0], color[1], color[2], 1.0))

    if emissive_strength > 0.0:
        material.set_editor_property(
            "shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
        strength = MATERIAL_EDITING.create_material_expression(
            material, unreal.MaterialExpressionConstant, -420, 90)
        strength.set_editor_property("r", emissive_strength)
        multiply = MATERIAL_EDITING.create_material_expression(
            material, unreal.MaterialExpressionMultiply, -180, -120)
        # Expression inputs are protected; the editing library is the supported way in.
        MATERIAL_EDITING.connect_material_expressions(base, "", multiply, "A")
        MATERIAL_EDITING.connect_material_expressions(strength, "", multiply, "B")
        MATERIAL_EDITING.connect_material_property(
            multiply, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    else:
        MATERIAL_EDITING.connect_material_property(
            base, "", unreal.MaterialProperty.MP_BASE_COLOR)

        roughness_expr = MATERIAL_EDITING.create_material_expression(
            material, unreal.MaterialExpressionConstant, -420, 60)
        roughness_expr.set_editor_property("r", roughness)
        MATERIAL_EDITING.connect_material_property(
            roughness_expr, "", unreal.MaterialProperty.MP_ROUGHNESS)

        metallic_expr = MATERIAL_EDITING.create_material_expression(
            material, unreal.MaterialExpressionConstant, -420, 190)
        metallic_expr.set_editor_property("r", metallic)
        MATERIAL_EDITING.connect_material_property(
            metallic_expr, "", unreal.MaterialProperty.MP_METALLIC)

    MATERIAL_EDITING.recompile_material(material)
    EDITOR_ASSETS.save_asset(path)
    log("Material ready: " + path)
    return material


def make_materials():
    return {
        "floor": make_material("M_Floor", (0.050, 0.060, 0.090), roughness=0.80, metallic=0.05),
        "platform": make_material("M_Platform", (0.020, 0.260, 0.300), roughness=0.55, metallic=0.05),
        "coin": make_material("M_Coin", (1.000, 0.480, 0.060), emissive_strength=2.6),
        "character": make_material("M_Character", (0.020, 0.360, 0.430), roughness=0.45, metallic=0.05),
        "accent": make_material("M_Accent", (0.400, 0.950, 1.000), emissive_strength=1.1),
    }


def spawn(cls, location, rotation=(0.0, 0.0, 0.0), label=None):
    actor = ACTORS.spawn_actor_from_class(
        cls, unreal.Vector(location[0], location[1], location[2]),
        unreal.Rotator(roll=rotation[0], pitch=rotation[1], yaw=rotation[2]))
    if actor is not None and label:
        actor.set_actor_label(label)
    return actor


CUBE = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")


def block(location, scale, material, label):
    actor = spawn(unreal.StaticMeshActor, location, label=label)
    if actor is None:
        return None
    component = actor.static_mesh_component
    safe_set(component, "mobility", unreal.ComponentMobility.STATIC)
    component.set_static_mesh(CUBE)
    if material is not None:
        component.set_material(0, material)
    actor.set_actor_scale3d(unreal.Vector(scale[0], scale[1], scale[2]))
    return actor


def build_arena(materials):
    floor_mat = materials.get("floor")
    platform_mat = materials.get("platform")

    # Ground: 36 m x 36 m slab whose top surface sits exactly on Z = 0.
    block((0.0, 0.0, -10.0), (36.0, 36.0, 0.2), floor_mat, "SM_Floor")

    # Low walls keep the player inside the arena.
    block((0.0, 1800.0, 120.0), (36.0, 1.0, 2.4), platform_mat, "SM_Wall_N")
    block((0.0, -1800.0, 120.0), (36.0, 1.0, 2.4), platform_mat, "SM_Wall_S")
    block((1800.0, 0.0, 120.0), (1.0, 36.0, 2.4), platform_mat, "SM_Wall_E")
    block((-1800.0, 0.0, 120.0), (1.0, 36.0, 2.4), platform_mat, "SM_Wall_W")

    # Central platform plus two steps leading up to it from the spawn side.
    block((0.0, 0.0, 60.0), (8.0, 8.0, 1.2), platform_mat, "SM_Platform_Center")
    block((0.0, -560.0, 30.0), (4.0, 1.2, 0.6), platform_mat, "SM_Step_A")
    block((0.0, -470.0, 60.0), (4.0, 1.2, 1.2), platform_mat, "SM_Step_B")

    # Side platforms with the higher coins.
    block((1400.0, 1400.0, 60.0), (7.0, 7.0, 1.2), platform_mat, "SM_Platform_E")
    block((-1400.0, 1400.0, 60.0), (7.0, 7.0, 1.2), platform_mat, "SM_Platform_W")

    # Decorative pillars in the two back corners.
    block((1400.0, -1400.0, 300.0), (1.5, 1.5, 6.0), platform_mat, "SM_Pillar_E")
    block((-1400.0, -1400.0, 300.0), (1.5, 1.5, 6.0), platform_mat, "SM_Pillar_W")


def build_props(materials):
    """Small props plus neon trim, so the arena is not just an empty box."""
    accent = materials.get("accent")
    platform_mat = materials.get("platform")

    # Crates near the spawn point.
    for x, y, size, z in (
        (-820.0, -1250.0, 1.4, 70.0),
        (820.0, -1250.0, 1.4, 70.0),
        (-820.0, -960.0, 1.0, 50.0),
        (820.0, -960.0, 1.0, 50.0),
    ):
        block((x, y, z), (size, size, size), platform_mat, "SM_Crate")

    # Gate pillars flanking the stairs up to the central platform.
    block((420.0, -520.0, 160.0), (1.2, 1.2, 3.2), platform_mat, "SM_Gate_E")
    block((-420.0, -520.0, 160.0), (1.2, 1.2, 3.2), platform_mat, "SM_Gate_W")
    block((420.0, -520.0, 330.0), (1.4, 1.4, 0.25), accent, "SM_GateTop_E")
    block((-420.0, -520.0, 330.0), (1.4, 1.4, 0.25), accent, "SM_GateTop_W")

    # Thin emissive strips along the wall bases.
    block((0.0, 1746.0, 12.0), (34.6, 0.08, 0.22), accent, "SM_Trim_N")
    block((0.0, -1746.0, 12.0), (34.6, 0.08, 0.22), accent, "SM_Trim_S")
    block((1746.0, 0.0, 12.0), (0.08, 34.6, 0.22), accent, "SM_Trim_E")
    block((-1746.0, 0.0, 12.0), (0.08, 34.6, 0.22), accent, "SM_Trim_W")

    # Glowing rim around the central platform.
    block((0.0, 396.0, 124.0), (8.2, 0.10, 0.10), accent, "SM_Rim_N")
    block((0.0, -396.0, 124.0), (8.2, 0.10, 0.10), accent, "SM_Rim_S")
    block((396.0, 0.0, 124.0), (0.10, 8.2, 0.10), accent, "SM_Rim_E")
    block((-396.0, 0.0, 124.0), (0.10, 8.2, 0.10), accent, "SM_Rim_W")


def build_lighting():
    sun = spawn(unreal.DirectionalLight, (0.0, 0.0, 1400.0), rotation=(0.0, -42.0, 35.0), label="Sun")
    if sun is not None:
        sun_component = sun.get_component_by_class(unreal.DirectionalLightComponent)
        sun_component.set_mobility(unreal.ComponentMobility.MOVABLE)
        safe_set(sun_component, "intensity", 12.0)
        safe_set(sun_component, "atmosphere_sun_light", True)
        safe_set(sun_component, "light_source_angle", 0.35)
        safe_set(sun_component, "cast_shadows", True)

    sky = spawn(unreal.SkyLight, (0.0, 0.0, 400.0), label="SkyLight")
    if sky is not None:
        sky_component = sky.get_component_by_class(unreal.SkyLightComponent)
        sky_component.set_mobility(unreal.ComponentMobility.MOVABLE)
        safe_set(sky_component, "real_time_capture", True)
        safe_set(sky_component, "intensity", 0.75)

    spawn(unreal.SkyAtmosphere, (0.0, 0.0, 0.0), label="SkyAtmosphere")

    fog = spawn(unreal.ExponentialHeightFog, (0.0, 0.0, 0.0), label="HeightFog")
    if fog is not None:
        fog_component = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
        safe_set(fog_component, "fog_density", 0.006)
        safe_set(fog_component, "fog_height_falloff", 0.2)
        safe_set(fog_component, "enable_volumetric_fog", False)

    post = spawn(unreal.PostProcessVolume, (0.0, 0.0, 0.0), label="PostProcess")
    if post is not None:
        safe_set(post, "unbound", True)
        settings = post.get_editor_property("settings")
        safe_set(settings, "override_bloom_intensity", True)
        safe_set(settings, "bloom_intensity", 0.7)
        safe_set(settings, "override_vignette_intensity", True)
        safe_set(settings, "vignette_intensity", 0.4)
        safe_set(settings, "override_auto_exposure_bias", True)
        safe_set(settings, "auto_exposure_bias", -1.4)

    spawn(unreal.PlayerStart, (0.0, -1500.0, 110.0), rotation=(0.0, 0.0, 0.0), label="PlayerStart")


def build_blueprint_props():
    """Decorations authored as Blueprints (see Docs/UE5-Knowledge/blueprint-authoring-via-mcp.md).

    Optional: if a Blueprint asset is missing or not compiled yet, the level still builds.
    """
    props = (
        # name, location, scale
        ("/Game/Blueprints/BP_SpinningCube", (350.0, -900.0, 60.0), (1.4, 1.4, 1.4)),
        ("/Game/Blueprints/BP_Turret", (300.0, -1000.0, 80.0), (4.0, 0.5, 0.5)),
    )

    for asset_path, location, scale in props:
        blueprint = EDITOR_ASSETS.load_asset(asset_path)
        if blueprint is None:
            log("skip Blueprint prop: %s not found" % asset_path)
            continue

        generated_class = blueprint.generated_class()
        if generated_class is None:
            log("skip Blueprint prop: %s has no generated class (compile it first)" % asset_path)
            continue

        actor = ACTORS.spawn_actor_from_class(
            generated_class, unreal.Vector(location[0], location[1], location[2]), unreal.Rotator(roll=0.0, pitch=0.0, yaw=0.0))
        if actor is not None:
            actor.set_actor_scale3d(unreal.Vector(scale[0], scale[1], scale[2]))
            actor.set_actor_label(asset_path.rsplit("/", 1)[-1])
            log("Spawned Blueprint prop %s" % asset_path.rsplit("/", 1)[-1])


def main():
    if not ensure_empty_level():
        raise RuntimeError("Could not create or load " + LEVEL_PATH)

    materials = make_materials()
    build_arena(materials)
    build_props(materials)
    build_blueprint_props()
    build_lighting()

    saved = LEVELS.save_current_level()
    log("save_current_level -> %s" % saved)
    log("Done. %d actors in %s" % (len(ACTORS.get_all_level_actors()), LEVEL_PATH))


try:
    main()
except Exception:
    unreal.log_error("[CoinRush] level generation failed:\n" + traceback.format_exc())
    raise
