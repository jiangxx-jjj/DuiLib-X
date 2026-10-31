#include "StdAfx.h"
#include "el_form.h"
#include "container_win32.h"
#include "document.h"
#include <gdiplus.h>

namespace Gp = Gdiplus;

namespace litehtml
{

// ============================ 基类 ============================

el_form_control::el_form_control(const document::ptr& doc, container_win32* container,
                                 const char* displayStyle)
    : html_tag(doc), m_container(container), m_focused(false)
{
    m_style.add_property(_display_, displayStyle);
}

position el_form_control::GetDrawPos(int x, int y, const std::shared_ptr<render_item>& ri) const
{
    position pos = ri->pos();
    pos.x += x;
    pos.y += y;
    pos += ri->get_paddings();
    pos += ri->get_borders();
    return pos;
}

size_t el_form_control::Utf8CharLen(const std::string& s, size_t bytePos)
{
    if (bytePos >= s.size()) return 0;
    unsigned char c = (unsigned char)s[bytePos];
    if ((c & 0x80) == 0) return 1;
    if ((c & 0xE0) == 0xC0) return 2;
    if ((c & 0xF0) == 0xE0) return 3;
    if ((c & 0xF8) == 0xF0) return 4;
    return 1;
}

size_t el_form_control::Utf8BytePos(const std::string& s, size_t charIdx)
{
    size_t bytePos = 0, c = 0;
    while (bytePos < s.size() && c < charIdx) {
        bytePos += Utf8CharLen(s, bytePos);
        c++;
    }
    return bytePos;
}

size_t el_form_control::Utf8CharCount(const std::string& s)
{
    size_t count = 0;
    for (size_t i = 0; i < s.size(); i += Utf8CharLen(s, i)) count++;
    return count;
}

// 关键：replaced 元素需要 render_item_image 才会在排版时调用 get_content_size
std::shared_ptr<render_item> el_form_control::create_render_item(const std::shared_ptr<render_item>& parent_ri)
{
    auto ret = std::make_shared<render_item_image>(shared_from_this());
    ret->parent(parent_ri);
    return ret;
}

// 取元素已计算的字体句柄（litehtml 在 compute_styles 时已创建）
static uint_ptr GetElementFont(html_tag* el)
{
    return el->css().get_font();
}

// 取 Gp 字体对象
static Gp::Font* GetGpFontOf(html_tag* el, container_win32* container)
{
    if (!container) return NULL;
    return container->GetGpFont(GetElementFont(el));
}

// 取元素文字颜色
static Gp::Color GetTextColor(html_tag* el)
{
    web_color c = el->css().get_color();
    return Gp::Color(c.alpha, c.red, c.green, c.blue);
}

// ============================ 文本输入框 ============================

el_input_text::el_input_text(const document::ptr& doc, container_win32* container, const char* type)
    : el_form_control(doc, container, "inline-block")
    , m_type(type ? type : "text")
    , m_caretPos(0)
{
}

void el_input_text::parse_attributes()
{
    const char* val = get_attr("value");
    if (val) m_value = val;
    m_caretPos = Utf8CharCount(m_value);

    // width/height 属性
    const char* aw = get_attr("width");
    if (aw) m_style.add_property(_width_, aw);
    const char* ah = get_attr("height");
    if (ah) m_style.add_property(_height_, ah);
}

void el_input_text::get_content_size(size& sz, int max_width)
{
    sz.width = 200;
    sz.height = 26;
}

void el_input_text::draw(uint_ptr hdc, int x, int y, const position* clip,
    const std::shared_ptr<render_item>& ri)
{
    HDC dc = (HDC)hdc;
    if (!dc) return;

    position pos = GetDrawPos(x, y, ri);
    if (!pos.does_intersect(clip)) return;

    // 背景与边框用 litehtml 标准绘制（CSS 可覆盖）
    html_tag::draw(hdc, x, y, clip, ri);

    Gp::Graphics g(dc);
    g.SetSmoothingMode(Gp::SmoothingModeAntiAlias);
    g.SetTextRenderingHint(Gp::TextRenderingHintAntiAlias);

    // 无 CSS 背景/边框时画默认白底 + 灰边框
    const background* pBg = get_background();
    bool hasBorder = css().get_borders().is_visible();
    if (!pBg && !hasBorder) {
        Gp::SolidBrush whiteBrush(Gp::Color(255, 255, 255, 255));
        g.FillRectangle(&whiteBrush, (Gp::REAL)pos.x, (Gp::REAL)pos.y,
                        (Gp::REAL)pos.width, (Gp::REAL)pos.height);
        Gp::Pen borderPen(Gp::Color(255, 170, 170, 170), 1.0f);
        g.DrawRectangle(&borderPen, (Gp::REAL)pos.x, (Gp::REAL)pos.y,
                        (Gp::REAL)(pos.width - 1), (Gp::REAL)(pos.height - 1));
    }

    // 显示文本（密码打码：按字符数打码）
    std::string displayText = m_value;
    if (m_type == "password" && !displayText.empty()) {
        displayText.assign(Utf8CharCount(displayText), '*');
    }
    if (displayText.empty()) return;

    Gp::Font* pFont = GetGpFontOf(this, m_container);
    if (!pFont) return;

    Gp::SolidBrush brush(GetTextColor(this));
    std::wstring wText;
    int n = ::MultiByteToWideChar(CP_UTF8, 0, displayText.c_str(), (int)displayText.size(), NULL, 0);
    if (n > 0) {
        wText.resize(n);
        ::MultiByteToWideChar(CP_UTF8, 0, displayText.c_str(), (int)displayText.size(), &wText[0], n);
    }

    Gp::RectF rc((Gp::REAL)(pos.x + 5), (Gp::REAL)pos.y,
                 (Gp::REAL)(pos.width - 10), (Gp::REAL)pos.height);
    Gp::StringFormat fmt;
    fmt.SetLineAlignment(Gp::StringAlignmentCenter);
    fmt.SetTrimming(Gp::StringTrimmingEllipsisCharacter);
    g.DrawString(wText.c_str(), (INT)wText.size(), pFont, rc, &fmt, &brush);

    // 焦点光标
    if (m_focused) {
        size_t caretByte = Utf8BytePos(displayText, m_caretPos);
        float caretX = (float)(pos.x + 5);
        if (caretByte > 0) {
            std::string beforeCaret = displayText.substr(0, caretByte);
            std::wstring wBefore;
            int nb = ::MultiByteToWideChar(CP_UTF8, 0, beforeCaret.c_str(), (int)beforeCaret.size(), NULL, 0);
            if (nb > 0) {
                wBefore.resize(nb);
                ::MultiByteToWideChar(CP_UTF8, 0, beforeCaret.c_str(), (int)beforeCaret.size(), &wBefore[0], nb);
            }
            if (!wBefore.empty()) {
                Gp::RectF bound;
                Gp::StringFormat mfmt;
                mfmt.SetFormatFlags(Gp::StringFormatFlagsMeasureTrailingSpaces | Gp::StringFormatFlagsNoWrap);
                g.MeasureString(wBefore.c_str(), (INT)wBefore.size(), pFont,
                                Gp::PointF(0, 0), &mfmt, &bound);
                caretX += bound.Width;
            }
        }
        Gp::Pen caretPen(Gp::Color(255, 30, 30, 30), 1.0f);
        g.DrawLine(&caretPen, caretX, (Gp::REAL)(pos.y + 4),
                   caretX, (Gp::REAL)(pos.y + pos.height - 4));
    }
}

void el_input_text::on_click()
{
    m_focused = true;
    m_caretPos = Utf8CharCount(m_value);
    if (m_container) m_container->SetFocusElement(shared_from_this());
}

void el_input_text::SetValue(const std::string& v)
{
    m_value = v;
    m_caretPos = Utf8CharCount(m_value);
}

bool el_input_text::InputChar(wchar_t ch)
{
    if (ch < 32) return false;
    size_t caretByte = Utf8BytePos(m_value, m_caretPos);
    std::string insert;
    if (ch > 127) {
        std::wstring w(1, ch);
        int n = ::WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), NULL, 0, NULL, NULL);
        if (n > 0) { insert.resize(n); ::WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), &insert[0], n, NULL, NULL); }
    } else {
        insert = (char)ch;
    }
    m_value.insert(caretByte, insert);
    m_caretPos++;
    if (m_container) m_container->OnValueChanged(GetName());
    return true;
}

bool el_input_text::InputKeyDown(int vkCode)
{
    size_t charCount = Utf8CharCount(m_value);

    if (vkCode == VK_BACK) {
        if (m_caretPos > 0) {
            size_t bytePos = Utf8BytePos(m_value, m_caretPos);
            size_t prevByte = Utf8BytePos(m_value, m_caretPos - 1);
            m_value.erase(prevByte, bytePos - prevByte);
            m_caretPos--;
        }
        if (m_container) m_container->OnValueChanged(GetName());
        return true;
    }
    if (vkCode == VK_DELETE) {
        if (m_caretPos < charCount) {
            size_t bytePos = Utf8BytePos(m_value, m_caretPos);
            size_t nextByte = Utf8BytePos(m_value, m_caretPos + 1);
            m_value.erase(bytePos, nextByte - bytePos);
        }
        if (m_container) m_container->OnValueChanged(GetName());
        return true;
    }
    if (vkCode == VK_LEFT)  { if (m_caretPos > 0) m_caretPos--; return true; }
    if (vkCode == VK_RIGHT) { if (m_caretPos < charCount) m_caretPos++; return true; }
    if (vkCode == VK_HOME)  { m_caretPos = 0; return true; }
    if (vkCode == VK_END)   { m_caretPos = charCount; return true; }
    return false;
}

// ============================ 勾选框 / 单选框 ============================

el_input_check::el_input_check(const document::ptr& doc, container_win32* container, bool isRadio)
    : el_form_control(doc, container, "inline-block")
    , m_checked(false), m_isRadio(isRadio)
{
}

// 注意：checked 属性在 parse_attributes 里读（此时 attrs 已挂载）

void el_input_check::get_content_size(size& sz, int max_width)
{
    sz.width = 14;
    sz.height = 14;
}

void el_input_check::draw(uint_ptr hdc, int x, int y, const position* clip,
    const std::shared_ptr<render_item>& ri)
{
    HDC dc = (HDC)hdc;
    if (!dc) return;

    position pos = GetDrawPos(x, y, ri);
    if (!pos.does_intersect(clip)) return;

    Gp::Graphics g(dc);
    g.SetSmoothingMode(Gp::SmoothingModeAntiAlias);

    float size = (pos.width < pos.height) ? (float)pos.width : (float)pos.height;
    if (size > 14) size = 14;
    float cx = pos.x + (pos.width - size) / 2;
    float cy = pos.y + (pos.height - size) / 2;

    if (m_isRadio) {
        Gp::SolidBrush bgBrush(Gp::Color(255, 255, 255, 255));
        g.FillEllipse(&bgBrush, cx, cy, size, size);
        Gp::Pen border(Gp::Color(255, 160, 160, 160), 1.0f);
        g.DrawEllipse(&border, cx, cy, size, size);
        if (m_checked) {
            Gp::SolidBrush dot(Gp::Color(255, 45, 91, 215));
            float d = size * 0.55f;
            g.FillEllipse(&dot, cx + (size - d) / 2, cy + (size - d) / 2, d, d);
        }
    } else {
        Gp::SolidBrush bgBrush(Gp::Color(255, 255, 255, 255));
        g.FillRectangle(&bgBrush, cx, cy, size, size);
        Gp::Pen border(Gp::Color(255, 160, 160, 160), 1.0f);
        g.DrawRectangle(&border, cx, cy, size, size);
        if (m_checked) {
            Gp::Pen tick(Gp::Color(255, 45, 91, 215), 2.0f);
            g.DrawLine(&tick, cx + size * 0.18f, cy + size * 0.5f,
                       cx + size * 0.4f, cy + size * 0.72f);
            g.DrawLine(&tick, cx + size * 0.4f, cy + size * 0.72f,
                       cx + size * 0.82f, cy + size * 0.26f);
        }
    }
}

