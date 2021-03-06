// Copyright Take Vos 2019-2021.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "../GUI/gui_window.hpp"
#include "../GUI/gui_device.hpp"
#include "../GUI/mouse_event.hpp"
#include "../GUI/hit_box.hpp"
#include "../GUI/keyboard_event.hpp"
#include "../GUI/theme.hpp"
#include "../GUI/draw_context.hpp"
#include "../GUI/keyboard_focus_direction.hpp"
#include "../GUI/keyboard_focus_group.hpp"
#include "../text/shaped_text.hpp"
#include "../alignment.hpp"
#include "../graphic_path.hpp"
#include "../color/sfloat_rgba16.hpp"
#include "../color/sfloat_rg32.hpp"
#include "../URL.hpp"
#include "../vspan.hpp"
#include "../utils.hpp"
#include "../cpu_utc_clock.hpp"
#include "../observable.hpp"
#include "../command.hpp"
#include "../unfair_recursive_mutex.hpp"
#include "../interval_vec2.hpp"
#include "../flow_layout.hpp"
#include "../ranged_numeric.hpp"
#include <limits>
#include <memory>
#include <vector>
#include <mutex>
#include <typeinfo>

namespace tt::pipeline_image {
struct Image;
struct vertex;
} // namespace tt::pipeline_image
namespace tt::pipeline_SDF {
struct vertex;
}
namespace tt::pipeline_flat {
struct vertex;
}
namespace tt::pipeline_box {
struct vertex;
}

namespace tt {
class abstract_container_widget;

/*! View of a widget.
 * A view contains the dynamic data for a Widget. It is often accompanied with a Backing
 * which contains that static data of an Widget and the drawing code. Backings are shared
 * between Views.
 *
 * All methods should make sure the mutex is locked by the current thread.
 *
 * Rendering is done in three distinct phases:
 *  1. Updating Constraints
 *  2. Updating Layout
 *  3. Drawing (optional phase)
 *
 * Each of these phases is executed to completion for all widget belonging to a
 * window before the next phases is executed.
 *
 * ## Updating Constraints
 * In this phases the widget will update the constraints to determine the position
 * and size of the widget inside the window.
 *
 * The `updateConstraints()` function will be called on each widget recursively.
 * You should minimize the cost of this function as much as possible.
 *
 * Since this function is called on each frame, the widget should first check
 * if constraint changes are needed.
 *
 * A widget should return true if any of the constraints has changed.
 *
 * ## Updating Layout
 * A widget may update its internal (expensive) layout calculations from the
 * `updateLayout()` function.
 *
 * Since this function is called on each frame, the widget should first check
 * if layout calculations are needed. If a constraint has changed (the window size
 * is also a constraint) then the `forceLayout` flag is set.
 *
 * A widget should return true if the window needs to be redrawn.
 *
 * ## Drawing (optional)
 * A widget can draw itself when the `draw()` function is called. This phase is only
 * entered when one of the widget's layout was changed. But if this phase is entered
 * then all the widgets' `draw()` functions are called.
 */
class widget : public std::enable_shared_from_this<widget> {
public:
    /** Convenient reference to the Window.
     */
    gui_window &window;


    /** The widget is enabled.
     */
    observable<bool> enabled = true;

    /*! Constructor for creating sub views.
     */
    widget(gui_window &window, std::shared_ptr<abstract_container_widget> parent) noexcept;

    virtual ~widget();
    widget(const widget &) = delete;
    widget &operator=(const widget &) = delete;
    widget(widget &&) = delete;
    widget &operator=(widget &&) = delete;

    /** Should be called right after allocating and constructing a widget.
     */
    virtual void init() noexcept {}

    /** Get the margin around the Widget.
     * A container widget should layout the children in such
     * a way that the maximum margin of neighbouring widgets is maintained.
     *
     * @pre `mutex` must be locked by current thread.
     * @return The margin for this widget.
     */
    [[nodiscard]] float margin() const noexcept
    {
        tt_axiom(gui_system_mutex.recurse_lock_count());
        return _margin;
    }

