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

// ============================ 按钮 ============================

el_button::el_button(const document::ptr& doc, container_win32* container)
    : el_form_control(doc, container, "inline-block")
{
}

void el_button::get_content_size(size& sz, int max_width)
{
    std::string label;
    const char* val = get_attr("value");
    if (val) {
        label = val;
    } else {
        string text;
        get_text(text);
        label = text;
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

    // 文字
    std::string label;
    const char* val = get_attr("value");
    if (val) label = val;
    if (label.empty()) {
        string text;
        get_text(text);
        label = text;
    }
    if (label.empty()) return;

    g.SetTextRenderingHint(Gp::TextRenderingHintAntiAlias);
    Gp::Font* pFont = GetGpFontOf(this, m_container);
    if (!pFont) return;

    Gp::SolidBrush brush(GetTextColor(this));
    std::wstring wText;
    int n = ::MultiByteToWideChar(CP_UTF8, 0, label.c_str(), (int)label.size(), NULL, 0);
    if (n > 0) {
        wText.resize(n);
        ::MultiByteToWideChar(CP_UTF8, 0, label.c_str(), (int)label.size(), &wText[0], n);
    }
    Gp::RectF rc((Gp::REAL)pos.x, (Gp::REAL)pos.y, (Gp::REAL)pos.width, (Gp::REAL)pos.height);
    Gp::StringFormat fmt;
    fmt.SetAlignment(Gp::StringAlignmentCenter);
    fmt.SetLineAlignment(Gp::StringAlignmentCenter);
    fmt.SetTrimming(Gp::StringTrimmingEllipsisCharacter);
    g.DrawString(wText.c_str(), (INT)wText.size(), pFont, rc, &fmt, &brush);
}

void el_button::on_click()
{
    if (m_container) m_container->OnButtonClicked(GetName());
}

// ============================ 下拉框 ============================

el_select::el_select(const document::ptr& doc, container_win32* container)
    : el_form_control(doc, container, "inline-block")
    , m_selectedIndex(0)
{
}

void el_select::parse_attributes()
{
    // 注意：parse_attributes 时子节点可能还没挂上来，
    // 选项文本在 draw 时实时收集（见 CollectOptions）
    const char* aw = get_attr("width");
    if (aw) m_style.add_property(_width_, aw);
    const char* ah = get_attr("height");
    if (ah) m_style.add_property(_height_, ah);

    // 默认选中 selected 的 option
    int idx = 0;
    for (const auto& child : m_children) {
        if (child->get_tagName() && _stricmp(child->get_tagName(), "option") == 0) {
            if (child->get_attr("selected")) { m_selectedIndex = idx; break; }
            idx++;
        }
    }
}

void el_select::get_content_size(size& sz, int max_width)
{
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

    // 实时收集 option
    m_options.clear();
    for (const auto& child : m_children) {
        if (child->get_tagName() && _stricmp(child->get_tagName(), "option") == 0) {
            string text;
            child->get_text(text);
            m_options.push_back(text);
        }
    }

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

} // namespace litehtml
