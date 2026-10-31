#include "StdAfx.h"
#include "UIHtmlBox.h"
#include "litehtml.h"

#pragma comment(lib, "msimg32.lib")
#pragma comment(lib, "gdiplus.lib")

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
    // 创建 litehtml 容器
    m_container = std::make_shared<container_win32>();

    // 图片加载回调：复用 DuiLib 的渲染引擎
    m_container->SetImageCallbacks(
        [](const std::string& src, int& w, int& h) -> bool {
            std::wstring wSrc = Utf8ToDuiW(src).GetData();
            TImageInfo* pInfo = (TImageInfo*)CRenderEngine::LoadImage(wSrc.c_str(), NULL, 0);
            if (pInfo == NULL) {
                pInfo = (TImageInfo*)CRenderEngine::GdiplusLoadImage(wSrc.c_str(), NULL, 0);
            }
            if (pInfo == NULL) { w = 0; h = 0; return false; }
            w = pInfo->nX;
            h = pInfo->nY;
            return true;
        },
        [](HDC hdc, const std::string& src, int x, int y, int w, int h) -> bool {
            std::wstring wSrc = Utf8ToDuiW(src).GetData();
            TImageInfo* pInfo = (TImageInfo*)CRenderEngine::LoadImage(wSrc.c_str(), NULL, 0);
            if (pInfo == NULL) {
                pInfo = (TImageInfo*)CRenderEngine::GdiplusLoadImage(wSrc.c_str(), NULL, 0);
            }
            if (pInfo == NULL || pInfo->hBitmap == NULL) return false;

            HDC hMemDC = ::CreateCompatibleDC(hdc);
            HBITMAP hOld = (HBITMAP)::SelectObject(hMemDC, pInfo->hBitmap);
            ::SetStretchBltMode(hdc, HALFTONE);
            ::SetBrushOrgEx(hdc, 0, 0, NULL);
            ::StretchBlt(hdc, x, y, w, h, hMemDC, 0, 0, pInfo->nX, pInfo->nY, SRCCOPY);
            ::SelectObject(hMemDC, hOld);
            ::DeleteDC(hMemDC);
            return true;
        });

    // 交互回调
    m_container->SetInteractCallbacks(
        [this](const std::string& href) {
            if (m_pManager != NULL) {
                m_sNotifyName = Utf8ToDuiW(href);
                m_pManager->SendNotify(this, DUI_MSGTYPE_HTMLLINK,
                                       (WPARAM)m_sNotifyName.GetData(), 0, true);
            }
        },
        [this](const std::string& name) {
            if (m_pManager != NULL) {
                m_sNotifyName = Utf8ToDuiW(name);
                m_pManager->SendNotify(this, DUI_MSGTYPE_HTMLFORMSUBMIT,
                                       (WPARAM)m_sNotifyName.GetData(), 0, true);
            }
        },
        [this](const std::string& name) {
            if (m_pManager != NULL) {
                m_sNotifyName = Utf8ToDuiW(name);
                m_pManager->SendNotify(this, DUI_MSGTYPE_HTMLCHANGED,
                                       (WPARAM)m_sNotifyName.GetData(), 0, true);
            }
        });
}

