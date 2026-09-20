"""Read horse bone positions used to place a rider seat socket."""
import unreal

base = '/Game/Steppe/ThirdParty/Quaternius/AnimatedAnimals/Horse/Horse/SkeletalMeshes/'
mesh = unreal.EditorAssetLibrary.load_asset(base + 'Horse.Horse')
anim = unreal.EditorAssetLibrary.load_asset(base + 'HorseIdle.HorseIdle')
for name in ('Body', 'Back', 'Head', 'Neck', 'root', 'Root'):
    try:
        pose = unreal.AnimationLibrary.get_bone_pose_for_time(anim, name, 0.0, True)
        unreal.log('STEPPE_HORSE_BONE {} loc={} rot={}'.format(name, pose.translation, pose.rotation))
    except Exception as exc:
        unreal.log('STEPPE_HORSE_BONE_MISSING {} {}'.format(name, exc))
unreal.log('STEPPE_HORSE_BOUNDS {}'.format(mesh.get_bounds()))
for name in ('RiderSeat', 'Head', 'Neck', 'Chest'):
    item=mesh.find_socket(name)
    if item is None:
        raise RuntimeError('Missing horse socket ' + name)
    unreal.log('STEPPE_HORSE_SOCKET {} bone={} loc={} rot={}'.format(
        name, item.get_editor_property('bone_name'),
        item.get_editor_property('relative_location'),
        item.get_editor_property('relative_rotation')))
unreal.SystemLibrary.quit_editor()
