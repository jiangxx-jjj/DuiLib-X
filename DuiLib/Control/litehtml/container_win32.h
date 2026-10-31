#ifndef LH_CONTAINER_WIN32_H
#define LH_CONTAINER_WIN32_H

// ==========================================================================
//  container_win32.h — litehtml 的 Win32/GDI+ 桥接层
//
//  实现 litehtml::document_container 接口，把 litehtml 的排版结果
//  通过 GDI+ 绘制到目标 HDC。
// ==========================================================================

// 先包含 windows.h，然后临时 #undef min/max 避免与 litehtml 的 std::min/std::max 冲突
#include <windows.h>
#pragma push_macro("min")
#pragma push_macro("max")
#undef min
#undef max

#include "litehtml.h"
#include <gdiplus.h>
#include <map>
#include <string>

// 恢复 windows.h 的 min/max 宏（DuiLib 自己的代码需要）
#pragma pop_macro("max")
#pragma pop_macro("min")

// GDI+ 命名空间别名，避免与 litehtml::Color 冲突
namespace Gp = Gdiplus;

class container_win32 : public litehtml::document_container
{
public:
    container_win32();
    virtual ~container_win32();

    // ---- 绘制目标 ----
    void SetOutput(HDC hdc, int clipX, int clipY, int clipW, int clipH);
    void SetBasePath(const std::string& path) { m_basePath = path; }

    // ---- 图片加载回调（由 DuiLib 注入）----
    typedef bool (*FnImageSize)(const std::string& src, int& w, int& h);
    typedef bool (*FnImageDraw)(HDC hdc, const std::string& src, int x, int y, int w, int h);
    void SetImageCallbacks(FnImageSize fnSize, FnImageDraw fnDraw)
    { m_fnImageSize = fnSize; m_fnImageDraw = fnDraw; }

    // ---- 交互回调（由 DuiLib 注入，用于通知到 C++）----
    typedef std::function<void(const std::string& href)> FnLinkClick;
    typedef std::function<void(const std::string& name)> FnButtonClick;
    typedef std::function<void(const std::string& name)> FnValueChanged;
    void SetInteractCallbacks(FnLinkClick fnLink, FnButtonClick fnBtn, FnValueChanged fnVal)
    { m_fnLinkClick = fnLink; m_fnButtonClick = fnBtn; m_fnValueChanged = fnVal; }

    // ---- 键盘输入（转发给当前焦点表单控件）----
    bool InputChar(wchar_t ch);
    bool InputKeyDown(int vkCode);

    // ---- 光标查询 ----
    const std::string& GetCursor() const { return m_cursor; }

    // ---- 焦点元素 ----
    void SetFocusElement(litehtml::element::ptr el) { m_focusElement = el; }
    litehtml::element::ptr GetFocusElement() const { return m_focusElement; }

    // ---- 表单控件支持 ----
    // 由 hFont 句柄取 GDI+ 字体对象（表单控件绘制用）
    Gp::Font* GetGpFont(litehtml::uint_ptr hFont);

    // 表单控件事件（由 el_form 元素回调）
    void OnRadioClicked(litehtml::element::ptr radio);
    void OnButtonClicked(const std::string& name);
    void OnValueChanged(const std::string& name);

    // 按 name 属性查找所有元素（全局遍历）
    std::vector<litehtml::element::ptr> FindControlsByName(litehtml::element::ptr root,
                                                           const std::string& name);
    // 递归收集（公开供内部使用）
    void CollectByName(litehtml::element::ptr root, const std::string& name,
                       std::vector<litehtml::element::ptr>& out);
    // 设置文档根（供查找遍历）
    void SetRootElement(litehtml::element::ptr root) { m_rootElement = root; }

