#include <iostream>

#include <common/application.h>

Application::Application(const WindowDesc &windowDesc)
    : windowDesc_(windowDesc), keyboard_(nullptr), mouse_(nullptr)
{
    
}

void Application::run()
{
    try
    {
        window_ = std::make_unique<Window>(windowDesc_);

        keyboard_ = window_->getKeyboard();
        mouse_    = window_->getMouse();

        initialize();

        while(frame())
            ;

        destroy();
    }
    catch(const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
    }
}
