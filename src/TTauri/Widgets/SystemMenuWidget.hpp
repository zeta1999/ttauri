// Copyright 2019 Pokitec
// All rights reserved.

#pragma once

#include "TTauri/Widgets/Widget.hpp"
#include "TTauri/GUI/PipelineImage_Image.hpp"
#include "TTauri/Foundation/Path.hpp"
#include "TTauri/GUI/Image.hpp"
#include <memory>
#include <string>
#include <array>


namespace tt {

class SystemMenuWidget : public Widget {
    Image icon;

    PipelineImage::Image backingImage;

    aarect systemMenuRectangle;

public:
    SystemMenuWidget(Window &window, Widget *parent, Image icon) noexcept;
    ~SystemMenuWidget() {}

    SystemMenuWidget(const SystemMenuWidget &) = delete;
    SystemMenuWidget &operator=(const SystemMenuWidget &) = delete;
    SystemMenuWidget(SystemMenuWidget &&) = delete;
    SystemMenuWidget &operator=(SystemMenuWidget &&) = delete;

    void layout(hires_utc_clock::time_point displayTimePoint) noexcept override;
    void draw(DrawContext const &drawContext, hires_utc_clock::time_point displayTimePoint) noexcept override;

    [[nodiscard]] HitBox hitBoxTest(vec position) const noexcept override;

private:

};

}
