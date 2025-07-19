#include "common.h"

void convertTimeToString(int minutes, int seconds, char* result, size_t bufferSize)
{
    snprintf(result, bufferSize, "%02d:%02d", minutes, seconds);
}