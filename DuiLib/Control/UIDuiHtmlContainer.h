#ifndef __UIDUIHTMLCONTAINER_H__
#define __UIDUIHTMLCONTAINER_H__

// ==========================================================================
// CDuiHtmlContainer — litehtml 绘制回调容器
//
// 实现 litehtml::document_container 全部接口，把 litehtml 的排版结果
// 绘制到 DuiLib 的渲染引擎上（GDI/GDI+）。
//
// 由 CHtmlBoxUI 持有并驱动：
//   SetHtml → litehtml::document::createFromString → render → draw
// ==========================================================================

#include "StdAfx.h"
// litehtml 使用 std::min/std::max，与 windows.h 的 min/max 宏冲突——包含前解除
#pragma push_macro("min")
#pragma push_macro("max")
#undef min
#undef max
#include "litehtml.h"
#pragma pop_macro("min")
#pragma pop_macro("max")
#include <map>
#include <vector>
#include <functional>

namespace DuiLib
{

// 前置声明：表单控件元素（定义在 UIDuiHtmlForm.h）
class CHtmlFormElement;

class CDuiHtmlContainer : public litehtml::document_container
{
public:
    CDuiHtmlContainer(void);
    virtual ~CDuiHtmlContainer(void);

    // ---- 文档管理 ----
    bool CreateDocument(LPCTSTR html, LPCTSTR css = NULL);
    void DestroyDocument(void);
    litehtml::document::ptr GetDocument(void) { return m_doc; }

    // ---- 绘制 ----
    // hdc      : 目标 DC（由 CHtmlBoxUI 传入，通常为内存 DC）
    // scrollY  : 垂直滚动偏移
    // rcClient : 容器客户区（决定排版宽度与裁剪）
    void Draw(HDC hdc, int scrollY, const RECT& rcClient);
    int  Render(int maxWidth);              // 返回文档高度

    // ---- 尺寸 ----
    int GetDocumentWidth(void) const;
    int GetDocumentHeight(void) const;

    // ---- 鼠标交互（由 CHtmlBoxUI 转发）----
    void OnMouseMove(POINT pt);
    void OnLeftButtonDown(POINT pt);
    void OnLeftButtonUp(POINT pt);
    void SetScrollOffset(int scrollY);
    int  GetScrollOffset(void) const { return m_scrollY; }

    // ---- 事件回调（宿主设置）----
    // on_link_click: <a href> 点击时回调（href 为链接地址）
    void SetLinkClickCallback(std::function<void(const CDuiString& href)> cb) { m_fnLinkClick = cb; }
    // 表单提交/数据变化回调
    void SetFormSubmitCallback(std::function<void(CHtmlFormElement* pEl)> cb) { m_fnFormSubmit = cb; }
    void SetFormChangedCallback(std::function<void(CHtmlFormElement* pEl)> cb) { m_fnFormChanged = cb; }

    // ---- 表单控件（由 CHtmlFormElement 回调，供 CHtmlBoxUI 使用）----
    void OnFormSubmit(CHtmlFormElement* pEl);
    void OnFormChanged(CHtmlFormElement* pEl);
    // 按 name 查找表单控件（递归遍历文档树）
    CHtmlFormElement* FindFormElement(LPCTSTR name);
    // 当前获得键盘焦点的表单控件
    CHtmlFormElement* GetFocusedForm(void) const { return m_pFocusedForm; }
    // 设置焦点控件（NULL 表示取消）
    void SetFocusedForm(CHtmlFormElement* pEl);
    // 键盘输入转发
    bool InputChar(wchar_t ch);
    bool InputKeyDown(int vkCode);
    // 在文档中查找命中的表单控件（供鼠标点击判断）
    CHtmlFormElement* HitTestForm(int x, int y);
    // 按 litehtml 字体句柄取真实 HFONT（表单控件自绘用）
    HFONT GetRealFont(litehtml::uint_ptr hFont);

