"""Add mesh-local rider, rein and lasso anchors without modifying gameplay collision."""
import unreal

assets = unreal.EditorAssetLibrary
horse_path = '/Game/Steppe/ThirdParty/Quaternius/AnimatedAnimals/Horse/Horse/SkeletalMeshes/Horse'
rider_path = '/Game/Mannequin/Character/Mesh/SK_Mannequin'

def socket(mesh, name, bone, location=(0, 0, 0), yaw=0):
    existing = mesh.find_socket(name)
    if existing:
        if str(existing.get_editor_property('bone_name')) != bone:
            raise RuntimeError('Socket {} is attached to {}, expected {}. Run with -ExtraArgs -SteppeFixSockets.'.format(
                name, existing.get_editor_property('bone_name'), bone))
        unreal.log('STEPPE_SOCKET_EXISTS {} {}'.format(mesh.get_name(), name))
        return
    component = unreal.SkeletalMeshComponent()
    component.set_skeletal_mesh_asset(mesh)
    bone_location = component.get_socket_location(bone)
    item = unreal.SkeletalMeshSocket(outer=mesh)
    item.initialize_socket_from_location(component, bone_location, unreal.Vector(0, 0, 1))
    item.set_editor_property('relative_location', unreal.Vector(*location))
    item.set_editor_property('relative_rotation', unreal.Rotator(0, yaw, 0))
    mesh.add_socket(item)
    old_name = item.get_editor_property('socket_name')
    unreal.log('STEPPE_SOCKET_RENAME {} {} -> {}'.format(mesh.get_name(), old_name, name))
    if not unreal.SkeletalMeshEditorSubsystem.rename_socket(mesh, old_name, name):
        raise RuntimeError('Could not rename socket ' + name)
    unreal.log('STEPPE_SOCKET_ADDED {} {} bone={} loc={}'.format(mesh.get_name(), name, bone, location))

horse = assets.load_asset(horse_path)
rider = assets.load_asset(rider_path)
if horse is None or rider is None:
    raise RuntimeError('Horse or rider mesh missing')

# The imported horse mesh is scaled to .45 and offset by -94 cm in the actor.
# Body is at ~91 cm in the import; +360 cm places the rider capsule centre at the former 110 cm seat.
socket(horse, 'RiderSeat', 'Body', (0, 0, 360), 90)
socket(horse, 'Head', 'Head')
socket(horse, 'Neck', 'Neck2')
socket(horse, 'Chest', 'Body', (0, 120, 70))
socket(rider, 'LassoHand_R', 'hand_r', (8, 0, 0))
socket(rider, 'Rein_L', 'hand_l')
socket(rider, 'Rein_R', 'hand_r')

for path in (horse_path, rider_path):
    if not assets.save_asset(path, only_if_is_dirty=False):
        raise RuntimeError('Failed to save ' + path)
unreal.log('STEPPE_PRESENTATION_SOCKETS_APPLIED')
unreal.SystemLibrary.quit_editor()
