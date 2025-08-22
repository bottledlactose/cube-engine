#pragma once

#include "macros/singleton.hpp"
#include <glm/glm.hpp>

class InputService {
MAKE_SINGLETON(InputService)
private:
    glm::vec2 mMousePosition;
    glm::vec2 mMouseDelta;

    bool mIsLeftMouseDown;
    bool mIsRightMouseDown;

    float mScrollY;

public:
    void SetMousePosition(float inX, float inY) {
        mMousePosition = glm::vec2(inX, inY);
    }

    void SetMouseDelta(float inX, float inY) {
        mMouseDelta = glm::vec2(inX, inY);
    }

    void SetLeftMouseDown(bool inValue) {
        mIsLeftMouseDown = inValue;
    }

    void SetRightMouseDown(bool inValue) {
        mIsRightMouseDown = inValue;
    }

    void SetScrollY(float inValue) {
        mScrollY = inValue;
    }

    void Update() {
        mMouseDelta = glm::vec2(0.0f);
        mScrollY = 0.0f;
    }

    inline glm::vec2 GetMousePosition() const {
        return mMousePosition;
    }

    inline glm::vec2 GetMouseDelta() const {
        return mMouseDelta;
    }

    inline bool IsLeftMouseDown() const {
        return mIsLeftMouseDown;
    }

    inline bool IsRightMouseDown() const {
        return mIsRightMouseDown;
    }

    inline float GetScrollY() const {
        return mScrollY;
    }
};