    /** The first drawing layer of the widget.
     * Drawing layers start at 0.0 and go up to 100.0.
     *
     * Each child widget that has drawing to do increases the layer by 1.0.
     *
     * The widget should draw within 0.0 and 1.0 of its drawing layer.
     * The toWindowTransfer and the DrawingContext will already include
     * the draw_layer.
     *
     * An overlay widget such as popups will increase the layer by 25.0,
     * to make sure the overlay will draw above other widgets in the window.
     *
     * @pre `mutex` must be locked by current thread.
     * @return The draw layer of this widget.
     */
    [[nodiscard]] float draw_layer() const noexcept
    {
        tt_axiom(gui_system_mutex.recurse_lock_count());
        return _draw_layer;
    }

    /** The logical layer of the widget.
     * The logical layer can be used to determine how for away
     * from the window-widget (root) the current widget is.
     *
     * Logical layers start at 0 for the window-widget.
     * Each child widget increases the logical layer by 1.
     *
     * @pre `mutex` must be locked by current thread.
     * @return The draw layer of this widget.
     */
    [[nodiscard]] int logical_layer() const noexcept
    {
        tt_axiom(gui_system_mutex.recurse_lock_count());
        return _logical_layer;
    }

    /** The semantic layer of the widget.
     *
     * The semantic layer is used mostly by the `draw()` function
     * for selecting colors from the theme, to denote nesting widgets
     * inside other widgets.
     *
     * Semantic layers start at 0 for the window-widget and for any pop-up
     * widgets.
     *
     * The semantic layer is increased by one, whenever a user of the
     * user-interface would understand the next layer to begin.
     *
     * In most cases it would mean that a container widget that does not
     * draw itself will not increase the semantic_layer number.
     *
     * @pre `mutex` must be locked by current thread.
     * @return The draw layer of this widget.
     */
    [[nodiscard]] int semantic_layer() const noexcept
    {
        tt_axiom(gui_system_mutex.recurse_lock_count());
        return _semantic_layer;
    }

    /** Get the resistance in width.
     * The amount of resistance the widget has for resizing to larger than the minimum size.
     *
     * @pre `mutex` must be locked by current thread.
     * @return The amount of resistance to horizontal resize.
     * @retval 0 Greedy: Widget will try to become the largest.
     * @retval 1 Normal: Widget will share the space with others.
     * @retval 2 Resist: Widget will try to maintain minimum size.
     */
    [[nodiscard]] ranged_int<3> width_resistance() const noexcept
    {
        tt_axiom(gui_system_mutex.recurse_lock_count());
        return _width_resistance;
    }

    /** Get the resistance in height.
     * The amount of resistance the widget has for resizing to larger than the minimum size.
     *
     * @pre `mutex` must be locked by current thread.
     * @return The amount of resistance to vertical resize.
     * @retval 0 Greedy: Widget will try to become the largest.
     * @retval 1 Normal: Widget will share the space with others.
     * @retval 2 Resist: Widget will try to maintain minimum size.
     */
    [[nodiscard]] ranged_int<3> height_resistance() const noexcept
    {
        tt_axiom(gui_system_mutex.recurse_lock_count());
        return _height_resistance;
    }

    /** Get the size-range of the widget.
     * This size-range is used to determine the size of container widgets
     * and eventually the size of the window.
     *
     * @pre `mutex` must be locked by current thread.
     * @pre `updateConstraint()` must be called first.
     * @return The minimum and maximum size as an interval of a 2D vector.
     */
    [[nodiscard]] interval_vec2 preferred_size() const noexcept
    {
        tt_axiom(gui_system_mutex.recurse_lock_count());
        return _preferred_size;
    }

    /** Get the relative_base_line of the widget.
     * This value is used by a container widget to align the text
     * of its children on the same base-line.
     *
     * By taking `std::max()`  of the `preferred_base_line()` of all children,
     * you will get the highest priority relative-base-line. The container
     * can then use the `relative_base_line::position()` method to get the
     * base-line position that should be used for all widgets in a row.
     *
     * @pre `mutex` must be locked by current thread.
     * @pre `updateConstraint()` must be called first.
     * @return A relative base-line position preferred by this widget.
     */
    [[nodiscard]] relative_base_line preferred_base_line() const noexcept
    {
        tt_axiom(gui_system_mutex.recurse_lock_count());
        return _preferred_base_line;
    }

