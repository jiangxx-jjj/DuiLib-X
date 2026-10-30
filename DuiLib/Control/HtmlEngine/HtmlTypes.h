#ifndef HTML_ENGINE_TYPES_H
#define HTML_ENGINE_TYPES_H

// ==========================================================================
//  HtmlTypes.h — 自研 HTML/CSS 引擎的基础类型
//
//  完全不依赖任何浏览器内核或第三方排版库：
//  本引擎自己解析 HTML5、解析 CSS3、计算布局、用 GDI+ 绘制。
// ==========================================================================

#include <windows.h>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <cmath>

namespace HtmlEngine
{

// ============================ 颜色 ============================

struct Color
{
    unsigned char r, g, b, a;

    Color() : r(0), g(0), b(0), a(0) {}
    Color(unsigned char R, unsigned char G, unsigned char B, unsigned char A = 255)
        : r(R), g(G), b(B), a(A) {}

    bool operator==(const Color& c) const
    {
        return r == c.r && g == c.g && b == c.b && a == c.a;
    }
    bool operator!=(const Color& c) const { return !(*this == c); }

    bool IsTransparent() const { return a == 0; }
    bool IsVisible() const { return a > 0; }

    COLORREF ToCOLORREF() const { return RGB(r, g, b); }

    // 解析 CSS 颜色：#fff / #ffffff / rgb() / rgba() / 颜色名
    static Color FromString(const std::string& s);
    // 判断字符串是否是合法颜色
    static bool IsColorString(const std::string& s);
    // 颜色名 → RGB
    static bool FromName(const std::string& name, Color& out);

    static Color Black()  { return Color(0, 0, 0, 255); }
    static Color White()  { return Color(255, 255, 255, 255); }
    static Color Transparent() { return Color(0, 0, 0, 0); }
};

// ============================ 长度 ============================

struct Length
{
    enum Unit { PX, PERCENT, EM, REM, PT, AUTO, NONE, VW, VH };

    float   value;
    Unit    unit;

    Length() : value(0.0f), unit(AUTO) {}
    Length(float v, Unit u) : value(v), unit(u) {}

    bool IsAuto()  const { return unit == AUTO || unit == NONE; }
    bool IsFixed() const { return unit == PX || unit == PT; }

    // 解析 "10px" / "50%" / "2em" / "auto" / "1.5rem"
    static Length FromString(const std::string& s);

    // 按基准解析为像素。base=参照宽度，fontSize=当前字号，rootFont=根字号
    float Resolve(float base, float fontSize, float rootFontSize) const;

    static Length Px(float v)        { return Length(v, PX); }
    static Length Percent(float v)   { return Length(v, PERCENT); }
    static Length Auto()             { return Length(0, AUTO); }
};

// ============================ 枚举 ============================

enum DisplayType
{
    DISPLAY_NONE,
    DISPLAY_BLOCK,
    DISPLAY_INLINE,
    DISPLAY_INLINE_BLOCK,
    DISPLAY_FLEX,
    DISPLAY_INLINE_FLEX,
    DISPLAY_LIST_ITEM,
    DISPLAY_TABLE,
    DISPLAY_TABLE_ROW,
    DISPLAY_TABLE_CELL,
    DISPLAY_TABLE_ROW_GROUP,
};

enum PositionType { POS_STATIC, POS_RELATIVE, POS_ABSOLUTE, POS_FIXED };

enum FloatType { FLOAT_NONE, FLOAT_LEFT, FLOAT_RIGHT };

enum ClearType { CLEAR_NONE, CLEAR_LEFT, CLEAR_RIGHT, CLEAR_BOTH };

enum TextAlign { ALIGN_LEFT, ALIGN_CENTER, ALIGN_RIGHT, ALIGN_JUSTIFY };

enum VerticalAlign { VALIGN_BASELINE, VALIGN_TOP, VALIGN_MIDDLE, VALIGN_BOTTOM };

enum BorderStyle { BORDER_NONE, BORDER_SOLID, BORDER_DASHED, BORDER_DOTTED, BORDER_DOUBLE, BORDER_GROOVE, BORDER_RIDGE };

enum OverflowType { OVERFLOW_VISIBLE, OVERFLOW_HIDDEN, OVERFLOW_AUTO, OVERFLOW_SCROLL };

enum WhiteSpaceType { WS_NORMAL, WS_NOWRAP, WS_PRE, WS_PRE_WRAP, WS_PRE_LINE };

enum FlexDirection { FLEX_ROW, FLEX_ROW_REVERSE, FLEX_COLUMN, FLEX_COLUMN_REVERSE };

enum FlexWrap { WRAP_NOWRAP, WRAP_WRAP, WRAP_WRAP_REVERSE };

enum JustifyContent { JUSTIFY_START, JUSTIFY_END, JUSTIFY_CENTER, JUSTIFY_SPACE_BETWEEN, JUSTIFY_SPACE_AROUND, JUSTIFY_SPACE_EVENLY };

enum AlignItems { ALIGN_ITEMS_STRETCH, ALIGN_ITEMS_START, ALIGN_ITEMS_END, ALIGN_ITEMS_CENTER, ALIGN_ITEMS_BASELINE };

enum ListStyleType { LIST_NONE, LIST_DISC, LIST_CIRCLE, LIST_SQUARE, LIST_DECIMAL, LIST_DECIMAL_ZERO };

enum FontStyle { FONT_NORMAL, FONT_ITALIC, FONT_OBLIQUE };

enum FontWeight { FONT_WEIGHT_NORMAL = 400, FONT_WEIGHT_BOLD = 700 };

enum BoxSizing { BOX_CONTENT, BOX_BORDER };

enum Visibility { VIS_VISIBLE, VIS_HIDDEN, VIS_COLLAPSE };

// ============================ 阴影 ============================

struct Shadow
{
    float   x, y;
    float   blur;
    float   spread;
    Color   color;
    bool    inset;