void el_input_check::on_click()
{
    if (m_isRadio) {
        m_checked = true;
        if (m_container) {
            m_container->OnRadioClicked(shared_from_this());
        }
    } else {
        m_checked = !m_checked;
    }
    if (m_container) m_container->OnValueChanged(GetName());
}

void el_input_check::parse_attributes()
{
    // checked 属性（parse_attributes 时 attrs 已挂载）
    if (get_attr("checked")) m_checked = true;
}

// ============================ 按钮 ============================

el_button::el_button(const document::ptr& doc, container_win32* container)
    : el_form_control(doc, container, "inline-block")
{
}

// 收集元素内可见文字（跳过 display:none 的子元素）
// 目的：<button><svg/><span>文字</span></button> 这种结构里，
// CSS 用 @media 把 span 设为 display:none 时，按钮不应再把这段文字画出来
// （否则窄窗口下按钮只剩 30px，文字被挤成竖排，看起来像排版塌陷）
// 返回值：true 表示按钮有子元素（此时以子元素可见文字为准，不再回退整段文字）
static bool CollectVisibleText(const litehtml::element::ptr& el, std::string& out)
{
    bool hasChild = false;
    if (!el) return hasChild;
    for (const auto& child : el->children()) {
        if (!child) continue;
        const char* tag = child->get_tagName();
        if (!tag) continue;
        if (_stricmp(tag, "svg") == 0) { hasChild = true; continue; }   // 图标单独绘制
        hasChild = true;
        if (child->css().get_display() == litehtml::display_none) continue;
        litehtml::string t;
        child->get_text(t);
        if (!t.empty()) out += t;
    }
    return hasChild;
}

void el_button::get_content_size(size& sz, int max_width)
{
    std::string label;
    const char* val = get_attr("value");
    if (val) {
        label = val;
    } else {
        // 优先取可见子元素文字，没有可见文字就不画（避免把 display:none 的文字画出来）
        CollectVisibleText(shared_from_this(), label);
    }
    m_label = label;

    int w = 0;
    if (m_container && !label.empty()) {
        uint_ptr hFont = css().get_font();
        if (!hFont) {
            font_metrics fm;
            hFont = get_document()->get_font(NULL, css().get_font_size(), "normal", "normal", "", &fm);
        }
        w = m_container->text_width(label.c_str(), hFont);
    }
    sz.width = w + 24;
    sz.height = 28;
}

// 前置声明：SVG 绘制核心实现在文件后部（el_svg 区域），按钮绘制图标时复用
static void DrawSvgInRect(Gp::Graphics& g, const litehtml::element::ptr& svgEl,
                          const Gp::RectF& rect);

// 统计按钮内可见的 <svg> 图标个数
static int CountVisibleIcons(const litehtml::element::ptr& btn)
{
    int cnt = 0;
    if (!btn) return cnt;
    for (const auto& child : btn->children()) {
        if (!child) continue;
        const char* tag = child->get_tagName();
        if (!tag || _stricmp(tag, "svg") != 0) continue;
        if (child->css().get_display() == litehtml::display_none) continue;
        cnt++;
    }
    return cnt;
}

// 取按钮内图标的绘制尺寸（按 svg 的 CSS width，缺省 16px）
static float ButtonIconSize(const litehtml::element::ptr& btn)
{
    if (!btn) return 16.0f;
    for (const auto& child : btn->children()) {
        if (!child) continue;
        const char* tag = child->get_tagName();
        if (!tag || _stricmp(tag, "svg") != 0) continue;
        if (child->css().get_display() == litehtml::display_none) continue;
        const litehtml::css_length& w = child->css().get_width();
        if (!w.is_predefined() && w.val() > 0) return (float)w.val();
        break;
    }
    return 16.0f;
}

void el_button::draw(uint_ptr hdc, int x, int y, const position* clip,
    const std::shared_ptr<render_item>& ri)
{
    HDC dc = (HDC)hdc;
    if (!dc) return;

    position pos = GetDrawPos(x, y, ri);
    if (!pos.does_intersect(clip)) return;

    const background* pBg = get_background();
    bool hasBorder = css().get_borders().is_visible();

    Gp::Graphics g(dc);
    g.SetSmoothingMode(Gp::SmoothingModeAntiAlias);

    if (!pBg && !hasBorder) {
        // 无 CSS 时画默认按钮底（3px 圆角灰底）
        Gp::SolidBrush bgBrush(Gp::Color(255, 240, 240, 240));
        Gp::GraphicsPath path;
        litehtml::border_radiuses br;
        br.top_left_x = br.top_left_y = 3;
        br.top_right_x = br.top_right_y = 3;
        br.bottom_right_x = br.bottom_right_y = 3;
        br.bottom_left_x = br.bottom_left_y = 3;
        container_win32::BuildRoundRectPath(path, pos.x, pos.y, pos.width, pos.height, br);
        g.FillPath(&bgBrush, &path);
        Gp::Pen borderPen(Gp::Color(255, 180, 180, 180), 1.0f);
        g.DrawPath(&borderPen, &path);
    } else {
        html_tag::draw(hdc, x, y, clip, ri);
    }

    Gp::RectF rc((Gp::REAL)pos.x, (Gp::REAL)pos.y, (Gp::REAL)pos.width, (Gp::REAL)pos.height);

    // 2) 取文字（与 get_content_size 一致：只取可见子元素文字）
    std::string label;
    const char* val = get_attr("value");
    if (val) label = val;
    if (label.empty()) {
        if (!CollectVisibleText(shared_from_this(), label)) {
            // 没有任何子元素（<button value="..."> 或纯文本按钮）才回退整段文字
            string text;
            get_text(text);
            label = text;
        }
    }

    Gp::Font* pFont = GetGpFontOf(this, m_container);
    std::wstring wText;
    if (!label.empty() && pFont) {
        int n = ::MultiByteToWideChar(CP_UTF8, 0, label.c_str(), (int)label.size(), NULL, 0);
        if (n > 0) {
            wText.resize(n);
            ::MultiByteToWideChar(CP_UTF8, 0, label.c_str(), (int)label.size(), &wText[0], n);
        }
    }

    // 1) 布局：图标 + 文字作为整体在按钮内水平居中（近似 flex:center + gap）
    float iconSize = ButtonIconSize(shared_from_this());
    int   iconCount = CountVisibleIcons(shared_from_this());
    float gap = 6.0f;
    float iconsW = (iconCount > 0) ? (iconSize * (float)iconCount + gap * (float)iconCount) : 0.0f;

    // 测量文字宽度，用于整体居中
    float textW = 0.0f;
    if (!wText.empty() && pFont) {
        Gp::RectF measured;
        g.MeasureString(wText.c_str(), (INT)wText.size(), pFont, Gp::PointF(0, 0), &measured);
        textW = measured.Width;
    }

    float totalW = iconsW + textW;
    float startX = rc.X + (rc.Width - totalW) * 0.5f;
    if (startX < rc.X) startX = rc.X;                   // 放不下时左对齐，避免溢出

    // 2) 画图标
    if (iconCount > 0) {
        float x0 = startX;
        const float y0 = rc.Y + (rc.Height - iconSize) * 0.5f;
        for (const auto& child : children()) {
            if (!child) continue;
            const char* tag = child->get_tagName();
            if (!tag || _stricmp(tag, "svg") != 0) continue;
            if (child->css().get_display() == litehtml::display_none) continue;
            Gp::RectF irect(x0, y0, iconSize, iconSize);
            DrawSvgInRect(g, child, irect);
            x0 += iconSize + gap;
        }
    }

    // 3) 画文字
    if (!wText.empty() && pFont) {
        g.SetTextRenderingHint(Gp::TextRenderingHintAntiAlias);
        Gp::SolidBrush brush(GetTextColor(this));
        Gp::RectF textRc(startX + iconsW, rc.Y, textW + 2.0f, rc.Height);
        if (textRc.Width > rc.Width) textRc.Width = rc.Width;
        Gp::StringFormat fmt;
        fmt.SetAlignment(Gp::StringAlignmentNear);
        fmt.SetLineAlignment(Gp::StringAlignmentCenter);
        // 关键：禁止自动换行。按钮很窄时（如侧边栏图标模式下只剩 30px），
        // 默认换行会把「暗色模式」拆成一列竖排文字，看起来像排版塌陷。
        // 这里改为单行 + 省略号，超出部分截断。
        fmt.SetFormatFlags(Gp::StringFormatFlagsNoWrap);
        fmt.SetTrimming(Gp::StringTrimmingEllipsisCharacter);
        g.DrawString(wText.c_str(), (INT)wText.size(), pFont, textRc, &fmt, &brush);
    }
}

void el_button::on_click()
{
    if (m_container) m_container->OnButtonClicked(GetName());
}

// ============================ 下拉框 ============================

el_select::el_select(const document::ptr& doc, container_win32* container)
    : el_form_control(doc, container, "inline-block")
    , m_selectedIndex(0), m_optionsCollected(false)
{
}

void el_select::parse_attributes()
{
    // 注意：parse_attributes 时子节点（option）还没挂上来，
    // selected 初始化放在首次 draw（DrawOptionsIfNeeded）
    const char* aw = get_attr("width");
    if (aw) m_style.add_property(_width_, aw);
    const char* ah = get_attr("height");
    if (ah) m_style.add_property(_height_, ah);
}

// 收集 option 选项（render/draw 时都会调用；含 selected 初始化，只做一次）
void el_select::CollectOptions()
{
    if (m_optionsCollected) return;
    m_optionsCollected = true;
    m_options.clear();
    int idx = 0;
    for (const auto& child : m_children) {
        if (child->get_tagName() && _stricmp(child->get_tagName(), "option") == 0) {
            string text;
            child->get_text(text);
            m_options.push_back(text);
            if (child->get_attr("selected")) m_selectedIndex = idx;
            idx++;
        }
    }
}

void el_select::get_content_size(size& sz, int max_width)
{
    CollectOptions();   // render 时子节点已挂载，在这里完成初始化
    sz.width = 160;
    sz.height = 26;
}

