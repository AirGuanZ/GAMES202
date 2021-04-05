#include <common/camera.h>

Camera::Camera() noexcept
    : vertRad_(0), horiRad_(0),
      fovDeg_(60), nearZ_(0.1f), farZ_(100.0f), wOverH_(1),
      speed_(0.02f), cursorSpeed_(0.003f)
{
    
}

void Camera::setPosition(const Float3 &position) noexcept
{
    pos_ = position;
}

void Camera::setDirection(float horiRad, float vertRad) noexcept
{
    horiRad_ = horiRad;
    vertRad_ = vertRad;
}

void Camera::setSpeed(float speed) noexcept
{
    speed_ = speed;
}

void Camera::setViewRotationSpeed(float speed) noexcept
{
    cursorSpeed_ = speed;
}

void Camera::update(const UpdateParams &params) noexcept
{
    // direction

    vertRad_ -= cursorSpeed_ * params.cursorRelY;
    horiRad_ -= cursorSpeed_ * params.cursorRelX;

    const float PI = agz::math::PI_f;
    while(horiRad_ < 0)       horiRad_ += 2 * PI;
    while(horiRad_ >= 2 * PI) horiRad_ -= 2 * PI;
    vertRad_ = agz::math::clamp(vertRad_, -PI / 2 + 0.01f, PI / 2 - 0.01f);

    // position

    const Float3 dir   = computeDirection();
    const Float3 front = Float3(dir.x, 0, dir.z).normalize();
    const Float3 left  = -cross({ 0, 1, 0 }, front).normalize();

    const int frontStep = params.front - params.back;
    const int leftStep  = params.left - params.right;

    pos_ += speed_ * (
        static_cast<float>(frontStep) * front +
        static_cast<float>(leftStep)  * left);

    if(params.up)
        pos_.y += speed_;
    if(params.down)
        pos_.y -= speed_;
}

void Camera::setPerspective(float fovDeg, float nearZ, float farZ) noexcept
{
    fovDeg_ = fovDeg;
    nearZ_  = nearZ;
    farZ_   = farZ;
}

void Camera::setWOverH(float wOverH) noexcept
{
    wOverH_ = wOverH;
}

void Camera::recalculateMatrics() noexcept
{
    const Float3 dir = computeDirection();
    view_ = Trans4::look_at(pos_, pos_ + dir, { 0, 1, 0 });

    proj_ = Trans4::perspective(
        agz::math::deg2rad(fovDeg_),
        wOverH_, nearZ_, farZ_);

    viewProj_ = view_ * proj_;
}

float Camera::getNearZ() const noexcept
{
    return nearZ_;
}

float Camera::getFarZ() const noexcept
{
    return farZ_;
}

const Float3 &Camera::getPosition() const noexcept
{
    return pos_;
}

Float2 Camera::getDirection() const noexcept
{
    return { horiRad_, vertRad_ };
}

const Mat4 &Camera::getView() const noexcept
{
    return view_;
}

const Mat4 &Camera::getProj() const noexcept
{
    return proj_;
}

const Mat4 &Camera::getViewProj() const noexcept
{
    return viewProj_;
}

Float3 Camera::computeDirection() const
{
    return {
        std::cos(horiRad_) * std::cos(vertRad_),
        std::sin(vertRad_),
        std::sin(horiRad_) * std::cos(vertRad_)
    };
}
