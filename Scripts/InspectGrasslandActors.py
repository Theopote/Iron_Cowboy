import unreal
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
levels.load_level('/Game/Steppe/Worlds/Prototype/L_Prototype_Grassland')
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for a in actors.get_all_level_actors():
 print(a.get_actor_label(), a.get_class().get_name(), a.get_actor_location())
 if isinstance(a, unreal.SteppeGrassField):
  print('STEPPE_GRASS_INSTANCES', a.get_component_by_class(unreal.HierarchicalInstancedStaticMeshComponent).get_instance_count())
unreal.SystemLibrary.quit_editor()