    /** Set the location and size of the widget inside the window.
     *
     * The parent should call this `set_layout_paramters()` before this `updateLayout()`.
     *
     * If the parent's layout did not change, it does not need to call this `set_layout_parameters()`.
     * This way the parent does not need to keep a cache, recalculate or query the client for these
     * layout parameters for each frame.
     *
     * @pre `mutex` must be locked by current thread.
     * @param window_rectangle The location and size of the widget inside the window.
     * @param window_clipping_rectangle The location and size of the clipping rectangle,
     *                                  beyond which not to draw or accept mouse events.
     * @param window_base_line The position of the text base-line from the bottom of the window. When not given
     *                         the middle of the _window_rectangle is used together with the preferred
     *                         relative base-line.
     */
    void set_layout_parameters(
        aarect const &window_rectangle,
        aarect const &window_clipping_rectangle,
        float window_base_line = std::numeric_limits<float>::infinity()) noexcept
    {
        if (std::isinf(window_base_line)) {
            window_base_line = _preferred_base_line.position(window_rectangle.bottom(), window_rectangle.top());
        }

        tt_axiom(gui_system_mutex.recurse_lock_count());

        if (_window_rectangle != window_rectangle) {
            // The previous position needs to be redrawn.
            window.request_redraw(_window_clipping_rectangle);

            _window_rectangle = window_rectangle;
            _request_relayout = true;
        }

        ttlet window_clipping_rectangle_clean =
            intersect(window_clipping_rectangle, expand(_window_rectangle, theme::global->borderWidth));
        if (_window_clipping_rectangle != window_clipping_rectangle_clean) {
            _window_clipping_rectangle = window_clipping_rectangle_clean;
            _request_relayout = true;
        }

        if (_window_base_line != window_base_line) {
            _window_base_line = window_base_line;
            _request_relayout = true;
        }
    }

    /** Get the rectangle in window coordinates.
     *
     * @pre `mutex` must be locked by current thread.
     */
    [[nodiscard]] aarect window_rectangle() const noexcept
    {
        tt_axiom(gui_system_mutex.recurse_lock_count());
        return _window_rectangle;
    }

    /** Get the clipping-rectangle in window coordinates.
     *
     * @pre `mutex` must be locked by current thread.
     */
    [[nodiscard]] virtual aarect window_clipping_rectangle() const noexcept
    {
        tt_axiom(gui_system_mutex.recurse_lock_count());
        return _window_clipping_rectangle;
    }

    /** Get the base-line distance from the bottom of the window.
     *
     * @pre `mutex` must be locked by current thread.
     */
    [[nodiscard]] float window_base_line() const noexcept
    {
        tt_axiom(gui_system_mutex.recurse_lock_count());
        return _window_base_line;
    }

    /** Get the rectangle in local coordinates.
     *
     * @pre `mutex` must be locked by current thread.
     */
    [[nodiscard]] aarect rectangle() const noexcept
    {
        tt_axiom(gui_system_mutex.recurse_lock_count());
        return aarect{_window_rectangle.extent()};
    }

    /** Get the base-line in local coordinates.
     *
     * @pre `mutex` must be locked by current thread.
     */
    [[nodiscard]] float base_line() const noexcept
    {
        tt_axiom(gui_system_mutex.recurse_lock_count());
        return _window_base_line - _window_rectangle.y();
    }

    [[nodiscard]] gui_device *device() const noexcept;

    /** Find the widget that is under the mouse cursor.
     * This function will recursively test with visual child widgets, when
     * widgets overlap on the screen the hitbox object with the highest elevation is returned.
     *
     * @param window_position The coordinate of the mouse on the window.
     *                        Use `fromWindowTransform` to convert to widget-local coordinates.
     * @return A hit_box object with the cursor-type and a reference to the widget.
     */
    [[nodiscard]] virtual hit_box hitbox_test(f32x4 window_position) const noexcept
    {
        ttlet lock = std::scoped_lock(gui_system_mutex);

        if (_window_clipping_rectangle.contains(window_position) && _window_rectangle.contains(window_position)) {
            return hit_box{weak_from_this(), _draw_layer};
        } else {
            return {};
        }
    }

    /** Check if the widget will accept keyboard focus.
     *
     * @pre `mutex` must be locked by current thread.
     */
    [[nodiscard]] virtual bool accepts_keyboard_focus(keyboard_focus_group group) const noexcept
    {
        tt_axiom(gui_system_mutex.recurse_lock_count());
        return false;
    }

