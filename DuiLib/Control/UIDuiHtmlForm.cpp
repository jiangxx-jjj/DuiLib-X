#include "StdAfx.h"
#include "UIDuiHtmlForm.h"
#include "UIDuiHtmlContainer.h"

namespace DuiLib
{

// ============================ 工具函数 ============================

// UTF-8 → 宽字符
static std::wstring FormUtf8ToW(const std::string& s)
{
    std::wstring w;
    if (s.empty()) return w;
    int n = ::MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), NULL, 0);
    if (n <= 0) return w;
    w.resize(n);
    ::MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &w[0], n);
    return w;
}

// 宽字符 → UTF-8
static std::string FormWToUtf8(const std::wstring& w)
{
    std::string s;
    if (w.empty()) return s;
    int n = ::WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), NULL, 0, NULL, NULL);
    if (n <= 0) return s;
    s.resize(n);
    ::WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), &s[0], n, NULL, NULL);
    return s;
}

// 取 UTF-8 字符串的字符数（不是字节数）
static int FormUtf8CharCount(const std::string& s)
{
    int count = 0;
    for (size_t i = 0; i < s.size(); i++) {
        if ((s[i] & 0xC0) != 0x80) count++;   // 跳过后续字节
    }
    return count;
}

// 取 UTF-8 字符串前 n 个字符对应的字节长度
static size_t FormUtf8ByteOffset(const std::string& s, int nChars)
{
    if (nChars <= 0) return 0;
    int count = 0;
    for (size_t i = 0; i < s.size(); i++) {
        if ((s[i] & 0xC0) != 0x80) {
            if (count == nChars) return i;
            count++;
        }
    }
    return s.size();
}

// ============================ 构造 ============================

CHtmlFormElement::CHtmlFormElement(const std::shared_ptr<litehtml::document>& doc)
    : litehtml::html_tag(doc)
    , m_iFormType(HtmlFormText)
    , m_bChecked(false)
    , m_iSize(20)
    , m_iCols(20)
    , m_iRows(3)
    , m_iCaret(0)
    , m_iSelIndex(0)
{
    // 表单控件一律是 inline-block（与浏览器默认行为一致）
    m_css.set_display(litehtml::display_inline_block);
}

// ============================ 属性解析 ============================

void CHtmlFormElement::parse_attributes(void)
{
    // 先让基类递归解析子元素
    litehtml::html_tag::parse_attributes();

    const char* tag = get_tagName();

    // name / value
    const char* name = get_attr("name");
    if (name) m_sName = name;
    const char* value = get_attr("value");
    if (value) m_sValue = value;

    // 判断控件类型
    if (tag && !strcmp(tag, "select")) {
        m_iFormType = HtmlFormSelect;
        ParseSelectOptions();
    } else if (tag && !strcmp(tag, "textarea")) {
        m_iFormType = HtmlFormTextArea;
        const char* cols = get_attr("cols");
        if (cols) m_iCols = atoi(cols);
        const char* rows = get_attr("rows");
        if (rows) m_iRows = atoi(rows);
        // textarea 的内容是子文本节点
        litehtml::string txt;
        get_text(txt);
        if (!txt.empty()) m_sValue = txt;
    } else if (tag && !strcmp(tag, "button")) {
        m_iFormType = HtmlFormButton;
        litehtml::string txt;
        get_text(txt);
        if (!txt.empty()) m_sLabel = txt;
        // 按钮文字由我们自己绘制，必须清空子文本节点，
        // 否则 litehtml 会把同一段文字再画一遍，与自绘内容重叠成黑块
        clearRecursive();
        m_children.clear();
    } else {
        // <input>
        const char* type = get_attr("type");
        ParseInputType(type);
    }

    // checked 属性（checkbox / radio）
    if (get_attr("checked")) m_bChecked = true;

    // 显式 width / height 属性覆盖（HTML 老式写法）
    const char* w = get_attr("width");
    if (w) m_style.add_property(litehtml::_width_, w);
    const char* h = get_attr("height");
    if (h) m_style.add_property(litehtml::_height_, h);
}

