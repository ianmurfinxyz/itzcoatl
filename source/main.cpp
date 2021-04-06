#include "pxr_engine.h"
#include "snake.h"

pxr::Engine engine;

int main()
{
  engine.initialize(std::unique_ptr<Snake>(new Snake{}));
  engine.run();
  engine.shutdown();
}
