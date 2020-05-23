// Copyright 2019 Pokitec
// All rights reserved.

#include "TTauri/GUI/Widget.hpp"
#include "TTauri/GUI/utils.hpp"

namespace TTauri::GUI::Widgets {

Widget::Widget(Window &window, Widget *parent, vec defaultExtent) noexcept :
    window(window),
    parent(parent),
    elevation(parent ? parent->elevation + 1.0f : 0.0f),
    enabled(true, [this](auto...){ forceRedraw = true; })
{
    minimumExtent = defaultExtent;
    minimumWidthConstraint = window.addConstraint(box.width >= minimumExtent.width());
    minimumHeightConstraint = window.addConstraint(box.height >= minimumExtent.height());

    preferedExtent = defaultExtent;
    preferedWidthConstraint = window.addConstraint(box.width >= preferedExtent.width(), rhea::strength::strong());
    preferedHeightConstraint = window.addConstraint(box.height >= preferedExtent.height(), rhea::strength::strong());
}

Widget::~Widget()
{
    window.removeConstraint(minimumWidthConstraint);
    window.removeConstraint(minimumHeightConstraint);
    window.removeConstraint(preferedWidthConstraint);
    window.removeConstraint(preferedHeightConstraint);
}

Device *Widget::device() const noexcept
{
    auto device = window.device;
    ttauri_assert(device);
    return device;
}

void Widget::setMinimumExtent(vec newMinimumExtent) noexcept
{
    if (newMinimumExtent != minimumExtent) {
        minimumExtent = newMinimumExtent;

        minimumWidthConstraint = window.replaceConstraint(
            minimumWidthConstraint,
            box.width >= minimumExtent.width()
        );

        minimumHeightConstraint = window.replaceConstraint(
            minimumHeightConstraint,
            box.height >= minimumExtent.height()
        );
    }
}

void Widget::setPreferedExtent(vec newPreferedExtent) noexcept
{
    if (newPreferedExtent != preferedExtent) {
        preferedExtent = newPreferedExtent;

        preferedWidthConstraint = window.replaceConstraint(
            preferedWidthConstraint,
            box.width >= preferedExtent.width(),
            rhea::strength::weak()
        );

        preferedHeightConstraint = window.replaceConstraint(
            preferedHeightConstraint,
            box.height >= preferedExtent.height(),
            rhea::strength::weak()
        );
    }
}

void Widget::setMinimumExtent(float width, float height) noexcept
{
    setMinimumExtent(vec{width, height});
}

void Widget::placeBelow(Widget const &rhs, float margin) const noexcept {
    window.addConstraint(this->box.top + margin == rhs.box.bottom);
}

void Widget::placeAbove(Widget const &rhs, float margin) const noexcept {
    window.addConstraint(this->box.bottom == rhs.box.top + margin);
}

void Widget::placeLeftOf(Widget const &rhs, float margin) const noexcept {
    window.addConstraint(this->box.right + margin == rhs.box.left);
}

void Widget::placeRightOf(Widget const &rhs, float margin) const noexcept {
    window.addConstraint(this->box.left == rhs.box.right + margin);
}

void Widget::placeAtTop(float margin) const noexcept {
    window.addConstraint(this->box.top + margin == parent->box.top);
}

void Widget::placeAtBottom(float margin) const noexcept {
    window.addConstraint(this->box.bottom - margin == parent->box.bottom);
}

void Widget::placeLeft(float margin) const noexcept {
    window.addConstraint(this->box.left - margin == parent->box.left);
}

void Widget::placeRight(float margin) const noexcept {
    window.addConstraint(this->box.right + margin == parent->box.right);
}

WidgetNeed Widget::needs(hires_utc_clock::time_point displayTimePoint) const noexcept
{
    auto layout = forceLayout.exchange(false, std::memory_order::memory_order_relaxed);
    layout |= box.hasResized();
    auto redraw = layout;
    redraw |= forceRedraw.exchange(false, std::memory_order::memory_order_relaxed);

    auto need =
        (static_cast<int>(layout) << 1) |
        static_cast<int>(redraw);

    return static_cast<WidgetNeed>(need);
}

void Widget::layout(hires_utc_clock::time_point displayTimePoint) noexcept
{
    windowRectangle = round(box.rectangle());

    clippingRectangle = expand(windowRectangle, Theme::margin);
    rectangle = windowRectangle.extent();

    offsetFromParent = parent ?
        windowRectangle.offset() - parent->windowRectangle.offset():
        windowRectangle.offset();
        
    fromWindowTransform = mat::T(-windowRectangle.x(), -windowRectangle.y(), -z());
    toWindowTransform = mat::T(windowRectangle.x(), windowRectangle.y(), z());
}

WidgetNeed Widget::layoutChildren(hires_utc_clock::time_point displayTimePoint, bool force) noexcept
{
    auto total_need = WidgetNeed::None;

    for (auto &&child: children) {
        let child_need = child->needs(displayTimePoint);
        total_need |= child_need;

        if (force || child_need >= WidgetNeed::Layout) {
            child->layout(displayTimePoint);
        }

        total_need |= child->layoutChildren(displayTimePoint, force);
    }

    return total_need;
}

void Widget::draw(DrawContext const &drawContext, hires_utc_clock::time_point displayTimePoint) noexcept
{
    auto childContext = drawContext;
    for (auto &child : children) {
        childContext.clippingRectangle = child->clippingRectangle;
        childContext.transform = child->toWindowTransform;

        // The default fill and border colors.
        let childNestingLevel = child->nestingLevel();
        childContext.color = theme->borderColor(childNestingLevel);
        childContext.fillColor = theme->fillColor(childNestingLevel);

        if (child->enabled) {
            if (child->focus) {
                childContext.color = theme->accentColor;
            } else if (child->hover) {
                childContext.color = theme->borderColor(childNestingLevel + 1);
            }

            if (child->hover) {
                childContext.fillColor = theme->fillColor(childNestingLevel + 1);
            }

        } else {
            // Disabled, only the outline is shown.
            childContext.color = theme->borderColor(childNestingLevel - 1);
            childContext.fillColor = theme->fillColor(childNestingLevel - 1);
        }

        child->draw(childContext, displayTimePoint);
    }
}

void Widget::handleCommand(string_ltag command) noexcept {
    if (command == "gui.widget.next"_ltag) {
        window.updateToNextKeyboardTarget(this);
    } else if (command == "gui.widget.prev"_ltag) {
        window.updateToPrevKeyboardTarget(this);
    }
}

HitBox Widget::hitBoxTest(vec position) const noexcept
{
    auto r = rectangle.contains(position) ?
        HitBox{this, elevation} :
        HitBox{};

    for (let &child : children) {
        r = std::max(r, child->hitBoxTest(position - child->offsetFromParent));
    }
    return r;
}

}
