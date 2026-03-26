#include "../winhelpgui.hpp"
#include "../../winhelp/src/ver3/winhelp.hpp"
using namespace winhelp;

#include <iostream>

winhelpgui::Button button({100, 100}, {50, 50}, 0, {100, 255, 100}, {0, 0, 0}, true, false, false);
winhelpgui::TextBox textbox("Hello, world", {200, 100}, {500, 100}, {20, 20, 50}, {255, 255, 255}, 24);
int state = 1; 

void printhi(winhelpgui::UIElement& self) {
    std::cout << "Hello, world\n";
    switch (state) {
        case 1: {
            textbox.text = "Hello, world.";
        } break;
        case 2: {
            textbox.text = "Hello, world..";
        } break;
        case 3: {
            textbox.text = "Hello, world...";
            state = 0;
        } break;
    }
    state++;
}

int main() {
    display screen = display({900, 600}, "GUI test");

    Font font(20);

    button.on_click(printhi);

    while (true) {
        screen.surface.fill({20, 20, 20, 255});
        std::vector<events::event> events = events::get();
        for (events::event event : events) {
            switch (event.type) {
                case events::eventTypes::quit: {
                    screen.close();
                    exit(0);
                } break;
            }
        }

        button.tick(screen.surface, events);
        textbox.tick(screen.surface, events);

        screen.surface.blit({0, 0}, font.render(std::to_string(fps), {255, 255, 255}, {0, 0, 0, 0}));

        screen.flip();
        tick(60);
    }
}


