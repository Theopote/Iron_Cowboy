"""Create the rider AnimBlueprint; its idle-to-slot graph is populated by the editor build command."""
import unreal

path = '/Game/Steppe/Presentation/ABP_Rider'
assets = unreal.EditorAssetLibrary
mesh = assets.load_asset('/Game/Mannequin/Character/Mesh/SK_Mannequin')
if mesh is None:
    raise RuntimeError('Rider skeletal mesh missing')
bp = assets.load_asset(path)
if bp is None:
    factory = unreal.AnimBlueprintFactory()
    factory.set_editor_property('target_skeleton', mesh.get_editor_property('skeleton'))
    factory.set_editor_property('preview_skeletal_mesh', mesh)
    factory.set_editor_property('parent_class', unreal.RiderAnimInstance.static_class())
    bp = unreal.AssetToolsHelpers.get_asset_tools().create_asset('ABP_Rider', '/Game/Steppe/Presentation',
        unreal.AnimBlueprint, factory)
if bp is None:
    raise RuntimeError('ABP_Rider creation failed')
assets.save_asset(path, only_if_is_dirty=False)
unreal.log('STEPPE_RIDER_ABP {}'.format(bp.get_path_name()))
unreal.SystemLibrary.quit_editor()
