add_rules("mode.debug", "mode.release")

set_languages("c17", "c++20")

includes("hash")
includes("flow")
includes("play_ground")

add_rules("plugin.compile_commands.autoupdate", {outputdir = ".vscode"})

