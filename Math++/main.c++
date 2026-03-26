#include <cmath>
#include <vector>
#include <array>
#include <string>
#include <iostream>
#include <memory>
#include <stack>
#include <sstream>
#include <cctype>

#include "../../winhelp/src/ver3/winhelp.hpp"
#include "../winhelpgui.hpp"

const winhelp::vec2 size = {400, 600};
winhelp::display screen(size, "Math");

const int ResultBufferSpace = 10;
const winhelp::vec2 ResultSize = {
    size.x - (ResultBufferSpace * 3.5f),
    ((size.y * (20.f / 100.f)) - (ResultBufferSpace * 3.5f))
};

winhelpgui::TextBox result(
    "",
    {ResultBufferSpace, ResultBufferSpace},
    ResultSize,
    {25, 25, 25},
    {255, 255, 255}
);

static std::string expr;

int precedence(char op) {
    if (op == '+' || op == '-') return 1;
    if (op == '*' || op == '/') return 2;
    return 0;
}

double apply(double a, double b, char op) {
    if (op == '+') return a + b;
    if (op == '-') return a - b;
    if (op == '*') return a * b;
    if (op == '/') return b != 0.0 ? a / b : 0.0;
    return 0.0;
}

double evaluate(const std::string& s) {
    std::stack<double> values;
    std::stack<char> ops;

    for (size_t i = 0; i < s.size(); i++) {

        if (std::isspace(s[i])) continue;

        if (s[i] == '(') {
            ops.push(s[i]);
        }
        else if (std::isdigit(s[i]) || s[i] == '.') {
            std::string num;
            while (i < s.size() && (std::isdigit(s[i]) || s[i] == '.')) {
                num += s[i++];
            }
            i--;
            values.push(std::stod(num));
        }
        else if (s[i] == ')') {
            while (!ops.empty() && ops.top() != '(') {
                double b = values.top(); values.pop();
                double a = values.top(); values.pop();
                values.push(apply(a, b, ops.top()));
                ops.pop();
            }
            if (!ops.empty()) ops.pop();
        }
        else {
            while (!ops.empty() && precedence(ops.top()) >= precedence(s[i])) {
                double b = values.top(); values.pop();
                double a = values.top(); values.pop();
                values.push(apply(a, b, ops.top()));
                ops.pop();
            }
            ops.push(s[i]);
        }
    }

    while (!ops.empty()) {
        double b = values.top(); values.pop();
        double a = values.top(); values.pop();
        values.push(apply(a, b, ops.top()));
        ops.pop();
    }

    return values.empty() ? 0.0 : values.top();
}

void update_display() {
    result.text = expr.empty() ? "0" : expr;
}

void keypadTextButtonPress(winhelpgui::UIElement& _self) {
    auto* self = dynamic_cast<winhelpgui::TextButton*>(&_self);
    if (!self) return;

    const std::string& t = self->text;

    if (t == "AC") {
        expr.clear();
    }
    else if (t == "C") {
        if (!expr.empty()) expr.pop_back();
    }
    else if (t == "=") {
        try {
            double val = evaluate(expr);
            std::ostringstream ss;
            ss << val;
            expr = ss.str();
        } catch (...) {
            expr = "ERR";
        }
    }
    else {
        if (expr == "ERR") expr.clear();
        expr += t;
    }

    update_display();
}

int main() {

    using KeyItem = winhelpgui::Keypad::KeyItem;
    using Row = std::vector<KeyItem>;
    using Layout = std::vector<Row>;

    Layout layout;

    auto make_btn = [](const std::string& txt) {
        auto btn = std::make_unique<winhelpgui::TextButton>(
            txt,
            winhelp::vec2{0,0},
            winhelp::vec2{50,50},
            winhelp::vec3{25,25,25},
            winhelp::vec3{0,0,0},
            winhelp::vec3{255,255,255},
            20,
            2
        );

        btn->on_click(keypadTextButtonPress);
        btn->fitToSizeMax(40);

        return btn;
    };

    {
        Row row;
        row.push_back(KeyItem{ make_btn("C"), std::nullopt });
        row.push_back(KeyItem{ make_btn("AC"), std::nullopt });
        row.push_back(KeyItem{ make_btn("("), std::nullopt });
        row.push_back(KeyItem{ make_btn(")"), std::nullopt });
        layout.push_back(std::move(row));
    }

    {
        Row row;
        row.push_back(KeyItem{ make_btn("1"), std::nullopt });
        row.push_back(KeyItem{ make_btn("2"), std::nullopt });
        row.push_back(KeyItem{ make_btn("3"), std::nullopt });
        row.push_back(KeyItem{ make_btn("+"), std::nullopt });
        layout.push_back(std::move(row));
    }

    {
        Row row;
        row.push_back(KeyItem{ make_btn("4"), std::nullopt });
        row.push_back(KeyItem{ make_btn("5"), std::nullopt });
        row.push_back(KeyItem{ make_btn("6"), std::nullopt });
        row.push_back(KeyItem{ make_btn("-"), std::nullopt });
        layout.push_back(std::move(row));
    }

    {
        Row row;
        row.push_back(KeyItem{ make_btn("7"), std::nullopt });
        row.push_back(KeyItem{ make_btn("8"), std::nullopt });
        row.push_back(KeyItem{ make_btn("9"), std::nullopt });
        row.push_back(KeyItem{ make_btn("*"), std::nullopt });
        layout.push_back(std::move(row));
    }

    {
        Row row;
        row.push_back(KeyItem{ make_btn("0"), std::nullopt });
        row.push_back(KeyItem{ make_btn("."), std::nullopt });
        row.push_back(KeyItem{ make_btn("="), std::nullopt });
        row.push_back(KeyItem{ make_btn("/"), std::nullopt });
        layout.push_back(std::move(row));
    }

    winhelpgui::Keypad keypad(
        {ResultBufferSpace, ((ResultBufferSpace*2) + ResultSize.y)},
        {ResultSize.x, (size.y - (ResultBufferSpace * 7) - ResultSize.y)},
        std::move(layout),
        winhelpgui::TextUI::Align::Middle,
        10
    );

    result.align = winhelpgui::TextUI::Align::MiddleRight;
    result.fitToSizeMax(40);

    update_display();

    while (1) {
        screen.surface.fill({40, 40, 40, 255});

        std::vector<winhelp::events::event> events = winhelp::events::get();

        for (auto& event : events) {
            if (event.type == winhelp::events::eventTypes::quit) {
                screen.close();
                return 0;
            }
        }

        result.tick(screen.surface, events);
        keypad.tick(screen.surface, events);

        screen.flip();
        winhelp::tick();
    }
}