void el_select::draw(uint_ptr hdc, int x, int y, const position* clip,
    const std::shared_ptr<render_item>& ri)
{
    HDC dc = (HDC)hdc;
    if (!dc) return;

    position pos = GetDrawPos(x, y, ri);
    if (!pos.does_intersect(clip)) return;

    CollectOptions();

    html_tag::draw(hdc, x, y, clip, ri);

    const background* pBg = get_background();
    bool hasBorder = css().get_borders().is_visible();
    Gp::Graphics g(dc);
    g.SetSmoothingMode(Gp::SmoothingModeAntiAlias);
    g.SetTextRenderingHint(Gp::TextRenderingHintAntiAlias);

    if (!pBg && !hasBorder) {
        Gp::SolidBrush whiteBrush(Gp::Color(255, 255, 255, 255));
        g.FillRectangle(&whiteBrush, (Gp::REAL)pos.x, (Gp::REAL)pos.y,
                        (Gp::REAL)pos.width, (Gp::REAL)pos.height);
        Gp::Pen borderPen(Gp::Color(255, 170, 170, 170), 1.0f);
        g.DrawRectangle(&borderPen, (Gp::REAL)pos.x, (Gp::REAL)pos.y,
                        (Gp::REAL)(pos.width - 1), (Gp::REAL)(pos.height - 1));
    }

    // 选中项文本
    std::string selText;
    if (!m_options.empty() && m_selectedIndex >= 0 &&
        m_selectedIndex < (int)m_options.size()) {
        selText = m_options[m_selectedIndex];
    }

    if (!selText.empty()) {
        Gp::Font* pFont = GetGpFontOf(this, m_container);
        if (pFont) {
            Gp::SolidBrush brush(GetTextColor(this));
            std::wstring wText;
            int n = ::MultiByteToWideChar(CP_UTF8, 0, selText.c_str(), (int)selText.size(), NULL, 0);
            if (n > 0) {
                wText.resize(n);
                ::MultiByteToWideChar(CP_UTF8, 0, selText.c_str(), (int)selText.size(), &wText[0], n);
            }
            Gp::RectF rc((Gp::REAL)(pos.x + 6), (Gp::REAL)pos.y,
                         (Gp::REAL)(pos.width - 24), (Gp::REAL)pos.height);
            Gp::StringFormat fmt;
            fmt.SetLineAlignment(Gp::StringAlignmentCenter);
            fmt.SetTrimming(Gp::StringTrimmingEllipsisCharacter);
            g.DrawString(wText.c_str(), (INT)wText.size(), pFont, rc, &fmt, &brush);
        }
    }

    // 下拉箭头
    Gp::SolidBrush arrow(Gp::Color(255, 110, 120, 140));
    float ax = pos.x + pos.width - 16;
    float ay = pos.y + pos.height / 2 - 2;
    Gp::PointF pts[3] = {
        Gp::PointF(ax, ay), Gp::PointF(ax + 9, ay), Gp::PointF(ax + 4.5f, ay + 6)
    };
    g.FillPolygon(&arrow, pts, 3);
}

void el_select::on_click()
{
    if (!m_options.empty()) {
        m_selectedIndex = (m_selectedIndex + 1) % (int)m_options.size();
        if (m_container) m_container->OnValueChanged(GetName());
    }
}

void el_select::SetSelectedIndex(int idx)
{
    if (m_options.empty()) return;
    if (idx < 0) idx = 0;
    if (idx >= (int)m_options.size()) idx = (int)m_options.size() - 1;
    m_selectedIndex = idx;
}

std::string el_select::GetValue() const
{
    if (m_options.empty() || m_selectedIndex < 0 ||
        m_selectedIndex >= (int)m_options.size()) return "";
    return m_options[m_selectedIndex];
}

// ============================ 多行文本框 ============================

el_textarea::el_textarea(const document::ptr& doc, container_win32* container)
    : el_form_control(doc, container, "inline-block")
    , m_caretPos(0)
{
}

void el_textarea::parse_attributes()
{
    // 注意：此时子文本可能还没挂上来，真实内容在 compute_styles 后才齐
    // 这里先取一次，draw 前的 parse 已由 litehtml 保证子节点就绪
    string text;
    get_text(text);
    m_value = text;
    size_t start = m_value.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) { m_value.clear(); }
    else {
        size_t end = m_value.find_last_not_of(" \t\r\n");
        m_value = m_value.substr(start, end - start + 1);
    }
    m_caretPos = Utf8CharCount(m_value);

    const char* aw = get_attr("width");
    if (aw) m_style.add_property(_width_, aw);
    const char* ah = get_attr("height");
    if (ah) {
        m_style.add_property(_height_, ah);
    } else {
        const char* rows = get_attr("rows");
        int h = 80;
        if (rows) h = atoi(rows) * 18 + 8;
        m_style.add_property(_height_, std::to_string(h).c_str());
    }
}

void el_textarea::get_content_size(size& sz, int max_width)
{
    sz.width = 260;
    sz.height = 80;
}

void el_textarea::draw(uint_ptr hdc, int x, int y, const position* clip,
    const std::shared_ptr<render_item>& ri)
{
    HDC dc = (HDC)hdc;
    if (!dc) return;

    position pos = GetDrawPos(x, y, ri);
    if (!pos.does_intersect(clip)) return;

    html_tag::draw(hdc, x, y, clip, ri);

    const background* pBg = get_background();
    bool hasBorder = css().get_borders().is_visible();
    Gp::Graphics g(dc);
    g.SetSmoothingMode(Gp::SmoothingModeAntiAlias);
    g.SetTextRenderingHint(Gp::TextRenderingHintAntiAlias);

    if (!pBg && !hasBorder) {
        Gp::SolidBrush whiteBrush(Gp::Color(255, 255, 255, 255));
        g.FillRectangle(&whiteBrush, (Gp::REAL)pos.x, (Gp::REAL)pos.y,
                        (Gp::REAL)pos.width, (Gp::REAL)pos.height);
        Gp::Pen borderPen(Gp::Color(255, 170, 170, 170), 1.0f);
        g.DrawRectangle(&borderPen, (Gp::REAL)pos.x, (Gp::REAL)pos.y,
                        (Gp::REAL)(pos.width - 1), (Gp::REAL)(pos.height - 1));
    }

    if (m_value.empty()) return;

    Gp::Font* pFont = GetGpFontOf(this, m_container);
    if (!pFont) return;

    Gp::SolidBrush brush(GetTextColor(this));
    std::wstring wText;
    int n = ::MultiByteToWideChar(CP_UTF8, 0, m_value.c_str(), (int)m_value.size(), NULL, 0);
    if (n > 0) {
        wText.resize(n);
        ::MultiByteToWideChar(CP_UTF8, 0, m_value.c_str(), (int)m_value.size(), &wText[0], n);
    }
    Gp::RectF rc((Gp::REAL)(pos.x + 5), (Gp::REAL)(pos.y + 4),
                 (Gp::REAL)(pos.width - 10), (Gp::REAL)(pos.height - 8));
    g.DrawString(wText.c_str(), (INT)wText.size(), pFont, rc, NULL, &brush);
}

void el_textarea::on_click()
{
    m_focused = true;
    m_caretPos = Utf8CharCount(m_value);
    if (m_container) m_container->SetFocusElement(shared_from_this());
}

void el_textarea::SetValue(const std::string& v)
{
    m_value = v;
    m_caretPos = Utf8CharCount(m_value);
}

bool el_textarea::InputChar(wchar_t ch)
{
    if (ch < 32) return false;
    size_t caretByte = Utf8BytePos(m_value, m_caretPos);
    std::string insert;
    if (ch > 127) {
        std::wstring w(1, ch);
        int n = ::WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), NULL, 0, NULL, NULL);
        if (n > 0) { insert.resize(n); ::WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), &insert[0], n, NULL, NULL); }
    } else {
        insert = (char)ch;
    }
    m_value.insert(caretByte, insert);
    m_caretPos++;
    if (m_container) m_container->OnValueChanged(GetName());
    return true;
}

bool el_textarea::InputKeyDown(int vkCode)
{
    size_t charCount = Utf8CharCount(m_value);

    if (vkCode == VK_BACK) {
        if (m_caretPos > 0) {
            size_t bytePos = Utf8BytePos(m_value, m_caretPos);
            size_t prevByte = Utf8BytePos(m_value, m_caretPos - 1);
            m_value.erase(prevByte, bytePos - prevByte);
            m_caretPos--;
        }
        if (m_container) m_container->OnValueChanged(GetName());
        return true;
    }
    if (vkCode == VK_DELETE) {
        if (m_caretPos < charCount) {
            size_t bytePos = Utf8BytePos(m_value, m_caretPos);
            size_t nextByte = Utf8BytePos(m_value, m_caretPos + 1);
            m_value.erase(bytePos, nextByte - bytePos);
        }
        if (m_container) m_container->OnValueChanged(GetName());
        return true;
    }
    if (vkCode == VK_LEFT)  { if (m_caretPos > 0) m_caretPos--; return true; }
    if (vkCode == VK_RIGHT) { if (m_caretPos < charCount) m_caretPos++; return true; }
    if (vkCode == VK_HOME)  { m_caretPos = 0; return true; }
    if (vkCode == VK_END)   { m_caretPos = charCount; return true; }
    return false;
}


el_media_placeholder::el_media_placeholder(const document::ptr& doc, container_win32* container,
                                           const char* tag)
    : el_form_control(doc, container, "inline-block")
    , m_tagName(tag ? tag : "media")
{
}

void el_media_placeholder::get_content_size(size& sz, int max_width)
{
    sz.width = 300;
    sz.height = 150;
}

void el_media_placeholder::draw(uint_ptr hdc, int x, int y, const position* clip,
    const std::shared_ptr<render_item>& ri)
{
    HDC dc = (HDC)hdc;
    if (!dc) return;

    position pos = GetDrawPos(x, y, ri);
    if (!pos.does_intersect(clip)) return;

    Gp::Graphics g(dc);
    g.SetSmoothingMode(Gp::SmoothingModeAntiAlias);
    g.SetTextRenderingHint(Gp::TextRenderingHintAntiAlias);

    Gp::SolidBrush bgBrush(Gp::Color(255, 240, 240, 242));
    g.FillRectangle(&bgBrush, (Gp::REAL)pos.x, (Gp::REAL)pos.y,
                    (Gp::REAL)pos.width, (Gp::REAL)pos.height);

    Gp::Pen borderPen(Gp::Color(255, 200, 202, 208), 1.0f);
    borderPen.SetDashStyle(Gp::DashStyleDash);
    g.DrawRectangle(&borderPen, (Gp::REAL)pos.x, (Gp::REAL)pos.y,
                    (Gp::REAL)(pos.width - 1), (Gp::REAL)(pos.height - 1));

    std::string hint = "<" + m_tagName + "> not supported";
    Gp::Font* pFont = GetGpFontOf(this, m_container);
    if (pFont) {
        Gp::SolidBrush txtBrush(Gp::Color(255, 140, 142, 150));
        std::wstring wHint;
        int n = ::MultiByteToWideChar(CP_UTF8, 0, hint.c_str(), (int)hint.size(), NULL, 0);
        if (n > 0) {
            wHint.resize(n);
            ::MultiByteToWideChar(CP_UTF8, 0, hint.c_str(), (int)hint.size(), &wHint[0], n);
        }
        Gp::RectF rc((Gp::REAL)pos.x, (Gp::REAL)pos.y, (Gp::REAL)pos.width, (Gp::REAL)pos.height);
        Gp::StringFormat fmt;
        fmt.SetAlignment(Gp::StringAlignmentCenter);
        fmt.SetLineAlignment(Gp::StringAlignmentCenter);
        g.DrawString(wHint.c_str(), (INT)wHint.size(), pFont, rc, &fmt, &txtBrush);
    }
}

// ============================ 音频 ============================

el_audio::el_audio(const document::ptr& doc, container_win32* container)
    : el_form_control(doc, container, "inline-block")
    , m_playing(false)
    , m_mciAlias("")
{
    static int s_audioSeq = 0;
    char buf[32];
    sprintf_s(buf, "duiaudio%d", ++s_audioSeq);
    m_mciAlias = buf;
}

