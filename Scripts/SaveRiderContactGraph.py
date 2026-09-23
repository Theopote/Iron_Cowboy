"""Persist the C++-built rider contact graph and fail if it did not link."""
import unreal

path = '/Game/Steppe/Presentation/ABP_Rider'
bp = unreal.EditorAssetLibrary.load_asset(path)
if bp is None:
    raise RuntimeError('Rider AnimBlueprint missing')
if not unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False):
    raise RuntimeError('Failed to save rider contact graph')
unreal.log('STEPPE_RIDER_CONTACT_GRAPH_SAVED')
unreal.SystemLibrary.quit_editor()
