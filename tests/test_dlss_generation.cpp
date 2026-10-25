// SPDX-License-Identifier: GPL-2.0-or-later
#include "upscale/dlss_generation.h"

#include <cassert>
#include <cstdio>

int main()
{
    // 310.9.16 (the packages'): K for DLAA, Quality and Balanced, M and L for Performance and
    // Ultra Performance.
    assert(DlssGeneration::Of("310.9.16", 0) == "4");
    assert(DlssGeneration::Of("310.9.16", 1) == "4");
    assert(DlssGeneration::Of("310.9.16", 2) == "4");
    assert(DlssGeneration::Of("310.9.16", 3) == "4.5");
    assert(DlssGeneration::Of("310.9.16", 4) == "4.5");
    assert(DlssGeneration::Of("310.5.0", 3) == "4.5");
    // Compared as numbers: 310.10 comes after 310.9.
    assert(DlssGeneration::Of("310.10.0", 4) == "4.5");
    // 310.3 and 310.4: K up to Performance, the CNN preset F for Ultra Performance.
    assert(DlssGeneration::Of("310.4.0", 3) == "4");
    assert(DlssGeneration::Of("310.4.0", 4) == "3");
    // 310.1 and 310.2: the transformer was not the default yet.
    assert(DlssGeneration::Of("310.2.1", 1) == "3");
    assert(DlssGeneration::Of("3.7.20", 1) == "3");
    assert(DlssGeneration::Of("2.4.0", 3) == "2");
    // Not known here: no name rather than a guess.
    assert(DlssGeneration::Of("320.1.0", 1).empty());
    assert(DlssGeneration::Of("", 1).empty());
    assert(DlssGeneration::Of("rel", 1).empty());
    std::puts("dlss-generation: library versions and quality modes named as NVIDIA's defaults run them");
}
