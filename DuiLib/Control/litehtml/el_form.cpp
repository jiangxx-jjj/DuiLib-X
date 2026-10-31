#include "StdAfx.h"
#include "el_form.h"
#include "container_win32.h"
#include <gdiplus.h>

namespace Gp = Gdiplus;

namespace litehtml
{

// ============================ UTF-8 工具 ============================

static size_t Utf8CharLen(const std::string& s, size_t bytePos)
{
    if (bytePos >= s.size()) return 0;
    unsigned char c = (unsigned char)s[bytePos];
    if ((c & 0x80) == 0) return 1;
    if ((c & 0xE0) == 0xC0) return 2;
    if ((c & 0xF0) == 0xE0) return 3;
    if ((c & 0xF8) == 0xF0) return 4;
    return 1;
}

static size_t Utf8BytePos(const std::string& s, size_t charIdx)
{
    size_t bytePos = 0, c = 0;
    while (bytePos < s.size() && c < charIdx) {
        bytePos += Utf8CharLen(s, bytePos);
        c++;
    }
    return bytePos;
}

static size_t Utf8CharCount(const std::string& s)
{
    size_t count = 0;
    for (size_t i = 0; i < s.size(); i += Utf8CharLen(s, i)) count++;
    return count;
}

// ============================ 输入框 ============================

el_input_text::el_input_text(const std::shared_ptr<document>& doc, container_win32* container)
    : html_tag(doc), m_container(container), m_caretPos(0), m_focused(false)
{
    const char* val = get_attr("value");
    if (val) m_value = val;
    m_caretPos = Utf8CharCount(m_value);
}

void el_input_text::draw(uint_ptr hdc, int x, int y, const position* clip,
    const std::shared_ptr<render_item>& ri)
{
    // 先画背景/边框（用默认实现）
    html_tag::draw(hdc, x, y, clip, ri);

    if (!m_container) return;
    HDC dc = (HDC)hdc;
    if (!dc) return;

    // 计算绘制区域
    position pos = ri->get_placement();

    Gp::Graphics g(dc);
    g.SetSmoothingMode(Gp::SmoothingModeAntiAlias);
    g.SetTextRenderingHint(Gp::TextRenderingHintAntiAlias);

    // 密码显示
    std::string displayText = m_value;
    const char* type = get_attr("type");
    if (type && _stricmp(type, "password") == 0 && !displayText.empty()) {
        displayText.assign(displayText.size(), '*');
    }

    // 绘制文本
    Gp::Font* pFont = NULL;
    auto it = m_container->m_fontById.find(get_document()->get_font(
        css().get_font_family().c_str(), css().get_font_size(),
        css().get_font_weight() >= 700 ? "bold" : "normal",
        css().get_font_style() == font_style_italic ? "italic" : "normal",
        "", NULL));
    if (it != m_container->m_fontById.end()) pFont = it->second.pFont;

    if (pFont) {
        Gp::Color textColor = container_win32::ToGpColor(css().get_color());
        Gp::SolidBrush brush(textColor);
        std::wstring wText;
        int n = ::MultiByteToWideChar(CP_UTF8, 0, displayText.c_str(), -1, NULL, 0);
        if (n > 0) { std::wstring w(n - 1, 0); ::MultiByteToWideChar(CP_UTF8, 0, displayText.c_str(), -1, &w[0], n); wText = w; }

        Gp::RectF rc((Gp::REAL)(pos.x + 4), (Gp::REAL)pos.y, (Gp::REAL)(pos.width - 8), (Gp::REAL)pos.height);
        Gp::StringFormat fmt;
        fmt.SetLineAlignment(Gp::StringAlignmentCenter);
        fmt.SetTrimming(Gp::StringTrimmingEllipsisCharacter);
        g.DrawString(wText.c_str(), (INT)wText.size(), pFont, rc, &fmt, &brush);

        // 焦点光标
        if (m_focused) {
            size_t caretByte = Utf8BytePos(displayText, m_caretPos);
            std::string beforeCaret = displayText.substr(0, caretByte);
            int caretX = pos.x + 4;
            if (!beforeCaret.empty()) {
                caretX += m_container->text_width(beforeCaret.c_str(),
                    get_document()->get_font(css().get_font_family().c_str(),
                        css().get_font_size(), css().get_font_weight() >= 700 ? "bold" : "normal",
                        css().get_font_style() == font_style_italic ? "italic" : "normal", "", NULL));
            }
            Gp::Pen caretPen(Gp::Color(255, 0, 0, 0), 1.0f);
            g.DrawLine(&caretPen, (Gp::REAL)caretX, (Gp::REAL)(pos.y + 3),
                       (Gp::REAL)caretX, (Gp::REAL)(pos.y + pos.height - 3));
        }
    }
}

void el_input_text::on_click()
{
    m_focused = true;
    m_caretPos = Utf8CharCount(m_value);
    html_tag::on_click();
}

bool el_input_text::on_lbutton_down()
{
    m_focused = true;
    m_caretPos = Utf8CharCount(m_value);
    return html_tag::on_lbutton_down();
}

bool el_input_text::InputChar(wchar_t ch)
{
    if (!m_focused || ch < 32) return false;
    size_t caretByte = Utf8BytePos(m_value, m_caretPos);
    std::string insert;
    if (ch > 127) {
        std::wstring w(1, ch);
        int n = ::WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, NULL, 0, NULL, NULL);
        if (n > 0) { std::string s(n - 1, 0); ::WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, &s[0], n, NULL, NULL); insert = s; }
    } else {
        insert = (char)ch;
    }
    m_value.insert(caretByte, insert);
    m_caretPos++;
    return true;
}

