#include "StdAfx.h"
#include "HtmlDocument.h"

namespace HtmlEngine
{

HtmlDocument::HtmlDocument()
    : m_bLoaded(false)
    , m_docWidth(0.0f)
    , m_docHeight(0.0f)
    , m_viewportWidth(0.0f)
{
    // 布局引擎接入 GDI+ 字体测量
    m_layout.SetMeasureCallbacks(HtmlPainter::MeasureText, HtmlPainter::MeasureLineHeight);
}

HtmlDocument::~HtmlDocument()
{
}

// ============================ 加载 ============================

bool HtmlDocument::LoadFromString(const std::string& html, const std::string& extraCss)
{
    m_bLoaded = false;
    m_sExtraCss = extraCss;
    m_formValues.clear();

    if (!HtmlParser::Parse(html, m_tree)) {
        return false;
    }

    // 收集 <style> 里的 CSS
    std::string styleCss;
    HtmlParser::CollectStyles(m_tree.root, styleCss);

    // 解析 CSS
    m_css.Clear();
    m_css.ParseStylesheet(styleCss);
    if (!extraCss.empty()) {
        m_css.ParseStylesheet(extraCss);
    }

    // 计算样式
    m_css.ApplyToTree(m_tree);

    // 初始化表单状态
    InitFormValues(m_tree.bodyNode);

    m_bLoaded = true;
    return true;
}

bool HtmlDocument::LoadFromFile(const std::string& path, const std::string& extraCss)
{
    HANDLE hFile = ::CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,
                                 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return false;

    DWORD dwSize = ::GetFileSize(hFile, NULL);
    if (dwSize == 0 || dwSize == INVALID_FILE_SIZE) {
        ::CloseHandle(hFile);
        return false;
    }

    BYTE* pBuf = new BYTE[dwSize + 2];
    DWORD dwRead = 0;
    ::ReadFile(hFile, pBuf, dwSize, &dwRead, NULL);
    ::CloseHandle(hFile);
    pBuf[dwRead] = 0;
    pBuf[dwRead + 1] = 0;

    std::string html;

    // UTF-16 LE BOM
    if (dwRead >= 2 && pBuf[0] == 0xFF && pBuf[1] == 0xFE) {
        int n = (int)(dwRead - 2) / 2;
        std::wstring w((const wchar_t*)(pBuf + 2), n);
        html = WideToUtf8(w);
    }
    // UTF-8 BOM
    else if (dwRead >= 3 && pBuf[0] == 0xEF && pBuf[1] == 0xBB && pBuf[2] == 0xBF) {
        html.assign((const char*)(pBuf + 3), dwRead - 3);
    }
    else {
        html.assign((const char*)pBuf, dwRead);
    }

    delete[] pBuf;
    return LoadFromString(html, extraCss);
}

// ============================ 排版 ============================

float HtmlDocument::Relayout(float viewportWidth)
{
    if (!m_bLoaded) return 0.0f;

    if (viewportWidth < 1.0f) viewportWidth = 1.0f;
    m_viewportWidth = viewportWidth;

    m_docHeight = m_layout.Layout(m_tree, viewportWidth);
    m_docWidth = viewportWidth;

    return m_docHeight;
}

// ============================ 绘制 ============================

void HtmlDocument::Paint(HDC hdc, float scrollX, float scrollY, float clipW, float clipH)
{
    if (!m_bLoaded || hdc == NULL) return;

    // 同步表单状态到 painter
    SyncFormState(m_tree.bodyNode);

    m_painter.Paint(hdc, m_tree, scrollX, scrollY, clipW, clipH);
}

// ============================ 交互 ============================

Node* HtmlDocument::HitTest(float x, float y)
{
    if (!m_bLoaded) return NULL;
    return m_layout.HitTest(m_tree, x, y);
}

bool HtmlDocument::OnMouseMove(float x, float y)
{
    Node* hit = HitTest(x, y);
    if (hit == m_painter.GetHoverNode()) return false;
    m_painter.SetHoverNode(hit);
    return true;
}

void HtmlDocument::OnMouseLeave()
{
    m_painter.SetHoverNode(NULL);
}

Node* HtmlDocument::OnClick(float x, float y)
{
    Node* hit = HitTest(x, y);

    // 向上找最近的 <a href> 或带 name 的元素
    Node* cur = hit;
    Node* anchorNode = NULL;
    Node* clickableNode = NULL;

    while (cur != NULL) {
        if (cur->IsElement()) {
            if (cur->tagName == "a" && cur->HasAttr("href")) {
                anchorNode = cur;
                break;
            }
            // 带 name 的元素都可点击（按钮 / 自定义控件）
            if (cur->HasAttr("name") && clickableNode == NULL) {
                clickableNode = cur;
            }
        }
        cur = cur->parent;
    }

    if (anchorNode != NULL) {
        if (m_fnLinkClick) m_fnLinkClick(anchorNode->GetAttr("href"));
        return anchorNode;
    }

    if (clickableNode != NULL) {
        std::string name = clickableNode->GetAttr("name");
        std::string tag = clickableNode->tagName;
        std::string type = ToLower(clickableNode->GetAttr("type", "text"));

        // checkbox / radio：切换选中
        if (tag == "input" && (type == "checkbox" || type == "radio")) {
            FormValue& fv = m_formValues[clickableNode];
            if (type == "radio") {
                // 同 name 的互斥
                std::string groupName = clickableNode->GetAttr("name");
                // 简单实现：只清除同父级下同名 radio
                if (clickableNode->parent != NULL) {
                    for (size_t i = 0; i < clickableNode->parent->children.size(); i++) {
                        Node* sib = clickableNode->parent->children[i];
                        if (sib->tagName == "input" &&
                            ToLower(sib->GetAttr("type")) == "radio" &&
                            sib->GetAttr("name") == groupName) {
                            m_formValues[sib].checked = false;
                        }
                    }
                }
                fv.checked = true;
            } else {
                fv.checked = !fv.checked;
            }
            if (m_fnFormChanged) m_fnFormChanged(name);
            return clickableNode;
        }

        // select：点击展开（简化：循环切换选项）
        if (tag == "select") {
            int optCount = 0;
            for (size_t i = 0; i < clickableNode->children.size(); i++) {
                if (clickableNode->children[i]->IsElement() &&
                    clickableNode->children[i]->tagName == "option") optCount++;
            }
            if (optCount > 0) {
                FormValue& fv = m_formValues[clickableNode];
                fv.selectedIndex = (fv.selectedIndex + 1) % optCount;
                if (m_fnFormChanged) m_fnFormChanged(name);
            }
            return clickableNode;
        }

        // 文本框：获得焦点
        if ((tag == "input" && (type == "text" || type == "password")) || tag == "textarea") {
            SetFocusNode(clickableNode);
            return clickableNode;
        }

        // 按钮 / 其它带 name 的元素：触发提交
        if (m_fnFormSubmit) m_fnFormSubmit(name);
        return clickableNode;
    }

    // 点空白处取消焦点
    SetFocusNode(NULL);
    return hit;
}

// ============================ 表单状态 ============================

void HtmlDocument::InitFormValues(Node* node)
{
    if (node == NULL) return;

    if (node->IsElement()) {
        if (node->IsFormControl()) {
            FormValue fv;
            fv.value = node->GetAttr("value");
            fv.checked = node->HasAttr("checked");
            fv.selectedIndex = 0;

            // select：找 selected 的 option
            if (node->tagName == "select") {
                int idx = 0;
                for (size_t i = 0; i < node->children.size(); i++) {
                    Node* opt = node->children[i];
                    if (!opt->IsElement() || opt->tagName != "option") continue;
                    if (opt->HasAttr("selected")) { fv.selectedIndex = idx; break; }
                    idx++;
                }
            }

            // textarea：内容是子文本
            if (node->tagName == "textarea") {
                std::string t;
                for (size_t i = 0; i < node->children.size(); i++) {
                    if (node->children[i]->IsText()) t += node->children[i]->text;
                }
                if (!t.empty()) fv.value = Trim(t);
            }

            m_formValues[node] = fv;
        }
    }

    for (size_t i = 0; i < node->children.size(); i++) {
        InitFormValues(node->children[i]);
    }
}

void HtmlDocument::SyncFormState(Node* node)
{
    if (node == NULL) return;

    if (node->IsElement() && node->IsFormControl()) {
        std::map<Node*, FormValue>::iterator it = m_formValues.find(node);
        if (it != m_formValues.end()) {
            HtmlPainter::FormState st;
            st.value = it->second.value;
            st.checked = it->second.checked;
            st.selectedIndex = it->second.selectedIndex;
            st.focused = (m_painter.GetFocusNode() == node);
            m_painter.SetFormState(node, st);
        }
    }

    for (size_t i = 0; i < node->children.size(); i++) {
        SyncFormState(node->children[i]);
    }
}

Node* HtmlDocument::FindFormByName(const std::string& name)
{
    if (!m_bLoaded || name.empty()) return NULL;
    return m_tree.FindFormByName(name);
}

std::string HtmlDocument::GetFormValue(Node* node)
{
    if (node == NULL) return "";
    std::map<Node*, FormValue>::iterator it = m_formValues.find(node);
    if (it == m_formValues.end()) return "";

    // select：返回选中项的 value 或文本
    if (node->tagName == "select") {
        int idx = 0;
        for (size_t i = 0; i < node->children.size(); i++) {
            Node* opt = node->children[i];
            if (!opt->IsElement() || opt->tagName != "option") continue;
            if (idx == it->second.selectedIndex) {
                std::string v = opt->GetAttr("value");
                if (!v.empty()) return v;
                std::string t;
                for (size_t k = 0; k < opt->children.size(); k++) {
                    if (opt->children[k]->IsText()) t += opt->children[k]->text;
                }
                return Trim(t);
            }
            idx++;
        }
        return "";
    }

    return it->second.value;
}

void HtmlDocument::SetFormValue(Node* node, const std::string& value)
{
    if (node == NULL) return;
    m_formValues[node].value = value;
}

bool HtmlDocument::GetFormChecked(Node* node)
{
    if (node == NULL) return false;
    std::map<Node*, FormValue>::iterator it = m_formValues.find(node);
    return (it == m_formValues.end()) ? false : it->second.checked;
}

void HtmlDocument::SetFormChecked(Node* node, bool bChecked)
{
    if (node == NULL) return;
    m_formValues[node].checked = bChecked;
}

int HtmlDocument::GetFormSelectedIndex(Node* node)
{
    if (node == NULL) return 0;
    std::map<Node*, FormValue>::iterator it = m_formValues.find(node);
    return (it == m_formValues.end()) ? 0 : it->second.selectedIndex;
}

void HtmlDocument::SetFormSelectedIndex(Node* node, int idx)
{
    if (node == NULL) return;
    if (idx < 0) idx = 0;
    m_formValues[node].selectedIndex = idx;
}

Node* HtmlDocument::GetFocusNode() const
{
    return m_painter.GetFocusNode();
}

void HtmlDocument::SetFocusNode(Node* node)
{
    m_painter.SetFocusNode(node);
}

// ============================ 键盘 ============================

bool HtmlDocument::InputChar(wchar_t ch)
{
    Node* focus = m_painter.GetFocusNode();
    if (focus == NULL) return false;

    std::string tag = focus->tagName;
    std::string type = ToLower(focus->GetAttr("type", "text"));
    bool bEditable = (tag == "textarea") ||
                     (tag == "input" && (type == "text" || type == "password"));
    if (!bEditable) return false;

    if (ch < 32) return false;

    FormValue& fv = m_formValues[focus];
    fv.value += (char)ch;   // 简化：单字节字符直接追加

    // 中文等宽字符：转 UTF-8
    if (ch > 127) {
        // 撤销刚才的追加，改为 UTF-8
        if (!fv.value.empty()) fv.value.erase(fv.value.size() - 1);
        std::wstring w(1, ch);
        fv.value += WideToUtf8(w);
    }

    if (m_fnFormChanged) m_fnFormChanged(focus->GetAttr("name"));
    return true;
}

bool HtmlDocument::InputKeyDown(int vkCode)
{
    Node* focus = m_painter.GetFocusNode();
    if (focus == NULL) return false;

    std::string tag = focus->tagName;
    std::string type = ToLower(focus->GetAttr("type", "text"));
    bool bEditable = (tag == "textarea") ||
                     (tag == "input" && (type == "text" || type == "password"));
    if (!bEditable) return false;

    FormValue& fv = m_formValues[focus];

    if (vkCode == VK_BACK) {
        if (!fv.value.empty()) {
            // 删除最后一个 UTF-8 字符
            size_t pos = fv.value.size() - 1;
            while (pos > 0 && (fv.value[pos] & 0xC0) == 0x80) pos--;
            fv.value.erase(pos);
        }
        if (m_fnFormChanged) m_fnFormChanged(focus->GetAttr("name"));
        return true;
    }
    if (vkCode == VK_DELETE) {
        fv.value.clear();
        if (m_fnFormChanged) m_fnFormChanged(focus->GetAttr("name"));
        return true;
    }

    return false;
}

} // namespace HtmlEngine
