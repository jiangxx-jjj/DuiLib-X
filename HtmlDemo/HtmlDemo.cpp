// ==========================================================================
//  HtmlDemo.cpp — 纯 HTML 界面演示
//  一个窗口 + 全屏 HtmlBox，界面全部用 HTML/CSS 描述
//  事件在 C++ 处理，数据通过 GetValue/SetValue 双向交互
// ==========================================================================

#include "StdAfx.h"
#include "UIlib.h"

using namespace DuiLib;
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "GdiPlus.lib")

static LPCTSTR PAGE_MAIN =
_T("<style>")
_T("body { font-family: 微软雅黑; font-size: 14px; color: #333333; background-color: #EEF2F7; margin: 0; }")
_T(".titlebar { background-color: #2D5BD7; padding: 14px 20px; }")
_T(".titlebar h2 { color: #FFFFFF; margin: 0; font-size: 18px; }")
_T(".card { background-color: #FFFFFF; border: 1px solid #D9E0EA; margin: 12px 16px; padding: 14px 18px; }")
_T(".card h3 { color: #2D5BD7; margin: 0 0 10px 0; font-size: 15px; }")
_T(".row { margin: 10px 0; }")
_T("label { color: #555555; margin-right: 8px; }")
_T("a { color: #1E6FFF; text-decoration: underline; }")
_T(".btn-row { margin-top: 14px; }")
_T(".footer { text-align: center; color: #98A2B3; font-size: 12px; padding: 10px; }")
_T(".ok { color: #28C76F; font-weight: bold; }")
_T("input[type=text], input[type=password] { width: 260px; height: 30px; border: 1px solid #C8CFDA; background-color: #FFFFFF; }")
_T("select { width: 180px; height: 30px; border: 1px solid #C8CFDA; background-color: #FFFFFF; }")
_T("button { width: 110px; height: 32px; border: 1px solid #C8CFDA; background-color: #F0F3F8; margin-right: 10px; }")
_T("input[type=checkbox], input[type=radio] { width: 16px; height: 16px; margin-right: 4px; }")
_T("</style>")

_T("<div class='titlebar'><h2>用户中心 — 纯 HTML 界面演示</h2></div>")

_T("<div class='card'>")
_T("<h3>登录信息</h3>")
_T("<div class='row'><label>用户名：</label><input type='text' name='user' value='张三'/></div>")
_T("<div class='row'><label>密码：</label><input type='password' name='pwd' value='123456'/></div>")
_T("<div class='row'><label>城市：</label>")
_T("<select name='city'><option>北京</option><option selected>上海</option><option>深圳</option></select>")
_T("</div>")
_T("<div class='row'>")
_T("<input type='checkbox' name='remember' checked/><label>记住密码</label>")
_T("<input type='radio' name='sex' value='m' checked/><label>男</label>")
_T("<input type='radio' name='sex' value='f'/><label>女</label>")
_T("</div>")
_T("</div>")

_T("<div class='card'>")
_T("<h3>操作</h3>")
_T("<div class='btn-row'>")
_T("<button name='do_login'>登 录</button>")
_T("<button name='do_reset'>重 置</button>")
_T("</div>")
_T("<p>提示：<span class='ok'>HTML 渲染正常，输入框为原生控件</span></p>")
_T("<p>更多信息请访问 <a href='https://github.com/jiangxx-jjj/DuiLib-X'>DuiLib-X 项目主页</a>。</p>")
_T("</div>")

_T("<p class='footer'>DuiLib-X · litehtml 排版 · DuiLib 原生渲染 · 无浏览器内核</p>");

class CHtmlDemoFrame : public CWindowWnd, public INotifyUI
{
public:
    CHtmlDemoFrame() : m_pm(NULL) {}

    LPCTSTR GetWindowClassName() const { return _T("HtmlDemoFrame"); }
    UINT GetClassStyle() const { return UI_CLASSSTYLE_FRAME | CS_DBLCLKS; }
    void OnFinalMessage(HWND) { delete this; }

    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        if (uMsg == WM_CREATE)
        {
            m_pm = new CPaintManagerUI();
            m_pm->Init(m_hWnd, _T("HtmlDemo"));

            CHtmlBoxUI* pBox = new CHtmlBoxUI();
            pBox->SetName(_T("htmlBox"));
            pBox->SetBkColor(0xFFEEF2F7);
            pBox->SetManager(m_pm, NULL, true);
            m_pm->AttachDialog(pBox);
            pBox->SetHtml(PAGE_MAIN);
            m_pm->AddNotifier(this);
            return 0;
        }
        if (uMsg == WM_DESTROY) { PostQuitMessage(0); return 0; }

        LRESULT lRes = 0;
        if (m_pm != NULL && m_pm->MessageHandler(uMsg, wParam, lParam, lRes))
            return lRes;
        return __super::HandleMessage(uMsg, wParam, lParam);
    }

    void Notify(TNotifyUI& msg)
    {
        if (msg.sType == DUI_MSGTYPE_HTMLLINK)
        {
            CDuiString s;
            s.Format(_T("链接点击: %s"), (LPCTSTR)msg.wParam);
            ::MessageBox(m_hWnd, s.GetData(), _T("DuiLib-X HTML Demo"), MB_OK | MB_ICONINFORMATION);
        }
        else if (msg.sType == DUI_MSGTYPE_HTMLFORMSUBMIT)
        {
            // <button> 点击 → 后端读取表单数据
            CHtmlBoxUI* pBox = static_cast<CHtmlBoxUI*>(m_pm->FindControl(_T("htmlBox")));
            if (pBox == NULL) return;

            LPCTSTR pName = (LPCTSTR)msg.wParam;
            if (pName != NULL && _tcsicmp(pName, _T("do_reset")) == 0)
            {
                // 重置按钮
                pBox->SetValue(_T("user"), _T(""));
                pBox->SetValue(_T("pwd"), _T(""));
                pBox->SetChecked(_T("remember"), false);
                ::MessageBox(m_hWnd, _T("表单已重置"), _T("后端处理"), MB_OK | MB_ICONINFORMATION);
                return;
            }

            // 登录按钮：读取全部表单字段
            CDuiString s;
            s.Format(_T("登录请求\n\n用户名: %s\n密码长度: %d\n城市: %s\n记住密码: %s"),
                     pBox->GetValue(_T("user")).GetData(),
                     pBox->GetValue(_T("pwd")).GetLength(),
                     pBox->GetValue(_T("city")).GetData(),
                     pBox->IsChecked(_T("remember")) ? _T("是") : _T("否"));
            ::MessageBox(m_hWnd, s.GetData(), _T("后端收到表单数据"), MB_OK | MB_ICONINFORMATION);
        }
    }

private:
    CPaintManagerUI* m_pm;
};

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE, LPTSTR, int nCmdShow)
{
    ::CoInitialize(NULL);
    CPaintManagerUI::SetInstance(hInstance);
    CPaintManagerUI::SetCurrentPath(CPaintManagerUI::GetInstancePath());

    CHtmlDemoFrame* pFrame = new CHtmlDemoFrame;
    HWND hWnd = pFrame->Create(NULL, _T("DuiLib-X 纯 HTML 界面演示"),
        WS_OVERLAPPEDWINDOW & ~(WS_CAPTION | WS_THICKFRAME | WS_SYSMENU) | WS_VISIBLE,
        WS_EX_WINDOWEDGE);
    if (hWnd == NULL) return -1;
    pFrame->CenterWindow();
    ::ShowWindow(*pFrame, SW_SHOWNORMAL);

    CPaintManagerUI::MessageLoop();
    ::CoUninitialize();
    return 0;
}
