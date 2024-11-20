#pragma once

#include "macros/singleton.hpp"

class InputService {
MAKE_SINGLETON(InputService)
private:
    float mMouseX;
    float mMouseY;

    float mMouseRelX;
    float mMouseRelY;

    bool mIsLeftMouseDown;
    bool mIsRightMouseDown;

public:
    

    void Update() {
        mMouseRelX = 0.0f;
        mMouseRelY = 0.0f;
    }

    void SetMousePosition(float inX, float inY) {
        mMouseX = inX;
        mMouseY = inY;
    }

    void SetMouseRelativePosition(float inX, float inY) {
        mMouseRelX = inX;
        mMouseRelY = inY;
    }

    void SetLeftMouseDown(bool inValue) {
        mIsLeftMouseDown = inValue;
    }

    void SetRightMouseDown(bool inValue) {
        mIsRightMouseDown = inValue;
    }

    inline bool IsLeftMouseDown() const {
        return mIsLeftMouseDown;
    }

    inline bool IsRightMouseDown() const {
        return mIsRightMouseDown;
    }

    inline float GetMouseX() const {
        return mMouseX;
    }

    inline float GetMouseY() const {
        return mMouseY;
    }

    inline float GetMouseRelX() const {
        return mMouseRelX;
    }

    inline float GetMouseRelY() const {
        return mMouseRelY;
    }
};