    /** Update the constraints of the widget.
     * This function is called on each vertical sync, even if no drawing is to be done.
     * It should recursively call `updateConstraints()` on each of the visible children,
     * so they get a chance to update.
     *
     * This function may be used for expensive calculations, such as text-shaping, which
     * should only be done when the data changes. Because this function is called on every
     * vertical sync it should cache these calculations.
     *
     * Subclasses should call `updateConstraints()` on its base-class to check if the constraints where
     * changed. `Widget::update_constraints()` will check if `requestConstraints` was set.
     * `Container::update_constraints()` will check if any of the children changed constraints.
     *
     * If the container, due to a change in constraints, wants the window to resize to the minimum size
     * it should set window.requestResize to true.
     *
     * This function will change what is returned by `preferred_size()` and `preferred_base_line()`.
     *
     * @pre `mutex` must be locked by current thread.
     * @param display_time_point The time point when the widget will be shown on the screen.
     * @param need_reconstrain Force the widget to re-constrain.
     * @return True if its or any children's constraints has changed.
     */
    [[nodiscard]] virtual bool update_constraints(hires_utc_clock::time_point display_time_point, bool need_reconstrain) noexcept;

    /** Update the internal layout of the widget.
     * This function is called on each vertical sync, even if no drawing is to be done.
     * It should recursively call `updateLayout()` on each of the visible children,
     * so they get a chance to update.
     *
     * This function may be used for expensive calculations, such as geometry calculations,
     * which should only be done when the data or sizes change. Because this function is called
     * on every vertical sync it should cache these calculations.
     *
     * This function will likely call `set_window_rectangle()` and `set_window_base_line()` on
     * its children, before calling `updateLayout()` on that child.
     *
     * Subclasses should call `updateLayout()` on its children, call `updateLayout()` on its
     * base class with `forceLayout` argument to the result of `layoutRequest.exchange(false)`.
     *
     * @pre `mutex` must be locked by current thread.
     * @param display_time_point The time point when the widget will be shown on the screen.
     * @param need_layout Force the widget to layout
     */
    [[nodiscard]] virtual void update_layout(hires_utc_clock::time_point display_time_point, bool need_layout) noexcept;

    /** Make a draw context for this widget.
     * This function will make a draw context with the correct transformation
     * and default color values.
     *
     * @pre `mutex` must be locked by current thread.
     * @param context A template drawing context. This template may be taken
     *                from the parent's draw call.
     * @return A new draw context for drawing the current widget in the
     *         local coordinate system.
     */
    virtual draw_context make_draw_context(draw_context context) const noexcept;

    /** Draw the widget.
     * This function is called by the window (optionally) on every frame.
     * It should recursively call this function on every visible child.
     * This function is only called when `updateLayout()` has returned true.
     *
     * The overriding function should call the base class's `draw()`, the place
     * where the call this function will determine the order of the vertices into
     * each buffer. This is important when needing to do the painters algorithm
     * for alpha-compositing. However the pipelines are always drawn in the same
     * order.
     *
     * @pre `mutex` must be locked by current thread.
     * @param context The context to where the widget will draw.
     * @param display_time_point The time point when the widget will be shown on the screen.
     */
    virtual void draw(draw_context context, hires_utc_clock::time_point display_time_point) noexcept
    {
        tt_axiom(gui_system_mutex.recurse_lock_count());
    }

    /** Handle command.
     * If a widget does not fully handle a command it should pass the
     * command to the super class' `handle_event()`.
     */
    [[nodiscard]] virtual bool handle_event(command command) noexcept;

    [[nodiscard]] virtual bool handle_event(std::vector<command> const &commands) noexcept
    {
        for (ttlet command : commands) {
            if (handle_event(command)) {
                return true;
            }
        }
        return false;
    }

    /** Handle command recursive.
     * Handle a command and pass it to each child.
     *
     * @param command The command to handle by this widget.
     * @param reject_list The widgets that should ignore this command
     * @return True when the command was handled by this widget or recursed child.
     */
    [[nodiscard]] virtual bool
    handle_command_recursive(command command, std::vector<std::shared_ptr<widget>> const &reject_list) noexcept;

    /*! Handle mouse event.
     * Called by the operating system to show the position and button state of the mouse.
     * This is called very often so it must be made efficient.
     * This function is also used to determine the mouse cursor.
     *
     * In most cased overriding methods should call the super's `handle_event()` at the
     * start of the function, to detect `hover`.
     *
     * When this method does not handle the event the window will call `handle_event()`
     * on the widget's parent.
     *
     * @param event The mouse event, positions are in window coordinates.
     * @return If this widget has handled the mouse event.
     */
    [[nodiscard]] virtual bool handle_event(mouse_event const &event) noexcept;

