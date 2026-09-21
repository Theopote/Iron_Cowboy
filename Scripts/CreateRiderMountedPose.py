"""Build a temporary upright mounted pose from template-local limb transforms."""
import unreal

assets = unreal.EditorAssetLibrary
idle_path = '/Game/Mannequin/Animations/ThirdPersonIdle'
jump_path = '/Game/Mannequin/Animations/ThirdPersonJump_Loop'
jump_start_path = '/Game/Mannequin/Animations/ThirdPersonJump_Start'
pose_path = '/Game/Steppe/Presentation/Rider/RiderMounted_Pose'

idle = assets.load_asset(idle_path)
jump = assets.load_asset(jump_path)
jump_start = assets.load_asset(jump_start_path)
pose = assets.load_asset(pose_path)
if pose is None:
    if not assets.duplicate_asset(idle_path, pose_path):
        raise RuntimeError('Could not duplicate the rider idle animation')
    pose = assets.load_asset(pose_path)
if pose is None or jump is None or jump_start is None:
    raise RuntimeError('Mounted pose inputs are missing')

# Keep root, pelvis and spine from Idle so the character remains upright. The
# template jump loop contributes useful bent limbs without its airborne root.
legs = ('thigh_l','calf_l','foot_l','thigh_r','calf_r','foot_r')
arms = (
    'clavicle_l','upperarm_l','lowerarm_l','hand_l',
    'clavicle_r','upperarm_r','lowerarm_r','hand_r',
)
controller = pose.get_editor_property('controller')
for bone in legs:
    # Jump Start begins with both knees bent by roughly 90 degrees. Jump Loop
    # is deliberately asymmetric and made the mannequin look side-saddle.
    sample = unreal.AnimationLibrary.get_bone_pose_for_time(jump_start, bone, 0.0, False)
    controller.set_bone_track_keys(bone, [sample.translation], [sample.rotation], [sample.scale3d], False)
for bone in arms:
    sample = unreal.AnimationLibrary.get_bone_pose_for_time(jump, bone, 0.20, False)
    controller.set_bone_track_keys(bone, [sample.translation], [sample.rotation], [sample.scale3d], False)

assets.save_asset(pose_path, only_if_is_dirty=False)
unreal.log('STEPPE_RIDER_MOUNTED_POSE {} symmetric_legs={} arms={}'.format(pose.get_path_name(), len(legs), len(arms)))

# Four coarse upper-body phases make gameplay SwingPhase readable before a
# dedicated lasso animation is authored. Legs and torso remain in the seat.
right_arm = ('clavicle_r','upperarm_r','lowerarm_r','hand_r')
for index, sample_time in enumerate((0.0, 0.15, 0.30, 0.45)):
    swing_path = '/Game/Steppe/Presentation/Rider/RiderLassoSwing_{}'.format(index)
    swing = assets.load_asset(swing_path)
    if swing is None:
        if not assets.duplicate_asset(pose_path, swing_path):
            raise RuntimeError('Could not duplicate lasso swing phase {}'.format(index))
        swing = assets.load_asset(swing_path)
    swing_controller = swing.get_editor_property('controller')
    for bone in legs:
        sample = unreal.AnimationLibrary.get_bone_pose_for_time(jump_start, bone, 0.0, False)
        swing_controller.set_bone_track_keys(bone, [sample.translation], [sample.rotation], [sample.scale3d], False)
    for bone in right_arm:
        sample = unreal.AnimationLibrary.get_bone_pose_for_time(jump, bone, sample_time, False)
        swing_controller.set_bone_track_keys(bone, [sample.translation], [sample.rotation], [sample.scale3d], False)
    assets.save_asset(swing_path, only_if_is_dirty=False)
    unreal.log('STEPPE_RIDER_LASSO_POSE {} time={}'.format(index, sample_time))

def copy_pose(source_path, target_path):
    target = assets.load_asset(target_path)
    if target is None:
        if not assets.duplicate_asset(source_path, target_path):
            raise RuntimeError('Could not duplicate {}'.format(target_path))
        target = assets.load_asset(target_path)
    return target

def replace_arms(target, source, sample_time, both_arms=True):
    bones = ('clavicle_r','upperarm_r','lowerarm_r','hand_r')
    if both_arms:
        bones += ('clavicle_l','upperarm_l','lowerarm_l','hand_l')
    target_controller = target.get_editor_property('controller')
    for bone in bones:
        sample = unreal.AnimationLibrary.get_bone_pose_for_time(source, bone, sample_time, False)
        target_controller.set_bone_track_keys(bone, [sample.translation], [sample.rotation], [sample.scale3d], False)

def replace_mounted_legs(target):
    target_controller = target.get_editor_property('controller')
    for bone in legs:
        sample = unreal.AnimationLibrary.get_bone_pose_for_time(jump_start, bone, 0.0, False)
        target_controller.set_bone_track_keys(bone, [sample.translation], [sample.rotation], [sample.scale3d], False)

# Throw retains the last overhead swing phase. Brace uses both arms from the
# compact jump-start pose, while preserving seated or standing lower bodies.
mounted_throw_path = '/Game/Steppe/Presentation/Rider/RiderMountedThrow_Pose'
mounted_throw = copy_pose('/Game/Steppe/Presentation/Rider/RiderLassoSwing_3', mounted_throw_path)
replace_mounted_legs(mounted_throw)
assets.save_asset(mounted_throw_path, only_if_is_dirty=False)

for target_path, base_path in (
    ('/Game/Steppe/Presentation/Rider/RiderMountedBrace_Pose', pose_path),
    ('/Game/Steppe/Presentation/Rider/RiderOnFootBrace_Pose', idle_path),
):
    target = copy_pose(base_path, target_path)
    if target_path.endswith('MountedBrace_Pose'):
        replace_mounted_legs(target)
    replace_arms(target, jump_start, 0.20, True)
    assets.save_asset(target_path, only_if_is_dirty=False)

on_foot_throw_path = '/Game/Steppe/Presentation/Rider/RiderOnFootThrow_Pose'
on_foot_throw = copy_pose(idle_path, on_foot_throw_path)
replace_arms(on_foot_throw, jump, 0.45, False)
assets.save_asset(on_foot_throw_path, only_if_is_dirty=False)
unreal.log('STEPPE_RIDER_ACTION_POSES mounted_throw=1 mounted_brace=1 onfoot_throw=1 onfoot_brace=1')
unreal.SystemLibrary.quit_editor()
