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

    // 当前光标
    std::string m_cursor;

    // GDI+ 初始化（引用计数）
    static ULONG_PTR s_gdiplusToken;
    static int s_gdiplusRefCount;
    static bool InitGdiPlus();
    static void ShutdownGdiPlus();

    // 工具：litehtml::web_color -> Gdiplus::Color
    static Gp::Color ToGpColor(const litehtml::web_color& c, float opacity = 1.0f);

    // 工具：构建圆角路径
    static void BuildRoundRectPath(Gp::GraphicsPath& path, int x, int y, int w, int h,
        const litehtml::border_radiuses& br);
};

#endif // LH_CONTAINER_WIN32_H
