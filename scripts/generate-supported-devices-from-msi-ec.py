#!/usr/bin/env python3
"""Generate MsiController supported-devices.json from BeardOverflow/msi-ec."""

from __future__ import annotations

import argparse
import json
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Any


DEFAULT_REPO_URL = "https://github.com/BeardOverflow/msi-ec.git"
DEFAULT_REF = "main"
DEFAULT_OUTPUT = Path("src/service/supported-devices.json")

APP_DEFAULT_KEYS = (
    "BatteryChargeEc",
    "BatteryChargingStatusEc",
    "UsbPowerShareEc",
)

FALLBACK_APP_DEFAULTS = {
    "BatteryChargeEc": "0x42",
    "BatteryChargingStatusEc": "0x31",
    "UsbPowerShareEc": "0xbf",
}

MODE_LABELS = {
    "eco": "Eco",
    "comfort": "Comfort",
    "sport": "Sport",
    "turbo": "Turbo",
    "auto": "Auto",
    "silent": "Silent",
    "basic": "Basic",
    "advanced": "Advanced",
}


def strip_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    return re.sub(r"//.*", "", text)


def find_matching_brace(text: str, open_index: int) -> int:
    depth = 0
    for index in range(open_index, len(text)):
        char = text[index]
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                return index
    raise ValueError(f"Unmatched '{{' at offset {open_index}")


def decode_c_string(value: str) -> str:
    return bytes(value, "utf-8").decode("unicode_escape")


def normalize_label(name: str) -> str:
    mapped = MODE_LABELS.get(name.lower())
    if mapped:
        return mapped
    parts = re.split(r"[^A-Za-z0-9]+", name)
    return "".join(part.capitalize() for part in parts if part)


def format_hex(value: int) -> str:
    return f"0x{value:02x}"