void el_audio::parse_attributes()
{
    const char* src = get_attr("src");
    if (src) m_src = src;

    const char* aw = get_attr("width");
    if (aw) m_style.add_property(_width_, aw);
    const char* ah = get_attr("height");
    if (ah) m_style.add_property(_height_, ah);
}

void el_audio::get_content_size(size& sz, int max_width)
{
    sz.width = 240;
    sz.height = 36;
}

void el_audio::draw(uint_ptr hdc, int x, int y, const position* clip,
    const std::shared_ptr<render_item>& ri)
{
    HDC dc = (HDC)hdc;
    if (!dc) return;

    position pos = GetDrawPos(x, y, ri);
    if (!pos.does_intersect(clip)) return;

    Gp::Graphics g(dc);
    g.SetSmoothingMode(Gp::SmoothingModeAntiAlias);
    g.SetTextRenderingHint(Gp::TextRenderingHintAntiAlias);

    // 面板底
    Gp::SolidBrush bgBrush(Gp::Color(255, 248, 249, 252));
    Gp::GraphicsPath path;
    litehtml::border_radiuses br;
    br.top_left_x = br.top_left_y = 6;
    br.top_right_x = br.top_right_y = 6;
    br.bottom_right_x = br.bottom_right_y = 6;
    br.bottom_left_x = br.bottom_left_y = 6;
    container_win32::BuildRoundRectPath(path, pos.x, pos.y, pos.width, pos.height, br);
    g.FillPath(&bgBrush, &path);
    Gp::Pen borderPen(Gp::Color(255, 210, 212, 220), 1.0f);
    g.DrawPath(&borderPen, &path);

    // 播放/暂停按钮（左侧圆形 28px）
    float btnR = 13.0f;
    float btnCx = pos.x + 22;
    float btnCy = pos.y + pos.height / 2.0f;
    Gp::SolidBrush btnBrush(m_playing ? Gp::Color(255, 45, 91, 215) : Gp::Color(255, 80, 120, 230));
    g.FillEllipse(&btnBrush, btnCx - btnR, btnCy - btnR, btnR * 2, btnR * 2);

    // 播放三角 / 暂停双杠
    if (m_playing) {
        Gp::SolidBrush bar(Gp::Color(255, 255, 255, 255));
        g.FillRectangle(&bar, btnCx - 4, btnCy - 5, 3.0f, 10.0f);
        g.FillRectangle(&bar, btnCx + 1, btnCy - 5, 3.0f, 10.0f);
    } else {
        Gp::SolidBrush tri(Gp::Color(255, 255, 255, 255));
        Gp::PointF pts[3] = {
            Gp::PointF(btnCx - 4, btnCy - 6),
            Gp::PointF(btnCx - 4, btnCy + 6),
            Gp::PointF(btnCx + 7, btnCy)
        };
        g.FillPolygon(&tri, pts, 3);
    }

    // 进度条（右侧）
    float barX = pos.x + 46;
    float barY = btnCy - 2;
    float barW = pos.x + pos.width - 14 - barX;
    if (barW > 10) {
        Gp::SolidBrush track(Gp::Color(255, 224, 228, 238));
        g.FillRectangle(&track, barX, barY - 2, barW, 4.0f);
        // 播放头位置（简化：播放中显示 1/3 进度示意）
        Gp::SolidBrush thumb(m_playing ? Gp::Color(255, 45, 91, 215) : Gp::Color(255, 170, 174, 184));
        float tx = m_playing ? barX + barW / 3 : barX;
        g.FillEllipse(&thumb, tx - 4, barY - 5, 9.0f, 9.0f);
    }

    // 文件名提示
    if (!m_src.empty()) {
        Gp::Font* pFont = GetGpFontOf(this, m_container);
        if (pFont) {
            // 只显示文件名部分
            std::string name = m_src;
            size_t slash = name.find_last_of("/\\");
            if (slash != std::string::npos) name = name.substr(slash + 1);
            Gp::SolidBrush txtBrush(Gp::Color(255, 120, 124, 134));
            std::wstring wName;
            int n = ::MultiByteToWideChar(CP_UTF8, 0, name.c_str(), (int)name.size(), NULL, 0);
            if (n > 0) {
                wName.resize(n);
                ::MultiByteToWideChar(CP_UTF8, 0, name.c_str(), (int)name.size(), &wName[0], n);
            }
            Gp::RectF rc((Gp::REAL)barX, (Gp::REAL)(btnCy - 12), (Gp::REAL)barW, (Gp::REAL)10);
            Gp::StringFormat fmt;
            fmt.SetTrimming(Gp::StringTrimmingEllipsisCharacter);
            g.DrawString(wName.c_str(), (INT)wName.size(), pFont, rc, &fmt, &txtBrush);
        }
    }
}

el_audio::HitZone el_audio::HitTest(int x, int y, const position& pos) const
{
    // 播放按钮圆区域
    float btnCx = pos.x + 22;
    float btnCy = pos.y + pos.height / 2.0f;
    float dx = x - btnCx, dy = y - btnCy;
    if (dx * dx + dy * dy <= 15 * 15) return HIT_PLAY;
    return HIT_NONE;
}

bool el_audio::on_lbutton_down()
{
    return true;   // 接管点击，on_click 里处理
}

void el_audio::on_click()
{
    m_playing = !m_playing;
    if (m_playing) Play(); else Pause();
}

void el_audio::Play()
{
    if (m_src.empty()) return;
    // MCI 播放本地文件（mp3/wav/wma）
    std::string cmd = "open \"" + m_src + "\" type mpegvideo alias " + m_mciAlias;
    if (mciSendStringA(cmd.c_str(), NULL, 0, NULL) != 0) {
        // 失败静默（文件不存在/格式不支持）
        m_playing = false;
        return;
    }
    cmd = "play " + m_mciAlias;
    mciSendStringA(cmd.c_str(), NULL, 0, NULL);
}

void el_audio::Pause()
{
    std::string cmd = "pause " + m_mciAlias;
    mciSendStringA(cmd.c_str(), NULL, 0, NULL);
}

void el_audio::Stop()
{
    std::string cmd = "close " + m_mciAlias;
    mciSendStringA(cmd.c_str(), NULL, 0, NULL);
}

// ============================ 画布 ============================

el_canvas::el_canvas(const document::ptr& doc, container_win32* container)
    : el_form_control(doc, container, "inline-block")
    , m_width(300), m_height(150)
    , m_dirty(true)
    , m_hBitmap(NULL), m_bitmapW(0), m_bitmapH(0)
{
}

void el_canvas::parse_attributes()
{
    const char* aw = get_attr("width");
    if (aw) { int v = atoi(aw); if (v > 0) m_width = v; }
    const char* ah = get_attr("height");
    if (ah) { int v = atoi(ah); if (v > 0) m_height = v; }

    // 同时写进 style 让布局用
    m_style.add_property(_width_, std::to_string(m_width).c_str());
    m_style.add_property(_height_, std::to_string(m_height).c_str());

    InitBitmap(m_width, m_height);
}

void el_canvas::get_content_size(size& sz, int max_width)
{
    sz.width = m_width;
    sz.height = m_height;
}

bool el_canvas::InitBitmap(int w, int h)
{
    if (w <= 0 || h <= 0 || w > 4096 || h > 4096) return false;
    if (m_hBitmap && w == m_bitmapW && h == m_bitmapH) return true;

    if (m_hBitmap) { ::DeleteObject(m_hBitmap); m_hBitmap = NULL; }

    BITMAPINFO bmi = { 0 };
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = w;
    bmi.bmiHeader.biHeight = -h;    // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* bits = NULL;
    HDC screenDC = ::GetDC(NULL);
    m_hBitmap = ::CreateDIBSection(screenDC, &bmi, DIB_RGB_COLORS, &bits, NULL, 0);
    ::ReleaseDC(NULL, screenDC);
    if (!m_hBitmap) return false;

    m_bitmapW = w;
    m_bitmapH = h;
    m_dirty = true;

    // 默认透明白底
    if (bits) {
        memset(bits, 0xFF, w * h * 4);
    }
    return true;
}

bool el_canvas::Clear(const char* color)
{
    if (!m_hBitmap) return false;
    Gp::Color c = container_win32::ParseColor(color ? color : "#FFFFFF");
    Gp::Bitmap bmp(m_bitmapW, m_bitmapH, m_bitmapW * 4, PixelFormat32bppARGB, (BYTE*)NULL);
    // 简化：直接用 GDI 绘制到 DIB（通过内存 DC）
    HDC memDC = ::CreateCompatibleDC(NULL);
    HBITMAP old = (HBITMAP)::SelectObject(memDC, m_hBitmap);
    RECT rc = { 0, 0, m_bitmapW, m_bitmapH };
    Gp::Graphics g(memDC);
    Gp::SolidBrush brush(c);
    g.FillRectangle(&brush, 0, 0, m_bitmapW, m_bitmapH);
    ::SelectObject(memDC, old);
    ::DeleteDC(memDC);
    m_dirty = true;
    return true;
}

bool el_canvas::FillRect(int x, int y, int w, int h, const char* color)
{
    if (!m_hBitmap) return false;
    HDC memDC = ::CreateCompatibleDC(NULL);
    HBITMAP old = (HBITMAP)::SelectObject(memDC, m_hBitmap);
    Gp::Graphics g(memDC);
    Gp::SolidBrush brush(container_win32::ParseColor(color ? color : "#000000"));
    g.FillRectangle(&brush, (Gp::REAL)x, (Gp::REAL)y, (Gp::REAL)w, (Gp::REAL)h);
    ::SelectObject(memDC, old);
    ::DeleteDC(memDC);
    m_dirty = true;
    return true;
}

bool el_canvas::DrawRect(int x, int y, int w, int h, const char* color, float lineWidth)
{
    if (!m_hBitmap) return false;
    HDC memDC = ::CreateCompatibleDC(NULL);
    HBITMAP old = (HBITMAP)::SelectObject(memDC, m_hBitmap);
    Gp::Graphics g(memDC);
    Gp::Pen pen(container_win32::ParseColor(color ? color : "#000000"), lineWidth);
    g.DrawRectangle(&pen, (Gp::REAL)x, (Gp::REAL)y, (Gp::REAL)w, (Gp::REAL)h);
    ::SelectObject(memDC, old);
    ::DeleteDC(memDC);
    m_dirty = true;
    return true;
}

bool el_canvas::DrawLine(int x1, int y1, int x2, int y2, const char* color, float lineWidth)
{
    if (!m_hBitmap) return false;
    HDC memDC = ::CreateCompatibleDC(NULL);
    HBITMAP old = (HBITMAP)::SelectObject(memDC, m_hBitmap);
    Gp::Graphics g(memDC);
    Gp::Pen pen(container_win32::ParseColor(color ? color : "#000000"), lineWidth);
    g.DrawLine(&pen, (Gp::REAL)x1, (Gp::REAL)y1, (Gp::REAL)x2, (Gp::REAL)y2);
    ::SelectObject(memDC, old);
    ::DeleteDC(memDC);
    m_dirty = true;
    return true;
}

