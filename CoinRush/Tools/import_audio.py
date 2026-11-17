"""Imports the generated WAV files in Tools/AudioSrc into /Game/Audio.

Run by Tools/BuildAll.ps1 after the build:

    UnrealEditor-Cmd.exe CoinRush.uproject -run=pythonscript -script=Tools/import_audio.py
"""

import os

import unreal

SOURCE_DIR = os.path.join(unreal.Paths.project_dir(), "Tools", "AudioSrc")
DEST_DIR = "/Game/Audio"
SOUNDS = ("SFX_Coin", "SFX_Land")

TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
ASSETS = unreal.EditorAssetLibrary


def import_sound(name):
    source = os.path.join(SOURCE_DIR, name + ".wav")
    if not os.path.exists(source):
        unreal.log_error("[CoinRush] audio source missing: " + source)
        return

    task = unreal.AssetImportTask()
    task.filename = source
    task.destination_path = DEST_DIR
    task.destination_name = name
    task.automated = True
    task.replace_existing = True
    task.save = True

    TOOLS.import_asset_tasks([task])

    asset_path = DEST_DIR + "/" + name
    if ASSETS.does_asset_exist(asset_path):
        unreal.log("[CoinRush] imported " + asset_path)
    else:
        unreal.log_error("[CoinRush] import failed: " + asset_path)


def main():
    for name in SOUNDS:
        import_sound(name)


main()
