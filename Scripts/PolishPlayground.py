"""Simple prototype colors and dynamic lighting; no final art or gameplay changes."""
import unreal

assets = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels.load_level('/Game/Steppe/Worlds/Prototype/L_Prototype_Grassland')

def material(name, rgb):
    path = '/Game/Steppe/Debug/' + name
    if assets.does_asset_exist(path):
        return assets.load_asset(path)
    mat = tools.create_asset(name, '/Game/Steppe/Debug', unreal.Material, unreal.MaterialFactoryNew())
    color = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector)
    color.set_editor_property('constant', unreal.LinearColor(*rgb, 1))
    unreal.MaterialEditingLibrary.connect_material_property(color, '', unreal.MaterialProperty.MP_BASE_COLOR)
    rough = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionConstant)
    rough.set_editor_property('r', 0.85)
    unreal.MaterialEditingLibrary.connect_material_property(rough, '', unreal.MaterialProperty.MP_ROUGHNESS)
    unreal.MaterialEditingLibrary.recompile_material(mat)
    assets.save_loaded_asset(mat)
    return mat

grass = material('M_PrototypeGrass', (.12,.21,.065))
marker = material('M_PrototypeMarker', (.65,.32,.045))
horse = material('M_PrototypeHorse', (.18,.065,.025))
rider = material('M_PrototypeRider', (.035,.14,.21))
for actor in actors.get_all_level_actors():
    if isinstance(actor, (unreal.DirectionalLight, unreal.SkyLight)):
        actor.get_component_by_class(unreal.SceneComponent).set_mobility(unreal.ComponentMobility.MOVABLE)
    if isinstance(actor, unreal.StaticMeshActor):
        component = actor.get_component_by_class(unreal.StaticMeshComponent)
        if actor.get_actor_label() == 'Grassland_2km_TestFloor':
            component.set_material(0, grass)
        elif actor.get_actor_label().startswith(('Slalom_', 'DistanceMarker_')):
            component.set_material(0, marker)
for path, mat in [('/Game/Steppe/Characters/Horses/BP_SteppeHorse', horse), ('/Game/Steppe/Characters/Rider/BP_SteppeRider', rider)]:
    default = unreal.get_default_object(assets.load_blueprint_class(path))
    component = default.get_component_by_class(unreal.StaticMeshComponent)
    if component:
        component.set_material(0, mat)
    assets.save_asset(path, only_if_is_dirty=False)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property('force_no_precomputed_lighting', True)
levels.save_current_level()
unreal.log('STEPPE_PLAYGROUND_POLISHED')
unreal.SystemLibrary.quit_editor()
