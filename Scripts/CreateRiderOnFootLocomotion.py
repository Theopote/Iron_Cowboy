"""Build temporary walking and running rope poses with animated legs."""
import unreal

assets = unreal.EditorAssetLibrary
root = '/Game/Steppe/Presentation/Rider/'
arms = (
    'clavicle_l', 'upperarm_l', 'lowerarm_l', 'hand_l',
    'clavicle_r', 'upperarm_r', 'lowerarm_r', 'hand_r',
)
for action in ('Brace', 'Throw'):
    pose = assets.load_asset(root + 'RiderOnFoot' + action + '_Pose')
    if pose is None:
        raise RuntimeError('Missing on-foot action pose: ' + action)
    for gait in ('Walk', 'Run'):
        source_path = '/Game/Mannequin/Animations/ThirdPerson' + gait
        target_path = root + 'RiderOnFoot' + action + '_' + gait
        if not assets.does_asset_exist(target_path) and not assets.duplicate_asset(source_path, target_path):
            raise RuntimeError('Could not duplicate ' + source_path)
        target = assets.load_asset(target_path)
        controller = target.get_editor_property('controller')
        for bone in arms:
            sample = unreal.AnimationLibrary.get_bone_pose_for_time(pose, bone, 0.0, False)
            controller.set_bone_track_keys(bone, [sample.translation], [sample.rotation], [sample.scale3d], False)
        if not assets.save_asset(target_path, only_if_is_dirty=False):
            raise RuntimeError('Could not save ' + target_path)
        print('STEPPE_ON_FOOT_GAIT', action, gait, target.get_play_length())
unreal.SystemLibrary.quit_editor()