bool el_canvas::DrawCircle(int cx, int cy, float r, const char* color, bool fill)
{
    if (!m_hBitmap) return false;
    HDC memDC = ::CreateCompatibleDC(NULL);
    HBITMAP old = (HBITMAP)::SelectObject(memDC, m_hBitmap);
    Gp::Graphics g(memDC);
    g.SetSmoothingMode(Gp::SmoothingModeAntiAlias);
    Gp::Color c = container_win32::ParseColor(color ? color : "#000000");
    if (fill) {
        Gp::SolidBrush brush(c);
        g.FillEllipse(&brush, (Gp::REAL)(cx - r), (Gp::REAL)(cy - r), r * 2, r * 2);
    } else {
        Gp::Pen pen(c, 1.5f);
        g.DrawEllipse(&pen, (Gp::REAL)(cx - r), (Gp::REAL)(cy - r), r * 2, r * 2);
    }
    ::SelectObject(memDC, old);
    ::DeleteDC(memDC);
    m_dirty = true;
    return true;
}

bool el_canvas::DrawText(const char* text, int x, int y, const char* color, float fontSize)
{
    if (!m_hBitmap || !text || !text[0]) return false;
    HDC memDC = ::CreateCompatibleDC(NULL);
    HBITMAP old = (HBITMAP)::SelectObject(memDC, m_hBitmap);
    Gp::Graphics g(memDC);
    g.SetTextRenderingHint(Gp::TextRenderingHintAntiAlias);
    Gp::Font font(L"微软雅黑", fontSize, Gp::FontStyleRegular, Gp::UnitPixel);
    Gp::SolidBrush brush(container_win32::ParseColor(color ? color : "#000000"));
    std::wstring wText;
    int n = ::MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
    if (n > 0) {
        wText.resize(n - 1);
        ::MultiByteToWideChar(CP_UTF8, 0, text, -1, &wText[0], n);
    }
    g.DrawString(wText.c_str(), (INT)wText.size(), &font, Gp::PointF((Gp::REAL)x, (Gp::REAL)y), &brush);
    ::SelectObject(memDC, old);
    ::DeleteDC(memDC);
    m_dirty = true;
    return true;
}

void el_canvas::draw(uint_ptr hdc, int x, int y, const position* clip,
    const std::shared_ptr<render_item>& ri)
{
    HDC dc = (HDC)hdc;
    if (!dc) return;

    position pos = GetDrawPos(x, y, ri);
    if (!pos.does_intersect(clip)) return;

    if (m_hBitmap) {
        // 画内存位图内容（缩放到布局尺寸）
        HDC memDC = ::CreateCompatibleDC(dc);
        HBITMAP old = (HBITMAP)::SelectObject(memDC, m_hBitmap);
        ::SetStretchBltMode(dc, HALFTONE);
        ::SetBrushOrgEx(dc, 0, 0, NULL);
        ::StretchBlt(dc, pos.x, pos.y, pos.width, pos.height,
                     memDC, 0, 0, m_bitmapW, m_bitmapH, SRCCOPY);
        ::SelectObject(memDC, old);
        ::DeleteDC(memDC);
    } else {
        // 无位图时画占位
        Gp::Graphics g(dc);
        Gp::SolidBrush bg(Gp::Color(255, 250, 250, 252));
        g.FillRectangle(&bg, (Gp::REAL)pos.x, (Gp::REAL)pos.y,
                        (Gp::REAL)pos.width, (Gp::REAL)pos.height);
        Gp::Pen pen(Gp::Color(255, 200, 202, 210), 1.0f);
        g.DrawRectangle(&pen, (Gp::REAL)pos.x, (Gp::REAL)pos.y,
                        (Gp::REAL)(pos.width - 1), (Gp::REAL)(pos.height - 1));
    }
}

void el_canvas::on_click()
{
    // 画布点击：通知宿主（可由 JS 扩展处理）
}

// ============================ 内嵌框架 ============================

el_iframe::el_iframe(const document::ptr& doc, container_win32* container)
    : el_form_control(doc, container, "inline-block")
    , m_childLoaded(false)
{
}

void el_iframe::parse_attributes()
{
    const char* src = get_attr("src");
    if (src) m_src = src;

    const char* aw = get_attr("width");
    if (aw) m_style.add_property(_width_, aw);
    const char* ah = get_attr("height");
    if (ah) m_style.add_property(_height_, ah);
}

void el_iframe::get_content_size(size& sz, int max_width)
{
    sz.width = 400;
    sz.height = 200;
}

void el_iframe::draw(uint_ptr hdc, int x, int y, const position* clip,
    const std::shared_ptr<render_item>& ri)
{
    HDC dc = (HDC)hdc;
    if (!dc) return;

    position pos = GetDrawPos(x, y, ri);
    if (!pos.does_intersect(clip)) return;

    Gp::Graphics g(dc);
    g.SetSmoothingMode(Gp::SmoothingModeAntiAlias);

    // 边框 + 白底
    Gp::SolidBrush bg(Gp::Color(255, 255, 255, 255));
    g.FillRectangle(&bg, (Gp::REAL)pos.x, (Gp::REAL)pos.y,
                    (Gp::REAL)pos.width, (Gp::REAL)pos.height);
    Gp::Pen pen(Gp::Color(255, 200, 202, 210), 1.0f);
    g.DrawRectangle(&pen, (Gp::REAL)pos.x, (Gp::REAL)pos.y,
                    (Gp::REAL)(pos.width - 1), (Gp::REAL)(pos.height - 1));

    // 子文档渲染（懒加载，本地 html）
    if (!m_childLoaded && !m_src.empty() && m_container) {
        m_childLoaded = true;
        // 读文件
        HANDLE hFile = ::CreateFileA(m_src.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,
                                     OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile != INVALID_HANDLE_VALUE) {
            DWORD dwSize = ::GetFileSize(hFile, NULL);
            if (dwSize > 0 && dwSize != INVALID_FILE_SIZE) {
                std::string content;
                content.resize(dwSize);
                DWORD dwRead = 0;
                ::ReadFile(hFile, &content[0], dwSize, &dwRead, NULL);
                ::CloseHandle(hFile);
                if (content.size() >= 3 && content[0] == '\xEF' && content[1] == '\xBB' && content[2] == '\xBF')
                    content = content.substr(3);

                m_childDoc = document::createFromString(content.c_str(), m_container, NULL, "");
                if (m_childDoc) {
                    m_childDoc->render(pos.width > 0 ? pos.width : 400);
                }
            } else {
                ::CloseHandle(hFile);
            }
        }
    }

    if (m_childDoc) {
        // 子文档绘制到当前区域（平移到 pos 位置，裁剪）
        g.SetClip(Gp::Rect(pos.x, pos.y, pos.width, pos.height), Gp::CombineModeIntersect);
        // 保存平移状态：用 GDI 的视口原点（litehtml 内部绘制基于 m_container 的 m_hdc）
        // 简化：把子文档画到临时区域
        // 注意：litehtml 的 container 绘制是全局坐标，这里直接让子文档画（其内部有平移参数）
        // draw(hdc, x, y, clip) 的 x/y 就是平移量
        m_childDoc->draw(hdc, pos.x, pos.y, clip);
        g.ResetClip();
    }
}

void el_iframe::on_click()
{
}

// ============================ 矢量图 ============================

el_svg::el_svg(const document::ptr& doc, container_win32* container)
    : el_form_control(doc, container, "inline-block")
    , m_width(300), m_height(150)
{
}

void el_svg::parse_attributes()
{
    const char* aw = get_attr("width");
    if (aw) { int v = atoi(aw); if (v > 0) m_width = v; }
    const char* ah = get_attr("height");
    if (ah) { int v = atoi(ah); if (v > 0) m_height = v; }

    // 没有 viewBox 时，用 width/height 属性作为内部分辨率，保证 1:1 绘制
    if (!get_attr("viewBox")) {
        m_style.add_property(_width_, std::to_string(m_width).c_str());
        m_style.add_property(_height_, std::to_string(m_height).c_str());
    }
}

void el_svg::get_content_size(size& sz, int max_width)
{
    // CSS width/height 优先（图标场景 .ni svg{width:17px}），其次 width/height 属性
    int w = m_width, h = m_height;
    document::ptr doc = get_document();
    if (doc) {
        const css_length& cw = css().get_width();
        const css_length& ch = css().get_height();
        if (!cw.is_predefined()) {
            int v = doc->to_pixels(cw, css().get_font_size(), max_width);
            if (v > 0) w = v;
        }
        if (!ch.is_predefined()) {
            int v = doc->to_pixels(ch, css().get_font_size(), m_height);
            if (v > 0) h = v;
        }
    }
    sz.width = w;
    sz.height = h;
}

// ============================ SVG 辅助函数 ============================
// 跳过 path d 中的空白与逗号分隔符
static void SvgSkipSep(const std::string& d, size_t& i)
{
    while (i < d.size() && (d[i] == ' ' || d[i] == ',' || d[i] == '\t' ||
                            d[i] == '\r' || d[i] == '\n')) i++;
}

// 读取一个 SVG 数字：支持 1 / -1 / .5 / 1.5 / 1e3 / 1.5e-3
// 关键点：小数点最多出现一次，遇到第二个小数点必须断开
// （否则 "7.1.9" 这种紧凑写法会被 atof 吞掉，后续坐标全部错位）
static bool SvgReadNumber(const std::string& d, size_t& i, float& out)
{
    SvgSkipSep(d, i);
    const size_t n = d.size();
    if (i >= n) return false;

    const size_t start = i;
    if (d[i] == '+' || d[i] == '-') i++;

    bool hasDigit = false;
    while (i < n && isdigit((unsigned char)d[i])) { i++; hasDigit = true; }
    if (i < n && d[i] == '.') {
        i++;
        while (i < n && isdigit((unsigned char)d[i])) { i++; hasDigit = true; }
    }
    if (!hasDigit) { i = start; return false; }   // 不是数字，回退

    // 科学计数法
    if (i < n && (d[i] == 'e' || d[i] == 'E')) {
        const size_t save = i;
        i++;
        if (i < n && (d[i] == '+' || d[i] == '-')) i++;
        bool expDigit = false;
        while (i < n && isdigit((unsigned char)d[i])) { i++; expDigit = true; }
        if (!expDigit) i = save;                  // 不是合法指数，回退
    }

    out = (float)atof(d.substr(start, i - start).c_str());
    return true;
}

// 读取弧线标志位：SVG 规定 large-arc / sweep 只能是单个 0 或 1，
// 且允许紧贴后续数字（例如 "A6.47 6.47 0 0016 9.5" 里的 "0016"）
static bool SvgReadFlag(const std::string& d, size_t& i, bool& out)
{
    SvgSkipSep(d, i);
    if (i >= d.size()) return false;
    if (d[i] == '0') { out = false; i++; return true; }
    if (d[i] == '1') { out = true;  i++; return true; }
    return false;
}

