#include <iostream>
#include <thread>

#include "insight/platform_time.h"

int main() {
    using namespace insight;

    auto start = PlatformTime::Now();
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
    auto end = PlatformTime::Now();

    auto elapsed = PlatformTime::Elapsed(start, end);

    std::cout << "Elapsed (ms):  " << PlatformTime::ToMilli(elapsed)  << "\n";
    std::cout << "Elapsed (us):  " << PlatformTime::ToMicro(elapsed)  << "\n";
    std::cout << "Elapsed (sec): " << PlatformTime::ToSeconds(elapsed) << "\n";

    return 0;
}