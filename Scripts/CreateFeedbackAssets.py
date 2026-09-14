"""Create licensed-in-project surface metadata for P13 feedback routing."""
import unreal

assets = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
root = '/Game/Steppe/Feedback'

def physical_material(name, surface):
    path = root + '/' + name
    material = assets.load_asset(path) if assets.does_asset_exist(path) else tools.create_asset(
        name, root, unreal.PhysicalMaterial, unreal.PhysicalMaterialFactoryNew())
    if not material:
        raise RuntimeError('Could not create ' + path)
    material.set_editor_property('surface_type', surface)
    assets.save_loaded_asset(material)
    return material

grass = physical_material('PM_Grass', unreal.PhysicalSurface.SURFACE_TYPE1)
hard = physical_material('PM_Hard', unreal.PhysicalSurface.SURFACE_TYPE2)

for path, physical in [
    ('/Game/Steppe/Debug/M_PrototypeGrass', grass),
    ('/Game/Steppe/Debug/M_PrototypeMarker', hard),
]:
    material = assets.load_asset(path)
    if material:
        material.set_editor_property('phys_material', physical)
        assets.save_loaded_asset(material)

# Keep a small, clearly labelled hard-surface pad in the prototype map so the
# two routes can be heard and inspected without importing production scenery.
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels.load_level('/Game/Steppe/Worlds/Prototype/L_Prototype_Grassland')
pad = next((actor for actor in actor_subsystem.get_all_level_actors()
            if actor.get_actor_label() == 'HardSurface_TestPad'), None)
if not pad:
    pad = actor_subsystem.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(1200, -1400, 0))
    pad.set_actor_label('HardSurface_TestPad')
component = pad.get_component_by_class(unreal.StaticMeshComponent)
component.set_static_mesh(assets.load_asset('/Engine/BasicShapes/Cube'))
component.set_material(0, assets.load_asset('/Game/Steppe/Debug/M_PrototypeMarker'))
pad.set_actor_location(unreal.Vector(1200, -1400, 0), False, False)
pad.set_actor_scale3d(unreal.Vector(5, 5, .1))
levels.save_current_level()

unreal.log('STEPPE_P13_SURFACE_ASSETS: Grass=SurfaceType1 Hard=SurfaceType2 Pad=HardSurface_TestPad')
unreal.SystemLibrary.quit_editor()
