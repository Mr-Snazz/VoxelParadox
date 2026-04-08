from pathlib import Path

path = Path.home() / "AppData" / "Local" / "VoxelParadoxData" / "Saves" / "worlds"

def delete_world_dat_files():
    if not path.exists():
        print(f"Caminho não existe: {path}")
        return

    files_to_delete = [file_path for file_path in path.glob("*.dat") if file_path.is_file()]
    if not files_to_delete:
        print("Nenhum arquivo .dat encontrado para deletar.")
        return

    print("Arquivos que serão apagados:")
    for file_path in files_to_delete:
        print(f"- {file_path}")

    removed = 0
    for file_path in files_to_delete:
        try:
            file_path.unlink()
            removed += 1
            print(f"Apagado com sucesso: {file_path}")
        except Exception as e:
            print(f"Erro ao deletar {file_path}: {e}")

    print(f"Arquivos .dat removidos: {removed}")


if __name__ == "__main__":
    delete_world_dat_files()
    input("Pressione Enter para fechar o terminal...")