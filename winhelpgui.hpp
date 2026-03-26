#pragma once
#include <chrono>
#include <functional>
#include <vector>
#include <memory>
#include <optional>
#include <string>
#include <initializer_list>

#if !defined(WINHELP)
#define WINHELP "../winhelp/src/ver3/winhelp.hpp"
#endif
#include WINHELP

#if defined(__GNUC__) || defined(__clang__)
#define FORCE_INLINE inline __attribute__((always_inline))
#else
#define FORCE_INLINE inline
#endif

namespace winhelpgui {
    struct UIElement {
        winhelp::vec2 pos;
        winhelp::vec2 size;

        bool visible = true;
        bool enabled = true;

        UIElement() {}
        UIElement(winhelp::vec2 pos, winhelp::vec2 size)
            : pos(pos), size(size) {}

        virtual ~UIElement() {}

        virtual void render(winhelp::Surface& screen) = 0;

        virtual void tick(
            winhelp::Surface& screen,
            const std::vector<winhelp::events::event>& events,
            winhelp::vec2 mouse_pos = winhelp::mouse()
        ) {
            if (!visible) return;
            render(screen);
        }

        FORCE_INLINE bool contains(const winhelp::vec2& p) const {
            return p.x >= pos.x && p.x <= pos.x + size.x &&
                p.y >= pos.y && p.y <= pos.y + size.y;
        }

    };

    typedef std::function<void(UIElement&)> callBackFunction;
    typedef std::vector<std::unique_ptr<UIElement>> UIElementList; 

    struct TextUI : public UIElement {
        std::string text;
        winhelp::vec3 text_color;
        winhelp::Font font;

        unsigned int font_size;
        unsigned int line_spacing = 5;

        enum class Align {
            TopLeft, Top, TopRight,
            MiddleLeft, Middle, MiddleRight,
            BottomLeft, Bottom, BottomRight
        };

        Align align = Align::TopLeft;

        TextUI(
            std::string text,
            winhelp::vec2 pos,
            winhelp::vec2 size,
            winhelp::vec3 text_color,
            unsigned int font_size,
            Align align = Align::TopLeft
        ) : UIElement(pos, size),
            text(text),
            text_color(text_color),
            font_size(font_size),
            align(align),
            font(font_size) {}

        FORCE_INLINE std::vector<std::string> split_words(const std::string& str) {
            std::vector<std::string> words;
            std::string current;
            for (char c : str) {
                if (c == ' ') {
                    if (!current.empty()) {
                        words.push_back(current);
                        current.clear();
                    }
                } else {
                    current += c;
                }
            }
            if (!current.empty()) words.push_back(current);
            return words;
        }

        FORCE_INLINE std::vector<std::string> wrap_lines() {
            std::vector<std::string> words = split_words(text);
            std::vector<std::string> lines;

            std::string current;

            for (auto& word : words) {
                std::string test = current.empty() ? word : current + " " + word;
                if (font.sizeOf(test).x < (int)std::max(0.0f, size.x - 10)) {
                    current = test;
                } else {
                    lines.push_back(current);
                    current = word + " ";
                }
            }

            if (!current.empty()) lines.push_back(current);
            return lines;
        }

        FORCE_INLINE winhelp::vec2 compute_start(const std::vector<std::string>& lines) {
            float total_height = 0;
            for (size_t i = 0; i < lines.size(); ++i) {
                total_height += font.lineHeight;
                if (i + 1 < lines.size()) total_height += line_spacing;
            }

            float y = 0;

            switch (align) {
                case Align::TopLeft:
                case Align::Top:
                case Align::TopRight:
                    y = 5;
                    break;

                case Align::MiddleLeft:
                case Align::Middle:
                case Align::MiddleRight:
                    y = (size.y - total_height) * 0.5f;
                    break;

                case Align::BottomLeft:
                case Align::Bottom:
                case Align::BottomRight:
                    y = size.y - total_height - 5;
                    break;
            }

            return {0, y};
        }

        FORCE_INLINE float compute_x(const std::string& line) {
            float line_width = font.sizeOf(line).x;

            switch (align) {
                case Align::TopLeft:
                case Align::MiddleLeft:
                case Align::BottomLeft:
                    return 5;

                case Align::Top:
                case Align::Middle:
                case Align::Bottom:
                    return (size.x - line_width) * 0.5f;

                case Align::TopRight:
                case Align::MiddleRight:
                case Align::BottomRight:
                    return size.x - line_width - 5;
            }

            return 5;
        }

        virtual void render(winhelp::Surface& screen) {
            render_text(screen);
        }

        void tick(winhelp::Surface& screen,
                const std::vector<winhelp::events::event>& events,
                winhelp::vec2 mouse_pos = winhelp::mouse()) override {
            if (!visible) return;
            render(screen);
        }

