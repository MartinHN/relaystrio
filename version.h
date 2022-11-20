#pragma once
#include <Arduino.h>
#if !HAS_VERSION_HEADER
static String GIT_HASH = "Unknown";
#else
#include "gitVersion.h"
#endif
