#include "main_functions.h"

int main(int argc, char *argv[]) {
  Setup();
  while (true) {
    Inference();
  }
}
