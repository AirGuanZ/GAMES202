#pragma once

#include <common/common.h>

class Camera
{
public:

    struct UpdateParams
    {
        bool front = false;
        bool left  = false;
        bool right = false;
        bool back  = false;

        bool up   = false;
        bool down = false;

        float cursorRelX = 0;
        float cursorRelY = 0;
    };

    Camera() noexcept;

    void setPosition(const Float3 &position) noexcept;

    void setDirection(float horiRad, float vertRad) noexcept;

    void setSpeed(float speed) noexcept;

    void setViewRotationSpeed(float speed) noexcept;

    void update(const UpdateParams &params) noexcept;

    void setPerspective(float fovDeg, float nearZ, float farZ) noexcept;

    void setWOverH(float wOverH) noexcept;

    void recalculateMatrics() noexcept;

    float getNearZ() const noexcept;

    float getFarZ() const noexcept;

    const Float3 &getPosition() const noexcept;

    Float2 getDirection() const noexcept;

    const Mat4 &getView() const noexcept;

    const Mat4 &getProj() const noexcept;

    const Mat4 &getViewProj() const noexcept;

private:

    Float3 computeDirection() const;

    Float3 pos_;
    float vertRad_;
    float horiRad_;

    float fovDeg_;
    float nearZ_;
    float farZ_;

    float wOverH_;

    float speed_;
    float cursorSpeed_;

    Mat4 view_;
    Mat4 proj_;
    Mat4 viewProj_;
};