bool el_input_text::InputKeyDown(int vkCode)
{
    if (!m_focused) return false;
    size_t charCount = Utf8CharCount(m_value);

    if (vkCode == VK_BACK) {
        if (m_caretPos > 0) {
            size_t bytePos = Utf8BytePos(m_value, m_caretPos);
            size_t prevByte = Utf8BytePos(m_value, m_caretPos - 1);
            m_value.erase(prevByte, bytePos - prevByte);
            m_caretPos--;
        }
        return true;
    }
    if (vkCode == VK_DELETE) {
        if (m_caretPos < charCount) {
            size_t bytePos = Utf8BytePos(m_value, m_caretPos);
            size_t nextByte = Utf8BytePos(m_value, m_caretPos + 1);
            m_value.erase(bytePos, nextByte - bytePos);
        }
        return true;
    }
    if (vkCode == VK_LEFT) { if (m_caretPos > 0) m_caretPos--; return true; }
    if (vkCode == VK_RIGHT) { if (m_caretPos < charCount) m_caretPos++; return true; }
    if (vkCode == VK_HOME) { m_caretPos = 0; return true; }
    if (vkCode == VK_END) { m_caretPos = charCount; return true; }
    return false;
}

// ============================ 勾选框 / 单选框 ============================

el_input_check::el_input_check(const std::shared_ptr<document>& doc, container_win32* container, bool isRadio)
    : html_tag(doc), m_container(container), m_checked(false), m_isRadio(isRadio)
{
    if (get_attr("checked")) m_checked = true;
}

void el_input_check::draw(uint_ptr hdc, int x, int y, const position* clip,
    const std::shared_ptr<render_item>& ri)
{
    html_tag::draw(hdc, x, y, clip, ri);

    if (!m_container) return;
    HDC dc = (HDC)hdc;
    if (!dc) return;

    position pos = ri->get_placement();
    Gp::Graphics g(dc);
    g.SetSmoothingMode(Gp::SmoothingModeAntiAlias);

    float size = (pos.width < pos.height) ? (float)pos.width : (float)pos.height;
    float cx = pos.x + (pos.width - size) / 2;
    float cy = pos.y + (pos.height - size) / 2;

    if (m_isRadio) {
        // 圆
        Gp::SolidBrush bg(Gp::Color(255, 255, 255, 255));
        g.FillEllipse(&bg, cx, cy, size, size);
        Gp::Pen border(Gp::Color(255, 160, 160, 160), 1.0f);
        g.DrawEllipse(&border, cx, cy, size, size);
        if (m_checked) {
            Gp::SolidBrush dot(Gp::Color(255, 45, 91, 215));
            float d = size * 0.6f;
            g.FillEllipse(&dot, cx + (size - d) / 2, cy + (size - d) / 2, d, d);
        }
    } else {
        // 方
        Gp::SolidBrush bg(Gp::Color(255, 255, 255, 255));
        g.FillRectangle(&bg, cx, cy, size, size);
        Gp::Pen border(Gp::Color(255, 160, 160, 160), 1.0f);
        g.DrawRectangle(&border, cx, cy, size, size);
        if (m_checked) {
            Gp::Pen tick(Gp::Color(255, 45, 91, 215), 2.0f);
            g.DrawLine(&tick, cx + size * 0.2f, cy + size * 0.5f,
                       cx + size * 0.42f, cy + size * 0.72f);
            g.DrawLine(&tick, cx + size * 0.42f, cy + size * 0.72f,
                       cx + size * 0.8f, cy + size * 0.28f);
        }
    }
}

