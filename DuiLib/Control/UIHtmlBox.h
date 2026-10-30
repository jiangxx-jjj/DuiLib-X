#ifndef __UIHTMLBOX_H__
#define __UIHTMLBOX_H__

// ==========================================================================
// CHtmlBoxUI — HTML/CSS 渲染控件
//
// 用 HTML 描述界面，litehtml 排版，DuiLib 原生渲染。
// 支持滚动、链接点击回调、CSS 选择器/级联/盒模型。
//
// XML 用法：<HtmlBox html="page.html" or SetHtml(...) />
// ==========================================================================

#include "../Layout/UIVerticalLayout.h"
#include "UIDuiHtmlContainer.h"
#include "UIDuiHtmlForm.h"

namespace DuiLib
{

class UILIB_API CHtmlBoxUI : public CVerticalLayoutUI
{
    DECLARE_DUICONTROL(CHtmlBoxUI)

public:
    CHtmlBoxUI(void);
    virtual ~CHtmlBoxUI(void);

    LPCTSTR GetClass() const override;
    LPVOID GetInterface(LPCTSTR pstrName) override;

    // ---- 内容加载 ----
    bool SetHtml(LPCTSTR html);                 // 直接设置 HTML 字符串
    bool SetHtmlFromFile(LPCTSTR file);         // 从文件加载（相对 ResourcePath）
    void SetCss(LPCTSTR css);                   // 设置全局样式表
    LPCTSTR GetHtml(void) const { return m_sHtml.GetData(); }

    // ---- 基础路径（图片/css 相对路径基准）----
    void SetBasePath(LPCTSTR path);

    // ---- 表单数据交互（按 HTML name 属性）----
    CDuiString GetValue(LPCTSTR name);              // 读 input/select/textarea 的值
    void SetValue(LPCTSTR name, LPCTSTR value);     // 写值（同步到控件）
    bool IsChecked(LPCTSTR name);                   // radio/checkbox 选中状态
    void SetChecked(LPCTSTR name, bool bChecked);

protected:
    static CControlUI* FindChildByName(CControlUI* pParent, LPCTSTR name);

    // UTF-8 与 TSTR 转换辅助
    static char*    DuiHtmlWToUtf8(LPCTSTR wstr);
    static wchar_t* DuiHtmlUtf8ToW(const std::string& s);

    // ---- CVerticalLayoutUI 覆盖 ----
    void SetPos(RECT rc, bool bNeedInvalidate = true) override;
    bool DoPaint(HDC hDC, const RECT& rcPaint, CControlUI* pStopControl) override;
    void DoEvent(TEventUI& event) override;
    SIZE EstimateSize(SIZE szAvailable) override;

    // ---- 属性 ----
    void SetAttribute(LPCTSTR pstrName, LPCTSTR pstrValue) override;

protected:
    void EnsureDocument(void);                  // 懒加载文档（需有效宽度）
    void Relayout(void);                        // 重新排版
    void UpdateScrollRange(void);               // 更新滚动条范围

protected:
    CDuiHtmlContainer m_container;
    CDuiString m_sHtml;
    CDuiString m_sCss;
    int  m_iDocHeight;
    bool m_bDocDirty;            // HTML 待（重新）排版标记
};

} // namespace DuiLib

#endif // __UIHTMLBOX_H__
