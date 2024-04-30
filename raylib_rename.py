help_text = """
This is a helper script which may be used to rename
declarations in the raylib library.

Unfortunately, raylib uses some function and struct names
which are likely to conflict with other libraries,
such as the struct type "Rectangle".

This script solves the problem by adding a "Raylib" prefix
to all functions and structs not already prefixed with "rl"
and a "RL_" prefix to all defines, macros, and enum
values not already prefixed with "RL_".

You can perform this prefix renaming in the raylib source
code by using this script like so:

```
python raylib_rename.py raylib \
    [path to raylib directory]
```

The script can also be used to update references to raylib
declarations in some existing source code, like so:

```
python raylib_rename.py dep \
    [path to raylib directory] \
    [one or more source file glob patterns...]
```
"""

import argparse
import glob
import json
import os
import re
import shutil
import sys

# Prefix functions and types with this string
ID_PREFIX_LOWER = "RL_"
# Prefix defines and enum members with this string
ID_PREFIX_UPPER = "RL_"

ID_EXCEPTIONS = {
    "__declspec": None,
    "fopen": None,
    "fclose": None,
    "MAX_PATH": None,
    "APIENTRY": None,
    "WINGDIAPI": None,
    "PLATFORM": "RL_PLATFORM",
    "OPENGL_VERSION": "RL_OPENGL_VERSION",
    "PLATFORM_DESKTOP": "RL_PLATFORM_DESKTOP",
    "PLATFORM_ANDROID": "RL_PLATFORM_ANDROID",
    "PLATFORM_RPI": "RL_PLATFORM_RPI",
    "PLATFORM_DRM": "RL_PLATFORM_DRM",
    "PLATFORM_WEB": "RL_PLATFORM_WEB",
    "PLATFORM_DESKTOP_SDL": "RL_PLATFORM_DESKTOP_SDL",
    "GRAPHICS_API_OPENGL_11": "RL_GRAPHICS_API_OPENGL_11",
    "GRAPHICS_API_OPENGL_21": "RL_GRAPHICS_API_OPENGL_21",
    "GRAPHICS_API_OPENGL_33": "RL_GRAPHICS_API_OPENGL_33",
    "GRAPHICS_API_OPENGL_43": "RL_GRAPHICS_API_OPENGL_43",
    "GRAPHICS_API_OPENGL_ES2": "RL_GRAPHICS_API_OPENGL_ES2",
    "GRAPHICS_API_OPENGL_ES3": "RL_GRAPHICS_API_OPENGL_ES3",
    # Cmake Options
    "BUILD_EXAMPLES": "RL_BUILD_EXAMPLES",
    "CUSTOMIZE_BUILD": "RL_CUSTOMIZE_BUILD",
    "ENABLE_ASAN": "RL_ENABLE_ASAN",
    "ENABLE_UBSAN": "RL_ENABLE_UBSAN",
    "ENABLE_MSAN": "RL_ENABLE_MSAN",
    "WITH_PIC": "RL_WITH_PIC",
    "BUILD_SHARED_LIBS": "RL_BUILD_SHARED_LIBS",
    "MACOS_FATLIB": "RL_MACOS_FATLIB",
    "USE_AUDIO": "RL_USE_AUDIO",
    "USE_EXTERNAL_GLFW": "RL_USE_EXTERNAL_GLFW",
    "GLFW_BUILD_WAYLAND": "RL_GLFW_BUILD_WAYLAND",
    "GLFW_BUILD_X11": "RL_GLFW_BUILD_X11",
    "INCLUDE_EVERYTHING": "RL_INCLUDE_EVERYTHING",
    "SUPPORT_MODULE_RSHAPES": "RL_SUPPORT_MODULE_RSHAPES",
    "SUPPORT_MODULE_RTEXTURES": "RL_SUPPORT_MODULE_RTEXTURES",
    "SUPPORT_MODULE_RTEXT": "RL_SUPPORT_MODULE_RTEXT",
    "SUPPORT_MODULE_RMODELS": "RL_SUPPORT_MODULE_RMODELS",
    "SUPPORT_MODULE_RAUDIO": "RL_SUPPORT_MODULE_RAUDIO",
    "SUPPORT_CAMERA_SYSTEM": "RL_SUPPORT_CAMERA_SYSTEM",
    "SUPPORT_GESTURES_SYSTEM": "RL_SUPPORT_GESTURES_SYSTEM",
    "SUPPORT_RPRAND_GENERATOR": "RL_SUPPORT_RPRAND_GENERATOR",
    "SUPPORT_MOUSE_GESTURES": "RL_SUPPORT_MOUSE_GESTURES",
    "SUPPORT_SSH_KEYBOARD_RPI": "RL_SUPPORT_SSH_KEYBOARD_RPI",
    "SUPPORT_DEFAULT_FONT": "RL_SUPPORT_DEFAULT_FONT",
    "SUPPORT_SCREEN_CAPTURE": "RL_SUPPORT_SCREEN_CAPTURE",
    "SUPPORT_GIF_RECORDING": "RL_SUPPORT_GIF_RECORDING",
    "SUPPORT_BUSY_WAIT_LOOP": "RL_SUPPORT_BUSY_WAIT_LOOP",
    "SUPPORT_EVENTS_WAITING": "RL_SUPPORT_EVENTS_WAITING",
    "SUPPORT_WINMM_HIGHRES_TIMER": "RL_SUPPORT_WINMM_HIGHRES_TIMER",
    "SUPPORT_COMPRESSION_API": "RL_SUPPORT_COMPRESSION_API",
    "SUPPORT_EVENTS_AUTOMATION": "RL_SUPPORT_EVENTS_AUTOMATION",
    "SUPPORT_CUSTOM_FRAME_CONTROL": "RL_SUPPORT_CUSTOM_FRAME_CONTROL",
    "SUPPORT_QUADS_DRAW_MODE": "RL_SUPPORT_QUADS_DRAW_MODE",
    "SUPPORT_IMAGE_EXPORT": "RL_SUPPORT_IMAGE_EXPORT",
    "SUPPORT_IMAGE_GENERATION": "RL_SUPPORT_IMAGE_GENERATION",
    "SUPPORT_IMAGE_MANIPULATION": "RL_SUPPORT_IMAGE_MANIPULATION",
    "SUPPORT_FILEFORMAT_PNG": "RL_SUPPORT_FILEFORMAT_PNG",
    "SUPPORT_FILEFORMAT_DDS": "RL_SUPPORT_FILEFORMAT_DDS",
    "SUPPORT_FILEFORMAT_HDR": "RL_SUPPORT_FILEFORMAT_HDR",
    "SUPPORT_FILEFORMAT_PIC": "RL_SUPPORT_FILEFORMAT_PIC",
    "SUPPORT_FILEFORMAT_PNM": "RL_SUPPORT_FILEFORMAT_PNM",
    "SUPPORT_FILEFORMAT_KTX": "RL_SUPPORT_FILEFORMAT_KTX",
    "SUPPORT_FILEFORMAT_ASTC": "RL_SUPPORT_FILEFORMAT_ASTC",
    "SUPPORT_FILEFORMAT_BMP": "RL_SUPPORT_FILEFORMAT_BMP",
    "SUPPORT_FILEFORMAT_TGA": "RL_SUPPORT_FILEFORMAT_TGA",
    "SUPPORT_FILEFORMAT_JPG": "RL_SUPPORT_FILEFORMAT_JPG",
    "SUPPORT_FILEFORMAT_GIF": "RL_SUPPORT_FILEFORMAT_GIF",
    "SUPPORT_FILEFORMAT_QOI": "RL_SUPPORT_FILEFORMAT_QOI",
    "SUPPORT_FILEFORMAT_PSD": "RL_SUPPORT_FILEFORMAT_PSD",
    "SUPPORT_FILEFORMAT_PKM": "RL_SUPPORT_FILEFORMAT_PKM",
    "SUPPORT_FILEFORMAT_PVR": "RL_SUPPORT_FILEFORMAT_PVR",
    "SUPPORT_FILEFORMAT_SVG": "RL_SUPPORT_FILEFORMAT_SVG",
    "SUPPORT_FILEFORMAT_FNT": "RL_SUPPORT_FILEFORMAT_FNT",
    "SUPPORT_FILEFORMAT_TTF": "RL_SUPPORT_FILEFORMAT_TTF",
    "SUPPORT_TEXT_MANIPULATION": "RL_SUPPORT_TEXT_MANIPULATION",
    "SUPPORT_FONT_ATLAS_WHITE_REC": "RL_SUPPORT_FONT_ATLAS_WHITE_REC",
    "SUPPORT_MESH_GENERATION": "RL_SUPPORT_MESH_GENERATION",
    "SUPPORT_FILEFORMAT_OBJ": "RL_SUPPORT_FILEFORMAT_OBJ",
    "SUPPORT_FILEFORMAT_MTL": "RL_SUPPORT_FILEFORMAT_MTL",
    "SUPPORT_FILEFORMAT_IQM": "RL_SUPPORT_FILEFORMAT_IQM",
    "SUPPORT_FILEFORMAT_GLTF": "RL_SUPPORT_FILEFORMAT_GLTF",
    "SUPPORT_FILEFORMAT_VOX": "RL_SUPPORT_FILEFORMAT_VOX",
    "SUPPORT_FILEFORMAT_M3D": "RL_SUPPORT_FILEFORMAT_M3D",
    "SUPPORT_FILEFORMAT_WAV": "RL_SUPPORT_FILEFORMAT_WAV",
    "SUPPORT_FILEFORMAT_OGG": "RL_SUPPORT_FILEFORMAT_OGG",
    "SUPPORT_FILEFORMAT_XM": "RL_SUPPORT_FILEFORMAT_XM",
    "SUPPORT_FILEFORMAT_MOD": "RL_SUPPORT_FILEFORMAT_MOD",
    "SUPPORT_FILEFORMAT_MP3": "RL_SUPPORT_FILEFORMAT_MP3",
    "SUPPORT_FILEFORMAT_QOA": "RL_SUPPORT_FILEFORMAT_QOA",
    "SUPPORT_FILEFORMAT_FLAC": "RL_SUPPORT_FILEFORMAT_FLAC",
    "SUPPORT_STANDARD_FILEIO": "RL_SUPPORT_STANDARD_FILEIO",
    "SUPPORT_TRACELOG": "RL_SUPPORT_TRACELOG",
    # Config.h defines (Deduped with Cmake Options)
    "SUPPORT_MODULE_RSHAPES": "RL_SUPPORT_MODULE_RSHAPES",
    "SUPPORT_AUTOMATION_EVENTS": "RL_SUPPORT_AUTOMATION_EVENTS",
    "MAX_FILEPATH_CAPACITY": "RL_MAX_FILEPATH_CAPACITY",
    "MAX_FILEPATH_LENGTH": "RL_MAX_FILEPATH_LENGTH",
    "MAX_KEYBOARD_KEYS": "RL_MAX_KEYBOARD_KEYS",
    "MAX_MOUSE_BUTTONS": "RL_MAX_MOUSE_BUTTONS",
    "MAX_GAMEPADS": "RL_MAX_GAMEPADS",
    "MAX_GAMEPAD_AXIS": "RL_MAX_GAMEPAD_AXIS",
    "MAX_GAMEPAD_BUTTONS": "RL_MAX_GAMEPAD_BUTTONS",
    "MAX_GAMEPAD_VIBRATION_TIME": "RL_MAX_GAMEPAD_VIBRATION_TIME",
    "MAX_TOUCH_POINTS": "RL_MAX_TOUCH_POINTS",
    "MAX_KEY_PRESSED_QUEUE": "RL_MAX_KEY_PRESSED_QUEUE",
    "MAX_CHAR_PRESSED_QUEUE": "RL_MAX_CHAR_PRESSED_QUEUE",
    "MAX_DECOMPRESSION_SIZE": "RL_MAX_DECOMPRESSION_SIZE",
    "MAX_AUTOMATION_EVENTS": "RL_MAX_AUTOMATION_EVENTS",
    "SPLINE_SEGMENT_DIVISIONS": "RL_SPLINE_SEGMENT_DIVISIONS",
    "SUPPORT_FILEFORMAT_BDF": "RL_SUPPORT_FILEFORMAT_BDF",
    "MAX_TEXT_BUFFER_LENGTH": "RL_MAX_TEXT_BUFFER_LENGTH",
    "MAX_TEXTSPLIT_COUNT": "RL_MAX_TEXTSPLIT_COUNT",
    "MAX_MATERIAL_MAPS": "RL_MAX_MATERIAL_MAPS",
    "MAX_MESH_VERTEX_BUFFERS": "RL_MAX_MESH_VERTEX_BUFFERS",
    "AUDIO_DEVICE_FORMAT": "RL_AUDIO_DEVICE_FORMAT",
    "AUDIO_DEVICE_CHANNELS": "RL_AUDIO_DEVICE_CHANNELS",
    "AUDIO_DEVICE_SAMPLE_RATE": "RL_AUDIO_DEVICE_SAMPLE_RATE",
    "MAX_AUDIO_BUFFER_POOL_CHANNELS": "RL_MAX_AUDIO_BUFFER_POOL_CHANNELS",
    "SUPPORT_TRACELOG_DEBUG": "RL_SUPPORT_TRACELOG_DEBUG",
    "MAX_TRACELOG_MSG_LENGTH": "RL_MAX_TRACELOG_MSG_LENGTH",
}
ID_PREFIX_EXCLUSIONS = [
    "_GLFW",
    "_glfw",
    "_POSIX",
    "_WIN",
    "gl",
    "GL_",
    "glad_",
    "GLAD_",
    "glfw",
    "GLFW_",
    "rl",
    "RL_",
    "RLGL_",
    "STB_",
    "STBI_",
    "WGL_",
    "SDL_",
    "LOG_",
    "PIXELFORMAT_",
    "SHADER_LOC",
    "SHADER_ATTRIB_",
    "SHADER_UNIFORM_",
    "BLEND_",
    "TEXTURE_FILTER_",
    "MAX",
    "MIN",
]