        FORCE_INLINE void render_text(winhelp::Surface& screen) {
            std::vector<std::string> lines = wrap_lines();

            winhelp::vec2 start = compute_start(lines);
            float y = start.y;

            for (auto& line : lines) {
                if ((y + font.lineHeight) > (size.y - 5)) break;

                float x = compute_x(line);

                winhelp::Surface surf = font.render(line, text_color, {0,0,0,0});
                screen.blit(pos + winhelp::vec2{x, y}, surf);

                y += font.lineHeight + line_spacing;
            }
        }

        void fitToSize() { fitToSize(1, 200); }
        void fitToSizeMin(unsigned int min_size) { fitToSize(min_size, 200); }
        void fitToSizeMax(unsigned int max_size) { fitToSize(1, max_size); }

        void fitToSize(unsigned int min_size, unsigned int max_size) {
            int low = (int)min_size;
            int high = (int)max_size;

            unsigned int best = min_size;

            while (low <= high) {
                int mid = low + (high - low) / 2;;
                font.setSize(mid);

                std::vector<std::string> lines = wrap_lines();

                int total_height = 5;
                for (auto& l : lines) {
                    total_height += font.lineHeight + line_spacing;
                }

                bool fits_height = total_height <= size.y;
                bool fits_width = true;

                for (auto& l : lines) {
                    if (font.sizeOf(l).x > (int)std::max(0.0f, size.x - 10)) {
                        fits_width = false;
                        break;
                    }
                }

                if (fits_height && fits_width) {
                    best = mid;
                    low = mid + 1;
                } else {
                    high = mid - 1;
                }
            }

            font_size = best;
            font.setSize(best);
        }
    };

    struct InteractUI {
    protected:
        bool _was_clicked = false;
        bool _is_hovering = false;
        bool _is_pressed = false;

        callBackFunction _click_callback;
        callBackFunction _toggle_on_callback;
        callBackFunction _toggle_off_callback;
        callBackFunction _on_hit_callback;
        callBackFunction _on_released_callback;
        callBackFunction _while_pressed_callback;
        callBackFunction _on_hover_callback;

        FORCE_INLINE bool point_in_rect(const winhelp::vec2& p, const winhelp::vec2& pos, const winhelp::vec2& size) {
            return p.x >= pos.x && p.x <= pos.x + size.x &&
                   p.y >= pos.y && p.y <= pos.y + size.y;
        }

        FORCE_INLINE bool allowed_button(winhelp::events::mouse m) {
            if (m == winhelp::events::mouse::left) return leftClickAllowed;
            if (m == winhelp::events::mouse::right) return rightClickAllowed;
            if (m == winhelp::events::mouse::middle) return middleClickAllowed;
            return false;
        }

    public:
        bool toggled = false;

        bool leftClickAllowed = true;
        bool rightClickAllowed = false;
        bool middleClickAllowed = false;

        FORCE_INLINE void on_click(callBackFunction f){ _click_callback = f; }
        FORCE_INLINE void on_toggle_on(callBackFunction f){ _toggle_on_callback = f; }
        FORCE_INLINE void on_toggle_off(callBackFunction f){ _toggle_off_callback = f; }
        FORCE_INLINE void on_hit(callBackFunction f){ _on_hit_callback = f; }
        FORCE_INLINE void on_released(callBackFunction f){ _on_released_callback = f; }
        FORCE_INLINE void while_pressed(callBackFunction f){ _while_pressed_callback = f; }
        FORCE_INLINE void on_hover(callBackFunction f){ _on_hover_callback = f; }

        void interact(UIElement& self, const std::vector<winhelp::events::event>& events, winhelp::vec2 mouse_pos) {
            _is_hovering = point_in_rect(mouse_pos, self.pos, self.size);

            if (_is_hovering && _on_hover_callback) _on_hover_callback(self);

            for (const auto& e : events) {

                if (e.type == winhelp::events::eventTypes::mouse_down && allowed_button(e.click)) {

                    if (_is_hovering) {
                        if (!_is_pressed) {
                            _is_pressed = true;
                            if (_on_hit_callback) _on_hit_callback(self);
                        }

                        if (!_was_clicked) {
                            _was_clicked = true;

                            if (_click_callback) _click_callback(self);

                            toggled = !toggled;
                            if (toggled && _toggle_on_callback) _toggle_on_callback(self);
                            if (!toggled && _toggle_off_callback) _toggle_off_callback(self);
                        }
                    }
                }

                if (e.type == winhelp::events::eventTypes::mouse_up && allowed_button(e.click)) {
                    if (_is_pressed) {
                        _is_pressed = false;
                        if (_on_released_callback) _on_released_callback(self);
                    }
                    _was_clicked = false;
                }
            }

            if (_is_pressed && _while_pressed_callback) {
                _while_pressed_callback(self);
            }

            if (!_is_hovering && !_is_pressed) {
                _was_clicked = false;
            }
        }
    };

