"""Build a temporary upright mounted pose from template-local limb transforms."""
import unreal

assets = unreal.EditorAssetLibrary
idle_path = '/Game/Mannequin/Animations/ThirdPersonIdle'
jump_path = '/Game/Mannequin/Animations/ThirdPersonJump_Loop'
pose_path = '/Game/Steppe/Presentation/Rider/RiderMounted_Pose'

idle = assets.load_asset(idle_path)
jump = assets.load_asset(jump_path)
pose = assets.load_asset(pose_path)
if pose is None:
    if not assets.duplicate_asset(idle_path, pose_path):
        raise RuntimeError('Could not duplicate the rider idle animation')
    pose = assets.load_asset(pose_path)
if pose is None or jump is None:
    raise RuntimeError('Mounted pose inputs are missing')

# Keep root, pelvis and spine from Idle so the character remains upright. The
# template jump loop contributes useful bent limbs without its airborne root.
limbs = (
    'thigh_l','calf_l','foot_l','thigh_r','calf_r','foot_r',
    'clavicle_l','upperarm_l','lowerarm_l','hand_l',
    'clavicle_r','upperarm_r','lowerarm_r','hand_r',
)
controller = pose.get_editor_property('controller')
for bone in limbs:
    sample = unreal.AnimationLibrary.get_bone_pose_for_time(jump, bone, 0.20, False)
    controller.set_bone_track_keys(bone, [sample.translation], [sample.rotation], [sample.scale3d], False)

assets.save_asset(pose_path, only_if_is_dirty=False)
unreal.log('STEPPE_RIDER_MOUNTED_POSE {} limbs={}'.format(pose.get_path_name(), len(limbs)))
unreal.SystemLibrary.quit_editor()
