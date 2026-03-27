#pragma once

// Core
#include "insights.h"

// GPU
#if defined(INSIGHTS_GPU)
#include "gpu/d3d11_gpu_profiler_backend.h"
#endif