void el_input_check::on_click()
{
    if (m_isRadio) {
        // 单选框：清除同 name 组
        const char* name = get_attr("name");
        if (name) {
            auto doc = get_document();
            if (doc) {
                // 遍历所有元素找同 name 的 radio
                // 简化：通过 select_all("input[type=radio]") 再过滤
                elements_list radios;
                doc->root()->select_all("input[type=radio]");
                // 实际 litehtml 的 select_all 不支持属性选择器，需要手动遍历
                // 简化实现：只清除自己（后续在 container 里做全局互斥）
            }
        }
        m_checked = true;
    } else {
        m_checked = !m_checked;
    }
    html_tag::on_click();
}

// ============================ 按钮 ============================

el_button::el_button(const std::shared_ptr<document>& doc, container_win32* container)
    : html_tag(doc), m_container(container)
{
}

void el_button::draw(uint_ptr hdc, int x, int y, const position* clip,
    const std::shared_ptr<render_item>& ri)
{
    html_tag::draw(hdc, x, y, clip, ri);

    if (!m_container) return;
    HDC dc = (HDC)hdc;
    if (!dc) return;

    position pos = ri->get_placement();
    Gp::Graphics g(dc);
    g.SetSmoothingMode(Gp::SmoothingModeAntiAlias);
    g.SetTextRenderingHint(Gp::TextRenderingHintAntiAlias);

    // 按钮文字
    std::string label;
    const char* val = get_attr("value");
    if (val) label = val;
    if (label.empty()) {
        string text;
        get_text(text);
        label = text;
    }

    Gp::Font* pFont = NULL;
    auto it = m_container->m_fontById.find(get_document()->get_font(
        css().get_font_family().c_str(), css().get_font_size(),
        css().get_font_weight() >= 700 ? "bold" : "normal",
        css().get_font_style() == font_style_italic ? "italic" : "normal",
        "", NULL));
    if (it != m_container->m_fontById.end()) pFont = it->second.pFont;

    if (pFont && !label.empty()) {
        Gp::Color textColor = container_win32::ToGpColor(css().get_color());
        Gp::SolidBrush brush(textColor);
        std::wstring wText;
        int n = ::MultiByteToWideChar(CP_UTF8, 0, label.c_str(), -1, NULL, 0);
        if (n > 0) { std::wstring w(n - 1, 0); ::MultiByteToWideChar(CP_UTF8, 0, label.c_str(), -1, &w[0], n); wText = w; }

        Gp::RectF rc((Gp::REAL)pos.x, (Gp::REAL)pos.y, (Gp::REAL)pos.width, (Gp::REAL)pos.height);
        Gp::StringFormat fmt;
        fmt.SetAlignment(Gp::StringAlignmentCenter);
        fmt.SetLineAlignment(Gp::StringAlignmentCenter);
        g.DrawString(wText.c_str(), (INT)wText.size(), pFont, rc, &fmt, &brush);
    }
}

void el_button::on_click()
{
    // 触发按钮点击回调
    const char* name = get_attr("name");
    if (name && m_container) {
        // 通过 container 通知到 DuiLib
        // TODO: 实现按钮点击通知
    }
    html_tag::on_click();
}

// ============================ 下拉框 ============================

