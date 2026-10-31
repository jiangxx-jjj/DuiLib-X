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

    m_style.add_property(_width_, std::to_string(m_width).c_str());
    m_style.add_property(_height_, std::to_string(m_height).c_str());
}

void el_svg::get_content_size(size& sz, int max_width)
{
    sz.width = m_width;
    sz.height = m_height;
}

// 递归绘制 SVG 子元素（rect/circle/line/text 基本形状）
static void DrawSvgElement(Gp::Graphics& g, const litehtml::element::ptr& el,
                           float ox, float oy)
{
    if (!el) return;
    const char* tag = el->get_tagName();
    if (!tag) return;

    // 通用取属性
    auto attrF = [&](const char* name, float def) -> float {
        const char* v = el->get_attr(name);
        return v ? (float)atof(v) : def;
    };
    auto attrS = [&](const char* name, const char* def) -> const char* {
        const char* v = el->get_attr(name);
        return v ? v : def;
    };
    Gp::Color fill = container_win32::ParseColor(attrS("fill", "#000000"));
    Gp::Color stroke = container_win32::ParseColor(attrS("stroke", "#000000"));
    const char* strokeW = el->get_attr("stroke-width");
    float sw = strokeW ? (float)atof(strokeW) : 1.0f;

    if (_stricmp(tag, "rect") == 0) {
        float rx = ox + attrF("x", 0), ry = oy + attrF("y", 0);
        float rw = attrF("width", 0), rh = attrF("height", 0);
        if (el->get_attr("fill") && _stricmp(el->get_attr("fill"), "none") != 0) {
            Gp::SolidBrush b(fill);
            g.FillRectangle(&b, rx, ry, rw, rh);
        }
        if (el->get_attr("stroke")) {
            Gp::Pen p(stroke, sw);
            g.DrawRectangle(&p, rx, ry, rw, rh);
        }
    } else if (_stricmp(tag, "circle") == 0) {
        float cx = ox + attrF("cx", 0), cy = oy + attrF("cy", 0);
        float r = attrF("r", 0);
        if (el->get_attr("fill") && _stricmp(el->get_attr("fill"), "none") != 0) {
            Gp::SolidBrush b(fill);
            g.FillEllipse(&b, cx - r, cy - r, r * 2, r * 2);
        }
        if (el->get_attr("stroke")) {
            Gp::Pen p(stroke, sw);
            g.DrawEllipse(&p, cx - r, cy - r, r * 2, r * 2);
        }
    } else if (_stricmp(tag, "line") == 0) {
        Gp::Pen p(stroke, sw);
        g.DrawLine(&p, ox + attrF("x1", 0), oy + attrF("y1", 0),
                       ox + attrF("x2", 0), oy + attrF("y2", 0));
    } else if (_stricmp(tag, "text") == 0) {
        litehtml::string t;
        el->get_text(t);
        if (!t.empty()) {
            Gp::Font font(L"微软雅黑", attrF("font-size", 14), Gp::FontStyleRegular, Gp::UnitPixel);
            Gp::SolidBrush b(fill);
            std::wstring wt;
            int n = ::MultiByteToWideChar(CP_UTF8, 0, t.c_str(), (int)t.size(), NULL, 0);
            if (n > 0) {
                wt.resize(n);
                ::MultiByteToWideChar(CP_UTF8, 0, t.c_str(), (int)t.size(), &wt[0], n);
            }
            g.DrawString(wt.c_str(), (INT)wt.size(), &font,
                         Gp::PointF(ox + attrF("x", 0), oy + attrF("y", 0)), &b);
        }
    }

    // 递归子元素（g 分组等）
    for (const auto& child : el->children()) {
        DrawSvgElement(g, child, ox, oy);
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

    // 白底
    Gp::SolidBrush bg(Gp::Color(255, 255, 255, 255));
    g.FillRectangle(&bg, (Gp::REAL)pos.x, (Gp::REAL)pos.y,
                    (Gp::REAL)pos.width, (Gp::REAL)pos.height);

    // 递归绘制子形状
    for (const auto& child : m_children) {
        DrawSvgElement(g, child, (float)pos.x, (float)pos.y);
    }
}

void el_svg::on_click()
{
}

} // namespace litehtml
