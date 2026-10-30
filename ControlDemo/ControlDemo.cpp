// ==========================================================================
//  ControlDemo.cpp — DuiLib_Ultimate 控件全家桶示例
//
//  覆盖 Help/Duilib属性列表.html 中的全部控件：
//    窗口节点   : Window / Font / Default / Style / Include / Import
//    布局容器   : Container / VerticalLayout / HorizontalLayout / TileLayout
//                 TabLayout / AnimationTabLayout / ChildLayout
//    基础控件   : Label / Text / Button / FadeButton / Option / CheckBox /
//                 GroupBox / Progress / Slider / ScrollBar
//    输入控件   : Edit / RichEdit / Combo / HotKey / DateTime / IPAddress
//    列表树形   : List / ListHeader / ListHeaderItem / ListContainerElement /
//                 ListLabelElement / ListTextElement / TreeView / TreeNode
//    动效多媒体 : GifAnim / Loading / Ring / RollText / Flash / WebBrowser
//    嵌入其他   : ActiveX / Menu / MenuElement / ColorPalette / PageControl
//
//  工程：静态链接 DuiLib（SRelease 配置，与库工程一致使用 Unicode 字符集）
// ==========================================================================

#include "StdAfx.h"
#include "UIlib.h"
#include "Control/UIHtmlBox.h"
// 使用库封装好的窗口基类（静态库方式）
#include "Utils/WinImplBase.h"

using namespace DuiLib;

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "GdiPlus.lib")

// --------------------------------------------------------------------------
// 自定义容器：XML 中的 ScrollableVerticalLayout 由 CreateControl 回调创建
// --------------------------------------------------------------------------
class CScrollableVerticalLayoutUI : public CVerticalLayoutUI
{
public:
    LPCTSTR GetClass() const override { return _T("ScrollableVerticalLayoutUI"); }
    LPVOID GetInterface(LPCTSTR pstrName) override
    {
        if (_tcsicmp(pstrName, _T("ScrollableVerticalLayout")) == 0)
            return static_cast<CScrollableVerticalLayoutUI*>(this);
        return CVerticalLayoutUI::GetInterface(pstrName);
    }
};

// --------------------------------------------------------------------------
// 主窗口：继承 WindowImplBase，皮肤来自 XML
// --------------------------------------------------------------------------
class CControlDemoFrame : public CWindowWnd, public INotifyUI, public IDialogBuilderCallback
{
public:
    CControlDemoFrame() : m_pPaintManager(NULL) {}

    LPCTSTR GetWindowClassName() const override { return _T("ControlDemoFrame"); }
    // 无边框窗口：WS_POPUP 去掉系统边框，圆角由 XML roundcorner + layered 实现；
    // 拖动（caption）与缩放（sizebox）由下方 OnNcHitTest + CPaintManagerUI 的 GetSizeBox/GetCaptionRect 提供
    UINT GetClassStyle() const override { return UI_CLASSSTYLE_FRAME | CS_DBLCLKS; }
    void OnFinalMessage(HWND /*hWnd*/) override { delete this; }

    // IDialogBuilderCallback：XML 中出现自定义控件名时由宿主创建
    CControlUI* CreateControl(LPCTSTR pstrClass) override
    {
        if (_tcsicmp(pstrClass, _T("ScrollableVerticalLayout")) == 0)
            return new CScrollableVerticalLayoutUI();
        return NULL;
    }

