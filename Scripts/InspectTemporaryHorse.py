"""Check the imported horse asset independently of the import session."""
import unreal

base = '/Game/Steppe/ThirdParty/Quaternius/AnimatedAnimals/Horse/Horse/SkeletalMeshes/'
mesh = unreal.EditorAssetLibrary.load_asset(base + 'Horse.Horse')
if not mesh:
    raise RuntimeError('Imported horse skeletal mesh is missing')
bounds = mesh.get_bounds()
unreal.log('STEPPE_TEMP_HORSE_MESH: bounds={} skeleton={}'.format(
    bounds, mesh.get_editor_property('skeleton').get_path_name()))
for slot in mesh.get_editor_property('materials'):
    material = slot.get_editor_property('material_interface')
    unreal.log('STEPPE_TEMP_HORSE_MATERIAL: {} {}'.format(
        slot.get_editor_property('material_slot_name'), material.get_path_name() if material else '<missing>'))
    if material and material.get_class().get_name() == 'MaterialInstanceConstant':
        unreal.log('STEPPE_TEMP_HORSE_MATERIAL_PARAMS: {} {}'.format(
            material.get_name(), [(entry.get_editor_property('parameter_info').get_editor_property('name'),
                                   entry.get_editor_property('parameter_value'))
                                  for entry in material.get_editor_property('vector_parameter_values')]))
blend = unreal.EditorAssetLibrary.load_asset('/Game/Steppe/Animation/Horses/BS_TemporaryHorseSpeed')
if blend:
    unreal.log('STEPPE_TEMP_HORSE_BLEND_CHECK: {}'.format([
        (sample.get_editor_property('animation').get_name(),sample.get_editor_property('sample_value').x)
        for sample in blend.get_editor_property('sample_data')]))
for name in ('HorseIdle', 'HorseWalk', 'HorseGallop', 'HorseIdle_2'):
    asset = unreal.EditorAssetLibrary.load_asset(base + name + '.' + name)
    if not asset:
        raise RuntimeError('Missing temporary animation: ' + name)
    unreal.log('STEPPE_TEMP_HORSE_ANIM: {} length={}'.format(name, asset.get_play_length()))
unreal.SystemLibrary.quit_editor()
