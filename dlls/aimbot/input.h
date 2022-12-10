#include "dllmain.h"
#include <queue>

#pragma once

namespace Input {

    struct FutureInput {
        INPUT input;
        DWORD tick;
        friend bool operator< (FutureInput a, FutureInput b) {
            return a.tick > b.tick;
        }
    };

    std::priority_queue<FutureInput> inputQueue;

    void click(DWORD downMilis, bool queue) {
        if (!queue && inputQueue.size() > 0)
            return;
        
        INPUT input = {};

        DWORD tick = GetTickCount();

        input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
        input.mi.time = 0;
        inputQueue.push({input, tick});
        
        input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
        input.mi.time = 0;
        inputQueue.push({input, tick + downMilis});
    }
    void click() { click(20, false); }

    void update() {
        
        DWORD tick = GetTickCount();

        while (inputQueue.size() > 0) {
            FutureInput fu = inputQueue.top();
            if (fu.tick > tick) 
                return;
            SendInput(1, &fu.input, sizeof(INPUT));
            inputQueue.pop();
        }

    }

}