    void Notify(TNotifyUI& msg) override
    {
        const CDuiString& type = msg.sType;
        const CDuiString& name = msg.pSender->GetName();

        if (type == DUI_MSGTYPE_CLICK)
            OnClick(msg);
        else if (type == DUI_MSGTYPE_SELECTCHANGED && name == _T("comboCity"))
            SetStatus(CDuiString(_T("下拉框选择：")) + msg.pSender->GetText());
        else if (type == DUI_MSGTYPE_VALUECHANGED && name == _T("sliderDemo"))
        {
            CSliderUI* pSlider = static_cast<CSliderUI*>(msg.pSender);
            CDuiString s;
            s.Format(_T("%d%%"), pSlider->GetValue());
            m_pPaintManager->FindControl(_T("lblSliderValue"))->SetText(s);
            static_cast<CProgressUI*>(m_pPaintManager->FindControl(_T("progressDemo")))->SetValue(pSlider->GetValue());
        }
        else if (type == DUI_MSGTYPE_ITEMSELECT && name == _T("listDemo")) {
            CDuiString s;
            s.Format(_T("列表选中行：%d"), (int)msg.wParam);
            SetStatus(s);
        }
        else if (type == DUI_MSGTYPE_ITEMACTIVATE && name == _T("listDemo"))
            SetStatus(_T("双击了列表项（DUI_MSGTYPE_ITEMACTIVATE）"));
        else if (type == DUI_MSGTYPE_MENU)
        {
            CDuiString sMenuName = msg.pSender->GetName();
            SetStatus(CDuiString(_T("菜单项：")) + msg.pSender->GetText());
            if (sMenuName == _T("menu_exit")) {
                // 退出：先让菜单窗口自行关闭（它有自己的销毁流程），主窗口异步退出避免销毁竞态
                ::PostMessage(m_hWnd, WM_CLOSE, 0, 0);
            }
        }
        else if (type == DUI_MSGTYPE_HTMLLINK)
        {
            CDuiString s;
            s.Format(_T("HTML 链接点击：%s"), (LPCTSTR)msg.wParam);
            SetStatus(s);
        }
        else if (type == DUI_MSGTYPE_HTMLFORMSUBMIT)
        {
            CDuiString s;
            s.Format(_T("HTML 按钮点击：%s"), (LPCTSTR)msg.wParam);
            SetStatus(s);
            ReadHtmlForm();
        }
        else if (type == DUI_MSGTYPE_LINK)
            SetStatus(CDuiString(_T("点击链接：")) + msg.pSender->GetText());
    }


    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        m_pPaintManager = new CPaintManagerUI;
        m_pPaintManager->Init(m_hWnd, _T("ControlDemo"));
        // 资源路径 = exe 目录\skin，使用外置 XML（无 zip 时直接读文件夹）
        CPaintManagerUI::SetResourcePath(CPaintManagerUI::GetInstancePath() + _T("skin"));

        CDialogBuilder builder;
        CControlUI* pRoot = builder.Create(_T("main.xml"), (UINT)0, this, m_pPaintManager);
        if (pRoot == NULL) return -1;
        m_pPaintManager->AttachDialog(pRoot);
        m_pPaintManager->AddNotifier(this);

        InitDemoData();
        return 0;
    }

    LRESULT OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
    {
        bHandled = FALSE;
        PostQuitMessage(0);
        return 0;
    }

    LRESULT ResponseDefaultKeyEvent(WPARAM wParam)
    {
        // ESC 关闭窗口
        if (wParam == VK_ESCAPE)
        {
            Close();
            return TRUE;
        }
        return FALSE;
    }