// 椭圆弧转三次贝塞尔（SVG 规范 F.6.5 端点参数 → 中心参数，再按 <=90° 分段逼近）
static void SvgArcTo(Gp::GraphicsPath& path, float& cx, float& cy,
                     float rx, float ry, float rotDeg, bool largeArc, bool sweep,
                     float ex, float ey)
{
    const float PI = 3.14159265358979f;
    if (rx == 0.0f || ry == 0.0f) {
        path.AddLine(Gp::PointF(cx, cy), Gp::PointF(ex, ey));
        cx = ex; cy = ey;
        return;
    }
    rx = (float)fabs(rx); ry = (float)fabs(ry);
    const float phi = rotDeg * PI / 180.0f;
    const float cosPhi = (float)cos(phi), sinPhi = (float)sin(phi);

    const float dx2 = (cx - ex) * 0.5f, dy2 = (cy - ey) * 0.5f;
    const float x1p =  cosPhi * dx2 + sinPhi * dy2;
    const float y1p = -sinPhi * dx2 + cosPhi * dy2;

    // 半径不足时按比例放大
    const float lambda = (x1p * x1p) / (rx * rx) + (y1p * y1p) / (ry * ry);
    if (lambda > 1.0f) {
        const float s = (float)sqrt(lambda);
        rx *= s; ry *= s;
    }

    const float rx2 = rx * rx, ry2 = ry * ry;
    const float num = rx2 * ry2 - rx2 * y1p * y1p - ry2 * x1p * x1p;
    const float den = rx2 * y1p * y1p + ry2 * x1p * x1p;
    float coef = (den > 0.0f) ? (float)sqrt(num > 0.0f ? num / den : 0.0f) : 0.0f;
    if (largeArc == sweep) coef = -coef;

    const float cxp =  coef * rx * y1p / ry;
    const float cyp = -coef * ry * x1p / rx;
    const float ccx = cosPhi * cxp - sinPhi * cyp + (cx + ex) * 0.5f;
    const float ccy = sinPhi * cxp + cosPhi * cyp + (cy + ey) * 0.5f;

    // 向量夹角（带符号）
    auto angleOf = [](float ux, float uy, float vx, float vy) -> float {
        const float len = (float)sqrt((ux * ux + uy * uy) * (vx * vx + vy * vy));
        if (len <= 0.0f) return 0.0f;
        float c = (ux * vx + uy * vy) / len;
        if (c > 1.0f) c = 1.0f;
        if (c < -1.0f) c = -1.0f;
        float a = (float)acos(c);
        if (ux * vy - uy * vx < 0.0f) a = -a;
        return a;
    };

    const float ux = (x1p - cxp) / rx, uy = (y1p - cyp) / ry;
    const float vx = (-x1p - cxp) / rx, vy = (-y1p - cyp) / ry;
    const float theta1 = angleOf(1.0f, 0.0f, ux, uy);
    float dtheta = angleOf(ux, uy, vx, vy);
    if (!sweep && dtheta > 0.0f)       dtheta -= 2.0f * PI;
    else if (sweep && dtheta < 0.0f)   dtheta += 2.0f * PI;

    int segs = (int)ceil((double)fabs(dtheta) / (PI / 2.0));
    if (segs < 1) segs = 1;
    const float delta = dtheta / (float)segs;
    const float t = 4.0f / 3.0f * (float)tan(delta / 4.0f);

    // 椭圆上的点与导数（参数方程 → 用户坐标）
    auto pointAt = [&](float ca, float sa) {
        return Gp::PointF(ccx + cosPhi * rx * ca - sinPhi * ry * sa,
                          ccy + sinPhi * rx * ca + cosPhi * ry * sa);
    };
    auto derivAt = [&](float ca, float sa) {
        return Gp::PointF(-cosPhi * rx * sa - sinPhi * ry * ca,
                          -sinPhi * rx * sa + cosPhi * ry * ca);
    };

    float px = cx, py = cy;
    for (int s = 0; s < segs; s++) {
        const float a1 = theta1 + delta * (float)s;
        const float a2 = a1 + delta;
        const float c1 = (float)cos(a1), s1 = (float)sin(a1);
        const float c2 = (float)cos(a2), s2 = (float)sin(a2);
        const Gp::PointF p1 = pointAt(c1, s1), p2 = pointAt(c2, s2);
        const Gp::PointF d1 = derivAt(c1, s1), d2 = derivAt(c2, s2);
        path.AddBezier(Gp::PointF(px, py),
                       Gp::PointF(p1.X + t * d1.X, p1.Y + t * d1.Y),
                       Gp::PointF(p2.X - t * d2.X, p2.Y - t * d2.Y),
                       p2);
        px = p2.X; py = p2.Y;
    }
    cx = ex; cy = ey;
}

// ---------------------------------------------------------------------------
// SVG path d 属性解析（轻量版）：支持 M m L l H h V v C c S s Q q T t A a Z z
// 输出为 GDI+ GraphicsPath，配合 viewBox 缩放绘制图标
// ---------------------------------------------------------------------------
static void SvgParsePath(const std::string& d, Gp::GraphicsPath& path)
{
    float cx = 0, cy = 0;        // 当前点
    float sx = 0, sy = 0;        // 子路径起点（Z 闭合用）
    float lastC2x = 0, lastC2y = 0;  // 上一个三次贝塞尔第二控制点（S 平滑用）
    float lastQx = 0, lastQy = 0;    // 上一个二次贝塞尔控制点（T 平滑用）
    char prevU = 0;              // 上一条命令（判断 S/T 是否需要反射）
    char cmd = 0;                // 当前命令
    size_t i = 0;
    const size_t n = d.size();

    auto readNum = [&](float& out) -> bool { return SvgReadNumber(d, i, out); };

    // 二次贝塞尔转三次（GDI+ 只支持三次）
    auto quadTo = [&](float qx, float qy, float ex, float ey) {
        float c1x = cx + (qx - cx) * 2.0f / 3.0f, c1y = cy + (qy - cy) * 2.0f / 3.0f;
        float c2x = ex + (qx - ex) * 2.0f / 3.0f, c2y = ey + (qy - ey) * 2.0f / 3.0f;
        path.AddBezier(Gp::PointF(cx, cy), Gp::PointF(c1x, c1y),
                       Gp::PointF(c2x, c2y), Gp::PointF(ex, ey));
        lastC2x = qx; lastC2y = qy; lastQx = qx; lastQy = qy;
        cx = ex; cy = ey;
    };

    while (i < n) {
        SvgSkipSep(d, i);
        if (i >= n) break;
        char ch = d[i];
        if (isalpha((unsigned char)ch)) { cmd = ch; i++; }
        else if (!cmd) { i++; continue; }

        bool rel = (cmd >= 'a' && cmd <= 'z');   // 小写 = 相对坐标
        char U = rel ? (char)(cmd - 32) : cmd;
        float a = 0, b = 0, c2 = 0, d2 = 0, e = 0, f2 = 0;

        switch (U) {
        case 'M':
        case 'L':
            while (readNum(a) && readNum(b)) {
                float nx = rel ? cx + a : a, ny = rel ? cy + b : b;
                if (U == 'M') { path.StartFigure(); sx = nx; sy = ny; cx = nx; cy = ny; cmd = rel ? 'l' : 'L'; }
                else { path.AddLine(Gp::PointF(cx, cy), Gp::PointF(nx, ny)); cx = nx; cy = ny; }
            }
            break;
        case 'H':
            while (readNum(a)) { float nx = rel ? cx + a : a; path.AddLine(Gp::PointF(cx, cy), Gp::PointF(nx, cy)); cx = nx; }
            break;
        case 'V':
            while (readNum(a)) { float ny = rel ? cy + a : a; path.AddLine(Gp::PointF(cx, cy), Gp::PointF(cx, ny)); cy = ny; }
            break;
        case 'C':
            while (readNum(a) && readNum(b) && readNum(c2) && readNum(d2) && readNum(e) && readNum(f2)) {
                float x1 = rel ? cx + a : a, y1 = rel ? cy + b : b;
                float x2 = rel ? cx + c2 : c2, y2 = rel ? cy + d2 : d2;
                float x3 = rel ? cx + e : e, y3 = rel ? cy + f2 : f2;
                path.AddBezier(Gp::PointF(cx, cy), Gp::PointF(x1, y1),
                               Gp::PointF(x2, y2), Gp::PointF(x3, y3));
                lastC2x = x2; lastC2y = y2;
                cx = x3; cy = y3;
            }
            break;
        case 'S':
            while (readNum(a) && readNum(b) && readNum(c2) && readNum(d2)) {
                // S 只有 4 个参数：第二控制点 + 终点
                float x2 = rel ? cx + a : a, y2 = rel ? cy + b : b;
                float x3 = rel ? cx + c2 : c2, y3 = rel ? cy + d2 : d2;
                // 第一控制点：上一条是 C/S 时对上一第二控制点做中心反射，否则等于当前点
                float x1 = (prevU == 'C' || prevU == 'S') ? cx + (cx - lastC2x) : cx;
                float y1 = (prevU == 'C' || prevU == 'S') ? cy + (cy - lastC2y) : cy;
                path.AddBezier(Gp::PointF(cx, cy), Gp::PointF(x1, y1),
                               Gp::PointF(x2, y2), Gp::PointF(x3, y3));
                lastC2x = x2; lastC2y = y2;
                cx = x3; cy = y3;
            }
            break;
        case 'Q':
            while (readNum(a) && readNum(b) && readNum(c2) && readNum(d2)) {
                float qx = rel ? cx + a : a, qy = rel ? cy + b : b;
                float ex = rel ? cx + c2 : c2, ey = rel ? cy + d2 : d2;
                quadTo(qx, qy, ex, ey);
            }
            break;
        case 'T':
            while (readNum(a) && readNum(b)) {
                float ex = rel ? cx + a : a, ey = rel ? cy + b : b;
                // 控制点：上一条是 Q/T 时反射上一控制点，否则等于当前点
                float qx = (prevU == 'Q' || prevU == 'T') ? cx + (cx - lastQx) : cx;
                float qy = (prevU == 'Q' || prevU == 'T') ? cy + (cy - lastQy) : cy;
                quadTo(qx, qy, ex, ey);
            }
            break;
        case 'A': {
            // 椭圆弧：rx ry rotation large-arc-flag sweep-flag x y
            while (true) {
                float rx, ry, rot, ex, ey;
                bool laf = false, sf = false;
                if (!readNum(rx)) break;
                if (!readNum(ry)) break;
                if (!readNum(rot)) break;
                if (!SvgReadFlag(d, i, laf)) break;
                if (!SvgReadFlag(d, i, sf)) break;
                if (!readNum(ex)) break;
                if (!readNum(ey)) break;
                float nx = rel ? cx + ex : ex, ny = rel ? cy + ey : ey;
                SvgArcTo(path, cx, cy, rx, ry, rot, laf, sf, nx, ny);
            }
            break; }
        case 'Z':
            path.CloseFigure();
            cx = sx; cy = sy;
            break;
        default:
            i++;
            break;
        }
        prevU = U;
    }
}

// ============================ SVG 属性工具 ============================
// 去掉首尾空白
static std::string SvgTrim(const std::string& s)
{
    size_t b = 0, e = s.size();
    while (b < e && (s[b] == ' ' || s[b] == '\t' || s[b] == '\r' || s[b] == '\n')) b++;
    while (e > b && (s[e - 1] == ' ' || s[e - 1] == '\t' || s[e - 1] == '\r' || s[e - 1] == '\n')) e--;
    return s.substr(b, e - b);
}

// 全部转小写
static std::string SvgLower(std::string s)
{
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] >= 'A' && s[i] <= 'Z') s[i] = (char)(s[i] + 32);
    }
    return s;
}

