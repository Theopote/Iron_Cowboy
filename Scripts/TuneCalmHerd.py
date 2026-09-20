"""Apply calmer unalarmed herd movement to the shipped horse config asset."""
import unreal

path = '/Game/Steppe/Data/Horses/DA_WildHorse_Default'
config = unreal.EditorAssetLibrary.load_asset(path)
if config is None:
    raise RuntimeError('Missing ' + path)

config.set_editor_property('roam_speed', 115.0)
config.set_editor_property('calm_turn_intent_limit', 0.25)
unreal.log('STEPPE_CALM_HERD_VALUES speed={} turn={}'.format(
    config.get_editor_property('roam_speed'),
    config.get_editor_property('calm_turn_intent_limit')))
if not unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False):
    raise RuntimeError('Could not save ' + path)

unreal.log('STEPPE_CALM_HERD_TUNING_APPLIED')
unreal.SystemLibrary.quit_editor()