void CHtmlFormElement::ParseInputType(const char* type)
{
    if (type == NULL || type[0] == '\0') {
        m_iFormType = HtmlFormText;
        return;
    }
    if (!_stricmp(type, "password"))      m_iFormType = HtmlFormPassword;
    else if (!_stricmp(type, "checkbox")) m_iFormType = HtmlFormCheckbox;
    else if (!_stricmp(type, "radio"))    m_iFormType = HtmlFormRadio;
    else if (!_stricmp(type, "hidden"))   m_iFormType = HtmlFormHidden;
    else if (!_stricmp(type, "submit") ||
             !_stricmp(type, "button") ||
             !_stricmp(type, "reset"))    m_iFormType = HtmlFormButton;
    else                                  m_iFormType = HtmlFormText;

    if (m_iFormType == HtmlFormButton && m_sLabel.empty()) {
        // <input type=submit> 的显示文字取 value
        m_sLabel = m_sValue.empty() ? "Submit" : m_sValue;
    }
}

void CHtmlFormElement::ParseSelectOptions(void)
{
    m_options.clear();
    m_iSelIndex = 0;

    // 遍历 <option> 子元素
    for (const auto& child : m_children) {
        if (child->get_tagName() && !strcmp(child->get_tagName(), "option")) {
            const char* val = child->get_attr("value");
            litehtml::string txt;
            child->get_text(txt);
            std::string sValue = val ? val : txt;
            std::string sText  = txt.empty() ? sValue : txt;
            if (child->get_attr("selected")) m_iSelIndex = (int)m_options.size();
            m_options.push_back(std::make_pair(sValue, sText));
        }
    }
    if (m_options.empty()) {
        m_options.push_back(std::make_pair(std::string(""), std::string("")));
    }
}

// ============================ 尺寸计算 ============================

bool CHtmlFormElement::is_replaced(void) const
{
    return true;   // 按替换元素处理：尺寸由自己决定
}

int CHtmlFormElement::GetFontHeight(void) const
{
    litehtml::font_metrics fm = css().get_font_metrics();
    int h = fm.height;
    if (h <= 0) h = 18;
    return h;
}

int CHtmlFormElement::MeasureText(const std::string& text) const
{
    litehtml::uint_ptr font = css().get_font();
    if (font == 0) return (int)text.size() * 8;
    return get_document()->container()->text_width(text.c_str(), font);
}

void CHtmlFormElement::get_content_size(litehtml::size& sz, int max_width)
{
    int fh = GetFontHeight();

    switch (m_iFormType)
    {
    case HtmlFormHidden:
        sz.width = 0;
        sz.height = 0;
        break;

    case HtmlFormCheckbox:
    case HtmlFormRadio:
        sz.width = fh;                 // 方形，边长 = 行高
        sz.height = fh;
        break;

    case HtmlFormButton:
        {
            std::string label = m_sLabel.empty() ? std::string("Button") : m_sLabel;
            sz.width = MeasureText(label) + 36;    // 左右各 18px 内边距
            if (sz.width < 76) sz.width = 76;      // 最小按钮宽度
            sz.height = fh + 12;
        }
        break;

    case HtmlFormSelect:
        {
            // 宽度取最长选项
            int maxW = 0;
            for (size_t i = 0; i < m_options.size(); i++) {
                int w = MeasureText(m_options[i].second);
                if (w > maxW) maxW = w;
            }
            sz.width = maxW + 42;      // 左侧内边距 + 右侧箭头区
            if (sz.width < 90) sz.width = 90;
            sz.height = fh + 12;
        }
        break;

    case HtmlFormTextArea:
        {
            sz.width = m_iCols * (fh / 2) + 10;
            sz.height = m_iRows * fh + 8;
        }
        break;

    default:   // text / password
        {
            // 默认 20 字符宽
            int w = m_iSize * (fh / 2) + 12;
            sz.width = w;
            sz.height = fh + 10;
        }
        break;
    }

    // 不超过可用宽度
    if (max_width > 0 && sz.width > max_width) sz.width = max_width;
}

// ============================ 绘制 ============================