// 展开 var(--x)：SVG 属性里的自定义属性沿父链查找
// （litehtml 只对 CSS 声明做 var 替换，写在 HTML 属性上的 var() 需自行展开）
static std::string SvgExpandVars(const litehtml::element::ptr& el, const std::string& src)
{
    std::string s = src;
    for (int guard = 0; guard < 16; guard++) {
        size_t start = s.find("var(");
        if (start == std::string::npos) break;
        size_t end = s.find(')', start + 4);
        if (end == std::string::npos) break;
        std::string name = SvgTrim(s.substr(start + 4, end - start - 4));
        std::string val;
        if (el && !name.empty()) val = el->get_custom_property(_id(name), "");
        if (val.empty()) s.erase(start, end - start + 1);      // 未定义则删掉，走默认值
        else             s.replace(start, end - start + 1, val);
    }
    return s;
}

// 解析 style="a:b;c:d" 中某个属性的值
static std::string SvgStyleValue(const std::string& style, const char* prop)
{
    std::string p = std::string(prop) + ":";
    size_t pos = style.find(p);
    if (pos == std::string::npos) return "";
    pos += p.size();
    size_t end = style.find(';', pos);
    return SvgTrim(style.substr(pos, end == std::string::npos ? std::string::npos : end - pos));
}

// 按透明度系数调整颜色 alpha
static Gp::Color SvgApplyAlpha(const Gp::Color& c, float a)
{
    if (a >= 1.0f) return c;
    int na = (int)(c.GetA() * a + 0.5f);
    if (na < 0) na = 0;
    if (na > 255) na = 255;
    return Gp::Color((BYTE)na, c.GetR(), c.GetG(), c.GetB());
}

// 解析 SVG 绘制色。返回 false 表示 fill="none"（不绘制）
// gradId 非空表示要用 url(#id) 引用的渐变
static bool SvgResolvePaint(const litehtml::element::ptr& el, const std::string& raw,
                            const Gp::Color& fallback, Gp::Color& out, std::string& gradId)
{
    std::string s = SvgLower(SvgTrim(SvgExpandVars(el, raw)));
    gradId.clear();
    if (s.empty()) { out = fallback; return true; }
    if (s == "none") return false;
    if (s == "currentcolor" || s == "inherit") { out = fallback; return true; }
    if (s.compare(0, 4, "url(") == 0) {
        // url(#gradId)
        size_t h = s.find('#');
        size_t e = s.find(')', h == std::string::npos ? 0 : h);
        if (h != std::string::npos && e != std::string::npos) gradId = s.substr(h + 1, e - h - 1);
        out = fallback;
        return true;
    }
    out = container_win32::ParseColor(s);
    return true;
}

// 在 svg 子树内按 id 查找元素（渐变/图案定义）
static litehtml::element::ptr SvgFindById(const litehtml::element::ptr& root, const std::string& id)
{
    if (!root) return nullptr;
    const char* pid = root->get_attr("id");
    if (pid && id == pid) return root;
    for (const auto& c : root->children()) {
        litehtml::element::ptr r = SvgFindById(c, id);
        if (r) return r;
    }
    return nullptr;
}

// 解析 transform 属性（translate/scale/rotate/matrix/skewX/skewY）
static bool SvgBuildTransform(const std::string& str, Gp::Matrix& out)
{
    bool any = false;
    size_t i = 0;
    const size_t n = str.size();
    while (i < n) {
        while (i < n && !isalpha((unsigned char)str[i])) i++;
        if (i >= n) break;
        size_t nameStart = i;
        while (i < n && isalpha((unsigned char)str[i])) i++;
        std::string fn = SvgLower(str.substr(nameStart, i - nameStart));
        while (i < n && str[i] != '(') i++;
        if (i >= n) break;
        i++;   // 跳过 '('
        float a[6] = { 0, 0, 0, 0, 0, 0 };
        int cnt = 0;
        while (cnt < 6 && SvgReadNumber(str, i, a[cnt])) cnt++;
        Gp::Matrix m;
        if (fn == "translate" && cnt >= 1) {
            m.Translate(cnt >= 2 ? a[0] : a[0], cnt >= 2 ? a[1] : 0.0f);
        } else if (fn == "scale" && cnt >= 1) {
            m.Scale(a[0], cnt >= 2 ? a[1] : a[0]);
        } else if (fn == "rotate" && cnt >= 1) {
            if (cnt >= 3) {
                // rotate(angle cx cy) 等价于 T(-cx,-cy) * R(angle) * T(cx,cy)
                Gp::Matrix t1, r;
                t1.Translate(-a[1], -a[2]);
                r.Rotate(a[0]);
                Gp::Matrix t2;
                t2.Translate(a[1], a[2]);
                m.Multiply(&t1, Gp::MatrixOrderAppend);
                m.Multiply(&r,  Gp::MatrixOrderAppend);
                m.Multiply(&t2, Gp::MatrixOrderAppend);
            } else {
                m.Rotate(a[0]);
            }
        } else if (fn == "matrix" && cnt >= 6) {
            m.SetElements(a[0], a[1], a[2], a[3], a[4], a[5]);
        } else if (fn == "skewx" && cnt >= 1) {
            m.Shear((Gp::REAL)tan(a[0] * 3.14159265f / 180.0f), 0.0f);
        } else if (fn == "skewy" && cnt >= 1) {
            m.Shear(0.0f, (Gp::REAL)tan(a[0] * 3.14159265f / 180.0f));
        } else {
            while (i < n && str[i] != ')') i++;
            i++;
            continue;
        }
        out.Multiply(&m, Gp::MatrixOrderAppend);
        any = true;
        while (i < n && str[i] != ')') i++;
        i++;
    }
    return any;
}

// 用 linearGradient 定义创建渐变画刷（按图形包围盒做 objectBoundingBox 映射）
static Gp::Brush* SvgCreateGradientBrush(const litehtml::element::ptr& gradEl,
                                         const Gp::RectF& bounds, float alpha)
{
    if (!gradEl || bounds.Width <= 0 || bounds.Height <= 0) return nullptr;

    // 起止点：支持百分比与绝对数值
    auto readPt = [&](const char* name, float def) -> float {
        const char* v = gradEl->get_attr(name);
        if (!v) return def;
        std::string s = SvgTrim(v);
        if (!s.empty() && s[s.size() - 1] == '%') return (float)atof(s.c_str()) / 100.0f;
        float f = (float)atof(s.c_str());
        // 无单位按 objectBoundingBox 处理
        return f > 1.0f ? f : f;
    };
    float x1 = readPt("x1", 0.0f), y1 = readPt("y1", 0.0f);
    float x2 = readPt("x2", 1.0f), y2 = readPt("y2", 0.0f);

    // 收集 stop
    std::vector<Gp::Color> colors;
    std::vector<Gp::REAL>  positions;
    for (const auto& c : gradEl->children()) {
        const char* tag = c->get_tagName();
        if (!tag || _stricmp(tag, "stop") != 0) continue;

        std::string style = c->get_attr("style") ? c->get_attr("style") : "";
        std::string colStr = SvgStyleValue(style, "stop-color");
        if (colStr.empty() && c->get_attr("stop-color")) colStr = c->get_attr("stop-color");
        std::string opStr = SvgStyleValue(style, "stop-opacity");
        if (opStr.empty() && c->get_attr("stop-opacity")) opStr = c->get_attr("stop-opacity");

        float pos = 0.0f;
        const char* off = c->get_attr("offset");
        if (off) {
            std::string s = SvgTrim(off);
            pos = (!s.empty() && s[s.size() - 1] == '%') ? (float)atof(s.c_str()) / 100.0f
                                                         : (float)atof(s.c_str());
        }
        if (pos < 0.0f) pos = 0.0f;
        if (pos > 1.0f) pos = 1.0f;

        Gp::Color col = colStr.empty() ? Gp::Color(255, 0, 0, 0)
                                       : container_win32::ParseColor(SvgExpandVars(c, colStr));
        float op = opStr.empty() ? 1.0f : (float)atof(opStr.c_str());
        colors.push_back(SvgApplyAlpha(col, op * alpha));
        positions.push_back((Gp::REAL)pos);
    }
    if (colors.empty()) return nullptr;
    if (colors.size() == 1) return new Gp::SolidBrush(colors[0]);

    Gp::PointF p1(bounds.X + bounds.Width * x1, bounds.Y + bounds.Height * y1);
    Gp::PointF p2(bounds.X + bounds.Width * x2, bounds.Y + bounds.Height * y2);
    if (p1.X == p2.X && p1.Y == p2.Y) p2.X += 1.0f;    // 避免零长度渐变

    Gp::LinearGradientBrush* lgb = new Gp::LinearGradientBrush(p1, p2, colors[0], colors[colors.size() - 1]);
    if (colors.size() > 2) {
        lgb->SetInterpolationColors(&colors[0], &positions[0], (INT)colors.size());
    }
    return lgb;
}

