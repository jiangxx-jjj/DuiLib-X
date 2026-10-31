// ==========================================================================
//  HtmlDemo.cpp — 纯 HTML 界面演示（圆角无边框窗口 + HTML 自绘标题栏）
//
//  界面 100% 由 HTML/CSS 描述：
//    · 圆角窗口（Win11 DWM 原生圆角，Win10 回退 SetWindowRgn）
//    · 标题栏与右上角三个按键（最小化/最大化/关闭）都是 HTML 元素
//    · 表单控件、渐变、圆角、阴影全部由 CSS 控制
//
//  HTML 采用内嵌原始字符串（Raw String Literal）格式，见下方 kHtmlPage。
//  这样不用一行行 _T("...") 拼接，可以直接从浏览器里复制粘贴 HTML。
// ==========================================================================

#include "StdAfx.h"
#include "UIlib.h"

using namespace DuiLib;
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "GdiPlus.lib")
#pragma comment(lib, "msimg32.lib")

// ==========================================================================
//  HTML 页面：外部文件方式
//
//  HTML 内容较大（约 140KB），超过 MSVC 单字符串字面量 65535 字节的
//  编译上限（C2026），因此放在外部文件 html_page.html 中，
//  用 SetHtmlFromFile 加载（内容见 HtmlDemo\html_page.html，UTF-8）。
// ==========================================================================



// ==========================================================================
//  窗口框架：无边框 + 圆角 + HTML 自绘标题栏
// ==========================================================================
class CHtmlDemoFrame : public CWindowWnd, public INotifyUI
{
public:
    CHtmlDemoFrame() : m_pm(NULL), m_bMaximized(false) {}

    LPCTSTR GetWindowClassName() const { return _T("HtmlDemoFrame"); }
    UINT GetClassStyle() const { return UI_CLASSSTYLE_FRAME | CS_DBLCLKS; }
    void OnFinalMessage(HWND) { delete this; }

    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
        case WM_CREATE:
            return OnCreate();

        case WM_NCCREATE: {
            // 无边框：清除系统边框样式位
            LONG_PTR st = ::GetWindowLongPtr(m_hWnd, GWL_STYLE);
            st &= ~(WS_CAPTION | WS_THICKFRAME | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
            ::SetWindowLongPtr(m_hWnd, GWL_STYLE, st);
            break;
        }

        case WM_SIZE:
            ApplyRoundCorner();
            break;

        case WM_NCHITTEST:
            return OnNcHitTest(lParam);

        case WM_NCACTIVATE:
            // 无边框窗口：屏蔽系统非客户区激活重绘（避免顶边闪白线）
            if (!::IsIconic(m_hWnd)) return TRUE;
            break;

        case WM_GETMINMAXINFO: {
            MINMAXINFO* mmi = (MINMAXINFO*)lParam;
            mmi->ptMinTrackSize.x = 640;
            mmi->ptMinTrackSize.y = 480;
            return 0;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        }

        LRESULT lRes = 0;
        if (m_pm != NULL && m_pm->MessageHandler(uMsg, wParam, lParam, lRes))
            return lRes;
        return __super::HandleMessage(uMsg, wParam, lParam);
    }