    struct ContainerUI : public UIElement {
        protected:
        UIElementList children;
        winhelp::Surface view;

        public:
        
        ContainerUI(winhelp::vec2 pos, winhelp::vec2 size) 
         : UIElement(pos, size), view(size) {
            view.hasAlpha = true;
         }
        
        ContainerUI(std::vector<std::unique_ptr<UIElement>>&& list)
        : UIElement({0, 0}, {100, 100}), view(size),
        children(std::move(list)) {
            view.hasAlpha = true;
        }

        void render(winhelp::Surface& screen) override {
            screen.blit(pos, view);
        }

        void tick(winhelp::Surface& screen,
                const std::vector<winhelp::events::event>& events,
                winhelp::vec2 mouse_pos = winhelp::mouse()) override {

            winhelp::vec2 local_mouse = mouse_pos - pos;

            view.fill({0, 0, 0, 0});

            for (std::unique_ptr<UIElement>& child : children) {
                child->tick(view, events, local_mouse);
            }

            if (!visible) return;
            render(screen);
        }
    };

    struct Button : public UIElement, public InteractUI {
        int border_size;
        winhelp::vec3 colour;
        winhelp::vec3 border_colour;

        Button(
            winhelp::vec2 pos,
            winhelp::vec2 size,
            int border_size,
            winhelp::vec3 colour,
            winhelp::vec3 border_colour,
            bool leftClickAllowed,
            bool rightClickAllowed,
            bool middleClickAllowed
        ) : UIElement(pos, size),
            border_size(border_size),
            colour(colour),
            border_colour(border_colour)
        {
            this->leftClickAllowed = leftClickAllowed;
            this->rightClickAllowed = rightClickAllowed;
            this->middleClickAllowed = middleClickAllowed;
        }

        void render(winhelp::Surface& screen) override {
            winhelp::draw::rect(screen, pos, size, colour);
            winhelp::draw::rect(screen, pos, size, border_colour, false, (float)border_size);
        }

        void tick(winhelp::Surface& screen, const std::vector<winhelp::events::event>& events, winhelp::vec2 mouse_pos = winhelp::mouse()) override {
            if (!visible) return;
            render(screen);
            interact(*this, events, mouse_pos);
        }
    };

    struct TextButton : public TextUI, public InteractUI {
        winhelp::vec3 background;
        winhelp::vec3 border;
        int border_size;

        TextButton(
            std::string text,
            winhelp::vec2 pos,
            winhelp::vec2 size,
            winhelp::vec3 bg,
            winhelp::vec3 border,
            winhelp::vec3 text_color,
            unsigned int font_size,
            int border_size = 2
        )
        : TextUI(text, pos, size, text_color, font_size),
          background(bg), border(border), border_size(border_size)
        {}

        void render(winhelp::Surface& screen) override {
            winhelp::draw::rect(screen, pos, size, background);
            winhelp::draw::rect(screen, pos, size, border, false, (float)border_size);

            render_text(screen);
        }

        void tick(winhelp::Surface& screen,
                const std::vector<winhelp::events::event>& events,
                winhelp::vec2 mouse_pos) override {
            if (!visible) return;
            render(screen);
            interact(*this, events, mouse_pos);
        }
    };

    struct TextBox : public TextUI {
        winhelp::vec3 background;
        std::optional<winhelp::vec3> outline;

        TextBox(
            std::string text,
            winhelp::vec2 pos,
            winhelp::vec2 size,
            winhelp::vec3 bg,
            winhelp::vec3 text_color,
            unsigned int font_size = 30
        )
        : TextUI(text, pos, size, text_color, font_size),
          background(bg)
        {}

        void render(winhelp::Surface& screen) override {
            winhelp::draw::rect(screen, pos, size, background);
            if (outline) {
                winhelp::draw::rect(screen, pos, size, *outline, false, 2.0f);
            }

            render_text(screen);
        }
    };

    struct Keypad : public ContainerUI {
        struct KeyItem {
            std::unique_ptr<UIElement> button;
            std::optional<winhelp::vec2> override_pos;
        };

        using Layout = std::vector<std::vector<KeyItem>>;

        TextUI::Align padAlignment;
        int padding;
        std::vector<std::vector<UIElement*>> raw_grid;

        Keypad(
            winhelp::vec2 pos,
            winhelp::vec2 size,
            Layout layout,
            TextUI::Align padAlignment,
            int padding
        )
        : ContainerUI(pos, size), padAlignment(padAlignment), padding(padding)
        {
            build_auto_size(std::move(layout));
        }