el_select::el_select(const std::shared_ptr<document>& doc, container_win32* container)
    : html_tag(doc), m_container(container), m_selectedIndex(0)
{
}

void el_select::draw(uint_ptr hdc, int x, int y, const position* clip,
    const std::shared_ptr<render_item>& ri)
{
    html_tag::draw(hdc, x, y, clip, ri);

    if (!m_container) return;
    HDC dc = (HDC)hdc;
    if (!dc) return;

    position pos = ri->get_placement();
    Gp::Graphics g(dc);
    g.SetSmoothingMode(Gp::SmoothingModeAntiAlias);
    g.SetTextRenderingHint(Gp::TextRenderingHintAntiAlias);

    // 当前选中项文字
    std::string selText;
    int idx = 0;
    for (const auto& child : m_children) {
        if (child->get_tagName() && _stricmp(child->get_tagName(), "option") == 0) {
            if (idx == m_selectedIndex) {
                string text;
                child->get_text(text);
                selText = text;
                break;
            }
            idx++;
        }
    }

    Gp::Font* pFont = NULL;
    auto it = m_container->m_fontById.find(get_document()->get_font(
        css().get_font_family().c_str(), css().get_font_size(),
        css().get_font_weight() >= 700 ? "bold" : "normal",
        css().get_font_style() == font_style_italic ? "italic" : "normal",
        "", NULL));
    if (it != m_container->m_fontById.end()) pFont = it->second.pFont;

    if (pFont && !selText.empty()) {
        Gp::Color textColor = container_win32::ToGpColor(css().get_color());
        Gp::SolidBrush brush(textColor);
        std::wstring wText;
        int n = ::MultiByteToWideChar(CP_UTF8, 0, selText.c_str(), -1, NULL, 0);
        if (n > 0) { std::wstring w(n - 1, 0); ::MultiByteToWideChar(CP_UTF8, 0, selText.c_str(), -1, &w[0], n); wText = w; }

        Gp::RectF rc((Gp::REAL)(pos.x + 4), (Gp::REAL)pos.y, (Gp::REAL)(pos.width - 20), (Gp::REAL)pos.height);
        Gp::StringFormat fmt;
        fmt.SetLineAlignment(Gp::StringAlignmentCenter);
        g.DrawString(wText.c_str(), (INT)wText.size(), pFont, rc, &fmt, &brush);
    }

    // 下拉箭头
    Gp::SolidBrush arrow(Gp::Color(255, 110, 120, 140));
    float ax = pos.x + pos.width - 18;
    float ay = pos.y + pos.height / 2 - 2;
    Gp::PointF pts[3] = {
        Gp::PointF(ax, ay), Gp::PointF(ax + 9, ay), Gp::PointF(ax + 4.5f, ay + 6)
    };
    g.FillPolygon(&arrow, pts, 3);
}

void el_select::on_click()
{
    // 循环切换选项（简化实现）
    int optCount = 0;
    for (const auto& child : m_children) {
        if (child->get_tagName() && _stricmp(child->get_tagName(), "option") == 0) optCount++;
    }
    if (optCount > 0) {
        m_selectedIndex = (m_selectedIndex + 1) % optCount;
    }
    html_tag::on_click();
}

void el_select::SetSelectedIndex(int idx)
{
    int optCount = 0;
    for (const auto& child : m_children) {
        if (child->get_tagName() && _stricmp(child->get_tagName(), "option") == 0) optCount++;
    }
    if (optCount > 0) {
        m_selectedIndex = idx % optCount;
        if (m_selectedIndex < 0) m_selectedIndex = 0;
    }
}

std::string el_select::GetValue() const
{
    int idx = 0;
    for (const auto& child : m_children) {
        if (child->get_tagName() && _stricmp(child->get_tagName(), "option") == 0) {
            if (idx == m_selectedIndex) {
                const char* val = child->get_attr("value");
                if (val) return val;
                string text;
                child->get_text(text);
                return text;
            }
            idx++;
        }
    }
    return "";
}

// ============================ 多行文本框 ============================

el_textarea::el_textarea(const std::shared_ptr<document>& doc, container_win32* container)
    : html_tag(doc), m_container(container), m_caretPos(0), m_focused(false)
{
    string text;
    get_text(text);
    m_value = text;
    m_caretPos = Utf8CharCount(m_value);
}

