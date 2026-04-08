#include <cstdio>

#include "editor/biome_maker_app.hpp"

int main() {
  std::printf("-------------------------------------------------------------\n");
  std::printf("VoxelParadox BiomeMaker\n");
  std::printf("-------------------------------------------------------------\n");

  BiomeMaker::BiomeMakerApp app;
  return app.run() ? 0 : -1;
}