// 递归绘制 SVG 子元素（path/rect/circle/line/text/g 等）
// svgRoot 用于查找 url(#id) 引用的渐变定义；inheritedFill 为父级继承的填充色
// base 为「用户坐标 → 设备坐标」的基准变换（含 viewBox 缩放/偏移）
static void DrawSvgElement(Gp::Graphics& g, const litehtml::element::ptr& el,
                           const litehtml::element::ptr& svgRoot,
                           const Gp::Matrix& base, float scale,
                           const Gp::Color& inheritedFill)
{
    if (!el) return;
    const char* tag = el->get_tagName();
    if (!tag) return;

    // 定义类元素只做数据源，不直接绘制
    if (_stricmp(tag, "defs") == 0 || _stricmp(tag, "linearGradient") == 0 ||
        _stricmp(tag, "radialGradient") == 0 || _stricmp(tag, "clipPath") == 0 ||
        _stricmp(tag, "animateTransform") == 0 || _stricmp(tag, "animate") == 0 ||
        _stricmp(tag, "style") == 0 || _stricmp(tag, "title") == 0 || _stricmp(tag, "desc") == 0)
        return;

    // 通用取属性
    auto attrF = [&](const char* name, float def) -> float {
        const char* v = el->get_attr(name);
        return v ? (float)atof(v) : def;
    };
    auto attrS = [&](const char* name, const char* def) -> std::string {
        const char* v = el->get_attr(name);
        return v ? std::string(v) : std::string(def);
    };

    // ---- 填充色 / 描边色 ----
    // 自身 fill 属性优先；否则继承父级；都没有则继承传入色（CSS fill/color 透传）
    std::string fillRaw = attrS("fill", "");
    if (fillRaw.empty()) {
        // 兼容 <path style="fill:#fff"> 这种写法
        std::string st = attrS("style", "");
        if (!st.empty()) fillRaw = SvgStyleValue(st, "fill");
    }
    Gp::Color fill = inheritedFill;
    std::string fillGrad;
    bool doFill = true;
    if (!fillRaw.empty()) doFill = SvgResolvePaint(el, fillRaw, inheritedFill, fill, fillGrad);

    std::string strokeRaw = attrS("stroke", "");
    if (strokeRaw.empty()) {
        std::string st = attrS("style", "");
        if (!st.empty()) strokeRaw = SvgStyleValue(st, "stroke");
    }
    Gp::Color stroke = Gp::Color(255, 0, 0, 0);
    std::string strokeGrad;
    bool doStroke = false;
    if (!strokeRaw.empty()) doStroke = SvgResolvePaint(el, strokeRaw, Gp::Color(255, 0, 0, 0), stroke, strokeGrad);

    // 线宽 / 透明度
    float sw = attrF("stroke-width", 1.0f);
    float opacity = attrF("opacity", 1.0f);
    float fillOpacity = attrF("fill-opacity", 1.0f);
    float strokeOpacity = attrF("stroke-opacity", 1.0f);
    if (opacity < 0.0f) opacity = 0.0f;
    if (opacity > 1.0f) opacity = 1.0f;

    // 几何变换（SVG 用户坐标系内）
    Gp::Matrix tm;
    bool hasTm = false;
    std::string transformRaw = attrS("transform", "");
    if (!transformRaw.empty()) hasTm = SvgBuildTransform(transformRaw, tm);

    // 把用户坐标路径变换到设备坐标：先 transform，再叠加基准变换（viewBox 缩放 + 原点平移）
    // 注意：Gdiplus::Matrix 的拷贝构造/赋值是私有的，只能用 Clone()
    auto toDevice = [&](Gp::GraphicsPath& p) {
        if (hasTm) p.Transform(&tm);
        Gp::Matrix* mb = base.Clone();
        if (mb) { p.Transform(mb); delete mb; }
    };

    // 用给定画刷填充路径（支持 url(#id) 渐变）
    auto fillPath = [&](Gp::GraphicsPath& p) {
        if (!doFill || p.GetPointCount() == 0) return;
        Gp::Color col = SvgApplyAlpha(fill, fillOpacity * opacity);
        if (!fillGrad.empty()) {
            Gp::RectF bnd;
            p.GetBounds(&bnd);
            litehtml::element::ptr gradEl = SvgFindById(svgRoot, fillGrad);
            Gp::Brush* br = SvgCreateGradientBrush(gradEl, bnd, fillOpacity * opacity);
            if (br) { g.FillPath(br, &p); delete br; return; }
        }
        Gp::SolidBrush b(col);
        g.FillPath(&b, &p);
    };

    // 用给定画笔描边路径
    auto strokePath = [&](Gp::GraphicsPath& p) {
        if (!doStroke || p.GetPointCount() == 0) return;
        Gp::Pen pen(SvgApplyAlpha(stroke, strokeOpacity * opacity), sw * scale);
        if (pen.GetWidth() <= 0.0f) pen.SetWidth(1.0f);
        // stroke-linecap
        std::string cap = attrS("stroke-linecap", "");
        if (cap == "round")      pen.SetLineCap(Gp::LineCapRound, Gp::LineCapRound, Gp::DashCapRound);
        else if (cap == "square") pen.SetLineCap(Gp::LineCapSquare, Gp::LineCapSquare, Gp::DashCapFlat);
        // stroke-dasharray（按线宽换算成 GDI+ 的笔宽倍数）
        std::string dash = attrS("stroke-dasharray", "");
        if (!dash.empty() && dash != "none") {
            std::vector<Gp::REAL> dashes;
            size_t di = 0;
            float dv = 0;
            while (SvgReadNumber(dash, di, dv)) dashes.push_back((Gp::REAL)(dv / (sw > 0 ? sw : 1.0f)));
            if (!dashes.empty()) pen.SetDashPattern(&dashes[0], (INT)dashes.size());
        }
        if (!strokeGrad.empty()) {
            Gp::RectF bnd;
            p.GetBounds(&bnd);
            litehtml::element::ptr gradEl = SvgFindById(svgRoot, strokeGrad);
            Gp::Brush* br = SvgCreateGradientBrush(gradEl, bnd, strokeOpacity * opacity);
            if (br) { g.DrawPath(&pen, &p); delete br; return; }
        }
        g.DrawPath(&pen, &p);
    };

    if (_stricmp(tag, "path") == 0) {
        const char* dStr = el->get_attr("d");
        if (dStr && dStr[0]) {
            Gp::GraphicsPath ppath;
            SvgParsePath(dStr, ppath);
            toDevice(ppath);
            fillPath(ppath);
            strokePath(ppath);
        }
    } else if (_stricmp(tag, "rect") == 0) {
        float rx = attrF("x", 0), ry = attrF("y", 0);
        float rw = attrF("width", 0), rh = attrF("height", 0);
        float cr = attrF("rx", 0);
        Gp::GraphicsPath ppath;
        if (cr > 0) {
            float d = cr * 2;
            if (d > rw) d = rw;
            if (d > rh) d = rh;
            ppath.AddArc(rx, ry, d, d, 180, 90);
            ppath.AddArc(rx + rw - d, ry, d, d, 270, 90);
            ppath.AddArc(rx + rw - d, ry + rh - d, d, d, 0, 90);
            ppath.AddArc(rx, ry + rh - d, d, d, 90, 90);
            ppath.CloseFigure();
        } else {
            ppath.AddRectangle(Gp::RectF(rx, ry, rw, rh));
        }
        toDevice(ppath);
        fillPath(ppath);
        strokePath(ppath);
    } else if (_stricmp(tag, "circle") == 0) {
        float cx = attrF("cx", 0), cy = attrF("cy", 0), r = attrF("r", 0);
        Gp::GraphicsPath ppath;
        ppath.AddEllipse(cx - r, cy - r, r * 2, r * 2);
        toDevice(ppath);
        fillPath(ppath);
        strokePath(ppath);
    } else if (_stricmp(tag, "ellipse") == 0) {
        float cx = attrF("cx", 0), cy = attrF("cy", 0);
        float rx = attrF("rx", 0), ry = attrF("ry", 0);
        Gp::GraphicsPath ppath;
        ppath.AddEllipse(cx - rx, cy - ry, rx * 2, ry * 2);
        toDevice(ppath);
        fillPath(ppath);
        strokePath(ppath);
    } else if (_stricmp(tag, "line") == 0) {
        Gp::GraphicsPath ppath;
        ppath.AddLine(attrF("x1", 0), attrF("y1", 0), attrF("x2", 0), attrF("y2", 0));
        toDevice(ppath);
        strokePath(ppath);
    } else if (_stricmp(tag, "polyline") == 0 || _stricmp(tag, "polygon") == 0) {
        std::string pts = attrS("points", "");
        std::vector<float> nums;
        size_t pi = 0;
        float pv = 0;
        while (SvgReadNumber(pts, pi, pv)) nums.push_back(pv);
        if (nums.size() >= 4) {
            Gp::GraphicsPath ppath;
            ppath.AddLines(&Gp::PointF(nums[0], nums[1]), (INT)(nums.size() / 2));
            if (_stricmp(tag, "polygon") == 0) ppath.CloseFigure();
            toDevice(ppath);
            fillPath(ppath);
            strokePath(ppath);
        }
    } else if (_stricmp(tag, "text") == 0) {
        litehtml::string t;
        el->get_text(t);
        if (!t.empty()) {
            float fsz = attrF("font-size", 14) * scale;
            if (fsz < 1.0f) fsz = 1.0f;
            Gp::Font font(L"微软雅黑", fsz, Gp::FontStyleRegular, Gp::UnitPixel);
            Gp::SolidBrush b(SvgApplyAlpha(fill, fillOpacity * opacity));
            std::wstring wt;
            int n = ::MultiByteToWideChar(CP_UTF8, 0, t.c_str(), (int)t.size(), NULL, 0);
            if (n > 0) {
                wt.resize(n);
                ::MultiByteToWideChar(CP_UTF8, 0, t.c_str(), (int)t.size(), &wt[0], n);
            }
            // 文字位置也走基准变换（viewBox 缩放 + 原点平移）
            Gp::PointF org(attrF("x", 0), attrF("y", 0));
            Gp::PointF pts[1] = { org };
            base.TransformPoints(pts, 1);
            g.DrawString(wt.c_str(), (INT)wt.size(), &font, pts[0], &b);
        }
    }

    // 递归子元素（g 分组等）：填充色向子元素继承
    for (const auto& child : el->children()) {
        DrawSvgElement(g, child, svgRoot, base, scale, doFill ? fill : inheritedFill);
    }
}

// 在指定矩形区域内绘制一个 <svg> 元素（供 el_svg 与 el_button 复用）
// svgEl：svg 元素本身；rect：设备坐标绘制区域
static void DrawSvgInRect(Gp::Graphics& g, const litehtml::element::ptr& svgEl,
                          const Gp::RectF& rect)
{
    if (!svgEl) return;

    // 基准变换：把 viewBox 用户坐标系映射到控件显示区域
    // 有 viewBox 时按 preserveAspectRatio 默认值 xMidYMid meet 等比缩放并居中
    Gp::Matrix base;
    float scale = 1.0f;
    const char* vb = svgEl->get_attr("viewBox");
    if (vb && vb[0]) {
        float vx = 0, vy = 0, vw = 0, vh = 0;
        if (sscanf_s(vb, "%f %f %f %f", &vx, &vy, &vw, &vh) == 4 && vw > 0 && vh > 0) {
            const float sx = rect.Width / vw;
            const float sy = rect.Height / vh;
            scale = sx < sy ? sx : sy;                       // meet：取小值保证完整显示
            const float dw = vw * scale, dh = vh * scale;
            const float dx = rect.X + (rect.Width  - dw) * 0.5f;
            const float dy = rect.Y + (rect.Height - dh) * 0.5f;
            base.Translate(dx - vx * scale, dy - vy * scale);
            base.Scale(scale, scale, Gp::MatrixOrderAppend);
        }
    } else {
        // 没有 viewBox：直接平移到绘制原点，1:1 绘制
        base.Translate(rect.X, rect.Y);
    }

    // 继承填充色：图标 svg 通常不写 fill 属性，而是靠 CSS 的 fill（或 color + currentColor）指定。
    // litehtml 会把未知属性 fill 原样存为字符串，get_custom_property 还能沿父链继承。
    std::string cssFill = svgEl->get_custom_property(_id("fill"), "");
    Gp::Color inheritColor = container_win32::ToGpColor(svgEl->css().get_color());
    if (!cssFill.empty()) {
        Gp::Color rc = inheritColor;
        std::string rg;
        std::string lower = SvgLower(SvgTrim(cssFill));
        if (lower == "currentcolor" || lower == "inherit") {
            rc = inheritColor;                    // currentColor → 元素 color
        } else if (SvgResolvePaint(svgEl, cssFill, inheritColor, rc, rg)) {
            inheritColor = rc;
        }
    }
    // svg 元素自身的 fill 属性优先级高于 CSS
    const char* rootFill = svgEl->get_attr("fill");
    if (rootFill && rootFill[0]) {
        Gp::Color rc;
        std::string rg;
        if (SvgResolvePaint(svgEl, rootFill, inheritColor, rc, rg)) inheritColor = rc;
    }

    // 递归绘制子形状（基准变换 + 继承填充色）
    for (const auto& child : svgEl->children()) {
        DrawSvgElement(g, child, svgEl, base, scale, inheritColor);
    }
}

void el_svg::draw(uint_ptr hdc, int x, int y, const position* clip,
    const std::shared_ptr<render_item>& ri)
{
    HDC dc = (HDC)hdc;
    if (!dc) return;

    position pos = GetDrawPos(x, y, ri);
    if (!pos.does_intersect(clip)) return;

    Gp::Graphics g(dc);
    g.SetSmoothingMode(Gp::SmoothingModeAntiAlias);
    g.SetTextRenderingHint(Gp::TextRenderingHintAntiAlias);

    Gp::RectF rect((Gp::REAL)pos.x, (Gp::REAL)pos.y, (Gp::REAL)pos.width, (Gp::REAL)pos.height);
    DrawSvgInRect(g, shared_from_this(), rect);
}

void el_svg::on_click()
{
}

} // namespace litehtml