private:
    // ---------------- Demo 行为 ----------------
    void OnClick(TNotifyUI& msg)
    {
        const CDuiString& name = msg.pSender->GetName();
        if (name == _T("btnClose"))            Close();
        else if (name == _T("btnMin"))         ::SendMessage(m_hWnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
        else if (name == _T("btnMsg"))         ::MessageBox(m_hWnd, _T("Hello DuiLib！\n这是 Button 点击触发的 DUI_MSGTYPE_CLICK 通知。"), _T("提示"), MB_OK | MB_ICONINFORMATION);
        else if (name == _T("btnSubmit"))      SubmitForm();
        else if (name == _T("btnTabPrev"))     SwitchMiniTab(-1);
        else if (name == _T("btnTabNext"))     SwitchMiniTab(1);
        else if (name == _T("btnGo"))          NavigateUrl();
        else if (name == _T("btnFlashPage"))   SwitchFlashTab();
        else if (name == _T("btnGifPlay"))     PlayGif(true);
        else if (name == _T("btnGifPause"))    PlayGif(false);
        else if (name == _T("btnRollStart"))   StartRollText();
        else if (name == _T("btnRollStop"))    StopRollText();
        else if (name == _T("btnRound"))       ToggleRoundCorner();
        else if (name == _T("btnShadow"))      ToggleShadow();
        else if (name == _T("btnDpi"))         ToggleDpi();
        else if (name == _T("btnHSL"))         ToggleHSL();
        else if (name == _T("btnHtmlReload"))  ReloadHtml();
        else if (name == _T("btnHtmlRead"))    ReadHtmlForm();
    }

    // ---------------- Demo 行为 ----------------
    void SetStatus(const CDuiString& sText)
    {
        CControlUI* pStatus = m_pPaintManager->FindControl(_T("lblStatus"));
        if (pStatus != NULL) pStatus->SetText(sText);
    }

    void InitDemoData()
    {
        FillReportList();
        FillMultiList();
        FillContainerList();
        StartRollText();
        SetStatus(_T("就绪。覆盖属性列表全部控件；右键红色区域体验 Menu 菜单。"));
    }

    // List + ListTextElement（回调式文本，配合 IListCallbackUI 需要 owner 实现；
    // 这里改用 ListLabelElement + ListContainerElement 演示两种数据行）
    void FillReportList()
    {
        CListUI* pList = static_cast<CListUI*>(m_pPaintManager->FindControl(_T("listDemo")));
        if (pList == NULL) return;

        struct Row { LPCTSTR name; LPCTSTR dept; LPCTSTR city; LPCTSTR job; };
        static const Row rows[] = {
            { _T("张三"), _T("研发部"), _T("北京"), _T("C++ 工程师") },
            { _T("李四"), _T("设计部"), _T("上海"), _T("UI 设计师") },
            { _T("王五"), _T("测试部"), _T("广州"), _T("测试工程师") },
            { _T("赵六"), _T("产品部"), _T("深圳"), _T("产品经理") },
            { _T("钱七"), _T("运营部"), _T("杭州"), _T("运营专员") },
        };

        for (int i = 0; i < (int)_countof(rows); i++)
        {
            CListTextElementUI* pRow = new CListTextElementUI;
            pRow->SetOwner(pList);
            pList->Add(pRow);
            pRow->SetText(0, rows[i].name);
            pRow->SetText(1, rows[i].dept);
            pRow->SetText(2, rows[i].city);
            pRow->SetText(3, rows[i].job);
        }
    }

    void FillMultiList()
    {
        CListUI* pList = static_cast<CListUI*>(m_pPaintManager->FindControl(_T("listMulti")));
        if (pList == NULL) return;
        for (int i = 1; i <= 8; i++)
        {
            CListLabelElementUI* pRow = new CListLabelElementUI;
            CDuiString s;
            s.Format(_T("多选行 %d —— 按住 Ctrl 或 Shift 可多选；右键可弹菜单"), i);
            pRow->SetText(s);
            pList->Add(pRow);
        }
    }

    void FillContainerList()
    {
        CListUI* pList = static_cast<CListUI*>(m_pPaintManager->FindControl(_T("listContainer")));
        if (pList == NULL) return;
        for (int i = 1; i <= 4; i++)
        {
            CListContainerElementUI* pRow = new CListContainerElementUI;
            pRow->SetFixedHeight(34);
            pRow->SetInset(CDuiRect(8, 2, 8, 2));
            pList->Add(pRow);

            CHorizontalLayoutUI* pLine = new CHorizontalLayoutUI;
            pLine->SetChildPadding(10);
            pRow->Add(pLine);

            CLabelUI* pIndex = new CLabelUI;
            CDuiString s;
            s.Format(_T("#%d"), i);
            pIndex->SetText(s);
            pIndex->SetFixedWidth(40);
            pLine->Add(pIndex);

            COptionUI* pFlag = new COptionUI;
            pFlag->SetText(_T("启用"));
            pFlag->SetFixedWidth(70);
            pFlag->Selected(true);
            pLine->Add(pFlag);

            CSliderUI* pSlider = new CSliderUI;
            pSlider->SetValue(i * 20);
            pSlider->SetFixedHeight(14);
            pLine->Add(pSlider);

            CButtonUI* pBtn = new CButtonUI;
            pBtn->SetText(_T("行内按钮"));
            pBtn->SetFixedWidth(90);
            pLine->Add(pBtn);
        }
    }

    void SubmitForm()
    {
        CEditUI* pUser = static_cast<CEditUI*>(m_pPaintManager->FindControl(_T("editUser")));
        CEditUI* pPwd  = static_cast<CEditUI*>(m_pFindPwd());
        CEditUI* pAge  = static_cast<CEditUI*>(m_pPaintManager->FindControl(_T("editAge")));
        CDuiString s;
        s.Format(_T("提交结果 → 用户名:%s  密码长度:%d  年龄:%s"),
                 pUser->GetText().GetData(), pPwd->GetText().GetLength(), pAge->GetText().GetData());
        SetStatus(s);
    }

    CControlUI* m_pFindPwd() { return m_pPaintManager->FindControl(_T("editPwd")); }

    void SwitchMiniTab(int delta)
    {
        CTabLayoutUI* pTab = static_cast<CTabLayoutUI*>(m_pPaintManager->FindControl(_T("tabMini")));
        if (pTab == NULL) return;
        int n = pTab->GetCurSel() + delta;
        if (n < 0) n = pTab->GetCount() - 1;
        if (n >= pTab->GetCount()) n = 0;
        pTab->SelectItem(n);
    }

    void NavigateUrl()
    {
        CEditUI* pUrl = static_cast<CEditUI*>(m_pPaintManager->FindControl(_T("editUrl")));
        CWebBrowserUI* pWeb = static_cast<CWebBrowserUI*>(m_pPaintManager->FindControl(_T("webDemo")));
        if (pUrl != NULL && pWeb != NULL) pWeb->NavigateUrl(pUrl->GetText());
    }

    void SwitchFlashTab()
    {
        // Flash 页签在"动效与多媒体"页，切过去展示
        CTabLayoutUI* pTab = static_cast<CTabLayoutUI*>(m_pPaintManager->FindControl(_T("tabMain")));
        if (pTab != NULL) pTab->SelectItem(5);
        SetStatus(_T("Flash 控件依赖系统 ShockwaveFlash 插件（微软已停用），通常仅保留兼容。"));
    }

    void PlayGif(bool bPlay)
    {
        // 按类型查找页面里的 GifAnim 控件并控制播放
        CStdPtrArray* pArr = m_pPaintManager->FindSubControlsByClass(m_pPaintManager->GetRoot(), _T("GifAnimUI"));
        if (pArr == NULL || pArr->GetSize() == 0) { SetStatus(_T("未找到 GifAnim 控件")); return; }
        for (int i = 0; i < pArr->GetSize(); i++)
        {
            CGifAnimUI* pGif = static_cast<CGifAnimUI*>(pArr->GetAt(i));
            if (pGif == NULL) continue;
            if (bPlay) pGif->PlayGif();
            else       pGif->PauseGif();
        }
        SetStatus(bPlay ? _T("GifAnim: PlayGif()") : _T("GifAnim: PauseGif()"));
    }

    void StartRollText()
    {
        CRollTextUI* pRoll = static_cast<CRollTextUI*>(m_pPaintManager->FindControl(_T("rollNotice")));
        if (pRoll != NULL) pRoll->BeginRoll(ROLLTEXT_LEFT, 60, 0);
    }

    void StopRollText()
    {
        CRollTextUI* pRoll = static_cast<CRollTextUI*>(m_pPaintManager->FindControl(_T("rollNotice")));
        if (pRoll != NULL) pRoll->EndRoll();
    }

    void ToggleRoundCorner()
    {
        static bool bRound = true;
        bRound = !bRound;
        m_pPaintManager->SetRoundCorner(bRound ? 16 : 0, bRound ? 16 : 0);
        SetStatus(bRound ? _T("窗口圆角：开（SetRoundCorner）") : _T("窗口圆角：关"));
    }

    void ToggleShadow()
    {
        CShadowUI* pShadow = m_pPaintManager->GetShadow();
        if (pShadow == NULL) return;
        static bool bShadow = pShadow->IsShowShadow();
        bShadow = !bShadow;
        pShadow->ShowShadow(bShadow);   // ShowShadow(true) 时由管理器自动创建/刷新阴影
        SetStatus(bShadow ? _T("窗口阴影：开（CShadowUI）") : _T("窗口阴影：关"));
    }

    void ToggleDpi()
    {
        static bool bHi = false;
        bHi = !bHi;
        m_pPaintManager->SetDPI(bHi ? 120 : 96);
        SetStatus(bHi ? _T("DPI：125%（SetDPI 自适应缩放）") : _T("DPI：100%"));
    }

    void ToggleHSL()
    {
        static bool bHsl = false;
        static short H = 180, S = 100, L = 100;
        bHsl = !bHsl;
        m_pPaintManager->SetHSL(bHsl, H, S, L);
        SetStatus(bHsl ? _T("HSL 调色：开（对 bkimage 图片生效）") : _T("HSL 调色：关"));
    }

    // ---- HTML 界面 ----
    void ReloadHtml()
    {
        CHtmlBoxUI* pBox = static_cast<CHtmlBoxUI*>(m_pPaintManager->FindControl(_T("htmlBox")));
        if (pBox != NULL) {
            pBox->SetHtmlFromFile(_T("html_demo.html"));
            SetStatus(_T("HTML 已重新加载"));
        }
    }

    void ReadHtmlForm()
    {
        CHtmlBoxUI* pBox = static_cast<CHtmlBoxUI*>(m_pPaintManager->FindControl(_T("htmlBox")));
        if (pBox == NULL) return;
        CDuiString s;
        s.Format(_T("表单值 → 用户名:%s  城市:%s"),
                 pBox->GetValue(_T("user")).GetData(),
                 pBox->GetValue(_T("city")).GetData());
        SetStatus(s);
    }



    void ShowDemoMenu()
    {
        POINT pt;
        ::GetCursorPos(&pt);
        // 注意：LoadFromFile 会自动拼接 ResourcePath，传相对文件名即可；
        // 传绝对路径会拼接成无效路径 → 解析失败 → AttachDialog(NULL) 断言崩溃
        CMenuWnd::CreateMenu(NULL,
            _T("menu.xml"),
            pt, m_pPaintManager, NULL,
            eMenuAlignment_Left | eMenuAlignment_Bottom);
    }

    // ---------------- 消息处理 ----------------
    // 模式参考 WindowImplBase::HandleMessage：
    //   1. 先处理框架级消息（CREATE/CLOSE/CONTEXTMENU/NCHITTEST）
    //   2. 未处理的消息交给 CPaintManagerUI::MessageHandler 分发给控件
    //   3. 仍未处理的交给默认窗口过程（不可吞掉，否则光标/NC 区域全部异常）
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override
    {
        LRESULT lRes = 0;
        BOOL bHandled = TRUE;
        switch (uMsg)
        {
        case WM_CREATE:     lRes = OnCreate(uMsg, wParam, lParam, bHandled); break;
        case WM_CLOSE:      lRes = OnClose(uMsg, wParam, lParam, bHandled); break;
        case WM_NCCREATE: {
            // 无边框：在 WM_NCCREATE 时清除系统残留的边框样式位（WS_CAPTION/WS_THICKFRAME/WS_SYSMENU）
            LONG_PTR st = ::GetWindowLongPtr(m_hWnd, GWL_STYLE);
            st &= ~(WS_CAPTION | WS_THICKFRAME | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
            ::SetWindowLongPtr(m_hWnd, GWL_STYLE, st);
            bHandled = FALSE;
            break;
        }
        case WM_SIZE: {
            // 圆角：优先 Win11 DWM 原生圆角（系统自带抗锯齿，边缘平滑）；
            // Win10 无该 API 时回退 SetWindowRgn（1-bit 蒙版有轻微锯齿，属系统限制）
            if (m_pPaintManager != NULL && !::IsIconic(m_hWnd)) {
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
                if (!bDwmOk) {
                    // 回退：RoundRect 区域圆角
                    SIZE sz = m_pPaintManager->GetRoundCorner();
                    RECT rcWnd;
                    ::GetWindowRect(m_hWnd, &rcWnd);
                    ::OffsetRect(&rcWnd, -rcWnd.left, -rcWnd.top);
                    rcWnd.right++; rcWnd.bottom++;
                    HRGN hRgn = ::CreateRoundRectRgn(rcWnd.left, rcWnd.top, rcWnd.right, rcWnd.bottom,
                                                     MAX(sz.cx, 12), MAX(sz.cy, 12));
                    ::SetWindowRgn(m_hWnd, hRgn, TRUE);
                    ::DeleteObject(hRgn);
                }
            }
            bHandled = FALSE;
            break;
        }
        case WM_NCACTIVATE:
            // 无边框窗口：屏蔽系统非客户区激活重绘（避免顶边闪白线），返回 TRUE 阻止默认处理
            if (!::IsIconic(m_hWnd)) return TRUE;
            bHandled = FALSE;
            break;
        case WM_NCHITTEST:
            lRes = OnNcHitTest(uMsg, wParam, lParam, bHandled); break;
        case WM_GETMINMAXINFO:
            lRes = OnGetMinMaxInfo(uMsg, wParam, lParam, bHandled); break;
        case WM_SYSCOMMAND:
            // 注意：不要拦截 SC_MOVE！HTCAPTION 拖动依赖系统 SC_MOVE 流程，
            // 拦截后窗口将无法拖动（仅跳过屏幕抖动相关的 SC_MASK_FMS）
            if ((wParam & 0xFFF0) == 0xF010 + 2) {   // SC_MASK_FMS 内部值
                return DefWindowProc(m_hWnd, uMsg, wParam, lParam);
            }
            bHandled = FALSE;
            break;
        case WM_CONTEXTMENU: {
            // 右键菜单区域：在指定控件上弹出 Menu
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            ::ScreenToClient(m_hWnd, &pt);
            CControlUI* pHit = m_pPaintManager != NULL ? m_pPaintManager->FindControl(pt) : NULL;
            if (pHit != NULL)
            {
                CDuiString sName = pHit->GetName();
                if (sName == _T("menuArea") || sName == _T("listMulti")) {
                    ShowDemoMenu();
                    return 0;
                }
            }

            bHandled = FALSE;   // 其余区域走默认处理
            break;
        }
        default: bHandled = FALSE; break;
        }
        if (bHandled) return lRes;

        // 控件消息分发（鼠标/键盘/绘制/光标全部在这里处理）
        if (m_pPaintManager != NULL) {
            LRESULT lRes2 = 0;
            if (m_pPaintManager->MessageHandler(uMsg, wParam, lParam, lRes2))
                return lRes2;
        }

        // 未处理消息必须交还默认窗口过程（否则 WM_SETCURSOR 等无人响应 → 沙漏光标）
        return __super::HandleMessage(uMsg, wParam, lParam);
    }

private:
    // 无边框窗口命中测试：sizebox 四周缩放 + caption 标题栏拖动（逻辑同 WindowImplBase::OnNcHitTest）
    LRESULT OnNcHitTest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        POINT pt; pt.x = GET_X_LPARAM(lParam); pt.y = GET_Y_LPARAM(lParam);
        ::ScreenToClient(m_hWnd, &pt);

        RECT rcClient;
        ::GetClientRect(m_hWnd, &rcClient);

        if (!::IsZoomed(m_hWnd))
        {
            RECT rcSizeBox = m_pPaintManager->GetSizeBox();
            if (pt.y < rcClient.top + rcSizeBox.top) {
                if (pt.x < rcClient.left + rcSizeBox.left)   return HTTOPLEFT;
                if (pt.x > rcClient.right - rcSizeBox.right) return HTTOPRIGHT;
                return HTTOP;
            }
            else if (pt.y > rcClient.bottom - rcSizeBox.bottom) {
                if (pt.x < rcClient.left + rcSizeBox.left)   return HTBOTTOMLEFT;
                if (pt.x > rcClient.right - rcSizeBox.right) return HTBOTTOMRIGHT;
                return HTBOTTOM;
            }
            if (pt.x < rcClient.left + rcSizeBox.left)   return HTLEFT;
            if (pt.x > rcClient.right - rcSizeBox.right) return HTRIGHT;
        }

        // caption 区域内、且命中的是静态控件时返回 HTCAPTION，让系统完成拖动
        RECT rcCaption = m_pPaintManager->GetCaptionRect();
        if (pt.x >= rcClient.left + rcCaption.left && pt.x < rcClient.right - rcCaption.right
            && pt.y >= rcCaption.top && pt.y < rcCaption.bottom)
        {
            CControlUI* pControl = m_pPaintManager->FindControl(pt);
            if (IsInStaticControl(pControl))
                return HTCAPTION;
        }
        return HTCLIENT;
    }

    // 限制窗口最小尺寸（XML mininfo 已配置，此处为无边框窗口补充系统级限制）
    LRESULT OnGetMinMaxInfo(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        MINMAXINFO* pMMI = (MINMAXINFO*)lParam;
        pMMI->ptMinTrackSize.x = 960;
        pMMI->ptMinTrackSize.y = 640;
        bHandled = TRUE;
        return 0;
    }

    // 判断命中控件是否为静态控件（不可交互），静态控件上才允许按住拖动窗口
    BOOL IsInStaticControl(CControlUI* pControl)
    {
        if (pControl == NULL) return FALSE;
        CDuiString strClassName = pControl->GetClass();
        strClassName.MakeLower();
        if (strClassName == _T("controlui") || strClassName == _T("labelui")
            || strClassName == _T("textui") || strClassName == _T("containerui")
            || strClassName == _T("horizontallayoutui") || strClassName == _T("verticallayoutui")
            || strClassName == _T("tablayoutui"))
        {
            // 逐层向上检查父控件，全部为静态才允许拖动
            CControlUI* pParent = pControl->GetParent();
            while (pParent != NULL)
            {
                CDuiString sParent = pParent->GetClass();
                sParent.MakeLower();
                if (!(sParent == _T("horizontallayoutui") || sParent == _T("verticallayoutui")
                    || sParent == _T("containerui") || sParent == _T("tablayoutui")))
                    return FALSE;
                pParent = pParent->GetParent();
            }
            return TRUE;
        }
        return FALSE;
    }

private:
    CPaintManagerUI* m_pPaintManager;
};

