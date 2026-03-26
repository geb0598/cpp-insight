#pragma once

// Core
#include "insight.h"

// GPU
#if defined(INSIGHTS_GPU)
#include "gpu/d3d11_gpu_profiler_backend.h"
#endif