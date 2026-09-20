"""Create and inspect the first horse AnimBlueprint; graph is populated by the editor-only build command."""
import unreal

path = '/Game/Steppe/Presentation/ABP_Horse'
assets = unreal.EditorAssetLibrary
mesh = assets.load_asset('/Game/Steppe/ThirdParty/Quaternius/AnimatedAnimals/Horse/Horse/SkeletalMeshes/Horse')
if mesh is None:
    raise RuntimeError('Horse skeletal mesh missing')
bp = assets.load_asset(path)
if bp is None:
    factory = unreal.AnimBlueprintFactory()
    factory.set_editor_property('target_skeleton', mesh.get_editor_property('skeleton'))
    factory.set_editor_property('preview_skeletal_mesh', mesh)
    factory.set_editor_property('parent_class', unreal.HorseAnimInstance.static_class())
    bp = unreal.AssetToolsHelpers.get_asset_tools().create_asset('ABP_Horse', '/Game/Steppe/Presentation',
        unreal.AnimBlueprint, factory)
if bp is None:
    raise RuntimeError('ABP_Horse creation failed')
unreal.log('STEPPE_HORSE_ABP {} graphs={}'.format(bp.get_path_name(),
    [g.get_name() for g in bp.get_animation_graphs()]))
assets.save_asset(path, only_if_is_dirty=False)
unreal.SystemLibrary.quit_editor()