    void Notify(TNotifyUI& msg)
    {
        // HTML 表单按钮点击
        if (msg.sType == DUI_MSGTYPE_HTMLFORMSUBMIT)
        {
            LPCTSTR pName = (LPCTSTR)msg.wParam;
            if (pName == NULL) return;

            // ---- 右上角三个窗口按键 ----
            if (_tcsicmp(pName, _T("win_min")) == 0) {
                ::SendMessage(m_hWnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
                return;
            }
            if (_tcsicmp(pName, _T("win_max")) == 0) {
                m_bMaximized = !m_bMaximized;
                ::ShowWindow(m_hWnd, m_bMaximized ? SW_MAXIMIZE : SW_RESTORE);
                return;
            }
            if (_tcsicmp(pName, _T("win_close")) == 0) {
                ::PostMessage(m_hWnd, WM_CLOSE, 0, 0);
                return;
            }

            // ---- 表单按钮 ----
            CHtmlBoxUI* pBox = static_cast<CHtmlBoxUI*>(m_pm->FindControl(_T("htmlBox")));
            if (pBox == NULL) return;

            if (_tcsicmp(pName, _T("do_reset")) == 0)
            {
                pBox->SetValue(_T("user"), _T(""));
                pBox->SetValue(_T("pwd"), _T(""));
                pBox->SetChecked(_T("remember"), false);
                return;
            }

            // do_login：读取全部表单字段
            CDuiString s;
            s.Format(_T("登录请求\n\n用户名: %s\n密码长度: %d\n城市: %s\n记住密码: %s"),
                     pBox->GetValue(_T("user")).GetData(),
                     pBox->GetValue(_T("pwd")).GetLength(),
                     pBox->GetValue(_T("city")).GetData(),
                     pBox->IsChecked(_T("remember")) ? _T("是") : _T("否"));
            ::MessageBox(m_hWnd, s.GetData(), _T("后端收到表单数据"), MB_OK | MB_ICONINFORMATION);
        }
        else if (msg.sType == DUI_MSGTYPE_HTMLLINK)
        {
            CDuiString s;
            s.Format(_T("链接点击: %s"), (LPCTSTR)msg.wParam);
            ::MessageBox(m_hWnd, s.GetData(), _T("DuiLib-X HTML Demo"), MB_OK | MB_ICONINFORMATION);
        }
    }

private:
    // ------------------------------------------------------------------
    // 创建：HTML 作为根控件铺满窗口
    // ------------------------------------------------------------------
    LRESULT OnCreate(void)
    {
        m_pm = new CPaintManagerUI();
        m_pm->Init(m_hWnd, _T("HtmlDemo"));
        m_pm->SetRoundCorner(10, 10);       // 与 CSS border-radius 保持一致

        CHtmlBoxUI* pBox = new CHtmlBoxUI();
        pBox->SetName(_T("htmlBox"));
        pBox->SetBkColor(0xFFEEF2F7);
        pBox->SetManager(m_pm, NULL, true);
        m_pm->AttachDialog(pBox);
    pBox->SetHtmlFromFile(_T("html_page.html"));
        m_pm->AddNotifier(this);
        return 0;
    }

    // ------------------------------------------------------------------
    // 圆角：Win11 用 DWM 原生圆角（抗锯齿），Win10 回退 SetWindowRgn
    // ------------------------------------------------------------------
    void ApplyRoundCorner(void)
    {
        if (::IsIconic(m_hWnd)) return;

        static bool bDwmTried = false;
        static bool bDwmOk = false;
        if (!bDwmTried) {
            bDwmTried = true;
            HMODULE hDwm = ::LoadLibrary(_T("dwmapi.dll"));
            if (hDwm != NULL) {
                typedef HRESULT (WINAPI *PFN_DWMSETWA)(HWND, DWORD, LPCVOID, DWORD);
                PFN_DWMSETWA pfn = (PFN_DWMSETWA)::GetProcAddress(hDwm, "DwmSetWindowAttribute");
                if (pfn != NULL) {
                    // DWMWA_WINDOW_CORNER_PREFERENCE = 33, DWMWCP_ROUND = 2
                    UINT pref = 2;
                    bDwmOk = SUCCEEDED(pfn(m_hWnd, 33, &pref, sizeof(pref)));
                }
                ::FreeLibrary(hDwm);
            }
        }
        if (bDwmOk) return;

        // 回退：RoundRect 区域
        SIZE sz = m_pm != NULL ? m_pm->GetRoundCorner() : CDuiSize(10, 10);
        if (sz.cx <= 0) sz.cx = 10;
        if (sz.cy <= 0) sz.cy = 10;
        RECT rcWnd;
        ::GetWindowRect(m_hWnd, &rcWnd);
        ::OffsetRect(&rcWnd, -rcWnd.left, -rcWnd.top);
        rcWnd.right++; rcWnd.bottom++;
        HRGN hRgn = ::CreateRoundRectRgn(rcWnd.left, rcWnd.top, rcWnd.right, rcWnd.bottom,
                                         sz.cx * 2, sz.cy * 2);
        ::SetWindowRgn(m_hWnd, hRgn, TRUE);
        ::DeleteObject(hRgn);
    }

    // ------------------------------------------------------------------
    // 命中测试：标题栏拖动 + 四周缩放
    // ------------------------------------------------------------------
    LRESULT OnNcHitTest(LPARAM lParam)
    {
        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        ::ScreenToClient(m_hWnd, &pt);

        RECT rcClient;
        ::GetClientRect(m_hWnd, &rcClient);

        if (!::IsZoomed(m_hWnd))
        {
            const int kBorder = 4;
            if (pt.y < rcClient.top + kBorder) {
                if (pt.x < rcClient.left + kBorder)   return HTTOPLEFT;
                if (pt.x > rcClient.right - kBorder)  return HTTOPRIGHT;
                return HTTOP;
            }
            if (pt.y > rcClient.bottom - kBorder) {
                if (pt.x < rcClient.left + kBorder)   return HTBOTTOMLEFT;
                if (pt.x > rcClient.right - kBorder)  return HTBOTTOMRIGHT;
                return HTBOTTOM;
            }
            if (pt.x < rcClient.left + kBorder)   return HTLEFT;
            if (pt.x > rcClient.right - kBorder)  return HTRIGHT;
        }

        // 标题栏区域（高度 52px）：空白处可拖动，按键处不可
        if (pt.y >= 0 && pt.y < 52)
        {
            CControlUI* pHit = m_pm != NULL ? m_pm->FindControl(pt) : NULL;
            // HTML 元素不是 DuiLib 控件，这里用坐标判断：
            // 右侧 3 个按键区（宽 46*3 + 间距）留给按钮，其余可拖动
            int w = rcClient.right - rcClient.left;
            if (pt.x < w - 170)
                return HTCAPTION;
        }

        return HTCLIENT;
    }

private:
    CPaintManagerUI* m_pm;
    bool m_bMaximized;
};


int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE, LPTSTR, int nCmdShow)
{
    ::CoInitialize(NULL);
    CPaintManagerUI::SetInstance(hInstance);
    CPaintManagerUI::SetCurrentPath(CPaintManagerUI::GetInstancePath());

    CHtmlDemoFrame* pFrame = new CHtmlDemoFrame;
    // 无边框窗口：WS_POPUP 风格，圆角由 ApplyRoundCorner 处理
    HWND hWnd = pFrame->Create(NULL, _T("DuiLib-X 纯 HTML 界面演示"),
        WS_OVERLAPPEDWINDOW & ~(WS_CAPTION | WS_THICKFRAME | WS_SYSMENU) | WS_VISIBLE,
        WS_EX_WINDOWEDGE);
    if (hWnd == NULL) return -1;

    // 初始尺寸
    RECT rc = { 0, 0, 880, 640 };
    ::AdjustWindowRectEx(&rc, ::GetWindowLong(hWnd, GWL_STYLE), FALSE, ::GetWindowLong(hWnd, GWL_EXSTYLE));
    ::SetWindowPos(hWnd, NULL, 0, 0, rc.right - rc.left, rc.bottom - rc.top, SWP_NOMOVE | SWP_NOZORDER);

    pFrame->CenterWindow();
    ::ShowWindow(hWnd, SW_SHOWNORMAL);

    CPaintManagerUI::MessageLoop();
    ::CoUninitialize();
    return 0;
}