RE_META_CHARS = (
    "\\",
    "|",
    "*",
    ".",
    "?",
    "^",
    "$",
    "(",
    ")",
    "[",
    "]",
    "{",
    "}",
)

RE_DEFINE = r"(?m)^(\s*)#define ([^\s\(]+)"
RE_DEFINE_REPL = r"\1#define \2"
RE_TYPEDEF_STRUCT = r"(?s)typedef struct ([^\s]+) (\{.+?\} )?(\1);"
RE_TYPEDEF_STRUCT_REPL = r"typedef struct \1 \2\1;"
RE_TYPEDEF_ENUM = r"(?s)typedef enum (\{.+?\}) ([^\s]+);"
RE_TYPEDEF_ENUM_REPL = r"typedef enum \1 \2;"
RE_TYPEDEF_ENUM_MEMBER = r"(?m)^(\s*)([A-Z0-9_][A-Za-z0-9_]+)(\s*(=.+?)?,?)"
RE_TYPEDEF_ENUM_MEMBER_REPL = r"\1\2\3"
RE_TYPEDEF_CALLBACK = r"typedef (.+?) (\**)\(\*([^\s\(]+)\)\("
RE_TYPEDEF_CALLBACK_REPL = r"typedef \1 \2(*\3)("
RE_API_FUNCTION = r"(R[LM]API) (.+?) (\**)([^\s\(]+)\("
RE_API_FUNCTION_REPL = r"\1 \2 \3\4("


