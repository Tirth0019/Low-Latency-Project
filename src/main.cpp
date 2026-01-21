#include <iostream>


#include "core/types.h"
#include "core/time.h"


#include "engine/engine.h"
int main() {
    std::cout << "Low Latency Project - C++" << std::endl;

   //Engine Configuration
   EngineConfig config();
   config.maxorders = 1000000;

   // 2.Construct Engine
   Engine engine(config);

   // 3.
   engine.start()

   cout << "Engine started" << endl;

   engine.stop();

   cout << "Engine stopped" << endl;
    return 0;
}
