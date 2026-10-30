#include "StdAfx.h"
#include "UIHtmlBox.h"

#pragma comment(lib, "msimg32.lib")

namespace DuiLib
{

IMPLEMENT_DUICONTROL(CHtmlBoxUI)

// ============================ 编码转换 ============================

static std::string DuiWToUtf8(LPCTSTR wstr)
{
    if (wstr == NULL) return std::string();
    int n = ::WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
    if (n <= 0) return std::string();
    std::string s(n - 1, 0);
    ::WideCharToMultiByte(CP_UTF8, 0, wstr, -1, &s[0], n, NULL, NULL);
    return s;
}

static CDuiString Utf8ToDuiW(const std::string& s)
{
    if (s.empty()) return _T("");
    int n = ::MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), NULL, 0);
    if (n <= 0) return _T("");
    std::wstring w(n, 0);
    ::MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &w[0], n);
    return CDuiString(w.c_str());
}

// ============================ 构造 ============================

CHtmlBoxUI::CHtmlBoxUI(void)
    : m_fDocHeight(0.0f)
    , m_bDocDirty(false)
{
    // 链接点击 → 转发为 DuiLib 通知
    m_doc.SetLinkClickCallback([this](const std::string& href) {
        if (m_pManager != NULL) {
            m_sNotifyName = Utf8ToDuiW(href);
            m_pManager->SendNotify(this, DUI_MSGTYPE_HTMLLINK,
                                   (WPARAM)m_sNotifyName.GetData(), 0, true);
        }
    });

    // 按钮/可点击元素 → 转发
    m_doc.SetFormSubmitCallback([this](const std::string& name) {
        if (m_pManager != NULL) {
            // 注意：SendNotify 的 wParam 必须指向调用期间一直有效的内存，
            // 因此把字符串存到成员变量里，避免局部变量析构后成为悬垂指针
            m_sNotifyName = Utf8ToDuiW(name);
            m_pManager->SendNotify(this, DUI_MSGTYPE_HTMLFORMSUBMIT,
                                   (WPARAM)m_sNotifyName.GetData(), 0, true);
        }
    });

    // 表单值变化 → 转发
    m_doc.SetFormChangedCallback([this](const std::string& name) {
        if (m_pManager != NULL) {
            m_sNotifyName = Utf8ToDuiW(name);
            m_pManager->SendNotify(this, DUI_MSGTYPE_HTMLCHANGED,
                                   (WPARAM)m_sNotifyName.GetData(), 0, true);
        }
        Invalidate();
    });

    // 图片加载：复用 DuiLib 的渲染引擎
    m_doc.SetImageCallbacks(
        [](const std::string& src, float& w, float& h) -> bool {
            std::wstring wSrc = HtmlEngine::Utf8ToWide(src);
            TImageInfo* pInfo = (TImageInfo*)CRenderEngine::LoadImage(wSrc.c_str(), NULL, 0);
            if (pInfo == NULL) {
                pInfo = (TImageInfo*)CRenderEngine::GdiplusLoadImage(wSrc.c_str(), NULL, 0);
            }
            if (pInfo == NULL) { w = 0; h = 0; return false; }
            w = (float)pInfo->nX;
            h = (float)pInfo->nY;
            // 缓存交给引擎（简化：不缓存，交给 DuiLib 内部缓存）
            return true;
        },
        [](HDC hdc, const std::string& src, float x, float y, float w, float h) -> bool {
            std::wstring wSrc = HtmlEngine::Utf8ToWide(src);
            TImageInfo* pInfo = (TImageInfo*)CRenderEngine::LoadImage(wSrc.c_str(), NULL, 0);
            if (pInfo == NULL) {
                pInfo = (TImageInfo*)CRenderEngine::GdiplusLoadImage(wSrc.c_str(), NULL, 0);
            }
            if (pInfo == NULL || pInfo->hBitmap == NULL) return false;

            HDC hMemDC = ::CreateCompatibleDC(hdc);
            HBITMAP hOld = (HBITMAP)::SelectObject(hMemDC, pInfo->hBitmap);
            ::SetStretchBltMode(hdc, HALFTONE);
            ::SetBrushOrgEx(hdc, 0, 0, NULL);
            ::StretchBlt(hdc, (int)x, (int)y, (int)w, (int)h,
                         hMemDC, 0, 0, pInfo->nX, pInfo->nY, SRCCOPY);
            ::SelectObject(hMemDC, hOld);
            ::DeleteDC(hMemDC);
            return true;
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

// ============================ 内容加载 ============================

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
    if (file == NULL) return false;

    CDuiString sPath = CPaintManagerUI::GetResourcePath();
    sPath += file;

    std::string utf8Path = DuiWToUtf8(sPath.GetData());
    if (!m_doc.LoadFromFile(utf8Path, DuiWToUtf8(m_sCss.GetData()))) {
        return false;
    }

    // 记录路径（供后续重新加载）
    m_sHtml = file;
    m_bDocDirty = false;

    int width = m_rcItem.right - m_rcItem.left;
    if (width <= 0) width = 800;
    m_fDocHeight = m_doc.Relayout((float)width);
    UpdateScrollRange();
    Invalidate();
    return true;
}

void CHtmlBoxUI::SetCss(LPCTSTR css)
{
    m_sCss = css;
    if (!m_sHtml.IsEmpty()) SetHtml(m_sHtml.GetData());
}

void CHtmlBoxUI::SetBasePath(LPCTSTR path)
{
    m_doc.SetBasePath(DuiWToUtf8(path));
}

// ============================ 表单读写 ============================

CDuiString CHtmlBoxUI::GetValue(LPCTSTR name)
{
    HtmlEngine::Node* pNode = m_doc.FindFormByName(DuiWToUtf8(name));
    if (pNode == NULL) return _T("");
    return Utf8ToDuiW(m_doc.GetFormValue(pNode));
}

void CHtmlBoxUI::SetValue(LPCTSTR name, LPCTSTR value)
{
    HtmlEngine::Node* pNode = m_doc.FindFormByName(DuiWToUtf8(name));
    if (pNode == NULL) return;
    m_doc.SetFormValue(pNode, DuiWToUtf8(value));
    Invalidate();
}

bool CHtmlBoxUI::IsChecked(LPCTSTR name)
{
    HtmlEngine::Node* pNode = m_doc.FindFormByName(DuiWToUtf8(name));
    return pNode != NULL ? m_doc.GetFormChecked(pNode) : false;
}

void CHtmlBoxUI::SetChecked(LPCTSTR name, bool bChecked)
{
    HtmlEngine::Node* pNode = m_doc.FindFormByName(DuiWToUtf8(name));
    if (pNode == NULL) return;
    m_doc.SetFormChecked(pNode, bChecked);
    Invalidate();
}

int CHtmlBoxUI::GetSelectedIndex(LPCTSTR name)
{
    HtmlEngine::Node* pNode = m_doc.FindFormByName(DuiWToUtf8(name));
    return pNode != NULL ? m_doc.GetFormSelectedIndex(pNode) : -1;
}

void CHtmlBoxUI::SetSelectedIndex(LPCTSTR name, int idx)
{
    HtmlEngine::Node* pNode = m_doc.FindFormByName(DuiWToUtf8(name));
    if (pNode == NULL) return;
    m_doc.SetFormSelectedIndex(pNode, idx);
    Invalidate();
}

// ============================ 排版 ============================

void CHtmlBoxUI::EnsureDocument(void)
{
    if (!m_bDocDirty || m_sHtml.IsEmpty()) return;

    std::string html = DuiWToUtf8(m_sHtml.GetData());
    std::string css = DuiWToUtf8(m_sCss.GetData());

    // 如果是文件名（不以 < 开头且文件存在），走文件加载
    if (html.find('<') == std::string::npos && html.size() > 4) {
        CDuiString sPath = CPaintManagerUI::GetResourcePath();
        sPath += m_sHtml;
        if (m_doc.LoadFromFile(DuiWToUtf8(sPath.GetData()), css)) {
            int width = m_rcItem.right - m_rcItem.left;
            if (width <= 0) width = 800;
            m_fDocHeight = m_doc.Relayout((float)width);
            UpdateScrollRange();
            m_bDocDirty = false;
            return;
        }
    }

    if (!m_doc.LoadFromString(html, css)) {
        m_bDocDirty = false;
        return;
    }

    int width = m_rcItem.right - m_rcItem.left;
    if (width <= 0) width = 800;
    m_fDocHeight = m_doc.Relayout((float)width);
    UpdateScrollRange();
    m_bDocDirty = false;
}

void CHtmlBoxUI::Relayout(void)
{
    int width = m_rcItem.right - m_rcItem.left;
    if (width > 0 && m_doc.IsLoaded()) {
        m_fDocHeight = m_doc.Relayout((float)width);
        UpdateScrollRange();
    }
    Invalidate();
}

void CHtmlBoxUI::UpdateScrollRange(void)
{
    int viewH = m_rcItem.bottom - m_rcItem.top;
    int docH = (int)m_fDocHeight;
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

// ============================ 绘制 ============================

bool CHtmlBoxUI::DoPaint(HDC hDC, const RECT& rcPaint, CControlUI* pStopControl)
{
    EnsureDocument();

    // 白底填充，避免默认黑底
    RECT rcFill = m_rcItem;
    ::FillRect(hDC, &rcFill, (HBRUSH)::GetStockObject(WHITE_BRUSH));

    if (m_sHtml.IsEmpty() || !m_doc.IsLoaded()) {
        CVerticalLayoutUI::DoPaint(hDC, rcPaint, pStopControl);
        return true;
    }

    int w = m_rcItem.right - m_rcItem.left;
    int h = m_rcItem.bottom - m_rcItem.top;

    // 引擎绘制（GDI+ 内部处理滚动变换）
    SIZE szScroll = GetScrollPos();
    m_doc.Paint(hDC, (float)szScroll.cx, (float)szScroll.cy, (float)w, (float)h);

    return true;
}

// ============================ 事件 ============================

void CHtmlBoxUI::DoEvent(TEventUI& event)
{
    if (event.Type == UIEVENT_MOUSEMOVE) {
        float dx = (float)(event.ptMouse.x - m_rcItem.left);
        float dy = (float)(event.ptMouse.y - m_rcItem.top);
        if (m_doc.OnMouseMove(dx, dy)) Invalidate();
    }
    else if (event.Type == UIEVENT_MOUSELEAVE) {
        m_doc.OnMouseLeave();
        Invalidate();
    }
    else if (event.Type == UIEVENT_BUTTONDOWN) {
        float dx = (float)(event.ptMouse.x - m_rcItem.left);
        float dy = (float)(event.ptMouse.y - m_rcItem.top);
        SIZE szScroll = GetScrollPos();
        m_doc.OnClick(dx + (float)szScroll.cx, dy + (float)szScroll.cy);
        SetFocus();
        Invalidate();
    }
    else if (event.Type == UIEVENT_CHAR) {
        if (m_doc.InputChar(event.chKey)) Invalidate();
    }
    else if (event.Type == UIEVENT_KEYDOWN) {
        if (m_doc.InputKeyDown((int)event.wParam)) Invalidate();
    }
    else if (event.Type == UIEVENT_SCROLLWHEEL) {
        int delta = ((int)event.wParam > 0) ? -60 : 60;
        CScrollBarUI* pVBar = GetVerticalScrollBar();
        if (pVBar != NULL && pVBar->IsVisible()) {
            SIZE sz = GetScrollPos();
            int newY = sz.cy + delta;
            int maxScroll = (int)m_fDocHeight - (m_rcItem.bottom - m_rcItem.top);
            if (maxScroll < 0) maxScroll = 0;
            if (newY < 0) newY = 0;
            if (newY > maxScroll) newY = maxScroll;
            SetScrollPos(CDuiSize(sz.cx, newY));
            Invalidate();
            return;
        }
    }

    CVerticalLayoutUI::DoEvent(event);
}

SIZE CHtmlBoxUI::EstimateSize(SIZE szAvailable)
{
    SIZE sz = CVerticalLayoutUI::EstimateSize(szAvailable);
    if (!m_sHtml.IsEmpty() && m_fDocHeight > 0) sz.cy = (LONG)m_fDocHeight;
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
