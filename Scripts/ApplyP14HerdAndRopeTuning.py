"""Apply the P14.4 herd scale and cohesion tuning to serialized project assets."""
import unreal

assets = unreal.EditorAssetLibrary

mode_path = '/Game/Steppe/BP_SteppeGameMode'
mode_class = assets.load_blueprint_class(mode_path)
if mode_class is None:
    raise RuntimeError('Missing ' + mode_path)
mode = unreal.get_default_object(mode_class)
mode.set_editor_property('wild_horse_count', 12)

config_path = '/Game/Steppe/Data/Horses/DA_WildHorse_Default'
config = assets.load_asset(config_path)
if config is None:
    raise RuntimeError('Missing ' + config_path)
config.set_editor_property('cohesion_weight', .55)
config.set_editor_property('alignment_weight', .55)
config.set_editor_property('flight_alignment_weight', .9)
config.set_editor_property('flight_cohesion_weight', 1.1)
config.set_editor_property('flight_direction_weight', 1.4)
config.set_editor_property('individual_steering_degrees', 12.0)

for path in (mode_path, config_path):
    if not assets.save_asset(path, only_if_is_dirty=False):
        raise RuntimeError('Could not save ' + path)

unreal.log('STEPPE_P14_HERD_AND_ROPE_TUNING_APPLIED')
unreal.SystemLibrary.quit_editor()
