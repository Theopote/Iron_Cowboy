"""Build P16.6A's deterministic 806 m gameplay Landscape and route the loop through it."""
import unreal

MAP = '/Game/Steppe/Worlds/Prototype/L_Prototype_Grassland'
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assets = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
if not levels.load_level(MAP):
    raise RuntimeError('Could not load grassland map')

if not unreal.SteppeLandscapeBuilder.build_grassland_blockout():
    raise RuntimeError('C++ Landscape builder failed')

# Retire the flat arena and its tuning fixtures. P16.6 keeps a normal single
# level; the real Landscape now owns traversal and collision.
old_prefixes = ('Grassland_2km_TestFloor', 'Slope_Test_Ramp', 'Slalom_', 'DistanceMarker_')
for actor in list(actors.get_all_level_actors()):
    if actor.get_actor_label().startswith(old_prefixes + ('P16.6_',)) or actor.get_actor_label() == 'HardSurface_TestPad':
        actors.destroy_actor(actor)

def material(name, color, physical=None, wind=False):
    path = '/Game/Steppe/Debug/' + name
    mat = assets.load_asset(path)
    if mat is None:
        mat = tools.create_asset(name, '/Game/Steppe/Debug', unreal.Material, unreal.MaterialFactoryNew())
        node = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector)
        node.set_editor_property('constant', unreal.LinearColor(*color, 1))
        unreal.MaterialEditingLibrary.connect_material_property(node, '', unreal.MaterialProperty.MP_BASE_COLOR)
        rough = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionConstant)
        rough.set_editor_property('r', .88)
        unreal.MaterialEditingLibrary.connect_material_property(rough, '', unreal.MaterialProperty.MP_ROUGHNESS)
        if wind:
            clock = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionTime)
            speed = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionConstant)
            phase = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionMultiply)
            wave = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionSine)
            direction = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector)
            offset = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionMultiply)
            speed.set_editor_property('r', .55)
            direction.set_editor_property('constant', unreal.LinearColor(9.0, 4.0, 0.0, 1.0))
            unreal.MaterialEditingLibrary.connect_material_expressions(clock, '', phase, 'A')
            unreal.MaterialEditingLibrary.connect_material_expressions(speed, '', phase, 'B')
            unreal.MaterialEditingLibrary.connect_material_expressions(phase, '', wave, '')
            unreal.MaterialEditingLibrary.connect_material_expressions(wave, '', offset, 'A')
            unreal.MaterialEditingLibrary.connect_material_expressions(direction, '', offset, 'B')
            unreal.MaterialEditingLibrary.connect_material_property(offset, '', unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
    if physical:
        mat.set_editor_property('phys_material', assets.load_asset(physical))
    unreal.MaterialEditingLibrary.recompile_material(mat)
    assets.save_loaded_asset(mat)
    return mat

cube = assets.load_asset('/Engine/BasicShapes/Cube')
sphere = assets.load_asset('/Engine/BasicShapes/Sphere')
grass = assets.load_asset('/Game/Steppe/Debug/M_PrototypeGrass')
dirt = material('M_P16_6_HardGround', (.20, .13, .07), '/Game/Steppe/Feedback/PM_Hard')
water = material('M_P16_6_ShallowWater', (.035, .19, .25))
wood = material('M_P16_6_Wood', (.16, .075, .025))
canvas = material('M_P16_6_Canvas', (.42, .31, .17))
rock = material('M_P16_6_Rock', (.22, .23, .20))
assets.delete_asset('/Game/Steppe/Debug/M_P16_6_GrassBlade')
grass_blade = material('M_P16_6_GrassBlade', (.12, .28, .055), wind=True)
grass_blade.set_editor_property('used_with_instanced_static_meshes', True)
assets.save_loaded_asset(grass_blade)

def block(label, location, scale, mat, mesh=cube, collision=True, yaw=0):
    a = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*location), unreal.Rotator(0, yaw, 0))
    a.set_actor_label(label)
    a.set_actor_scale3d(unreal.Vector(*scale))
    c = a.get_component_by_class(unreal.StaticMeshComponent)
    c.set_static_mesh(mesh)
    c.set_material(0, mat)
    if not collision:
        c.set_collision_profile_name('NoCollision')
        c.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    return a

