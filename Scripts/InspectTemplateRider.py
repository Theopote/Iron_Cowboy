"""Inspect copied UE template rider resources before gameplay integration."""
import unreal

mesh = unreal.EditorAssetLibrary.load_asset('/Game/Mannequin/Character/Mesh/SK_Mannequin.SK_Mannequin')
if mesh is None:
    raise RuntimeError('Template mannequin mesh missing')
skeleton = mesh.get_editor_property('skeleton')
unreal.log('STEPPE_RIDER_MESH {} skeleton={} bounds={}'.format(
    mesh.get_path_name(), skeleton.get_path_name(), mesh.get_bounds()))
unreal.log('STEPPE_RIDER_SKELETON_API {}'.format(
    [name for name in dir(skeleton) if 'socket' in name.lower() or 'bone' in name.lower()]))
unreal.log('STEPPE_RIDER_MESH_API {}'.format(
    [name for name in dir(mesh) if 'bone' in name.lower() or 'socket' in name.lower()]))
unreal.log('STEPPE_SOCKET_SUBSYSTEM_API {}'.format(
    [name for name in dir(unreal.SkeletalMeshEditorSubsystem) if 'socket' in name.lower()]))
for name in ('LassoHand_R', 'Rein_L', 'Rein_R'):
    item=mesh.find_socket(name)
    if item is None:
        raise RuntimeError('Missing rider socket ' + name)
    unreal.log('STEPPE_RIDER_SOCKET {} bone={} loc={}'.format(
        name, item.get_editor_property('bone_name'), item.get_editor_property('relative_location')))
for name in ('ThirdPersonIdle', 'ThirdPersonWalk', 'ThirdPersonRun'):
    anim = unreal.EditorAssetLibrary.load_asset('/Game/Mannequin/Animations/' + name + '.' + name)
    if anim is None:
        raise RuntimeError('Template animation missing: ' + name)
    unreal.log('STEPPE_RIDER_ANIM {} length={}'.format(name, anim.get_play_length()))
unreal.SystemLibrary.quit_editor()
