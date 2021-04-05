#pragma once

#include <common/common.h>

class Application : agz::misc::uncopyable_t
{
    WindowDesc windowDesc_;

protected:

    std::unique_ptr<Window> window_;

    Keyboard *keyboard_;
    Mouse    *mouse_;

    virtual void initialize() = 0;

    virtual bool frame() = 0;

    virtual void destroy() = 0;

public:

    explicit Application(const WindowDesc &windowDesc);

    virtual ~Application() = default;

    void run();
};
