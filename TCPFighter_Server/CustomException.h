#pragma once

#include <iostream>
#include "StackWalker.h"

class StackTraceLogger : public StackWalker
{
protected:
    virtual void OnOutput(LPCSTR szText) {
        std::cout << szText;
    }
};