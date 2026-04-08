from __future__ import annotations

import copy
import uuid
import xml.etree.ElementTree as ET
from dataclasses import dataclass
from pathlib import Path


MSBUILD_NS = "http://schemas.microsoft.com/developer/msbuild/2003"
ET.register_namespace("", MSBUILD_NS)


def qname(tag: str) -> str:
    return f"{{{MSBUILD_NS}}}{tag}"


def prettify(tree: ET.ElementTree) -> None:
    ET.indent(tree, space="  ")


def write_xml(tree: ET.ElementTree, path: Path) -> None:
    prettify(tree)
    tree.write(path, encoding="utf-8", xml_declaration=True)


def title_case(part: str) -> str:
    aliases = {
        "ui": "UI",
        "hud": "Hud",
    }
    lowered = part.lower()
    if lowered in aliases:
        return aliases[lowered]
    return " ".join(token.capitalize() for token in lowered.split("_"))


@dataclass(frozen=True)
class SourceRoot:
    absolute_root: Path
    project_relative_root: Path
    root_file_filter: str
    base_filter: str
    top_level_map: dict[str, str]


@dataclass(frozen=True)
class ProjectConfig:
    project_file: Path
    filters_file: Path
    include_roots: tuple[SourceRoot, ...]


ROOT = Path(__file__).resolve().parents[1]

PROJECTS = (
    ProjectConfig(
        project_file=ROOT / "VoxelParadox.Engine" / "VoxelParadox.Engine.vcxproj",
        filters_file=ROOT / "VoxelParadox.Engine" / "VoxelParadox.Engine.vcxproj.filters",
        include_roots=(
            SourceRoot(
                absolute_root=ROOT / "VoxelParadox.Engine" / "src",
                project_relative_root=Path("src"),
                root_file_filter="Engine",
                base_filter="",
                top_level_map={
                    "engine": "Engine",
                    "path": "Path",
                    "ui": "UI",
                },
            ),
            SourceRoot(
                absolute_root=ROOT / "VoxelParadox.Client" / "src" / "world",
                project_relative_root=Path("..") / "VoxelParadox.Client" / "src" / "world",
                root_file_filter=r"Client Shared\World",
                base_filter=r"Client Shared\World",
                top_level_map={},
            ),
        ),
    ),
    ProjectConfig(
        project_file=ROOT / "VoxelParadox.Client" / "VoxelParadox.Client.vcxproj",
        filters_file=ROOT / "VoxelParadox.Client" / "VoxelParadox.Client.vcxproj.filters",
        include_roots=(
            SourceRoot(
                absolute_root=ROOT / "VoxelParadox.Client" / "src",
                project_relative_root=Path("src"),
                root_file_filter="Support",
                base_filter="",
                top_level_map={
                    "debug": "Debug",
                    "items": "Items",
                    "player": "Player",
                    "render": "Render",
                    "runtime": "Runtime",
                    "ui": "UI",
                    "world": "World",
                },
            ),
        ),
    ),
    ProjectConfig(
        project_file=ROOT / "VoxelParadox.BiomeMaker" / "VoxelParadox.BiomeMaker.vcxproj",
        filters_file=ROOT / "VoxelParadox.BiomeMaker" / "VoxelParadox.BiomeMaker.vcxproj.filters",
        include_roots=(
            SourceRoot(
                absolute_root=ROOT / "VoxelParadox.BiomeMaker" / "src",
                project_relative_root=Path("src"),
                root_file_filter="Support",
                base_filter="",
                top_level_map={
                    "app": "App",
                    "editor": "Editor",
                },
            ),
        ),
    ),
    ProjectConfig(
        project_file=ROOT / "VoxelParadox.ShaderEditor" / "VoxelParadox.ShaderEditor.vcxproj",
        filters_file=ROOT / "VoxelParadox.ShaderEditor" / "VoxelParadox.ShaderEditor.vcxproj.filters",
        include_roots=(
            SourceRoot(
                absolute_root=ROOT / "VoxelParadox.ShaderEditor" / "src",
                project_relative_root=Path("src"),
                root_file_filter="Support",
                base_filter="",
                top_level_map={
                    "app": "App",
                    "editor": "Editor",
                },
            ),
        ),
    ),
)


def project_path_from_root(project_dir: Path, root: SourceRoot, file_path: Path) -> str:
    relative_inside_root = file_path.relative_to(root.absolute_root)
    return str(root.project_relative_root / relative_inside_root).replace("/", "\\")


def filter_path_for(root: SourceRoot, file_path: Path) -> str:
    relative_inside_root = file_path.relative_to(root.absolute_root)
    directories = relative_inside_root.parent.parts

    if not directories:
        return root.root_file_filter

    filter_parts: list[str] = []
    if root.base_filter:
        filter_parts.extend(root.base_filter.split("\\"))

    top_level = directories[0]
    mapped = root.top_level_map.get(top_level.lower(), title_case(top_level))
    filter_parts.extend(mapped.split("\\"))
    filter_parts.extend(title_case(part) for part in directories[1:])
    return "\\".join(filter_parts)


