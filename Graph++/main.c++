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

const winhelp::vec2 size = {800, 600};
winhelp::display screen(size, "Graph");

const int BufferSpace = 10;
const winhelp::vec2 InputSize = {
    std::min(size.x - (BufferSpace * 3.5f), 200.0f),
    ((size.y * (40.f / 100.f)) - (BufferSpace * 3.5f))
};

winhelpgui::TextInputBox Input(
    "PLACEHOLDERPLACEHOLDERPLACEHOLDER",
    {BufferSpace, BufferSpace},
    InputSize,
    {25, 25, 25},
    {255, 255, 255},
    0
);

const winhelp::vec2 GraphSize = {size.x - (float)(BufferSpace * 5.) - InputSize.x,  size.y - (float)(BufferSpace * 10.)};
const winhelp::vec2 GraphPos = {InputSize.x + (BufferSpace * 2), BufferSpace};

winhelp::Surface GraphSurface(GraphSize);

struct Token {
    enum Type {Number, VarX, VarY, Op, LParen, RParen, Func} type;
    double value;
    char op;
    std::string func;
};

bool isFunc(const std::string& s) {
    return s == "sin" || s == "cos" || s == "tan";
}

std::vector<Token> tokenize(const std::string& in, bool& ok) {
    ok = true;
    std::vector<Token> t;

    for (size_t i = 0; i < in.size();) {

        if (isspace(in[i])) { i++; continue; }

        if (isdigit(in[i]) || in[i]=='.') {
            std::stringstream ss;
            while (i < in.size() && (isdigit(in[i]) || in[i]=='.'))
                ss << in[i++];

            double v;
            ss >> v;
            t.push_back({Token::Number,v});
            continue;
        }

        if (in[i]=='x') {
            t.push_back({Token::VarX});
            i++;
            continue;
        }

        if (in[i]=='y') {
            t.push_back({Token::VarY});
            i++;
            continue;
        }

        if (isalpha(in[i])) {
            std::string s;
            while (i<in.size() && isalpha(in[i])) s+=in[i++];
            if (!isFunc(s)) {
                std::cout<<"Unknown function: "<<s<<"\n";
                ok=false;
                return {};
            }
            t.push_back({Token::Func,0,0,s});
            continue;
        }

        if (strchr("+-*/^",in[i])) {
            t.push_back({Token::Op,0,in[i++]});
            continue;
        }

        if (in[i]=='(') { t.push_back({Token::LParen}); i++; continue; }
        if (in[i]==')') { t.push_back({Token::RParen}); i++; continue; }

        std::cout<<"Invalid character: "<<in[i]<<"\n";
        ok=false;
        return {};
    }

    std::vector<Token> out;

    for (size_t i=0;i<t.size();i++) {

        out.push_back(t[i]);

        if (i+1<t.size()) {

            bool left =
                t[i].type==Token::Number ||
                t[i].type==Token::VarX ||
                t[i].type==Token::VarY ||
                t[i].type==Token::RParen;

            bool right =
                t[i+1].type==Token::VarX ||
                t[i+1].type==Token::VarY ||
                t[i+1].type==Token::Number ||
                t[i+1].type==Token::Func ||
                t[i+1].type==Token::LParen;

            if (left && right)
                out.push_back({Token::Op,0,'*'});
        }
    }

    return out;
}

int prec(char op) {
    if (op=='+'||op=='-') return 1;
    if (op=='*'||op=='/') return 2;
    if (op=='^') return 3;
    return 0;
}

double applyOp(double a,double b,char op,bool& ok) {
    switch(op){
        case '+': return a+b;
        case '-': return a-b;
        case '*': return a*b;
        case '/':
            if (b==0){ ok=false; return 0;}
            return a/b;
        case '^': return pow(a,b);
    }
    ok=false;
    return 0;
}

double applyFunc(const std::string& f,double v){
    if (f=="sin") return sin(v);
    if (f=="cos") return cos(v);
    if (f=="tan") return tan(v);
    return 0;
}