    Shadow() : x(0), y(0), blur(0), spread(0), color(0, 0, 0, 128), inset(false) {}
};

// ============================ 渐变 ============================

struct GradientStop
{
    Color   color;
    float   pos;        // 0.0 ~ 1.0
    bool    hasPos;

    GradientStop() : pos(0.0f), hasPos(false) {}
};

struct Gradient
{
    bool                    radial;
    float                   angle;      // 线性渐变角度（度）
    std::vector<GradientStop> stops;

    Gradient() : radial(false), angle(180.0f) {}

    bool IsEmpty() const { return stops.empty(); }
    void Clear() { stops.clear(); radial = false; angle = 180.0f; }
};

// ============================ 背景图片 ============================

struct BackgroundImage
{
    enum Type { NONE, URL, GRADIENT };

    Type        type;
    std::string url;
    Gradient    gradient;
    bool        repeatX;
    bool        repeatY;

    BackgroundImage() : type(NONE), repeatX(true), repeatY(true) {}

    bool IsEmpty() const { return type == NONE; }
    void Clear() { type = NONE; url.clear(); gradient.Clear(); }
};

// ============================ 边框 ============================

struct BorderSide
{
    float       width;
    BorderStyle style;
    Color       color;

    BorderSide() : width(0.0f), style(BORDER_NONE), color(0, 0, 0, 0) {}
    bool IsVisible() const { return width > 0.0f && style != BORDER_NONE && color.a > 0; }
};

struct BorderRadius
{
    float tl, tr, br, bl;      // 四个角半径（水平方向）
    float tlv, trv, brv, blv;  // 四个角半径（垂直方向）

    BorderRadius() : tl(0), tr(0), br(0), bl(0), tlv(0), trv(0), brv(0), blv(0) {}

    bool IsZero() const { return tl == 0 && tr == 0 && br == 0 && bl == 0; }
    bool HasValue() const { return !IsZero(); }
};

// ============================ 工具函数 ============================

std::string Trim(const std::string& s);
std::string ToLower(const std::string& s);
std::vector<std::string> Split(const std::string& s, char delim);
// 按分隔符切分，跳过括号内的分隔符（用于 linear-gradient(a, b) 这类）
std::vector<std::string> SplitTopLevel(const std::string& s, char delim);
std::string ReplaceAll(const std::string& s, const std::string& from, const std::string& to);

// 数字格式化（避免 sprintf 的 locale 问题）
std::string FloatToString(float v);

// 字符串转宽字符 / 宽字符转 UTF-8
std::wstring Utf8ToWide(const std::string& s);
std::string  WideToUtf8(const std::wstring& s);

} // namespace HtmlEngine

#endif // HTML_ENGINE_TYPES_H
