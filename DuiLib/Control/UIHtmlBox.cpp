#include "StdAfx.h"
#include "UIHtmlBox.h"
#include "litehtml/el_form.h"
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

    // 创建后端 JS 引擎
    m_jsBridge = std::make_shared<litehtml::JsBridge>();
    m_jsBridge->BindHost(this);
    // JS 弹窗在前端显示（alert/confirm 转成 Win32 消息框，由宿主窗口弹出）
    m_jsBridge->SetAlertCallback([this](const std::string& msg) {
        CDuiString wMsg = Utf8ToDuiW(msg);
        HWND hWnd = m_pManager ? m_pManager->GetPaintWindow() : NULL;
        ::MessageBox(hWnd, wMsg.GetData(), _T("页面消息"), MB_OK | MB_ICONINFORMATION);
    });
    m_jsBridge->SetConfirmCallback([this](const std::string& msg) -> bool {
        CDuiString wMsg = Utf8ToDuiW(msg);
        HWND hWnd = m_pManager ? m_pManager->GetPaintWindow() : NULL;
        return ::MessageBox(hWnd, wMsg.GetData(), _T("页面确认"), MB_OKCANCEL | MB_ICONQUESTION) == IDOK;
    });

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
// 按 name 属性全局查找控件，读写值

CDuiString CHtmlBoxUI::GetValue(LPCTSTR name)
{
    if (!m_container || !m_liteDoc) return _T("");
    std::string utf8Name = DuiWToUtf8(name);
    auto found = m_container->FindControlsByName(m_liteDoc->root(), utf8Name);
    for (auto& el : found) {
        litehtml::el_form_control* pCtrl = dynamic_cast<litehtml::el_form_control*>(el.get());
        if (pCtrl) return Utf8ToDuiW(pCtrl->GetValue());
    }
    return _T("");
}

void CHtmlBoxUI::SetValue(LPCTSTR name, LPCTSTR value)
{
    if (!m_container || !m_liteDoc) return;
    std::string utf8Name = DuiWToUtf8(name);
    std::string utf8Val = DuiWToUtf8(value);
    auto found = m_container->FindControlsByName(m_liteDoc->root(), utf8Name);
    for (auto& el : found) {
        litehtml::el_form_control* pCtrl = dynamic_cast<litehtml::el_form_control*>(el.get());
        if (pCtrl) { pCtrl->SetValue(utf8Val); Invalidate(); }
    }
}

bool CHtmlBoxUI::IsChecked(LPCTSTR name)
{
    if (!m_container || !m_liteDoc) return false;
    std::string utf8Name = DuiWToUtf8(name);
    auto found = m_container->FindControlsByName(m_liteDoc->root(), utf8Name);
    for (auto& el : found) {
        litehtml::el_form_control* pCtrl = dynamic_cast<litehtml::el_form_control*>(el.get());
        if (pCtrl) return pCtrl->IsChecked();
    }
    return false;
}

void CHtmlBoxUI::SetChecked(LPCTSTR name, bool bChecked)
{
    if (!m_container || !m_liteDoc) return;
    std::string utf8Name = DuiWToUtf8(name);
    auto found = m_container->FindControlsByName(m_liteDoc->root(), utf8Name);
    for (auto& el : found) {
        litehtml::el_form_control* pCtrl = dynamic_cast<litehtml::el_form_control*>(el.get());
        if (pCtrl) { pCtrl->SetChecked(bChecked); Invalidate(); }
    }
}

int CHtmlBoxUI::GetSelectedIndex(LPCTSTR name)
{
    if (!m_container || !m_liteDoc) return -1;
    std::string utf8Name = DuiWToUtf8(name);
    auto found = m_container->FindControlsByName(m_liteDoc->root(), utf8Name);
    for (auto& el : found) {
        litehtml::el_form_control* pCtrl = dynamic_cast<litehtml::el_form_control*>(el.get());
        if (pCtrl) return pCtrl->GetSelectedIndex();
    }
    return -1;
}

void CHtmlBoxUI::SetSelectedIndex(LPCTSTR name, int idx)
{
    if (!m_container || !m_liteDoc) return;
    std::string utf8Name = DuiWToUtf8(name);
    auto found = m_container->FindControlsByName(m_liteDoc->root(), utf8Name);
    for (auto& el : found) {
        litehtml::el_form_control* pCtrl = dynamic_cast<litehtml::el_form_control*>(el.get());
        if (pCtrl) { pCtrl->SetSelectedIndex(idx); Invalidate(); }
    }
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
{    DestroyDocument();

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
    // 设置根元素（供表单按 name 全局查找）
    if (m_liteDoc && m_container) {
        m_container->SetRootElement(m_liteDoc->root());
        // 执行页面里的 <script>（后端 JS，弹窗在前端显示）
        ExecuteScripts();
    }
}

// ============================ JS 支持 ============================

bool CHtmlBoxUI::EvalJs(LPCTSTR jsCode)
{
    if (!m_jsBridge || jsCode == NULL) return false;
    return m_jsBridge->Eval(DuiWToUtf8(jsCode));
}

void CHtmlBoxUI::CloseHostWindow()
{
    // JS close_window() → 通过通知回调到后端逻辑（沿用表单提交通知）
    if (m_pManager != NULL) {
        m_sNotifyName = _T("js_close_window");
        m_pManager->SendNotify(this, DUI_MSGTYPE_HTMLFORMSUBMIT,
                               (WPARAM)m_sNotifyName.GetData(), 0, true);
    }
}

void CHtmlBoxUI::ExecuteScripts()
{
    if (!m_jsBridge || !m_liteDoc) return;

    // 递归收集 <script> 标签内容并按顺序执行
    std::function<void(const litehtml::element::ptr&)> walk =
        [&](const litehtml::element::ptr& el) {
        if (!el) return;
        const char* tag = el->get_tagName();
        if (tag && _stricmp(tag, "script") == 0) {
            // 取脚本内容（子文本）
            std::string js;
            litehtml::string text;
            el->get_text(text);
            js = text;
            if (!js.empty()) {
                m_jsBridge->Eval(js);   // 后端执行；错误静默（LastError 可查）
            }
            // script 无子元素需要遍历
            return;
        }
        for (const auto& child : el->children()) {
            walk(child);
        }
    };
    walk(m_liteDoc->root());
}

litehtml::el_canvas* CHtmlBoxUI::FindCanvas(const char* id)
{
    if (!m_liteDoc || !id || !id[0]) return NULL;

    // 按 id 属性优先，name 属性次之查找 canvas 元素
    std::function<litehtml::el_canvas*(const litehtml::element::ptr&)> walk =
        [&](const litehtml::element::ptr& el) -> litehtml::el_canvas* {
        if (!el) return NULL;
        const char* tag = el->get_tagName();
        if (tag && _stricmp(tag, "canvas") == 0) {
            const char* eid = el->get_attr("id");
            const char* ename = el->get_attr("name");
            if ((eid && strcmp(eid, id) == 0) || (ename && strcmp(ename, id) == 0)) {
                return dynamic_cast<litehtml::el_canvas*>(el.get());
            }
        }
        for (const auto& child : el->children()) {
            litehtml::el_canvas* r = walk(child);
            if (r) return r;
        }
        return NULL;
    };
    return walk(m_liteDoc->root());
}

void CHtmlBoxUI::DestroyDocument()
{
    m_liteDoc.reset();
}

} // namespace DuiLib
