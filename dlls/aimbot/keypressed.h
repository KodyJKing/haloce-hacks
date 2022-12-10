#include "dllmain.h"

#pragma once

bool wasPressed[0xFF] = {};
int keypressed(char vk) {
    int isPressed = GetAsyncKeyState(vk) != 0;
    int result = !wasPressed[vk] && isPressed;
    wasPressed[vk] = isPressed;
    return result;
}