    /** Handle keyboard event.
     * Called by the operating system when editing text, or entering special keys
     *
     * @param event The keyboard event.
     * @return If this widget has handled the keyboard event.
     */
    [[nodiscard]] virtual bool handle_event(keyboard_event const &event) noexcept;

    /** Find the next widget that handles keyboard focus.
     * This recursively looks for the current keyboard widget, then returns the next (or previous) widget
     * that returns true from `accepts_keyboard_focus()`.
     *
     * @param current_keyboard_widget The widget that currently has focus; or empty to get the first widget
     *                              that accepts focus.
     * @param group The group to which the widget must belong.
     * @param direction The direction in which to walk the widget tree.
     * @return A pointer to the next widget.
     * @retval currentKeyboardWidget when currentKeyboardWidget was found but no next widget was found.
     * @retval empty when currentKeyboardWidget is not found in this Widget.
     */
    [[nodiscard]] virtual std::shared_ptr<widget> find_next_widget(
        std::shared_ptr<widget> const &current_keyboard_widget,
        keyboard_focus_group group,
        keyboard_focus_direction direction) const noexcept;

    /** Get a shared_ptr to the parent.
     */
    [[nodiscard]] std::shared_ptr<abstract_container_widget const> shared_parent() const noexcept;

    /** Get a shared_ptr to the parent.
     */
    [[nodiscard]] std::shared_ptr<abstract_container_widget> shared_parent() noexcept;

    /** Get a reference to the parent.
     * It is undefined behavior to call this function when the widget does not have a parent.
     */
    [[nodiscard]] abstract_container_widget const &parent() const noexcept;

    /** Get a reference to the parent.
     * It is undefined behavior to call this function when the widget does not have a parent.
     */
    [[nodiscard]] abstract_container_widget &parent() noexcept;

    /** Is this widget the first widget in the parent container.
     */
    [[nodiscard]] bool is_first(keyboard_focus_group group) const noexcept;

    /** Is this widget the last widget in the parent container.
     */
    [[nodiscard]] bool is_last(keyboard_focus_group group) const noexcept;

    /** Get a list of parents of a given widget.
     * The chain includes the given widget.
     */
    [[nodiscard]] static std::vector<std::shared_ptr<widget>> parent_chain(std::shared_ptr<tt::widget> const &child_widget) noexcept;

protected:
    /** Pointer to the parent widget.
     * May be a nullptr only when this is the top level widget.
     */
    std::weak_ptr<abstract_container_widget> _parent;

    /** Mouse cursor is hovering over the widget.
     */
    bool _hover = false;

    /** The widget has keyboard focus.
     */
    bool _focus = false;

    /** Transformation matrix from window coords to local coords.
     */
    matrix3 _from_window_transform;

    /** Transformation matrix from local coords to window coords.
     */
    matrix3 _to_window_transform;

    /** When set to true the widget will recalculate the constraints on the next call to `updateConstraints()`
     */
    bool _request_reconstrain = true;

    /** When set to true the widget will recalculate the layout on the next call to `updateLayout()`
     */
    bool _request_relayout = true;

    /** The position of the widget on the window.
     */
    aarect _window_rectangle;

    /** The height of the base line from the bottom of the window.
     */
    float _window_base_line;

    /** The clipping rectangle beyond which not to draw or receive mouse events.
     */
    aarect _window_clipping_rectangle;

    interval_vec2 _preferred_size;

    relative_base_line _preferred_base_line = relative_base_line{};

    ranged_int<3> _width_resistance = 1;
    ranged_int<3> _height_resistance = 1;

    float _margin = theme::global->margin;

    /** The draw layer of the widget.
     * This value translates directly to the z-axis between 0.0 (far) and 100.0 (near)
     * of the clipping volume.
     *
     * This value is discontinues to handle overlay-panels which should be drawn on top
     * of widgets which may have a high `_logical_layer`.
     *
     * @sa draw_layer() const
     */
    float _draw_layer;

    /** The draw layer of the widget.
     * This value increments for each visible child-layer in the widget tree
     * and resets on overlay-panels.
     *
     * @sa semantic_layer() const
     */
    int _semantic_layer;

    /** The logical layer of the widget.
     * This value increments for each child-layer in the widget tree.
     *
     * @sa logical_layer() const
     */
    int _logical_layer;

private:
    typename decltype(enabled)::callback_ptr_type _enabled_callback;
};

} // namespace tt
