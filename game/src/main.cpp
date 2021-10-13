#include "pxr_engine.h"
#include "../include/itzcoatl.h"

pxr::Engine engine;

int main()
{
  engine.initialize(std::unique_ptr<Snake>(new Snake{}));
  engine.run();
  engine.shutdown();
}
