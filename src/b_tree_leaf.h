#pragma once
#include <cstdint>
#include "tuple.h"

class BTreeLeafPage {
public:
    explicit BTreeLeafPage(uint8_t* data);

private:
    uint8_t* data_; // pointer to first byte (so whole page)
};

