#include "StdAfx.h"
#include "UIHtmlBox.h"

namespace DuiLib
{

IMPLEMENT_DUICONTROL(CHtmlBoxUI)

CHtmlBoxUI::CHtmlBoxUI(void)
    : m_iDocHeight(0)
{
    m_container.SetLinkClickCallback([this](const CDuiString& href) {
        // 链接点击 → 转发通知给宿主
        if (m_pManager != NULL) {
            m_pManager->SendNotify(this, DUI_MSGTYPE_HTMLLINK, (WPARAM)href.GetData(), 0, true);
        }
    });
}

CHtmlBoxUI::~CHtmlBoxUI(void)
{
}

LPCTSTR CHtmlBoxUI::GetClass() const
{
    return _T("HtmlBoxUI");
}

LPVOID CHtmlBoxUI::GetInterface(LPCTSTR pstrName)
{
    if (_tcsicmp(pstrName, _T("HtmlBox")) == 0) return static_cast<CHtmlBoxUI*>(this);
    return CVerticalLayoutUI::GetInterface(pstrName);
}

bool CHtmlBoxUI::SetHtml(LPCTSTR html)
{
    if (html == NULL) return false;
    m_sHtml = html;
    // 懒加载：等首次布局（SetPos 拿到实际宽度）再创建文档，
    // 避免解析阶段宽度为 0 导致 litehtml render 异常
    m_bDocDirty = true;
    return true;
}

void CHtmlBoxUI::EnsureDocument(void)
{
    if (!m_bDocDirty || m_sHtml.IsEmpty()) return;
    if (m_pManager == NULL) return;

    m_container.SetBasePath(CPaintManagerUI::GetResourcePath());
    int width = m_rcItem.right - m_rcItem.left;
    if (width <= 0) return;   // 仍无有效宽度，等下次

    if (!m_container.CreateDocument(m_sHtml.GetData(), m_sCss.IsEmpty() ? NULL : m_sCss.GetData())) {
        m_bDocDirty = false;
        return;
    }
    m_iDocHeight = m_container.Render(width);
    UpdateScrollRange();
    m_bDocDirty = false;
}

bool CHtmlBoxUI::SetHtmlFromFile(LPCTSTR file)
{
    // 从 ResourcePath 读取 HTML 文件
    CDuiString sPath = CPaintManagerUI::GetResourcePath();
    sPath += file;
    if (CPaintManagerUI::GetResourceZip().IsEmpty()) {
        sPath = CPaintManagerUI::GetResourcePath() + file;
    }

    HANDLE hFile = ::CreateFile(sPath.GetData(), GENERIC_READ, FILE_SHARE_READ, NULL,
                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return false;
    DWORD dwSize = ::GetFileSize(hFile, NULL);
    if (dwSize == 0 || dwSize == INVALID_FILE_SIZE) { ::CloseHandle(hFile); return false; }

    BYTE* pBuf = new BYTE[dwSize + 1];
    DWORD dwRead = 0;
    ::ReadFile(hFile, pBuf, dwSize, &dwRead, NULL);
    ::CloseHandle(hFile);
    pBuf[dwRead] = 0;

    // 编码检测：UTF-8 BOM / UTF-16 BOM / 默认 UTF-8
    LPCTSTR html = NULL;
    if (dwRead >= 3 && pBuf[0] == 0xEF && pBuf[1] == 0xBB && pBuf[2] == 0xBF) {
        // UTF-8 BOM
        char* pUtf8 = (char*)(pBuf + 3);
        int wlen = MultiByteToWideChar(CP_UTF8, 0, pUtf8, -1, NULL, 0);
        wchar_t* wBuf = new wchar_t[wlen];
        MultiByteToWideChar(CP_UTF8, 0, pUtf8, -1, wBuf, wlen);
        html = (LPCTSTR)wBuf;
        bool ok = SetHtml(html);
        delete[] wBuf;
        delete[] pBuf;
        return ok;
    }
    else if (dwRead >= 2 && pBuf[0] == 0xFF && pBuf[1] == 0xFE) {
        html = (LPCTSTR)(pBuf + 2);
        bool ok = SetHtml(html);
        delete[] pBuf;
        return ok;
    }
    else {
        // 无 BOM，按 UTF-8 处理
        int wlen = MultiByteToWideChar(CP_UTF8, 0, (char*)pBuf, dwRead, NULL, 0);
        wchar_t* wBuf = new wchar_t[wlen + 1];
        MultiByteToWideChar(CP_UTF8, 0, (char*)pBuf, dwRead, wBuf, wlen);
        wBuf[wlen] = 0;
        html = (LPCTSTR)wBuf;
        bool ok = SetHtml(html);
        delete[] wBuf;
        delete[] pBuf;
        return ok;
    }
}

void CHtmlBoxUI::SetCss(LPCTSTR css)
{
    m_sCss = css;
    if (!m_sHtml.IsEmpty()) SetHtml(m_sHtml.GetData());
}

void CHtmlBoxUI::SetBasePath(LPCTSTR path)
{
    m_container.SetBasePath(path);
}

// ============================ 表单数据交互 ============================
// 阶段4简化实现：HTML 表单控件的值读写通过【原生控件叠加】实现。
// 当前版本：先实现基础架构，GetValue/SetValue 通过遍历子控件匹配 name。
// 完整的原生控件位置叠加在后续版本增强（litehtml 提供元素坐标）。

// 递归查找指定 name 的子控件（HTML name → 控件）
static CControlUI* FindChildByName(CControlUI* pParent, LPCTSTR name)
{
    if (pParent == NULL) return NULL;
    if (pParent->GetName() == name) return pParent;
    IContainerUI* pContainer = static_cast<IContainerUI*>(pParent->GetInterface(_T("IContainer")));
    if (pContainer == NULL) return NULL;
    int n = pContainer->GetCount();
    for (int i = 0; i < n; i++) {
        CControlUI* pFound = FindChildByName(pContainer->GetItemAt(i), name);
        if (pFound != NULL) return pFound;
    }
    return NULL;
}
CDuiString CHtmlBoxUI::GetValue(LPCTSTR name)
{
    CControlUI* pControl = FindChildByName(this, name);
    if (pControl == NULL) return _T("");
    // 文本类控件
    if (_tcsstr(pControl->GetClass(), _T("EditUI")) != NULL ||
        _tcsstr(pControl->GetClass(), _T("RichEditUI")) != NULL)
        return pControl->GetText();
    // 下拉框
    if (_tcsstr(pControl->GetClass(), _T("ComboUI")) != NULL)
        return pControl->GetText();
    return _T("");
}

void CHtmlBoxUI::SetValue(LPCTSTR name, LPCTSTR value)
{
    CControlUI* pControl = FindChildByName(this, name);
    if (pControl == NULL) return;
    pControl->SetText(value);
    pControl->Invalidate();
}

bool CHtmlBoxUI::IsChecked(LPCTSTR name)
{
    CControlUI* pControl = FindChildByName(this, name);
    if (pControl == NULL) return false;
    LPVOID pOption = pControl->GetInterface(_T("Option"));
    if (pOption != NULL) {
        COptionUI* pOpt = static_cast<COptionUI*>(pControl);
        return pOpt->IsSelected();
    }
    return false;
}

void CHtmlBoxUI::SetChecked(LPCTSTR name, bool bChecked)
{
    CControlUI* pControl = FindChildByName(this, name);
    if (pControl == NULL) return;
    LPVOID pOption = pControl->GetInterface(_T("Option"));
    if (pOption != NULL) {
        COptionUI* pOpt = static_cast<COptionUI*>(pControl);
        pOpt->Selected(bChecked, false);
    }
}

void CHtmlBoxUI::Relayout(void)
{
    int width = m_rcItem.right - m_rcItem.left;
    if (width <= 0) width = 800;
    m_iDocHeight = m_container.Render(width);
    UpdateScrollRange();
    Invalidate();
}

void CHtmlBoxUI::UpdateScrollRange(void)
{
    int viewH = m_rcItem.bottom - m_rcItem.top;
    int docH = m_iDocHeight;
    if (docH > viewH) {
        EnableScrollBar(true, GetHorizontalScrollBar() != NULL);
        if (GetVerticalScrollBar()) {
            GetVerticalScrollBar()->SetScrollRange(docH - viewH);
            GetVerticalScrollBar()->SetLineSize(20);
        }
    } else {
        EnableScrollBar(false, GetHorizontalScrollBar() != NULL);
    }
}

void CHtmlBoxUI::SetPos(RECT rc, bool bNeedInvalidate)
{
    bool bSizeChanged = (rc.right - rc.left) != (m_rcItem.right - m_rcItem.left);
    CVerticalLayoutUI::SetPos(rc, bNeedInvalidate);
    if (bSizeChanged && !m_sHtml.IsEmpty()) {
        Relayout();
    }
}

bool CHtmlBoxUI::DoPaint(HDC hDC, const RECT& rcPaint, CControlUI* pStopControl)
{
    CVerticalLayoutUI::DoPaint(hDC, rcPaint, pStopControl);   // 先画背景/边框

    if (m_sHtml.IsEmpty()) return true;

    // 容器内绘制 HTML
    RECT rcItem = m_rcItem;
    m_container.Draw(hDC, GetScrollPos().cy, rcItem);
    return true;
}

void CHtmlBoxUI::DoEvent(TEventUI& event)
{
    if (event.Type == UIEVENT_MOUSEMOVE) {
        m_container.OnMouseMove(event.ptMouse);
        Invalidate();
    }
    else if (event.Type == UIEVENT_BUTTONDOWN) {
        m_container.OnLeftButtonDown(event.ptMouse);
    }
    else if (event.Type == UIEVENT_BUTTONUP) {
        m_container.OnLeftButtonUp(event.ptMouse);
        Invalidate();
    }
    else if (event.Type == UIEVENT_SCROLLWHEEL) {
        // 滚轮 → 滚动 HTML 内容
        int delta = ((int)event.wParam > 0) ? -60 : 60;
        CScrollBarUI* pVBar = GetVerticalScrollBar();
        if (pVBar != NULL && pVBar->IsVisible()) {
            SIZE sz = GetScrollPos();
            int newY = sz.cy + delta;
            if (newY < 0) newY = 0;
            int maxScroll = m_iDocHeight - (m_rcItem.bottom - m_rcItem.top);
            if (maxScroll < 0) maxScroll = 0;
            if (newY > maxScroll) newY = maxScroll;
            SetScrollPos(CDuiSize(sz.cx, newY));
            m_container.SetScrollOffset(newY);
            Invalidate();
            return;
        }
    }

    CVerticalLayoutUI::DoEvent(event);
}

SIZE CHtmlBoxUI::EstimateSize(SIZE szAvailable)
{
    SIZE sz = CVerticalLayoutUI::EstimateSize(szAvailable);
    if (!m_sHtml.IsEmpty() && m_iDocHeight > 0) {
        sz.cy = m_iDocHeight;
    }
    return sz;
}

void CHtmlBoxUI::SetAttribute(LPCTSTR pstrName, LPCTSTR pstrValue)
{
    if (_tcsicmp(pstrName, _T("html")) == 0) {
        SetHtmlFromFile(pstrValue);
    }
    else if (_tcsicmp(pstrName, _T("htmltext")) == 0) {
        SetHtml(pstrValue);
    }
    else if (_tcsicmp(pstrName, _T("css")) == 0) {
        SetCss(pstrValue);
    }
    else if (_tcsicmp(pstrName, _T("basepath")) == 0) {
        SetBasePath(pstrValue);
    }
    else {
        CVerticalLayoutUI::SetAttribute(pstrName, pstrValue);
    }
}

} // namespace DuiLib
