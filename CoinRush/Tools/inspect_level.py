"""Dev helper: print every actor in /Game/Maps/CoinRush.

    UnrealEditor-Cmd.exe CoinRush.uproject -run=pythonscript -script=Tools/inspect_level.py
"""

import unreal

LEVELS = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
ACTORS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def run():
    if not LEVELS.load_level("/Game/Maps/CoinRush"):
        unreal.log_error("[inspect] could not load level")
        return
    for actor in ACTORS.get_all_level_actors():
        cls = actor.get_class().get_name()
        loc = actor.get_actor_location()
        scale = actor.get_actor_scale3d()
        unreal.log("[inspect] %-28s %-22s loc=(%.0f, %.0f, %.0f) scale=(%.2f, %.2f, %.2f)" % (
            actor.get_actor_label(), cls, loc.x, loc.y, loc.z, scale.x, scale.y, scale.z))


run()
