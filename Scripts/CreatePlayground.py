"""Run with UE Editor Python; creates genuine assets and preserves existing ones."""
import unreal

tools = unreal.AssetToolsHelpers.get_asset_tools()
assets = unreal.EditorAssetLibrary
level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

def blueprint(name, folder, parent):
    path = folder + '/' + name
    if not assets.does_asset_exist(path):
        factory = unreal.BlueprintFactory()
        factory.set_editor_property('parent_class', parent)
        tools.create_asset(name, folder, unreal.Blueprint, factory)
    return assets.load_blueprint_class(path)

config_path = '/Game/Steppe/Data/Horses/DA_HorseLocomotion_Default'
config = assets.load_asset(config_path) if assets.does_asset_exist(config_path) else None
if config is None:
    factory = unreal.DataAssetFactory()
    factory.set_editor_property('data_asset_class', unreal.HorseLocomotionConfig)
    config = tools.create_asset('DA_HorseLocomotion_Default', '/Game/Steppe/Data/Horses', unreal.HorseLocomotionConfig, factory)
horse_class = blueprint('BP_SteppeHorse', '/Game/Steppe/Characters/Horses', unreal.SteppeHorseCharacter)
rider_class = blueprint('BP_SteppeRider', '/Game/Steppe/Characters/Rider', unreal.SteppeRiderCharacter)
mode_class = blueprint('BP_SteppeGameMode', '/Game/Steppe', unreal.SteppeGameMode)
unreal.get_default_object(horse_class).set_editor_property('locomotion_config', config)
mode = unreal.get_default_object(mode_class)
mode.set_editor_property('horse_class', horse_class)
mode.set_editor_property('default_pawn_class', rider_class)
assets.save_directory('/Game/Steppe', only_if_is_dirty=False, recursive=True)

map_path = '/Game/Steppe/Worlds/Prototype/L_Prototype_Grassland'
if not assets.does_asset_exist(map_path):
    if not level.new_level(map_path):
        raise RuntimeError('Could not create prototype map')
    cube = assets.load_asset('/Engine/BasicShapes/Cube')
    def block(label, position, scale):
        actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*position))
        actor.set_actor_label(label)
        component = actor.get_component_by_class(unreal.StaticMeshComponent)
        component.set_static_mesh(cube)
        actor.set_actor_scale3d(unreal.Vector(*scale))
        return actor
    block('Grassland_2km_TestFloor', (0,0,-20), (2000,2000,.4))
    # Slalom and distance references make speed/turning legible without final art.
    for i in range(1, 21):
        block('DistanceMarker_%dm' % (i*25), (i*2500,900,150), (.7,.7,3))
    for i in range(7):
        block('Slalom_%d' % i, (2000+i*2000,(-1 if i%2 else 1)*550,125), (1,1,2.5))
    ramp = block('Slope_Test_Ramp', (3000,-2500,130), (14,10,.5))
    ramp.set_actor_rotation(unreal.Rotator(12,0,0), False)
    actors.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(0,0,110))
    sun = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0,0,1500), unreal.Rotator(-45,-30,0))
    sun.get_component_by_class(unreal.DirectionalLightComponent).set_editor_property('intensity', 3.0)
    actors.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0,0,1000))
    actors.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0,0,0))
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    world.get_world_settings().set_editor_property('default_game_mode', mode_class)
    if not level.save_current_level():
        raise RuntimeError('Failed to save prototype map')
unreal.log('STEPPE_ASSETS_CREATED: real map, blueprints, locomotion data asset')
unreal.SystemLibrary.quit_editor()