double evalTokens(const std::vector<Token>& tok,double x,double y,bool& ok) {

    std::stack<double> val;
    std::stack<Token> ops;
    ok=true;

    auto apply=[&](){

        Token op=ops.top(); ops.pop();

        if (op.type==Token::Func) {
            if(val.empty()){ok=false;return;}
            double v=val.top(); val.pop();
            val.push(applyFunc(op.func,v));
            return;
        }

        if (val.size()<2){ok=false;return;}

        double b=val.top(); val.pop();
        double a=val.top(); val.pop();

        val.push(applyOp(a,b,op.op,ok));
    };

    for (size_t i=0;i<tok.size();i++) {

        const Token& t=tok[i];

        if (t.type==Token::Number) val.push(t.value);
        else if (t.type==Token::VarX) val.push(x);
        else if (t.type==Token::VarY) val.push(y);

        else if (t.type==Token::Func) ops.push(t);

        else if (t.type==Token::Op) {

            while(!ops.empty() &&
                  ops.top().type==Token::Op &&
                  prec(ops.top().op)>=prec(t.op))
                apply();

            ops.push(t);
        }

        else if (t.type==Token::LParen)
            ops.push(t);

        else if (t.type==Token::RParen) {

            while(!ops.empty() && ops.top().type!=Token::LParen)
                apply();

            if (ops.empty()){
                std::cout<<"Mismatched parentheses\n";
                ok=false;
                return 0;
            }

            ops.pop();

            if (!ops.empty() && ops.top().type==Token::Func)
                apply();
        }
    }

    while(!ops.empty())
        apply();

    if (val.size()!=1){
        ok=false;
        return 0;
    }

    return val.top();
}

void RenderGraph(winhelpgui::UIElement& self) {
    GraphSurface.fill({25, 25, 25, 255});

    std::string eq = Input.text;

    size_t pos = eq.find('=');

    if (pos==std::string::npos) {
        std::cout<<"Equation must contain =\n";
        return;
    }

    std::string left = eq.substr(0,pos);
    std::string right = eq.substr(pos+1);

    bool ok;

    auto L = tokenize(left,ok);
    if(!ok) return;

    auto R = tokenize(right,ok);
    if(!ok) return;

    bool yForm = left.find('y')!=std::string::npos && right.find('y')==std::string::npos;

    for (int px=0; px<GraphSize.x; px++) {

        double x = ((double)px/GraphSize.x)*20.0 - 10.0;

        for (int py=0; py<GraphSize.y; py++) {

            double y = ((GraphSize.y-py)/(double)GraphSize.y)*20.0 - 10.0;

            double lv = evalTokens(L,x,y,ok);
            if(!ok){ std::cout<<"Eval error\n"; return; }

            double rv = evalTokens(R,x,y,ok);
            if(!ok){ std::cout<<"Eval error\n"; return; }

            if (fabs(lv-rv) < 0.05) {
                winhelp::draw::put_pixel(
                    GraphSurface,
                    px, py,
                    {255,255,255}
                );
            }
        }
    }
}

int main() {
    GraphSurface.fill({25, 25, 25, 255});

    Input.align = winhelpgui::Align::TopLeft;
    Input.fitToSize();
    Input.text = "";

    winhelpgui::TextButton GraphButton("Graph", {BufferSpace, (BufferSpace * 2) + InputSize.y}, {InputSize.x, 50}, {100, 255, 100}, {50, 100, 50});
    GraphButton.fitToSize();
    GraphButton.align = winhelpgui::Align::Middle;

    GraphButton.on_released(RenderGraph);

    while (1) {
        screen.surface.fill({40, 40, 40, 255});

        std::vector<winhelp::events::event> events = winhelp::events::get();

        for (auto& event : events) {
            if (event.type == winhelp::events::eventTypes::quit) {
                screen.close();
                return 0;
            }
        }

        Input.tick(screen.surface, events);
        GraphButton.tick(screen.surface, events);
        screen.surface.blit(GraphPos, GraphSurface);

        screen.flip();
        winhelp::tick();
    }
}