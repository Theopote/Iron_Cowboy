"""Create a speed blend space from the imported CC0 horse clips."""
import unreal

base = '/Game/Steppe/ThirdParty/Quaternius/AnimatedAnimals/Horse/Horse/SkeletalMeshes/'
skeleton = unreal.EditorAssetLibrary.load_asset(base + 'Horse_Skeleton.Horse_Skeleton')
clips = [unreal.EditorAssetLibrary.load_asset(base + name + '.' + name)
         for name in ('HorseIdle', 'HorseWalk', 'HorseGallop')]
if not skeleton or any(clip is None for clip in clips):
    raise RuntimeError('Imported horse skeleton or locomotion clips are missing')
destination = '/Game/Steppe/Animation/Horses'
name = 'BS_TemporaryHorseSpeed'
path = destination + '/' + name
blend = unreal.EditorAssetLibrary.load_asset(path)
if blend is None:
    factory = unreal.BlendSpaceFactory1D()
    factory.set_editor_property('target_skeleton', skeleton)
    blend = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, destination, unreal.BlendSpace1D, factory)
if blend is None:
    raise RuntimeError('Could not create temporary horse blend space')
samples = []
for speed, clip in zip((0, 35, 100), clips):
    sample = unreal.BlendSample()
    sample.set_editor_property('animation', clip)
    sample.set_editor_property('sample_value', unreal.Vector(speed, 0, 0))
    samples.append(sample)
blend.set_editor_property('sample_data', samples)
unreal.EditorAssetLibrary.save_loaded_asset(blend)
unreal.log('STEPPE_TEMP_HORSE_BLEND: {} samples={}'.format(path, len(blend.get_editor_property('sample_data'))))
unreal.SystemLibrary.quit_editor()