void el_textarea::draw(uint_ptr hdc, int x, int y, const position* clip,
    const std::shared_ptr<render_item>& ri)
{
    html_tag::draw(hdc, x, y, clip, ri);

    if (!m_container) return;
    HDC dc = (HDC)hdc;
    if (!dc) return;

    position pos = ri->get_placement();
    Gp::Graphics g(dc);
    g.SetSmoothingMode(Gp::SmoothingModeAntiAlias);
    g.SetTextRenderingHint(Gp::TextRenderingHintAntiAlias);

    if (m_value.empty()) return;

    Gp::Font* pFont = NULL;
    auto it = m_container->m_fontById.find(get_document()->get_font(
        css().get_font_family().c_str(), css().get_font_size(),
        css().get_font_weight() >= 700 ? "bold" : "normal",
        css().get_font_style() == font_style_italic ? "italic" : "normal",
        "", NULL));
    if (it != m_container->m_fontById.end()) pFont = it->second.pFont;

    if (pFont) {
        Gp::Color textColor = container_win32::ToGpColor(css().get_color());
        Gp::SolidBrush brush(textColor);
        std::wstring wText;
        int n = ::MultiByteToWideChar(CP_UTF8, 0, m_value.c_str(), -1, NULL, 0);
        if (n > 0) { std::wstring w(n - 1, 0); ::MultiByteToWideChar(CP_UTF8, 0, m_value.c_str(), -1, &w[0], n); wText = w; }

        Gp::RectF rc((Gp::REAL)(pos.x + 4), (Gp::REAL)(pos.y + 4),
                     (Gp::REAL)(pos.width - 8), (Gp::REAL)(pos.height - 8));
        g.DrawString(wText.c_str(), (INT)wText.size(), pFont, rc, NULL, &brush);
    }
}

void el_textarea::on_click()
{
    m_focused = true;
    m_caretPos = Utf8CharCount(m_value);
    html_tag::on_click();
}

bool el_textarea::on_lbutton_down()
{
    m_focused = true;
    m_caretPos = Utf8CharCount(m_value);
    return html_tag::on_lbutton_down();
}

bool el_textarea::InputChar(wchar_t ch)
{
    if (!m_focused || ch < 32) return false;
    size_t caretByte = Utf8BytePos(m_value, m_caretPos);
    std::string insert;
    if (ch > 127) {
        std::wstring w(1, ch);
        int n = ::WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, NULL, 0, NULL, NULL);
        if (n > 0) { std::string s(n - 1, 0); ::WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, &s[0], n, NULL, NULL); insert = s; }
    } else {
        insert = (char)ch;
    }
    m_value.insert(caretByte, insert);
    m_caretPos++;
    return true;
}

bool el_textarea::InputKeyDown(int vkCode)
{
    if (!m_focused) return false;
    size_t charCount = Utf8CharCount(m_value);

    if (vkCode == VK_BACK) {
        if (m_caretPos > 0) {
            size_t bytePos = Utf8BytePos(m_value, m_caretPos);
            size_t prevByte = Utf8BytePos(m_value, m_caretPos - 1);
            m_value.erase(prevByte, bytePos - prevByte);
            m_caretPos--;
        }
        return true;
    }
    if (vkCode == VK_DELETE) {
        if (m_caretPos < charCount) {
            size_t bytePos = Utf8BytePos(m_value, m_caretPos);
            size_t nextByte = Utf8BytePos(m_value, m_caretPos + 1);
            m_value.erase(bytePos, nextByte - bytePos);
        }
        return true;
    }
    if (vkCode == VK_LEFT) { if (m_caretPos > 0) m_caretPos--; return true; }
    if (vkCode == VK_RIGHT) { if (m_caretPos < charCount) m_caretPos++; return true; }
    if (vkCode == VK_HOME) { m_caretPos = 0; return true; }
    if (vkCode == VK_END) { m_caretPos = charCount; return true; }
    return false;
}

void el_textarea::SetValue(const std::string& v)
{
    m_value = v;
    m_caretPos = Utf8CharCount(m_value);
}

} // namespace litehtml
