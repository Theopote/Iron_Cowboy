"""Import the CC0 Quaternius horse into an isolated Unreal content folder."""
import os
import unreal

source = os.path.join(
    unreal.Paths.project_dir(),
    'ThirdParty', 'Quaternius', 'UltimateAnimatedAnimals', 'Source',
    'Model', 'glTF', 'Horse.gltf')
destination = '/Game/Steppe/ThirdParty/Quaternius/AnimatedAnimals/Horse'
if not os.path.isfile(source):
    raise RuntimeError('Missing source horse: ' + source)

task = unreal.AssetImportTask()
task.set_editor_property('filename', source)
task.set_editor_property('destination_path', destination)
task.set_editor_property('automated', True)
task.set_editor_property('save', True)
task.set_editor_property('replace_existing', False)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
assets = unreal.EditorAssetLibrary.list_assets(destination, recursive=True, include_folder=False)
for path in assets:
    asset = unreal.EditorAssetLibrary.load_asset(path)
    unreal.log('STEPPE_TEMP_HORSE_ASSET: {} {}'.format(path, asset.get_class().get_name() if asset else '<missing>'))
unreal.log('STEPPE_TEMP_HORSE_COUNT: {}'.format(len(assets)))
if not assets:
    raise RuntimeError('No UE assets imported from ' + source)
unreal.SystemLibrary.quit_editor()
