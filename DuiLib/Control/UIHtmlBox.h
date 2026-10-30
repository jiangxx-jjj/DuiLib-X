#ifndef __UIHTMLBOX_H__
#define __UIHTMLBOX_H__

// ==========================================================================
//  CHtmlBoxUI — HTML/CSS 渲染控件
//
//  用 HTML 描述界面，自研引擎排版与绘制，DuiLib 原生控件体系承载。
//  完全不依赖浏览器内核或第三方排版库。
//
//  XML 用法：<HtmlBox html="page.html" />
//  代码用法：pBox->SetHtml(LR"(<div>...</div>)");
// ==========================================================================

#include "../Layout/UIVerticalLayout.h"
#include "HtmlEngine/HtmlDocument.h"

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
    void SetCss(LPCTSTR css);                   // 追加全局样式表
    LPCTSTR GetHtml(void) const { return m_sHtml.GetData(); }

    // ---- 基础路径（图片/css 相对路径基准）----
    void SetBasePath(LPCTSTR path);

    // ---- 表单数据交互（按 HTML name 属性）----
    CDuiString GetValue(LPCTSTR name);              // 读 input/select/textarea 的值
    void SetValue(LPCTSTR name, LPCTSTR value);     // 写值
    bool IsChecked(LPCTSTR name);                   // radio/checkbox 选中状态
    void SetChecked(LPCTSTR name, bool bChecked);
    int  GetSelectedIndex(LPCTSTR name);            // select 选中项
    void SetSelectedIndex(LPCTSTR name, int idx);

protected:
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
    HtmlEngine::HtmlDocument m_doc;
    CDuiString m_sHtml;
    CDuiString m_sCss;
    CDuiString m_sNotifyName;      // 通知消息的 wParam 载体（保证生命周期）
    float m_fDocHeight;
    bool  m_bDocDirty;            // HTML 待（重新）排版标记
};

} // namespace DuiLib

#endif // __UIHTMLBOX_H__