# River and alternative hard-ground corridor both cross the camp-to-herd route.
block('P16.6_ShallowRiver', (0, -17500, -90), (350, 18, .10), water, collision=False)
block('P16.6_HardGroundRoute', (17000, -1500, 15), (18, 150, .22), dirt)

river_zone = actors.spawn_actor_from_class(unreal.SteppeTerrainZone, unreal.Vector(0, -17500, 80))
river_zone.set_actor_label('P16.6_RiverSlowZone')
river_zone.set_editor_property('zone_type', unreal.SteppeTerrainZoneType.SHALLOW_WATER)
river_zone.set_editor_property('movement_scale', .62)
river_zone.get_component_by_class(unreal.BoxComponent).set_box_extent(unreal.Vector(35000, 1900, 260))

# A lightweight instanced field follows the Landscape collision. The material
# remains deliberately simple during blockout; density and culling preserve the
# long sight lines needed to read the herd from hundreds of metres away.
grass_field = actors.spawn_actor_from_class(unreal.SteppeGrassField, unreal.Vector(0, 0, 0))
grass_field.set_actor_label('P16.6_LandscapeGrass')
grass_field.set_editor_property('grass_material', grass_blade)
grass_field.set_editor_property('instance_count', 3200)
grass_field.rebuild_instances()

# Sparse trees form a useful wrap corridor without becoming a forest.
tree_positions = ((-15000,-9000),(-12500,-6000),(-16200,-2500),(-11800,1500),(-15500,5200),(-10000,8500))
for i,(x,y) in enumerate(tree_positions):
    block('P16.6_TreeTrunk_%02d'%i, (x,y,260), (.55,.55,5.2), wood)
    block('P16.6_TreeCrown_%02d'%i, (x,y,670), (2.5,2.5,2.1), grass, mesh=sphere, collision=False)

# Rocky ground is readable from the open center and constrains the eastern line.
for i,(x,y,s) in enumerate(((22000,-7000,1.8),(24500,-2500,2.4),(21000,2500,1.5),(26000,6500,2.1),(19000,10000,1.4))):
    block('P16.6_Rock_%02d'%i, (x,y,80*s), (s*1.5,s,s*.8), rock, yaw=i*23)

# Minimal camp and pen: tent, fire marker, rail, trough and enclosing rails.
block('P16.6_Tent', (-2500,-32000,120), (3.2,2.3,1.2), canvas, yaw=12)
block('P16.6_Fire', (1200,-31900,25), (.55,.55,.25), material('M_P16_6_Fire',(.9,.22,.03)), collision=False)
block('P16.6_HorseRail', (3000,-30400,105), (4.5,.18,.18), wood)
block('P16.6_Trough', (3000,-32300,45), (2.2,.65,.45), water)
for i,(loc,scale) in enumerate((((-5200,-33600,90),(10,.18,.9)),((5200,-33600,90),(10,.18,.9)),((-5200,-32600,90),(.18,10,.9)),((5200,-32600,90),(.18,10,.9)))):
    block('P16.6_PenRail_%02d'%i, loc, scale, wood)

# Camp starts in the south, herd habitat sits beyond the river in the north.
for actor in actors.get_all_level_actors():
    if isinstance(actor, unreal.PlayerStart):
        actor.set_actor_location(unreal.Vector(0, -31000, 150), False, False)

mode_class = assets.load_blueprint_class('/Game/Steppe/BP_SteppeGameMode')
if mode_class is None:
    raise RuntimeError('Steppe game mode missing')
mode = unreal.get_default_object(mode_class)
mode.set_editor_property('horse_spawn_transform', unreal.Transform(location=unreal.Vector(100, -30900, 150)))
mode.set_editor_property('wild_horse_spawn_transform', unreal.Transform(location=unreal.Vector(1500, 12500, 180)))
mode.set_editor_property('herd_habitat_extents', unreal.Vector2D(6500, 4200))
mode.set_editor_property('delivery_zone_transform', unreal.Transform(location=unreal.Vector(-1200, -31500, 100)))
assets.save_asset('/Game/Steppe/BP_SteppeGameMode', only_if_is_dirty=False)

if not levels.save_current_level():
    raise RuntimeError('Could not save P16.6 Landscape map')
unreal.log('STEPPE_P16_6_BLOCKOUT_SAVED camp_y=-310m herd_y=125m traversal=435m')
unreal.SystemLibrary.quit_editor()