    // ---- 资源路径（图片/css 相对路径基准）----
    void SetBasePath(LPCTSTR path) { m_sBasePath = path; }

public:
    // ================= litehtml::document_container 实现 =================
    virtual litehtml::uint_ptr create_font(const char* faceName, int size, int weight, litehtml::font_style italic, unsigned int decoration, litehtml::font_metrics* fm) override;
    virtual void               delete_font(litehtml::uint_ptr hFont) override;
    virtual int                text_width(const char* text, litehtml::uint_ptr hFont) override;
    virtual void               draw_text(litehtml::uint_ptr hdc, const char* text, litehtml::uint_ptr hFont, litehtml::web_color color, const litehtml::position& pos) override;
    virtual int                pt_to_px(int pt) const override;
    virtual int                get_default_font_size() const override;
    virtual const char*        get_default_font_name() const override;
    virtual void               draw_list_marker(litehtml::uint_ptr hdc, const litehtml::list_marker& marker) override;
    virtual void               load_image(const char* src, const char* baseurl, bool redraw_on_ready) override;
    virtual void               get_image_size(const char* src, const char* baseurl, litehtml::size& sz) override;
    virtual void               draw_background(litehtml::uint_ptr hdc, const std::vector<litehtml::background_paint>& bg) override;
    virtual void               draw_borders(litehtml::uint_ptr hdc, const litehtml::borders& borders, const litehtml::position& draw_pos, bool root) override;
    virtual void               set_caption(const char* caption) override;
    virtual void               set_base_url(const char* base_url) override;
    virtual void               link(const std::shared_ptr<litehtml::document>& doc, const litehtml::element::ptr& el) override;
    virtual void               on_anchor_click(const char* url, const litehtml::element::ptr& el) override;
    virtual void               set_cursor(const char* cursor) override;
    virtual void               transform_text(litehtml::string& text, litehtml::text_transform tt) override;
    virtual void               import_css(litehtml::string& text, const litehtml::string& url, litehtml::string& baseurl) override;
    virtual void               set_clip(const litehtml::position& pos, const litehtml::border_radiuses& bdr_radius) override;
    virtual void               del_clip() override;
    virtual void               get_client_rect(litehtml::position& client) const override;
    virtual litehtml::element::ptr create_element(const char* tag_name, const litehtml::string_map& attributes, const std::shared_ptr<litehtml::document>& doc) override;
    virtual void               get_media_features(litehtml::media_features& media) const override;
    virtual void               get_language(litehtml::string& language, litehtml::string& culture) const override;

private:
    // ---- 内部工具 ----
    HFONT GetFontFromCache(int slot);
    COLORREF ToCOLORREF(const litehtml::web_color& c) const;
    void FillSolidRect(HDC hdc, const RECT& rc, COLORREF clr);
    bool LoadImageFile(LPCTSTR src, TImageInfo** ppImageInfo);

private:
    litehtml::document::ptr m_doc;
    int  m_scrollY;
    RECT m_rcClient;
    int  m_iDPI;
    CDuiString m_sBasePath;
    CDuiString m_sDefaultFontName;
    int  m_iDefaultFontSize;
    CDuiString m_sCaption;
    CDuiString m_sBaseUrl;

    // 字体缓存：slot → HFONT（litehtml 用 uint_ptr 句柄引用字体）
    std::map<litehtml::uint_ptr, HFONT> m_fontCache;
    int  m_nextFontSlot;

    // 图片缓存：src → TImageInfo*
    struct ImageEntry {
        TImageInfo* pInfo;
        bool        bFailed;   // 加载失败标记，避免重复尝试
    };
    std::map<CDuiString, ImageEntry> m_imageCache;

    // 链接点击回调
    std::function<void(const CDuiString&)> m_fnLinkClick;

    // 裁剪栈
    struct ClipInfo { HRGN hRgn; };
    std::vector<HRGN> m_clipStack;
    HDC m_hDrawDC;   // Draw 期间保存的 DC，供 set_clip/del_clip 使用

    // 表单相关
    std::function<void(CHtmlFormElement*)> m_fnFormSubmit;
    std::function<void(CHtmlFormElement*)> m_fnFormChanged;
    CHtmlFormElement* m_pFocusedForm;   // 当前焦点控件
};

} // namespace DuiLib

#endif // __UIDUIHTMLCONTAINER_H__