        Keypad(
            winhelp::vec2 pos,
            winhelp::vec2 size,
            Layout layout,
            TextUI::Align padAlignment,
            int padding,
            int buttonsize
        )
        : ContainerUI(pos, size), padAlignment(padAlignment), padding(padding)
        {
            build_fixed_size(std::move(layout), buttonsize);
        }

    private:

        FORCE_INLINE winhelp::vec2 align_offset(winhelp::vec2 total) {
            float x = 0, y = 0;

            switch (padAlignment) {
                case TextUI::Align::TopLeft: break;
                case TextUI::Align::Top: x = (size.x - total.x) * 0.5f; break;
                case TextUI::Align::TopRight: x = size.x - total.x; break;

                case TextUI::Align::MiddleLeft: y = (size.y - total.y) * 0.5f; break;
                case TextUI::Align::Middle:
                    x = (size.x - total.x) * 0.5f;
                    y = (size.y - total.y) * 0.5f;
                    break;
                case TextUI::Align::MiddleRight:
                    x = size.x - total.x;
                    y = (size.y - total.y) * 0.5f;
                    break;

                case TextUI::Align::BottomLeft: y = size.y - total.y; break;
                case TextUI::Align::Bottom:
                    x = (size.x - total.x) * 0.5f;
                    y = size.y - total.y;
                    break;
                case TextUI::Align::BottomRight:
                    x = size.x - total.x;
                    y = size.y - total.y;
                    break;
            }

            return {x, y};
        }

        void build_auto_size(Layout layout) {
            float max_row_width = 0;
            float total_height = 0;

            std::vector<float> row_heights;
            std::vector<float> row_widths;

            // measure natural layout
            for (auto& row : layout) {
                float row_width = 0;
                float row_height = 0;

                for (auto& item : row) {
                    row_width += item.button->size.x;
                    row_height = std::max(row_height, item.button->size.y);
                }

                if (!row.empty()) row_width += padding * (row.size() - 1);

                row_widths.push_back(row_width);
                row_heights.push_back(row_height);

                max_row_width = std::max(max_row_width, row_width);
                total_height += row_height;
            }

            if (!layout.empty()) total_height += padding * (layout.size() - 1);

            winhelp::vec2 total = {max_row_width, total_height};

            // scale factor
            float scale = std::min(size.x / total.x, size.y / total.y);

            // scaled total for alignment
            winhelp::vec2 scaled_total = total * scale;
            winhelp::vec2 offset = align_offset(scaled_total);

            float y = offset.y;

            // layout
            for (size_t r = 0; r < layout.size(); ++r) {
                float x = offset.x;

                for (size_t c = 0; c < layout[r].size(); ++c) {
                    auto& item = layout[r][c];

                    // scale BEFORE using size
                    winhelp::vec2 scaled_size = item.button->size * scale;
                    item.button->size = scaled_size;

                    if (item.override_pos) {
                        item.button->pos = *item.override_pos;
                    } else {
                        item.button->pos = winhelp::vec2{x, y};
                    }

                    // advance BEFORE move
                    float advance = scaled_size.x;

                    children.push_back(std::move(item.button));

                    x += advance;
                    if (c + 1 < layout[r].size()) x += padding * scale;
                }

                y += row_heights[r] * scale;
                if (r + 1 < layout.size()) y += padding * scale;
            }
        }

        void build_fixed_size(Layout layout, int buttonsize) {
            float max_row_width = 0;
            float total_height = 0;

            for (auto& row : layout) {
                float row_width = row.size() * buttonsize;
                if (!row.empty()) row_width += padding * (row.size() - 1);

                max_row_width = std::max(max_row_width, row_width);
                total_height += buttonsize;
            }

            if (!layout.empty()) total_height += padding * (layout.size() - 1);

            winhelp::vec2 total = {max_row_width, total_height};
            winhelp::vec2 offset = align_offset(total);

            float y = offset.y;

            for (size_t r = 0; r < layout.size(); ++r) {
                float x = offset.x;

                for (size_t c = 0; c < layout[r].size(); ++c) {
                    auto& item = layout[r][c];

                    item.button->size = { (float)buttonsize, (float)buttonsize };

                    if (item.override_pos) {
                        item.button->pos = *item.override_pos;
                    } else {
                        item.button->pos = pos + winhelp::vec2{x, y};
                    }

                    children.push_back(std::move(item.button));

                    x += buttonsize;
                    if (c + 1 < layout[r].size()) x += padding;
                }

                y += buttonsize;
                if (r + 1 < layout.size()) y += padding;
            }
        }
    };

}

#undef FORCE_INLINE