class MsiEcParser:
    def __init__(self, source_dir: Path) -> None:
        self.source_dir = source_dir
        self.c_text = strip_comments((source_dir / "msi-ec.c").read_text(encoding="utf-8"))
        self.h_text = strip_comments((source_dir / "ec_memory_configuration.h").read_text(encoding="utf-8"))
        self.string_macros: dict[str, str] = {}
        self.numeric_macros: dict[str, int] = {}
        self.allowed_fw: dict[str, list[str]] = {}
        self.config_bodies: dict[str, str] = {}

    def parse(self) -> list[dict[str, Any]]:
        self.parse_macros()
        self.parse_allowed_fw()
        self.parse_configs()

        order = self.configuration_order()
        if not order:
            order = list(self.config_bodies)

        profiles: list[dict[str, Any]] = []
        for config_name in order:
            body = self.config_bodies.get(config_name)
            if not body:
                continue
            values = self.profile_values(body)
            if not values.get("AllowedFw"):
                continue
            profiles.append({"id": config_name, "values": values})

        return profiles

    def parse_macros(self) -> None:
        define_pattern = re.compile(r"^\s*#define\s+(\w+)\s+(.+?)\s*$", re.M)
        for text in (self.h_text, self.c_text):
            for match in define_pattern.finditer(text):
                name, raw_value = match.groups()
                value = raw_value.strip()
                string_match = re.match(r'"((?:\\.|[^"\\])*)"', value)
                if string_match:
                    self.string_macros[name] = decode_c_string(string_match.group(1))
                    continue

                parsed = self.parse_numeric(value)
                if parsed is not None:
                    self.numeric_macros[name] = parsed

    def parse_allowed_fw(self) -> None:
        pattern = re.compile(
            r"static\s+const\s+char\s+\*(\w+)\s*\[\]\s*[^=]*=\s*\{(?P<body>.*?)\};",
            re.S,
        )
        for match in pattern.finditer(self.c_text):
            name = match.group(1)
            body = match.group("body")
            firmwares = [
                decode_c_string(item)
                for item in re.findall(r'"((?:\\.|[^"\\])*)"', body)
            ]
            self.allowed_fw[name] = firmwares

    def parse_configs(self) -> None:
        pattern = re.compile(r"static\s+struct\s+msi_ec_conf\s+(\w+)\b[^=]*=\s*\{")
        for match in pattern.finditer(self.c_text):
            name = match.group(1)
            open_index = match.end() - 1
            close_index = find_matching_brace(self.c_text, open_index)
            self.config_bodies[name] = self.c_text[open_index + 1 : close_index]

    def configuration_order(self) -> list[str]:
        pattern = re.compile(r"static\s+struct\s+msi_ec_conf\s+\*CONFIGURATIONS\s*\[\]\s*[^=]*=\s*\{")
        match = pattern.search(self.c_text)
        if not match:
            return []

        open_index = match.end() - 1
        close_index = find_matching_brace(self.c_text, open_index)
        body = self.c_text[open_index + 1 : close_index]
        return re.findall(r"&\s*(\w+)", body)

    def profile_values(self, body: str) -> dict[str, Any]:
        values: dict[str, Any] = {}
        allowed_name = self.field_value(body, "allowed_fw")
        values["AllowedFw"] = self.allowed_fw.get(allowed_name or "", [])

        self.add_hex(values, "BatteryThresholdEc", self.field_value(body, "charge_control_address"))

        webcam = self.field_block(body, "webcam")
        webcam_address = self.numeric_value(self.field_value(webcam, "address"))
        if self.is_supported(webcam_address):
            values["WebCamEc"] = format_hex(webcam_address)
            self.add_hex(values, "WebCamBlockEc", self.field_value(webcam, "block_address"))
            self.add_bit_mask(values, "WebCamMask", self.field_value(webcam, "bit"))

        fn_win_swap = self.field_block(body, "fn_win_swap")
        fn_win_address = self.numeric_value(self.field_value(fn_win_swap, "address"))
        if self.is_supported(fn_win_address):
            values["FnSuperSwapEc"] = format_hex(fn_win_address)
            self.add_bit_mask(values, "FnSuperSwapMask", self.field_value(fn_win_swap, "bit"))
            invert = self.bool_value(self.field_value(fn_win_swap, "invert"))
            if invert is not None:
                values["FnWinSwapInvert"] = invert

        cooler_boost = self.field_block(body, "cooler_boost")
        cooler_address = self.numeric_value(self.field_value(cooler_boost, "address"))
        if self.is_supported(cooler_address):
            values["CoolerBoostEc"] = format_hex(cooler_address)
            self.add_bit_mask(values, "CoolerBoostMask", self.field_value(cooler_boost, "bit"))

        shift_mode = self.field_block(body, "shift_mode")
        shift_address = self.numeric_value(self.field_value(shift_mode, "address"))
        shift_modes = self.mode_values(shift_mode)
        if self.is_supported(shift_address) and shift_modes:
            values["ShiftModeEc"] = format_hex(shift_address)
            values["ShiftModeAvailable"] = shift_modes

        super_battery = self.field_block(body, "super_battery")
        super_battery_address = self.numeric_value(self.field_value(super_battery, "address"))
        if self.is_supported(super_battery_address):
            values["SuperBatteryEc"] = format_hex(super_battery_address)
            self.add_hex(values, "SuperBatteryMask", self.field_value(super_battery, "mask"))

        fan_mode = self.field_block(body, "fan_mode")
        fan_address = self.numeric_value(self.field_value(fan_mode, "address"))
        fan_modes = self.mode_values(fan_mode)
        if self.is_supported(fan_address) and fan_modes:
            values["FanModeEc"] = format_hex(fan_address)
            values["FanModeAvailable"] = fan_modes

        cpu = self.field_block(body, "cpu")
        self.add_hex(values, "CpuTempEc", self.field_value(cpu, "rt_temp_address"))
        self.add_hex(values, "FanCpuEc", self.field_value(cpu, "rt_fan_speed_address"))

        gpu = self.field_block(body, "gpu")
        self.add_hex(values, "GpuTempEc", self.field_value(gpu, "rt_temp_address"))
        self.add_hex(values, "FanGpuEc", self.field_value(gpu, "rt_fan_speed_address"))

        leds = self.field_block(body, "leds")
        mic_mute_address = self.numeric_value(self.field_value(leds, "micmute_led_address"))
        mute_address = self.numeric_value(self.field_value(leds, "mute_led_address"))
        if self.is_supported(mic_mute_address):
            values["MicMuteEc"] = format_hex(mic_mute_address)
        if self.is_supported(mute_address):
            values["MuteLedEc"] = format_hex(mute_address)
        if self.is_supported(mic_mute_address) or self.is_supported(mute_address):
            self.add_bit_mask(values, "LedsMask", self.field_value(leds, "bit"))

        kbd_bl = self.field_block(body, "kbd_bl")
        kbd_mode_address = self.numeric_value(self.field_value(kbd_bl, "bl_mode_address"))
        if self.is_supported(kbd_mode_address):
            values["KeyboardBacklightModeEc"] = format_hex(kbd_mode_address)
            mode_values = self.numeric_array(kbd_bl, "bl_modes")
            if len(mode_values) >= 2:
                values["KeyboardBacklightMode"] = [format_hex(mode_values[0]), format_hex(mode_values[1])]

        kbd_state_address = self.numeric_value(self.field_value(kbd_bl, "bl_state_address"))
        if self.is_supported(kbd_state_address):
            values["KeyboardBacklightEc"] = format_hex(kbd_state_address)
            self.add_hex(values, "KeyboardBacklightStartState", self.field_value(kbd_bl, "state_base_value"))

        return values

    def field_block(self, text: str, field: str) -> str:
        match = re.search(rf"\.\s*{re.escape(field)}\s*=\s*\{{", text)
        if not match:
            return ""
        open_index = match.end() - 1
        close_index = find_matching_brace(text, open_index)
        return text[open_index + 1 : close_index]

    def field_value(self, text: str, field: str) -> str | None:
        if not text:
            return None
        match = re.search(rf"\.\s*{re.escape(field)}\s*=\s*([^,\n}}]+)", text)
        return match.group(1).strip() if match else None

    def mode_values(self, text: str) -> list[str]:
        modes_block = self.field_block(text, "modes")
        if not modes_block:
            return []

        modes: list[str] = []
        mode_pattern = re.compile(r"\{\s*([^,{}]+|\"(?:\\.|[^\"\\])*\")\s*,\s*([^,{}]+)\s*\}")
        for match in mode_pattern.finditer(modes_block):
            raw_name, raw_value = match.groups()
            name = self.string_value(raw_name.strip())
            value = self.numeric_value(raw_value.strip())
            if not name or not self.is_supported(value):
                continue
            modes.append(f"{normalize_label(name)}:{format_hex(value)}")
        return modes

    def numeric_array(self, text: str, field: str) -> list[int]:
        body = self.field_block(text, field)
        if not body:
            return []
        values: list[int] = []
        for item in body.split(","):
            value = self.numeric_value(item.strip())
            if self.is_supported(value):
                values.append(value)
        return values

    def add_hex(self, values: dict[str, Any], key: str, expression: str | None) -> None:
        value = self.numeric_value(expression)
        if self.is_supported(value):
            values[key] = format_hex(value)

    def add_bit_mask(self, values: dict[str, Any], key: str, expression: str | None) -> None:
        bit = self.numeric_value(expression)
        if bit is not None and 0 <= bit < 16:
            values[key] = format_hex(1 << bit)

    def string_value(self, expression: str | None) -> str | None:
        if not expression:
            return None
        expression = expression.strip()
        string_match = re.match(r'"((?:\\.|[^"\\])*)"', expression)
        if string_match:
            return decode_c_string(string_match.group(1))
        return self.string_macros.get(expression)

    def bool_value(self, expression: str | None) -> bool | None:
        if expression is None:
            return None
        value = expression.strip().lower()
        if value == "true":
            return True
        if value == "false":
            return False
        numeric = self.numeric_value(expression)
        if numeric is None:
            return None
        return bool(numeric)

    def numeric_value(self, expression: str | None) -> int | None:
        if not expression:
            return None
        return self.parse_numeric(expression.strip())

    def parse_numeric(self, expression: str) -> int | None:
        expression = expression.strip()
        if not expression:
            return None
        if expression in self.numeric_macros:
            return self.numeric_macros[expression]
        if re.fullmatch(r"0[xX][0-9A-Fa-f]+|\d+", expression):
            return int(expression, 0)

        def macro_replacement(match: re.Match[str]) -> str:
            name = match.group(0)
            if name in self.numeric_macros:
                return str(self.numeric_macros[name])
            return name

        safe_expression = re.sub(r"\b[A-Za-z_]\w*\b", macro_replacement, expression)
        safe_expression = re.sub(r"\bBIT\s*\(\s*(\d+)\s*\)", r"(1 << \1)", safe_expression)
        if not re.fullmatch(r"[0-9A-Fa-fxX|&<>()~+\-\s]+", safe_expression):
            return None
        try:
            return int(eval(safe_expression, {"__builtins__": {}}, {}))
        except Exception:
            return None

    def is_supported(self, value: int | None) -> bool:
        if value is None:
            return False
        unsupported = {
            self.numeric_macros.get("MSI_EC_ADDR_UNKNOWN"),
            self.numeric_macros.get("MSI_EC_ADDR_UNSUPP"),
        }
        return value not in unsupported