def __main__():
    command = sys.argv[1] if len(sys.argv) > 1 else None
    if not command or command in ("--help", "help", "usage"):
        print(help_text)
    elif command == "raylib":
        main_raylib(sys.argv[2])
    elif command == "dep":
        main_dep(sys.argv[2], sys.argv[3:])


def main_raylib(raylib_dir_path):
    print("Adding prefixes to raylib declaration names in directory ", raylib_dir_path)
    prefixer = RaylibPrefixer()
    raylib_id_json_path = os.path.join(raylib_dir_path, "renamed_identifiers.json")
    raylib_src = os.path.join(raylib_dir_path, "src")
    prefixer.read_raylib_src_files(raylib_src)
    prefixer.rename_declarations()
    prefixer.rename_identifiers()
    prefixer.write_identifiers(raylib_id_json_path)
    prefixer.write_raylib_src_files(raylib_src)


def main_dep(raylib_dir_path, dep_glob_patterns):
    print("Adding prefixes to raylib references in paths", dep_glob_patterns)
    prefixer = RaylibPrefixer()
    raylib_id_json_path = os.path.join(raylib_dir_path, "renamed_identifiers.json")
    prefixer.read_identifiers(raylib_id_json_path)
    prefixer.rename_identifiers_in_src_files(dep_glob_patterns)


