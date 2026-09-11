"""Create P2 assets using UE Editor; preserve the existing P1 map and tuning."""
import unreal

assets = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
config_path = '/Game/Steppe/Data/Horses/DA_WildHorse_Default'
if assets.does_asset_exist(config_path):
    config = assets.load_asset(config_path)
else:
    factory = unreal.DataAssetFactory()
    factory.set_editor_property('data_asset_class', unreal.WildHorseConfig)
    config = tools.create_asset('DA_WildHorse_Default', '/Game/Steppe/Data/Horses', unreal.WildHorseConfig, factory)

path = '/Game/Steppe/Characters/Horses/BP_SteppeWildHorse'
if not assets.does_asset_exist(path):
    factory = unreal.BlueprintFactory()
    factory.set_editor_property('parent_class', unreal.SteppeWildHorseCharacter)
    tools.create_asset('BP_SteppeWildHorse', '/Game/Steppe/Characters/Horses', unreal.Blueprint, factory)
wild_class = assets.load_blueprint_class(path)
default = unreal.get_default_object(wild_class)
default.get_component_by_class(unreal.HorseBrainComponent).set_editor_property('config', config)
default.set_editor_property('locomotion_config', assets.load_asset('/Game/Steppe/Data/Horses/DA_HorseLocomotion_Default'))

material_path = '/Game/Steppe/Debug/M_PrototypeWildHorse'
if assets.does_asset_exist(material_path):
    material = assets.load_asset(material_path)
else:
    material = tools.create_asset('M_PrototypeWildHorse', '/Game/Steppe/Debug', unreal.Material, unreal.MaterialFactoryNew())
    color = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionConstant3Vector)
    color.set_editor_property('constant', unreal.LinearColor(.55,.65,.8,1))
    unreal.MaterialEditingLibrary.connect_material_property(color, '', unreal.MaterialProperty.MP_BASE_COLOR)
    unreal.MaterialEditingLibrary.recompile_material(material)
default.get_component_by_class(unreal.StaticMeshComponent).set_material(0, material)
mode_path = '/Game/Steppe/BP_SteppeGameMode'
mode = unreal.get_default_object(assets.load_blueprint_class(mode_path))
mode.set_editor_property('wild_horse_class', wild_class)
for asset_path in (config_path, material_path, path, mode_path):
    if not assets.save_asset(asset_path, only_if_is_dirty=False):
        raise RuntimeError('Could not save ' + asset_path)
unreal.log('STEPPE_P2_ASSETS_CREATED')
unreal.SystemLibrary.quit_editor()