def find_root(config: ProjectConfig, file_path: Path) -> SourceRoot:
    candidates = [root for root in config.include_roots if file_path.is_relative_to(root.absolute_root)]
    if not candidates:
        raise ValueError(f"Could not map filter for {file_path}")
    return max(candidates, key=lambda root: len(root.absolute_root.parts))


def collect_include_items(config: ProjectConfig) -> list[str]:
    project_dir = config.project_file.parent
    includes: set[str] = set()
    for root in config.include_roots:
        for file_path in sorted(root.absolute_root.rglob("*.hpp")):
            includes.add(project_path_from_root(project_dir, root, file_path))
    return sorted(includes, key=str.lower)


def clone_compile_items(project_root: ET.Element) -> list[ET.Element]:
    compile_items: list[ET.Element] = []
    for item_group in project_root.findall(qname("ItemGroup")):
        for compile_item in item_group.findall(qname("ClCompile")):
            compile_items.append(copy.deepcopy(compile_item))
    return compile_items


def remove_clinclude_groups(project_root: ET.Element) -> None:
    for item_group in list(project_root.findall(qname("ItemGroup"))):
        clincludes = item_group.findall(qname("ClInclude"))
        for clinclude in clincludes:
            item_group.remove(clinclude)
        if len(item_group) == 0:
            project_root.remove(item_group)


def insert_include_group(project_root: ET.Element, include_items: list[str]) -> None:
    include_group = ET.Element(qname("ItemGroup"))
    for include_path in include_items:
        ET.SubElement(include_group, qname("ClInclude"), Include=include_path)

    insert_index = len(project_root)
    for index, child in enumerate(list(project_root)):
        if child.tag == qname("ItemGroup") and child.find(qname("ProjectReference")) is not None:
            insert_index = index
            break
        if child.tag == qname("Import") and child.attrib.get("Project", "").endswith("Microsoft.Cpp.targets"):
            insert_index = index
            break
    project_root.insert(insert_index, include_group)


def update_vcxproj(config: ProjectConfig) -> list[ET.Element]:
    tree = ET.parse(config.project_file)
    project_root = tree.getroot()
    compile_items = clone_compile_items(project_root)
    include_items = collect_include_items(config)
    remove_clinclude_groups(project_root)
    insert_include_group(project_root, include_items)
    write_xml(tree, config.project_file)
    return compile_items


def filter_guid(project_file: Path, filter_name: str) -> str:
    value = uuid.uuid5(uuid.NAMESPACE_URL, f"{project_file.as_posix()}::{filter_name}")
    return "{" + str(value).upper() + "}"


def build_filter_tree(config: ProjectConfig, compile_items: list[ET.Element], include_items: list[str]) -> ET.ElementTree:
    project_root = ET.Element(qname("Project"), ToolsVersion="18.0")

    entries: list[tuple[str, str, str]] = []

    project_dir = config.project_file.parent
    for compile_item in compile_items:
        include_path = compile_item.attrib["Include"]
        absolute_path = (project_dir / include_path).resolve()
        source_root = find_root(config, absolute_path)
        entries.append(("ClCompile", include_path, filter_path_for(source_root, absolute_path)))

    for include_path in include_items:
        absolute_path = (project_dir / include_path).resolve()
        source_root = find_root(config, absolute_path)
        entries.append(("ClInclude", include_path, filter_path_for(source_root, absolute_path)))

    filters: set[str] = set()
    for _, _, filter_name in entries:
        if not filter_name:
            continue
        parts = filter_name.split("\\")
        for index in range(1, len(parts) + 1):
            filters.add("\\".join(parts[:index]))

    filter_group = ET.SubElement(project_root, qname("ItemGroup"))
    for filter_name in sorted(filters, key=lambda value: (value.count("\\"), value.lower())):
        filter_element = ET.SubElement(filter_group, qname("Filter"), Include=filter_name)
        guid_element = ET.SubElement(filter_element, qname("UniqueIdentifier"))
        guid_element.text = filter_guid(config.project_file, filter_name)

    compile_group = ET.SubElement(project_root, qname("ItemGroup"))
    for _, include_path, filter_name in [entry for entry in entries if entry[0] == "ClCompile"]:
        compile_element = ET.SubElement(compile_group, qname("ClCompile"), Include=include_path)
        filter_element = ET.SubElement(compile_element, qname("Filter"))
        filter_element.text = filter_name

    include_group = ET.SubElement(project_root, qname("ItemGroup"))
    for _, include_path, filter_name in [entry for entry in entries if entry[0] == "ClInclude"]:
        include_element = ET.SubElement(include_group, qname("ClInclude"), Include=include_path)
        filter_element = ET.SubElement(include_element, qname("Filter"))
        filter_element.text = filter_name

    return ET.ElementTree(project_root)


def sync_project(config: ProjectConfig) -> None:
    compile_items = update_vcxproj(config)
    include_items = collect_include_items(config)
    filter_tree = build_filter_tree(config, compile_items, include_items)
    write_xml(filter_tree, config.filters_file)


def main() -> None:
    for config in PROJECTS:
        sync_project(config)
        print(f"Synced {config.project_file.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
