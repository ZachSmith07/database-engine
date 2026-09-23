#include <iostream>
#include <chrono>

#include "page_manager.h"
#include "insertion_test.h"
#include "wal.h"

int main()
{
    try
    {
        PageManager pm;
        WalManager wm(L"wal/");
        test_insert(pm, wm);
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }
    return 0;
}