def clone_source(repo_url: str, ref: str) -> tempfile.TemporaryDirectory[str]:
    temp_dir = tempfile.TemporaryDirectory(prefix="msicontroller-msi-ec-")
    command = ["git", "clone", "--depth", "1", "--branch", ref, repo_url, temp_dir.name]
    subprocess.run(command, check=True)
    return temp_dir


def existing_app_defaults(output: Path) -> dict[str, Any]:
    if not output.exists():
        return dict(FALLBACK_APP_DEFAULTS)
    try:
        data = json.loads(output.read_text(encoding="utf-8"))
    except json.JSONDecodeError:
        return dict(FALLBACK_APP_DEFAULTS)

    current_defaults = data.get("defaults", {})
    result = {
        key: current_defaults.get(key, FALLBACK_APP_DEFAULTS[key])
        for key in APP_DEFAULT_KEYS
        if key in current_defaults or key in FALLBACK_APP_DEFAULTS
    }
    return result


def build_document(source_dir: Path, output: Path, include_app_defaults: bool) -> dict[str, Any]:
    parser = MsiEcParser(source_dir)
    profiles = parser.parse()
    defaults = existing_app_defaults(output) if include_app_defaults else {}
    return {
        "schemaVersion": 1,
        "defaults": defaults,
        "profiles": profiles,
    }


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate src/service/supported-devices.json from BeardOverflow/msi-ec C configurations.",
    )
    parser.add_argument("--source-dir", type=Path, help="Use an already cloned msi-ec repository.")
    parser.add_argument("--repo-url", default=DEFAULT_REPO_URL, help=f"Repository URL, default: {DEFAULT_REPO_URL}")
    parser.add_argument("--ref", default=DEFAULT_REF, help=f"Git branch/tag to clone, default: {DEFAULT_REF}")
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT, help=f"Output file, default: {DEFAULT_OUTPUT}")
    parser.add_argument("--dry-run", action="store_true", help="Print generated JSON to stdout instead of writing it.")
    parser.add_argument(
        "--no-app-defaults",
        action="store_true",
        help="Do not keep MsiController-only defaults that are not present in msi-ec.",
    )
    return parser.parse_args(argv)


def main(argv: list[str]) -> int:
    args = parse_args(argv)

    temp_dir: tempfile.TemporaryDirectory[str] | None = None
    source_dir = args.source_dir
    if source_dir is None:
        if shutil.which("git") is None:
            print("git is required when --source-dir is not provided", file=sys.stderr)
            return 1
        temp_dir = clone_source(args.repo_url, args.ref)
        source_dir = Path(temp_dir.name)

    required_files = (source_dir / "msi-ec.c", source_dir / "ec_memory_configuration.h")
    missing = [str(path) for path in required_files if not path.exists()]
    if missing:
        print(f"Missing required msi-ec file(s): {', '.join(missing)}", file=sys.stderr)
        return 1

    document = build_document(source_dir, args.output, not args.no_app_defaults)
    content = json.dumps(document, indent=2, ensure_ascii=False) + "\n"

    if args.dry_run:
        sys.stdout.write(content)
    else:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(content, encoding="utf-8")

    print(f"Generated {len(document['profiles'])} profiles from {source_dir}", file=sys.stderr)
    if temp_dir is not None:
        temp_dir.cleanup()
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