CHtmlBoxUI::~CHtmlBoxUI(void)
{
    DestroyDocument();
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

UINT CHtmlBoxUI::GetControlFlags() const
{
    // 必须返回 UIFLAG_SETCURSOR，否则 DuiLib 管理器不会派发 UIEVENT_SETCURSOR 事件
    return UIFLAG_SETCURSOR;
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

    // 读取文件内容
    HANDLE hFile = ::CreateFileA(utf8Path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,
                                 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return false;

    DWORD dwSize = ::GetFileSize(hFile, NULL);
    if (dwSize == 0 || dwSize == INVALID_FILE_SIZE) {
        ::CloseHandle(hFile);
        return false;
    }

    std::string html;
    html.resize(dwSize);
    DWORD dwRead = 0;
    ::ReadFile(hFile, &html[0], dwSize, &dwRead, NULL);
    ::CloseHandle(hFile);

    // 去掉 UTF-8 BOM
    if (html.size() >= 3 && html[0] == '\xEF' && html[1] == '\xBB' && html[2] == '\xBF') {
        html = html.substr(3);
    }

    m_sHtml = file;
    m_bDocDirty = true;
    EnsureDocument();
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
    if (m_container) m_container->SetBasePath(DuiWToUtf8(path));
}

// ============================ 表单读写 ============================
// TODO: 阶段 3 实现 litehtml 表单交互

CDuiString CHtmlBoxUI::GetValue(LPCTSTR name)
{
    // TODO: litehtml 版实现
    return _T("");
}

void CHtmlBoxUI::SetValue(LPCTSTR name, LPCTSTR value)
{
    // TODO: litehtml 版实现
}

bool CHtmlBoxUI::IsChecked(LPCTSTR name)
{
    // TODO: litehtml 版实现
    return false;
}

void CHtmlBoxUI::SetChecked(LPCTSTR name, bool bChecked)
{
    // TODO: litehtml 版实现
}

int CHtmlBoxUI::GetSelectedIndex(LPCTSTR name)
{
    // TODO: litehtml 版实现
    return -1;
}

void CHtmlBoxUI::SetSelectedIndex(LPCTSTR name, int idx)
{
    // TODO: litehtml 版实现
}

// ============================ 排版 ============================

void CHtmlBoxUI::EnsureDocument(void)
{
    if (!m_bDocDirty || m_sHtml.IsEmpty()) return;

    CreateDocument();

    int width = m_rcItem.right - m_rcItem.left;
    if (width <= 0) width = 800;

    // litehtml 排版
    m_liteDoc->render(width);
    m_fDocHeight = (float)m_liteDoc->height();

    UpdateScrollRange();
    m_bDocDirty = false;
}

void CHtmlBoxUI::Relayout(void)
{
    int width = m_rcItem.right - m_rcItem.left;
    if (width > 0 && m_liteDoc) {
        m_liteDoc->render(width);
        m_fDocHeight = (float)m_liteDoc->height();
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

    if (m_sHtml.IsEmpty() || !m_liteDoc) {
        CVerticalLayoutUI::DoPaint(hDC, rcPaint, pStopControl);
        return true;
    }

    int w = m_rcItem.right - m_rcItem.left;
    int h = m_rcItem.bottom - m_rcItem.top;

    // 设置绘制目标
    SIZE szScroll = GetScrollPos();
    m_container->SetOutput(hDC, szScroll.cx, szScroll.cy, w, h);

    // litehtml 绘制
    m_liteDoc->draw((litehtml::uint_ptr)hDC, -szScroll.cx, -szScroll.cy, NULL);

    return true;
}

// ============================ 事件 ============================

void CHtmlBoxUI::DoEvent(TEventUI& event)
{
    if (event.Type == UIEVENT_MOUSEMOVE) {
        float dx = (float)(event.ptMouse.x - m_rcItem.left);
        float dy = (float)(event.ptMouse.y - m_rcItem.top);
        SIZE szScroll = GetScrollPos();
        // litehtml 鼠标移动事件
        if (m_liteDoc) {
            litehtml::position::vector redraw;
            m_liteDoc->on_mouse_over((int)(dx + szScroll.cx), (int)(dy + szScroll.cy),
                                     (int)(dx + szScroll.cx), (int)(dy + szScroll.cy), redraw);
            if (!redraw.empty()) Invalidate();
        }
    }
    else if (event.Type == UIEVENT_MOUSELEAVE) {
        if (m_liteDoc) {
            litehtml::position::vector redraw;
            m_liteDoc->on_mouse_leave(redraw);
            if (!redraw.empty()) Invalidate();
        }
    }
    else if (event.Type == UIEVENT_BUTTONDOWN) {
        float dx = (float)(event.ptMouse.x - m_rcItem.left);
        float dy = (float)(event.ptMouse.y - m_rcItem.top);
        SIZE szScroll = GetScrollPos();
        if (m_liteDoc) {
            litehtml::position::vector redraw;
            m_liteDoc->on_lbutton_down((int)(dx + szScroll.cx), (int)(dy + szScroll.cy),
                                       (int)(dx + szScroll.cx), (int)(dy + szScroll.cy), redraw);
            if (!redraw.empty()) Invalidate();
        }
        SetFocus();
        Invalidate();
    }
    else if (event.Type == UIEVENT_BUTTONUP) {
        float dx = (float)(event.ptMouse.x - m_rcItem.left);
        float dy = (float)(event.ptMouse.y - m_rcItem.top);
        SIZE szScroll = GetScrollPos();
        if (m_liteDoc) {
            litehtml::position::vector redraw;
            m_liteDoc->on_lbutton_up((int)(dx + szScroll.cx), (int)(dy + szScroll.cy),
                                     (int)(dx + szScroll.cx), (int)(dy + szScroll.cy), redraw);
            if (!redraw.empty()) Invalidate();
        }
        Invalidate();
    }
    else if (event.Type == UIEVENT_CHAR) {
        if (m_container && m_container->InputChar(event.chKey)) {
            Invalidate();
        }
    }
    else if (event.Type == UIEVENT_KEYDOWN) {
        if (m_container && m_container->InputKeyDown((int)event.wParam)) {
            Invalidate();
        }
    }
    else if (event.Type == UIEVENT_SETCURSOR) {
        // 根据 litehtml 的光标设置鼠标光标
        if (m_container) {
            const std::string& cursor = m_container->GetCursor();
            if (cursor == "pointer" || cursor == "hand") {
                ::SetCursor(::LoadCursor(NULL, MAKEINTRESOURCE(IDC_HAND)));
                return;
            }
            if (cursor == "text") {
                ::SetCursor(::LoadCursor(NULL, MAKEINTRESOURCE(IDC_IBEAM)));
                return;
            }
        }
        ::SetCursor(::LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW)));
        return;
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

// ============================ litehtml 文档管理 ============================

void CHtmlBoxUI::CreateDocument()
{
    DestroyDocument();

    if (m_sHtml.IsEmpty()) return;

    std::string html = DuiWToUtf8(m_sHtml.GetData());
    std::string css = DuiWToUtf8(m_sCss.GetData());

    // 如果是文件名（不以 < 开头且文件存在），走文件加载
    if (html.find('<') == std::string::npos && html.size() > 4) {
        CDuiString sPath = CPaintManagerUI::GetResourcePath();
        sPath += m_sHtml;
        html = DuiWToUtf8(sPath.GetData());

        HANDLE hFile = ::CreateFileA(html.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,
                                     OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile != INVALID_HANDLE_VALUE) {
            DWORD dwSize = ::GetFileSize(hFile, NULL);
            if (dwSize > 0 && dwSize != INVALID_FILE_SIZE) {
                std::string content;
                content.resize(dwSize);
                DWORD dwRead = 0;
                ::ReadFile(hFile, &content[0], dwSize, &dwRead, NULL);
                ::CloseHandle(hFile);
                // 去掉 UTF-8 BOM
                if (content.size() >= 3 && content[0] == '\xEF' && content[1] == '\xBB' && content[2] == '\xBF') {
                    content = content.substr(3);
                }
                html = content;
            }
        }
    }

    // 创建 litehtml 文档（litehtml v0.9+ 只接受 4 个参数）
    m_liteDoc = litehtml::document::createFromString(html.c_str(), m_container.get(),
                                                     css.c_str(), "");
}

void CHtmlBoxUI::DestroyDocument()
{
    m_liteDoc.reset();
}

} // namespace DuiLib
