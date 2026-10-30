#include "StdAfx.h"
#include "UIHtmlBox.h"
#include <string>

namespace DuiLib
{

IMPLEMENT_DUICONTROL(CHtmlBoxUI)

CHtmlBoxUI::CHtmlBoxUI(void)
    : m_iDocHeight(0)
    , m_bDocDirty(false)
{
    m_container.SetLinkClickCallback([this](const CDuiString& href) {
        if (m_pManager != NULL) {
            m_pManager->SendNotify(this, DUI_MSGTYPE_HTMLLINK, (WPARAM)href.GetData(), 0, true);
        }
    });
    // 表单提交（<button> 或 <input type=submit> 点击）
    m_container.SetFormSubmitCallback([this](CHtmlFormElement* pEl) {
        if (m_pManager != NULL) {
            const std::string& name = pEl->GetFormName();
            char* pUtf8 = NULL;
            (void)pUtf8;
            CDuiString sName;
            wchar_t* w = DuiHtmlUtf8ToW(name);
            if (w) { sName = w; delete[] w; }
            m_pManager->SendNotify(this, DUI_MSGTYPE_HTMLFORMSUBMIT, (WPARAM)sName.GetData(), 0, true);
        }
    });
    // 表单数据变化（输入、勾选、下拉切换）
    m_container.SetFormChangedCallback([this](CHtmlFormElement* pEl) {
        if (m_pManager != NULL) {
            const std::string& name = pEl->GetFormName();
            wchar_t* w = DuiHtmlUtf8ToW(name);
            CDuiString sName = w ? (LPCTSTR)w : _T("");
            if (w) delete[] w;
            m_pManager->SendNotify(this, DUI_MSGTYPE_HTMLCHANGED, (WPARAM)sName.GetData(), 0, true);
        }
        Invalidate();
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
    m_bDocDirty = true;
    EnsureDocument();
    Invalidate();
    return true;
}

bool CHtmlBoxUI::SetHtmlFromFile(LPCTSTR file)
{
    CDuiString sPath = CPaintManagerUI::GetResourcePath();
    sPath += file;
    if (CPaintManagerUI::GetResourceZip().IsEmpty()) {
        sPath = CPaintManagerUI::GetResourcePath() + file;
    }
    HANDLE hFile = ::CreateFile(sPath.GetData(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return false;
    DWORD dwSize = ::GetFileSize(hFile, NULL);
    if (dwSize == 0 || dwSize == INVALID_FILE_SIZE) { ::CloseHandle(hFile); return false; }
    BYTE* pBuf = new BYTE[dwSize + 1];
    DWORD dwRead = 0;
    ::ReadFile(hFile, pBuf, dwSize, &dwRead, NULL);
    ::CloseHandle(hFile);
    pBuf[dwRead] = 0;
    bool ok = false;
    if (dwRead >= 2 && pBuf[0] == 0xFF && pBuf[1] == 0xFE) {
        ok = SetHtml((LPCTSTR)(pBuf + 2));
    } else {
        int wlen = MultiByteToWideChar(CP_UTF8, 0, (char*)pBuf, dwRead, NULL, 0);
        wchar_t* wBuf = new wchar_t[wlen + 1];
        MultiByteToWideChar(CP_UTF8, 0, (char*)pBuf, dwRead, wBuf, wlen);
        wBuf[wlen] = 0;
        ok = SetHtml((LPCTSTR)wBuf);
        delete[] wBuf;
    }
    delete[] pBuf;
    return ok;
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

CDuiString CHtmlBoxUI::GetValue(LPCTSTR name)
{
    // 从 litehtml 文档树里按 name 找表单控件
    CHtmlFormElement* pForm = m_container.FindFormElement(name);
    if (pForm == NULL) return _T("");

    std::string val = pForm->GetFormValue();
    wchar_t* w = DuiHtmlUtf8ToW(val);
    CDuiString s = w ? (LPCTSTR)w : _T("");
    if (w) delete[] w;
    return s;
}

void CHtmlBoxUI::SetValue(LPCTSTR name, LPCTSTR value)
{
    CHtmlFormElement* pForm = m_container.FindFormElement(name);
    if (pForm == NULL) return;

    char* pUtf8 = DuiHtmlWToUtf8(value);
    if (pUtf8) {
        pForm->SetFormValue(pUtf8);
        delete[] pUtf8;
    }
    Invalidate();
}

bool CHtmlBoxUI::IsChecked(LPCTSTR name)
{
    CHtmlFormElement* pForm = m_container.FindFormElement(name);
    return pForm != NULL ? pForm->IsFormChecked() : false;
}

void CHtmlBoxUI::SetChecked(LPCTSTR name, bool bChecked)
{
    CHtmlFormElement* pForm = m_container.FindFormElement(name);
    if (pForm == NULL) return;
    pForm->SetFormChecked(bChecked);
    Invalidate();
}

CControlUI* CHtmlBoxUI::FindChildByName(CControlUI* pParent, LPCTSTR name)
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

// UTF-8 ↔ TSTR 转换辅助（表单数据交换用）
char* CHtmlBoxUI::DuiHtmlWToUtf8(LPCTSTR wstr)
{
    if (wstr == NULL) return NULL;
    int n = ::WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
    if (n <= 0) return NULL;
    char* p = new char[n];
    ::WideCharToMultiByte(CP_UTF8, 0, wstr, -1, p, n, NULL, NULL);
    return p;
}

wchar_t* CHtmlBoxUI::DuiHtmlUtf8ToW(const std::string& s)
{
    if (s.empty()) return NULL;
    int n = ::MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), NULL, 0);
    if (n <= 0) return NULL;
    wchar_t* p = new wchar_t[n + 1];
    ::MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), p, n);
    p[n] = 0;
    return p;
}

void CHtmlBoxUI::EnsureDocument(void)
{
    if (!m_bDocDirty || m_sHtml.IsEmpty()) return;
    if (m_pManager == NULL) return;

    m_container.SetBasePath(CPaintManagerUI::GetResourcePath());
    int width = m_rcItem.right - m_rcItem.left;
    if (width <= 0) width = 800;

    if (!m_container.CreateDocument(m_sHtml.GetData(), m_sCss.IsEmpty() ? NULL : m_sCss.GetData())) {
        m_bDocDirty = false;
        return;
    }
    m_iDocHeight = m_container.Render(width);
    UpdateScrollRange();
    m_bDocDirty = false;
}

void CHtmlBoxUI::Relayout(void)
{
    if (!m_sHtml.IsEmpty() && !m_bDocDirty) {
        int width = m_rcItem.right - m_rcItem.left;
        if (width > 0) {
            m_iDocHeight = m_container.Render(width);
            UpdateScrollRange();
        }
    }
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
        EnsureDocument();
        Relayout();
    }
}

bool CHtmlBoxUI::DoPaint(HDC hDC, const RECT& rcPaint, CControlUI* pStopControl)
{
    EnsureDocument();
    // 白底填充，避免默认黑底
    RECT rcFill = m_rcItem;
    ::FillRect(hDC, &rcFill, (HBRUSH)::GetStockObject(WHITE_BRUSH));
    CVerticalLayoutUI::DoPaint(hDC, rcPaint, pStopControl);
    if (m_sHtml.IsEmpty()) return true;
    RECT rcItem = m_rcItem;
    // Draw 的 rcClient 必须是【控件客户区尺寸】(0,0,w,h)，
    // 因为容器内部会对 DC 做 SetViewportOrgEx 平移，
    // 若再传 m_rcItem 的绝对坐标就会造成二次偏移、内容画到窗口外。
    RECT rcClient = { 0, 0, rcItem.right - rcItem.left, rcItem.bottom - rcItem.top };
    m_container.Draw(hDC, GetScrollPos().cy, rcClient);
    return true;
}

void CHtmlBoxUI::DoEvent(TEventUI& event)
{
    if (event.Type == UIEVENT_MOUSEMOVE) {
        m_container.OnMouseMove(event.ptMouse);
    }
    else if (event.Type == UIEVENT_BUTTONDOWN) {
        m_container.OnLeftButtonDown(event.ptMouse);
        SetFocus();      // 让控件获得焦点，才能接收键盘输入
        Invalidate();
    }
    else if (event.Type == UIEVENT_BUTTONUP) {
        m_container.OnLeftButtonUp(event.ptMouse);
        Invalidate();
    }
    else if (event.Type == UIEVENT_CHAR) {
        // 键盘字符输入（表单文本框用）
        if (m_container.InputChar(event.chKey)) Invalidate();
    }
    else if (event.Type == UIEVENT_KEYDOWN) {
        // 退格 / 方向键等
        if (m_container.InputKeyDown((int)event.wParam)) Invalidate();
    }
    else if (event.Type == UIEVENT_SCROLLWHEEL) {
        int delta = ((int)event.wParam > 0) ? -60 : 60;
        CScrollBarUI* pVBar = GetVerticalScrollBar();
        if (pVBar != NULL && pVBar->IsVisible()) {
            SIZE sz = GetScrollPos();
            int newY = sz.cy + delta;
            int maxScroll = m_iDocHeight - (m_rcItem.bottom - m_rcItem.top);
            if (maxScroll < 0) maxScroll = 0;
            if (newY < 0) newY = 0;
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
    if (!m_sHtml.IsEmpty() && m_iDocHeight > 0) sz.cy = m_iDocHeight;
    return sz;
}

void CHtmlBoxUI::SetAttribute(LPCTSTR pstrName, LPCTSTR pstrValue)
{
    if (_tcsicmp(pstrName, _T("html")) == 0) SetHtmlFromFile(pstrValue);
    else if (_tcsicmp(pstrName, _T("htmltext")) == 0) SetHtml(pstrValue);
    else if (_tcsicmp(pstrName, _T("css")) == 0) SetCss(pstrValue);
    else if (_tcsicmp(pstrName, _T("basepath")) == 0) SetBasePath(pstrValue);
    else CVerticalLayoutUI::SetAttribute(pstrName, pstrValue);
}

} // namespace DuiLib
