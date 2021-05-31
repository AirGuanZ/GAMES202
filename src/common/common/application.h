#pragma once

#include <common/common.h>

class Application : agz::misc::uncopyable_t
{
    WindowDesc windowDesc_;

protected:

    std::unique_ptr<Window> window_;

    Keyboard *keyboard_;
    Mouse    *mouse_;

    virtual void initialize() { }

    virtual bool frame() { return false; }

    virtual void destroy() { }

public:

    explicit Application(const WindowDesc &windowDesc);

    virtual ~Application() = default;

    void run();
};