void CHtmlFormElement::draw(litehtml::uint_ptr hdc, int x, int y,
                            const litehtml::position* clip,
                            const std::shared_ptr<litehtml::render_item>& ri)
{
    if (m_iFormType == HtmlFormHidden) return;

    HDC hDC = (HDC)hdc;

    // 元素在文档坐标系下的外框
    litehtml::position pos = ri->pos();
    pos.x += x;
    pos.y += y;
    pos += ri->get_paddings();
    pos += ri->get_borders();

    if (clip && !pos.does_intersect(clip)) return;

    // 先画 CSS 背景与边框（html_tag 的默认行为）
    litehtml::html_tag::draw(hdc, x, y, clip, ri);

    // 内容区（去掉 padding / border）
    RECT rcContent = { pos.x + ri->border_left() + ri->padding_left(),
                       pos.y + ri->border_top()  + ri->padding_top(),
                       pos.x + pos.width - ri->border_right() - ri->padding_right(),
                       pos.y + pos.height - ri->border_bottom() - ri->padding_bottom() };
    if (rcContent.right <= rcContent.left || rcContent.bottom <= rcContent.top) return;

    litehtml::web_color color = css().get_color();
    COLORREF clrText = RGB(color.red, color.green, color.blue);
    litehtml::uint_ptr font = css().get_font();

    // 用 HTML 指定的字体绘制控件文字（否则会用 DC 默认字体，中文会显示成方框）
    CDuiHtmlContainer* pHost = GetHostContainer();
    HFONT hFontReal = pHost ? pHost->GetRealFont(font) : NULL;
    HFONT hOldFont = hFontReal ? (HFONT)::SelectObject(hDC, hFontReal) : NULL;

    switch (m_iFormType)
    {
    case HtmlFormCheckbox:
    case HtmlFormRadio:
        {
            // 画方框/圆圈 + 勾选标记
            int size = (rcContent.right - rcContent.left) < (rcContent.bottom - rcContent.top)
                     ? (rcContent.right - rcContent.left) : (rcContent.bottom - rcContent.top);
            RECT rc = { rcContent.left, rcContent.top + (rcContent.bottom - rcContent.top - size) / 2,
                        rcContent.left + size, rcContent.top + (rcContent.bottom - rcContent.top - size) / 2 + size };

            ::SetBkMode(hDC, TRANSPARENT);
            if (m_iFormType == HtmlFormRadio) {
                // 圆
                HBRUSH hBrush = ::CreateSolidBrush(m_bChecked ? RGB(45, 91, 215) : RGB(255, 255, 255));
                HBRUSH hOld = (HBRUSH)::SelectObject(hDC, hBrush);
                HPEN hPen = ::CreatePen(PS_SOLID, 1, RGB(160, 170, 185));
                HPEN hOldPen = (HPEN)::SelectObject(hDC, hPen);
                ::Ellipse(hDC, rc.left, rc.top, rc.right, rc.bottom);
                ::SelectObject(hDC, hOldPen);
                ::SelectObject(hDC, hOld);
                ::DeleteObject(hPen);
                ::DeleteObject(hBrush);
            } else {
                // 方
                HBRUSH hBrush = ::CreateSolidBrush(RGB(255, 255, 255));
                ::FillRect(hDC, &rc, hBrush);
                ::DeleteObject(hBrush);
                HPEN hPen = ::CreatePen(PS_SOLID, 1, RGB(160, 170, 185));
                HPEN hOldPen = (HPEN)::SelectObject(hDC, hPen);
                ::Rectangle(hDC, rc.left, rc.top, rc.right, rc.bottom);
                ::SelectObject(hDC, hOldPen);
                ::DeleteObject(hPen);

                if (m_bChecked) {
                    // 打勾
                    HPEN hTick = ::CreatePen(PS_SOLID, 2, RGB(45, 91, 215));
                    HPEN hOld2 = (HPEN)::SelectObject(hDC, hTick);
                    int cx = (rc.left + rc.right) / 2;
                    int cy = (rc.top + rc.bottom) / 2;
                    ::MoveToEx(hDC, cx - size / 4, cy, NULL);
                    ::LineTo(hDC, cx - 1, cy + size / 5);
                    ::LineTo(hDC, cx + size / 4, cy - size / 5);
                    ::SelectObject(hDC, hOld2);
                    ::DeleteObject(hTick);
                }
            }
        }
        break;

    case HtmlFormButton:
        {
            // 按钮底：浅灰渐变感（用两层填充模拟）
            HBRUSH hBrush = ::CreateSolidBrush(RGB(240, 243, 248));
            ::FillRect(hDC, &rcContent, hBrush);
            ::DeleteObject(hBrush);

            ::SetBkMode(hDC, TRANSPARENT);
            ::SetTextColor(hDC, clrText);

            std::wstring wLabel = FormUtf8ToW(m_sLabel);
            RECT rcText = rcContent;
            ::DrawTextW(hDC, wLabel.c_str(), (int)wLabel.size(), &rcText,
                        DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }
        break;

    case HtmlFormSelect:
        {
            HBRUSH hBrush = ::CreateSolidBrush(RGB(255, 255, 255));
            ::FillRect(hDC, &rcContent, hBrush);
            ::DeleteObject(hBrush);

            ::SetBkMode(hDC, TRANSPARENT);
            ::SetTextColor(hDC, clrText);

            std::string text = (m_iSelIndex >= 0 && m_iSelIndex < (int)m_options.size())
                             ? m_options[m_iSelIndex].second : std::string("");
            std::wstring wText = FormUtf8ToW(text);
            RECT rcText = { rcContent.left + 6, rcContent.top, rcContent.right - 20, rcContent.bottom };
            ::DrawTextW(hDC, wText.c_str(), (int)wText.size(), &rcText,
                        DT_LEFT | DT_VCENTER | DT_SINGLELINE);

            // 右侧下拉箭头
            int ax = rcContent.right - 14;
            int ay = (rcContent.top + rcContent.bottom) / 2;
            POINT pts[3] = { { ax, ay - 2 }, { ax + 8, ay - 2 }, { ax + 4, ay + 3 } };
            HPEN hPen = ::CreatePen(PS_SOLID, 1, RGB(120, 130, 145));
            HPEN hOld = (HPEN)::SelectObject(hDC, hPen);
            HBRUSH hBr = ::CreateSolidBrush(RGB(120, 130, 145));
            HBRUSH hOldBr = (HBRUSH)::SelectObject(hDC, hBr);
            ::Polygon(hDC, pts, 3);
            ::SelectObject(hDC, hOldBr);
            ::SelectObject(hDC, hOld);
            ::DeleteObject(hBr);
            ::DeleteObject(hPen);
        }
        break;

    case HtmlFormTextArea:
    case HtmlFormText:
    case HtmlFormPassword:
        {
            // 白底（背景可能被 CSS 覆盖，这里保证可读）
            HBRUSH hBrush = ::CreateSolidBrush(RGB(255, 255, 255));
            ::FillRect(hDC, &rcContent, hBrush);
            ::DeleteObject(hBrush);

            ::SetBkMode(hDC, TRANSPARENT);
            ::SetTextColor(hDC, clrText);

            // 密码显示为 *
            std::string text = m_sValue;
            if (m_iFormType == HtmlFormPassword) {
                text.assign(FormUtf8CharCount(m_sValue), '*');
            }
            std::wstring wText = FormUtf8ToW(text);

            RECT rcText = { rcContent.left + 6, rcContent.top, rcContent.right - 4, rcContent.bottom };
            ::DrawTextW(hDC, wText.c_str(), (int)wText.size(), &rcText,
                        DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        }
        break;

    default:
        break;
    }

    // 恢复原字体
    if (hOldFont) ::SelectObject(hDC, hOldFont);
}

// ============================ 交互 ============================

bool CHtmlFormElement::on_lbutton_down(void)
{
    // 交给基类维护 :active 伪类
    return litehtml::html_tag::on_lbutton_down();
}

void CHtmlFormElement::on_click(void)
{
    switch (m_iFormType)
    {
    case HtmlFormCheckbox:
        m_bChecked = !m_bChecked;
        break;

    case HtmlFormRadio:
        m_bChecked = true;
        break;

    case HtmlFormButton:
        {
            CDuiHtmlContainer* pHost = GetHostContainer();
            if (pHost) pHost->OnFormSubmit(this);
        }
        break;

    default:
        break;
    }

    // 通知宿主数据已改变
    CDuiHtmlContainer* pHost = GetHostContainer();
    if (pHost) pHost->OnFormChanged(this);

    litehtml::html_tag::on_click();
}

std::shared_ptr<litehtml::render_item> CHtmlFormElement::create_render_item(
    const std::shared_ptr<litehtml::render_item>& parent_ri)
{
    // 复用图片的渲染项（替换元素专用：按 get_content_size 决定尺寸）
    std::shared_ptr<litehtml::render_item> ret = std::make_shared<litehtml::render_item_image>(shared_from_this());
    ret->parent(parent_ri);

    // 与 element::create_render_item 一致的初始化流程
    for (const auto& el : m_children) {
        auto ri = el->create_render_item(ret);
        if (ri) ret->add_child(ri);
    }
    add_render(ret);
    return ret;
}

// ============================ 数据读写 ============================

std::string CHtmlFormElement::GetFormValue(void) const
{
    switch (m_iFormType)
    {
    case HtmlFormCheckbox:
    case HtmlFormRadio:
        return m_bChecked ? (m_sValue.empty() ? std::string("on") : m_sValue) : std::string("");
    case HtmlFormSelect:
        if (m_iSelIndex >= 0 && m_iSelIndex < (int)m_options.size())
            return m_options[m_iSelIndex].first;
        return std::string("");
    default:
        return m_sValue;
    }
}

void CHtmlFormElement::SetFormValue(const std::string& val)
{
    if (m_iFormType == HtmlFormSelect) {
        // 按 value 或文本匹配选项
        for (size_t i = 0; i < m_options.size(); i++) {
            if (m_options[i].first == val || m_options[i].second == val) {
                m_iSelIndex = (int)i;
                return;
            }
        }
        return;
    }
    m_sValue = val;
    m_iCaret = FormUtf8CharCount(m_sValue);
}

void CHtmlFormElement::SetFormChecked(bool bChecked)
{
    m_bChecked = bChecked;
}

void CHtmlFormElement::SetSelectedIndex(int idx)
{
    if (idx >= 0 && idx < (int)m_options.size()) m_iSelIndex = idx;
}

bool CHtmlFormElement::IsEditable(void) const
{
    return m_iFormType == HtmlFormText || m_iFormType == HtmlFormPassword || m_iFormType == HtmlFormTextArea;
}

void CHtmlFormElement::InsertChar(wchar_t ch)
{
    if (!IsEditable()) return;

    std::wstring w = FormUtf8ToW(m_sValue);
    int caret = m_iCaret;
    if (caret < 0) caret = 0;
    if (caret > (int)w.size()) caret = (int)w.size();

    w.insert(w.begin() + caret, ch);
    m_iCaret = caret + 1;
    m_sValue = FormWToUtf8(w);
}

void CHtmlFormElement::Backspace(void)
{
    if (!IsEditable()) return;

    std::wstring w = FormUtf8ToW(m_sValue);
    int caret = m_iCaret;
    if (caret <= 0 || caret > (int)w.size()) return;

    w.erase(w.begin() + (caret - 1));
    m_iCaret = caret - 1;
    m_sValue = FormWToUtf8(w);
}

void CHtmlFormElement::DeleteChar(void)
{
    if (!IsEditable()) return;

    std::wstring w = FormUtf8ToW(m_sValue);
    int caret = m_iCaret;
    if (caret < 0 || caret >= (int)w.size()) return;

    w.erase(w.begin() + caret);
    m_sValue = FormWToUtf8(w);
}

void CHtmlFormElement::SetCaretPos(int pos)
{
    int n = FormUtf8CharCount(m_sValue);
    if (pos < 0) pos = 0;
    if (pos > n) pos = n;
    m_iCaret = pos;
}

CDuiHtmlContainer* CHtmlFormElement::GetHostContainer(void) const
{
    litehtml::document::ptr doc = get_document();
    if (!doc) return NULL;
    return dynamic_cast<CDuiHtmlContainer*>(doc->container());
}

} // namespace DuiLib