class RaylibPrefixer:
    def __init__(self):
        self.rl_src_files = {}
        self.identifier_map = {}
        for id_old, id_new in ID_EXCEPTIONS.items():
            if id_new:
                self.identifier_map[id_old] = id_new

    def add_identifier(self, identifier, prefix=ID_PREFIX_LOWER):
        if identifier in ID_EXCEPTIONS:
            return ID_EXCEPTIONS[identifier] or identifier
        if identifier.startswith(prefix) or any(
            identifier.startswith(pre) for pre in ID_PREFIX_EXCLUSIONS
        ):
            return identifier
        for meta_char in RE_META_CHARS:
            if meta_char in identifier:
                raise ValueError("Invalid identifier %s" % identifier)
        prefixed_identifier = prefix + identifier
        if identifier in self.identifier_map and (
            self.identifier_map[identifier] != prefixed_identifier
        ):
            raise ValueError(
                "Identifier conflict: %s and %s"
                % (
                    self.identifier_map[identifier],
                    prefixed_identifier,
                )
            )
        self.identifier_map[identifier] = prefixed_identifier
        return prefixed_identifier

    def read_identifiers(self, json_path):
        with open(json_path, "rt", encoding="utf-8") as json_file:
            self.identifier_map = json.load(json_file)
        if not isinstance(self.identifier_map, dict):
            raise ValueError("Expected %s to contain a JSON object." % json_path)

    def write_identifiers(self, json_path):
        with open(json_path, "wt", encoding="utf-8") as json_file:
            json.dump(self.identifier_map, json_file)

    def read_raylib_src_files(self, src_dir_path):
        src_dir_original_path = src_dir_path + "_original"
        if not os.path.exists(src_dir_original_path):
            print(
                "Copying raylib src files from %s to %s"
                % (src_dir_path, src_dir_original_path)
            )
            shutil.copytree(src_dir_path, src_dir_original_path)
        file_names = list(os.listdir(src_dir_original_path))
        file_names.append(os.path.join("external", "rl_gputex.h"))
        for file_name in file_names:
            if not (
                file_name == "build.zig"
                or file_name == "CMakeLists.txt"
                or file_name == "Makefile"
                or file_name.endswith(".h")
                or file_name.endswith(".c")
            ):
                continue
            file_path = os.path.join(src_dir_original_path, file_name)
            print("Reading original raylib src file:", file_path)
            with open(file_path, "rt", encoding="utf-8") as src_file:
                self.rl_src_files[file_name] = src_file.read()

    def write_raylib_src_files(self, src_dir_path):
        for file_name in self.rl_src_files.keys():
            file_path = os.path.join(src_dir_path, file_name)
            print("Writing modified raylib src file:", file_path)
            with open(file_path, "wt", encoding="utf-8") as src_file:
                src_file.write(self.rl_src_files[file_name])

    def rename_identifiers_in_src_files(self, src_glob_patterns):
        visited_files = set()
        for pattern in src_glob_patterns:
            for file_path in glob.glob(pattern):
                if file_path in visited_files:
                    continue
                visited_files.add(file_path)
                original_file_path = file_path + ".rloriginal"
                if not os.path.exists(original_file_path):
                    print(
                        "Copying dependent file from %s to %s",
                        (file_path, original_file_path),
                    )
                    shutil.copyfile(file_path, original_file_path)
                print("Reading original raylib dependent src file:", original_file_path)
                with open(original_file_path, "rt", encoding="utf-8") as read_file:
                    src = read_file.read()
                src = self.rename_identifiers_src(src)
                print("Writing modified raylib dependent src file:", file_path)
                with open(file_path, "wt", encoding="utf-8") as write_file:
                    write_file.write(src)

    def rename_declarations(self):
        for file_name in self.rl_src_files.keys():
            if not file_name.endswith(".h"):
                continue
            self.rl_src_files[file_name] = self.rename_decls_src(
                self.rl_src_files[file_name]
            )

    def rename_identifiers(self):
        for file_name in self.rl_src_files.keys():
            self.rl_src_files[file_name] = self.rename_identifiers_src(
                self.rl_src_files[file_name]
            )

    def make_repl_fn(
        self, repl_str, id_groups, group_map_fns=None, id_prefix=ID_PREFIX_LOWER
    ):
        def repl_fn(match):
            repl_str_result = repl_str
            for i in range(1, 1 + len(match.groups())):
                group_str = match.group(i) or ""
                if i in id_groups:
                    group_str = self.add_identifier(group_str, id_prefix)
                if group_map_fns and group_map_fns.get(i):
                    group_str = group_map_fns[i](group_str)
                repl_str_result = repl_str_result.replace(r"\%d" % i, group_str)
            return repl_str_result

        return repl_fn

    def enum_decl_repl_map_fn(self, enum_src):
        return re.sub(
            RE_TYPEDEF_ENUM_MEMBER,
            self.make_repl_fn(
                RE_TYPEDEF_ENUM_MEMBER_REPL, [2], id_prefix=ID_PREFIX_UPPER
            ),
            enum_src,
        )

    def rename_decls_src(self, src):
        src_renamed = src
        src_renamed = re.sub(
            RE_DEFINE,
            self.make_repl_fn(RE_DEFINE_REPL, [2], id_prefix=ID_PREFIX_UPPER),
            src_renamed,
        )
        src_renamed = re.sub(
            RE_TYPEDEF_STRUCT,
            self.make_repl_fn(RE_TYPEDEF_STRUCT_REPL, [1]),
            src_renamed,
        )
        src_renamed = re.sub(
            RE_TYPEDEF_ENUM,
            self.make_repl_fn(
                RE_TYPEDEF_ENUM_REPL, [2], {1: self.enum_decl_repl_map_fn}
            ),
            src_renamed,
        )
        src_renamed = re.sub(
            RE_TYPEDEF_CALLBACK,
            self.make_repl_fn(RE_TYPEDEF_CALLBACK_REPL, [3]),
            src_renamed,
        )
        src_renamed = re.sub(
            RE_API_FUNCTION,
            self.make_repl_fn(RE_API_FUNCTION_REPL, [4]),
            src_renamed,
        )
        return src_renamed

    def rename_identifiers_src(self, src):
        re_pattern = r"\b(%s)\b" % "|".join(self.identifier_map.keys())

        def id_repl(match):
            pos = match.start(0)
            if src[pos - 1] == "." or src[pos - 2 : pos] in ("->", "::"):
                return match.group(0)
            else:
                return self.identifier_map.get(match.group(0), "")

        return re.sub(
            re_pattern,
            id_repl,
            src,
        )


if __name__ == "__main__":
    __main__()
