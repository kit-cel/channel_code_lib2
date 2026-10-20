#pragma once

#include <channel_coding_lib/Utility.h>

struct Group
{
    std::vector<size_t> members;

    size_t group_index;

    size_t group_latency;
};