    // ---- document_container 接口 ----
    litehtml::uint_ptr create_font(const char* faceName, int size, int weight,
        litehtml::font_style italic, unsigned int decoration, litehtml::font_metrics* fm) override;
    void delete_font(litehtml::uint_ptr hFont) override;
    int text_width(const char* text, litehtml::uint_ptr hFont) override;
    void draw_text(litehtml::uint_ptr hdc, const char* text, litehtml::uint_ptr hFont,
        litehtml::web_color color, const litehtml::position& pos) override;
    int pt_to_px(int pt) const override;
    int get_default_font_size() const override;
    const char* get_default_font_name() const override;
    void draw_list_marker(litehtml::uint_ptr hdc, const litehtml::list_marker& marker) override;
    void load_image(const char* src, const char* baseurl, bool redraw_on_ready) override;
    void get_image_size(const char* src, const char* baseurl, litehtml::size& sz) override;
    void draw_background(litehtml::uint_ptr hdc,
        const std::vector<litehtml::background_paint>& bg) override;
    void draw_borders(litehtml::uint_ptr hdc, const litehtml::borders& borders,
        const litehtml::position& draw_pos, bool root) override;

    void set_caption(const char* caption) override;
    void set_base_url(const char* base_url) override;
    void link(const std::shared_ptr<litehtml::document>& doc,
        const litehtml::element::ptr& el) override;
    void on_anchor_click(const char* url, const litehtml::element::ptr& el) override;
    void set_cursor(const char* cursor) override;
    void transform_text(litehtml::string& text, litehtml::text_transform tt) override;
    void import_css(litehtml::string& text, const litehtml::string& url,
        litehtml::string& baseurl) override;
    void set_clip(const litehtml::position& pos, const litehtml::border_radiuses& bdr_radius) override;
    void del_clip() override;
    void get_client_rect(litehtml::position& client) const override;
    litehtml::element::ptr create_element(const char* tag_name,
        const litehtml::string_map& attributes,
        const std::shared_ptr<litehtml::document>& doc) override;

    void get_media_features(litehtml::media_features& media) const override;
    void get_language(litehtml::string& language, litehtml::string& culture) const override;

protected:
    // 字体缓存：key = face|size|weight|italic
    struct FontEntry
    {
        Gp::Font*   pFont;
        int         height;
        int         ascent;
        int         descent;
    };
    std::map<std::string, FontEntry> m_fonts;
    litehtml::uint_ptr m_nextFontId;
    std::map<litehtml::uint_ptr, FontEntry> m_fontById;

    // 绘制目标
    HDC     m_hdc;
    int     m_clipX, m_clipY, m_clipW, m_clipH;
    std::string m_basePath;

    // 图片回调
    FnImageSize m_fnImageSize;
    FnImageDraw m_fnImageDraw;

    // 交互回调
    std::function<void(const std::string& href)> m_fnLinkClick;
    std::function<void(const std::string& name)> m_fnButtonClick;
    std::function<void(const std::string& name)> m_fnValueChanged;

    // 当前焦点表单元素
    litehtml::element::ptr m_focusElement;

    // 文档根元素（供按 name 查找）
    litehtml::element::ptr m_rootElement;

    // 当前光标
    std::string m_cursor;

    // GDI+ 初始化（引用计数）
    static ULONG_PTR s_gdiplusToken;
    static int s_gdiplusRefCount;
    static bool InitGdiPlus();
    static void ShutdownGdiPlus();

    // 工具：litehtml::web_color -> Gdiplus::Color（公开：表单控件用）
public:
    static Gp::Color ToGpColor(const litehtml::web_color& c, float opacity = 1.0f);

    // 工具：构建圆角路径（公开：表单控件绘制也用）
    static void BuildRoundRectPath(Gp::GraphicsPath& path, int x, int y, int w, int h,
        const litehtml::border_radiuses& br);

protected:
    // 工具：绘制渐变
    void DrawGradient(Gp::Graphics& g, Gp::GraphicsPath& path,
        const std::string& gradientStr, int x, int y, int w, int h);

    // 工具：解析 CSS 颜色
    static Gp::Color ParseColor(const std::string& str);

    // 工具：按分隔符切分，跳过括号内
    static std::vector<std::string> SplitTopLevel(const std::string& s, char sep);

    // 工具：十六进制字符转整数
    static int HexCharToInt(char c);

    // 工具：字符串小写/去空白
    static std::string ToLower(const std::string& s);
    static std::string Trim(const std::string& s);
};

#endif // LH_CONTAINER_WIN32_H
