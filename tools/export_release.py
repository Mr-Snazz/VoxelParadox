from __future__ import annotations

import argparse
import shutil
import sys
from pathlib import Path


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Export a staged VoxelParadox release layout."
    )
    parser.add_argument("--configuration", default="Release")
    parser.add_argument("--platform", default="x64")
    parser.add_argument("--output", type=Path)
    return parser.parse_args()


def copy_runtime_artifacts(source_dir: Path, destination_dir: Path, exe_name: str) -> None:
    exe_path = source_dir / exe_name
    if not exe_path.is_file():
        raise FileNotFoundError(
            f"Missing build output: {exe_path}. Build the {source_dir.name} project first."
        )

    shutil.copy2(exe_path, destination_dir / exe_name)

    for dll_path in sorted(source_dir.glob("*.dll")):
        shutil.copy2(dll_path, destination_dir / dll_path.name)


def main() -> int:
    args = parse_args()
    root = Path(__file__).resolve().parents[1]
    output_dir = (
        args.output.resolve()
        if args.output is not None
        else root / "artifacts" / "export" / args.platform / args.configuration
    )

    if output_dir.exists():
        shutil.rmtree(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    bin_root = root / "artifacts" / "bin" / args.platform / args.configuration
    outputs = (
        ("VoxelParadox.Client", "VoxelParadox.exe"),
        ("VoxelParadox.BiomeMaker", "VoxelParadoxBiomeMaker.exe"),
        ("VoxelParadox.ShaderEditor", "VoxelParadoxShaderEditor.exe"),
    )

    for project_dir_name, exe_name in outputs:
        copy_runtime_artifacts(bin_root / project_dir_name, output_dir, exe_name)

    shutil.copytree(root / "res", output_dir / "Resources")
    print(output_dir)
    return 0


if __name__ == "__main__":
    sys.exit(main())