// --------------------------------------------------------------------------
// 入口
// --------------------------------------------------------------------------
int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR /*lpCmdLine*/, int nCmdShow)
{

    ::CoInitialize(NULL);
    // 初始化 GDI+ / 公共控件 / 资源路径
    CPaintManagerUI::SetInstance(hInstance);
    CPaintManagerUI::SetCurrentPath(CPaintManagerUI::GetInstancePath());

    CControlDemoFrame* pFrame = new CControlDemoFrame;
    if (pFrame == NULL) return 0;
    // 无边框窗口：WS_POPUP（无系统边框/标题栏），配合 XML 的 caption/sizebox/roundcorner
    // 实现自绘标题拖动、四周缩放、圆角外观
    // 注意：不要给窗口加 WS_EX_LAYERED！layered 窗口若未调用 SetLayeredWindowAttributes
    // 会完全不可见（白屏假象）。圆角由 SetWindowRgn 实现即可，无需 layered。
    HWND hWnd = pFrame->Create(NULL, _T("DuiLib 控件全家桶 Demo"),
        WS_OVERLAPPEDWINDOW & ~(WS_CAPTION | WS_THICKFRAME | WS_SYSMENU) | WS_VISIBLE,
        WS_EX_WINDOWEDGE);
    if (hWnd == NULL) {
        ::MessageBox(NULL, _T("主窗口创建失败！"), _T("ControlDemo"), MB_OK | MB_ICONERROR);
        return -1;
    }
    pFrame->CenterWindow();
    ::ShowWindow(*pFrame, nCmdShow ? SW_SHOWNORMAL : SW_SHOWNORMAL);

    CPaintManagerUI::MessageLoop();

    ::CoUninitialize();
    return 0;
}
