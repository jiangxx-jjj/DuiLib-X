#include "StdAfx.h"
#include "container_win32.h"
#include <wchar.h>

// GDI+ 初始化
ULONG_PTR container_win32::s_gdiplusToken = 0;
int container_win32::s_gdiplusRefCount = 0;

bool container_win32::InitGdiPlus()
{
    if (s_gdiplusRefCount > 0) { s_gdiplusRefCount++; return true; }
    Gp::GdiplusStartupInput input;
    input.GdiplusVersion = 1;
    input.DebugEventCallback = NULL;
    input.SuppressBackgroundThread = FALSE;
    input.SuppressExternalCodecs = FALSE;
    if (Gp::GdiplusStartup(&s_gdiplusToken, &input, NULL) != Gp::Status::Ok) {
        s_gdiplusToken = 0;
        return false;
    }
    s_gdiplusRefCount = 1;
    return true;
}

void container_win32::ShutdownGdiPlus()
{
    if (s_gdiplusRefCount <= 0) return;
    s_gdiplusRefCount--;
    if (s_gdiplusRefCount == 0 && s_gdiplusToken != 0) {
        Gp::GdiplusShutdown(s_gdiplusToken);
        s_gdiplusToken = 0;
    }
}

container_win32::container_win32()
    : m_hdc(NULL)
    , m_clipX(0), m_clipY(0), m_clipW(0), m_clipH(0)
    , m_fnImageSize(NULL), m_fnImageDraw(NULL)
    , m_nextFontId(1)
{
    InitGdiPlus();
}

container_win32::~container_win32()
{
    // 释放所有缓存字体
    for (auto& pair : m_fonts) delete pair.second.pFont;
    m_fonts.clear();
    m_fontById.clear();
    ShutdownGdiPlus();
}

void container_win32::SetOutput(HDC hdc, int clipX, int clipY, int clipW, int clipH)
{
    m_hdc = hdc;
    m_clipX = clipX; m_clipY = clipY;
    m_clipW = clipW; m_clipH = clipH;
}

Gp::Color container_win32::ToGpColor(const litehtml::web_color& c, float opacity)
{
    int a = (int)(c.alpha * opacity);
    if (a < 0) a = 0;
    if (a > 255) a = 255;
    return Gp::Color((BYTE)a, c.red, c.green, c.blue);
}

// ============================ 字体 ============================

litehtml::uint_ptr container_win32::create_font(const char* faceName, int size, int weight,
    litehtml::font_style italic, unsigned int decoration, litehtml::font_metrics* fm)
{
    std::string key = std::string(faceName) + "|" + std::to_string(size) + "|" +
                      std::to_string(weight) + (italic == litehtml::font_style_italic ? "|I" : "|N");

    auto it = m_fonts.find(key);
    if (it != m_fonts.end()) {
        litehtml::uint_ptr id = m_nextFontId++;
        m_fontById[id] = it->second;
        if (fm) { fm->height = it->second.height; fm->ascent = it->second.ascent;
                  fm->descent = it->second.descent; fm->x_height = it->second.height / 2; }
        return id;
    }

    // 新建 GDI+ 字体
    std::wstring wFace;
    if (faceName && faceName[0]) {
        int n = ::MultiByteToWideChar(CP_UTF8, 0, faceName, -1, NULL, 0);
        if (n > 0) { std::wstring w(n - 1, 0); ::MultiByteToWideChar(CP_UTF8, 0, faceName, -1, &w[0], n); wFace = w; }
    }
    if (wFace.empty()) wFace = L"微软雅黑";

    Gp::FontStyle style = Gp::FontStyleRegular;
    if (weight >= 700) style = (Gp::FontStyle)(style | Gp::FontStyleBold);
    if (italic == litehtml::font_style_italic) style = (Gp::FontStyle)(style | Gp::FontStyleItalic);

    Gp::Font* pFont = new Gp::Font(wFace.c_str(), (Gp::REAL)size, style, Gp::UnitPixel);
    if (!pFont || pFont->GetLastStatus() != Gp::Status::Ok) {
        delete pFont;
        pFont = new Gp::Font(L"微软雅黑", (Gp::REAL)size, style, Gp::UnitPixel);
    }

    // 测量行高
    Gp::Bitmap bmp(1, 1);
    Gp::Graphics g(&bmp);
    Gp::FontFamily ff;
    pFont->GetFamily(&ff);
    float emHeight = (float)ff.GetEmHeight(pFont->GetStyle());
    float lineSpacing = (float)ff.GetLineSpacing(pFont->GetStyle());
    int height = (int)(size * lineSpacing / emHeight);
    if (height <= 0) height = (int)(size * 1.4f);

    FontEntry entry;
    entry.pFont = pFont;
    entry.height = height;
    entry.ascent = (int)(height * 0.8f);
    entry.descent = height - entry.ascent;
    m_fonts[key] = entry;

    litehtml::uint_ptr id = m_nextFontId++;
    m_fontById[id] = entry;
    if (fm) { fm->height = height; fm->ascent = entry.ascent; fm->descent = entry.descent;
              fm->x_height = height / 2; }
    return id;
}

void container_win32::delete_font(litehtml::uint_ptr hFont)
{
    // 字体由缓存统一管理，不单独删除
}

int container_win32::text_width(const char* text, litehtml::uint_ptr hFont)
{
    if (!text || !text[0]) return 0;
    auto it = m_fontById.find(hFont);
    if (it == m_fontById.end()) return (int)strlen(text) * 8;

    std::wstring wText;
    int n = ::MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
    if (n > 0) { std::wstring w(n - 1, 0); ::MultiByteToWideChar(CP_UTF8, 0, text, -1, &w[0], n); wText = w; }
    if (wText.empty()) return 0;

    Gp::Bitmap bmp(1, 1);
    Gp::Graphics g(&bmp);
    g.SetTextRenderingHint(Gp::TextRenderingHintAntiAlias);
    Gp::RectF layoutRect(0, 0, 100000.0f, 100000.0f);
    Gp::RectF boundRect;
    Gp::StringFormat fmt;
    fmt.SetFormatFlags(Gp::StringFormatFlagsMeasureTrailingSpaces | Gp::StringFormatFlagsNoWrap);
    fmt.SetTrimming(Gp::StringTrimmingNone);
    g.MeasureString(wText.c_str(), (INT)wText.size(), it->second.pFont, layoutRect, &fmt, &boundRect);
    return (int)boundRect.Width;
}

void container_win32::draw_text(litehtml::uint_ptr hdc, const char* text, litehtml::uint_ptr hFont,
    litehtml::web_color color, const litehtml::position& pos)
{
    if (!m_hdc || !text || !text[0]) return;
    auto it = m_fontById.find(hFont);
    if (it == m_fontById.end()) return;

    std::wstring wText;
    int n = ::MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
    if (n > 0) { std::wstring w(n - 1, 0); ::MultiByteToWideChar(CP_UTF8, 0, text, -1, &w[0], n); wText = w; }
    if (wText.empty()) return;

    Gp::Graphics g(m_hdc);
    g.SetTextRenderingHint(Gp::TextRenderingHintAntiAlias);
    Gp::SolidBrush brush(ToGpColor(color));
    g.DrawString(wText.c_str(), (INT)wText.size(), it->second.pFont,
                 Gp::PointF((Gp::REAL)pos.x, (Gp::REAL)pos.y), &brush);
}

// ============================ 背景/边框 ============================

void container_win32::BuildRoundRectPath(Gp::GraphicsPath& path, int x, int y, int w, int h,
    const litehtml::border_radiuses& br)
{
    int tl = br.top_left_x, tr = br.top_right_x;
    int br_ = br.bottom_right_x, bl = br.bottom_left_x;
    int tlv = br.top_left_y > 0 ? br.top_left_y : tl;
    int trv = br.top_right_y > 0 ? br.top_right_y : tr;
    int brv = br.bottom_right_y > 0 ? br.bottom_right_y : br_;
    int blv = br.bottom_left_y > 0 ? br.bottom_left_y : bl;

    // 半径不能超过一半
    if (tl > w / 2) tl = w / 2;    if (tlv > h / 2) tlv = h / 2;
    if (tr > w / 2) tr = w / 2;    if (trv > h / 2) trv = h / 2;
    if (br_ > w / 2) br_ = w / 2;  if (brv > h / 2) brv = h / 2;
    if (bl > w / 2) bl = w / 2;    if (blv > h / 2) blv = h / 2;

    path.Reset();

    if (tl <= 0 && tr <= 0 && br_ <= 0 && bl <= 0) {
        path.AddRectangle(Gp::Rect(x, y, w, h));
        return;
    }

    const float k = 0.5522847498f;
    float fx = (float)x, fy = (float)y, fw = (float)w, fh = (float)h;

    // 左上
    if (tl > 0 || tlv > 0) {
        path.AddBezier(Gp::PointF(fx + tl, fy), Gp::PointF(fx + tl - tl * k, fy),
                       Gp::PointF(fx, fy + tlv - tlv * k), Gp::PointF(fx, fy + tlv));
    }
    // 上边
    path.AddLine(Gp::PointF(fx + tl, fy), Gp::PointF(fx + fw - tr, fy));
    // 右上
    if (tr > 0 || trv > 0) {
        path.AddBezier(Gp::PointF(fx + fw - tr, fy), Gp::PointF(fx + fw - tr + tr * k, fy),
                       Gp::PointF(fx + fw, fy + trv - trv * k), Gp::PointF(fx + fw, fy + trv));
    }
    // 右边
    path.AddLine(Gp::PointF(fx + fw, fy + trv), Gp::PointF(fx + fw, fy + fh - brv));
    // 右下
    if (br_ > 0 || brv > 0) {
        path.AddBezier(Gp::PointF(fx + fw, fy + fh - brv), Gp::PointF(fx + fw, fy + fh - brv + brv * k),
                       Gp::PointF(fx + fw - br_ + br_ * k, fy + fh), Gp::PointF(fx + fw - br_, fy + fh));
    }
    // 下边
    path.AddLine(Gp::PointF(fx + fw - br_, fy + fh), Gp::PointF(fx + bl, fy + fh));
    // 左下
    if (bl > 0 || blv > 0) {
        path.AddBezier(Gp::PointF(fx + bl, fy + fh), Gp::PointF(fx + bl - bl * k, fy + fh),
                       Gp::PointF(fx, fy + fh - blv + blv * k), Gp::PointF(fx, fy + fh - blv));
    }
    // 左边
    path.AddLine(Gp::PointF(fx, fy + fh - blv), Gp::PointF(fx, fy + tlv));
    path.CloseFigure();
}

void container_win32::draw_background(litehtml::uint_ptr hdc,
    const std::vector<litehtml::background_paint>& bg)
{
    if (!m_hdc || bg.empty()) return;

    for (size_t i = 0; i < bg.size(); i++) {
        const litehtml::background_paint& b = bg[i];
        int x = b.border_box.x, y = b.border_box.y;
        int w = b.border_box.width, h = b.border_box.height;
        if (w <= 0 || h <= 0) continue;

        Gp::Graphics g(m_hdc);
        g.SetSmoothingMode(Gp::SmoothingModeAntiAlias);
        g.SetPixelOffsetMode(Gp::PixelOffsetModeHalf);

        // 圆角路径
        Gp::GraphicsPath path;
        BuildRoundRectPath(path, x, y, w, h, b.border_radius);

        // 渐变背景（linear-gradient 等）
        if (!b.image.empty() && b.image.find("gradient") != std::string::npos) {
            // litehtml 把 gradient 作为 image 传入，格式如 "linear-gradient(...)"
            // 我们需要解析并绘制渐变
            // TODO: 阶段 2 实现渐变解析
        }

        // 背景图片
        if (!b.image.empty() && m_fnImageDraw) {
            std::string src = b.image;
            // 去掉 url() 包装
            if (src.find("url(") == 0) {
                size_t p1 = src.find('('), p2 = src.rfind(')');
                if (p1 != std::string::npos && p2 > p1) src = src.substr(p1 + 1, p2 - p1 - 1);
            }
            // 去掉引号
            if (src.size() >= 2 && (src[0] == '"' || src[0] == '\''))
                src = src.substr(1, src.size() - 2);

            m_fnImageDraw(m_hdc, src, x, y, w, h);
        }

        // 纯色背景（最后一层，只有最后一项有 color）
        if (i == bg.size() - 1 && b.color.alpha > 0) {
            Gp::SolidBrush brush(ToGpColor(b.color));
            g.FillPath(&brush, &path);
        }
    }
}

void container_win32::draw_borders(litehtml::uint_ptr hdc, const litehtml::borders& borders,
    const litehtml::position& draw_pos, bool root)
{
    if (!m_hdc) return;
    if (!borders.is_visible()) return;

    int x = draw_pos.x, y = draw_pos.y;
    int w = draw_pos.width, h = draw_pos.height;
    if (w <= 0 || h <= 0) return;

    Gp::Graphics g(m_hdc);
    g.SetSmoothingMode(Gp::SmoothingModeAntiAlias);

    // 圆角路径
    Gp::GraphicsPath path;
    BuildRoundRectPath(path, x, y, w, h, borders.radius);

    // 四边同色同宽时用一条描边路径
    bool bSame = borders.top.width > 0 &&
                 borders.top.color == borders.right.color &&
                 borders.top.color == borders.bottom.color &&
                 borders.top.color == borders.left.color &&
                 borders.top.width == borders.right.width &&
                 borders.top.width == borders.bottom.width &&
                 borders.top.width == borders.left.width;

    if (bSame) {
        Gp::Pen pen(ToGpColor(borders.top.color), (Gp::REAL)borders.top.width);
        pen.SetLineJoin(Gp::LineJoinRound);
        switch (borders.top.style) {
        case litehtml::border_style_dashed: pen.SetDashStyle(Gp::DashStyleDash); break;
        case litehtml::border_style_dotted: pen.SetDashStyle(Gp::DashStyleDot); break;
        default: pen.SetDashStyle(Gp::DashStyleSolid); break;
        }
        g.DrawPath(&pen, &path);
        return;
    }

    // 四边不同：分别绘制（简化：用矩形条）
    if (borders.top.width > 0 && borders.top.color.alpha > 0) {
        Gp::SolidBrush b(ToGpColor(borders.top.color));
        g.FillRectangle(&b, x, y, w, borders.top.width);
    }
    if (borders.bottom.width > 0 && borders.bottom.color.alpha > 0) {
        Gp::SolidBrush b(ToGpColor(borders.bottom.color));
        g.FillRectangle(&b, x, y + h - borders.bottom.width, w, borders.bottom.width);
    }
    if (borders.left.width > 0 && borders.left.color.alpha > 0) {
        Gp::SolidBrush b(ToGpColor(borders.left.color));
        g.FillRectangle(&b, x, y, borders.left.width, h);
    }
    if (borders.right.width > 0 && borders.right.color.alpha > 0) {
        Gp::SolidBrush b(ToGpColor(borders.right.color));
        g.FillRectangle(&b, x + w - borders.right.width, y, borders.right.width, h);
    }
}

// ============================ 图片 ============================

void container_win32::load_image(const char* src, const char* baseurl, bool redraw_on_ready)
{
    // 图片由 DuiLib 的 CRenderEngine 管理，这里不需要预加载
}

void container_win32::get_image_size(const char* src, const char* baseurl, litehtml::size& sz)
{
    if (!m_fnImageSize) { sz.width = 0; sz.height = 0; return; }

    std::string s = src ? src : "";
    // 去掉 url() 包装
    if (s.find("url(") == 0) {
        size_t p1 = s.find('('), p2 = s.rfind(')');
        if (p1 != std::string::npos && p2 > p1) s = s.substr(p1 + 1, p2 - p1 - 1);
    }
    if (s.size() >= 2 && (s[0] == '"' || s[0] == '\''))
        s = s.substr(1, s.size() - 2);

    int w = 0, h = 0;
    if (m_fnImageSize(s, w, h)) { sz.width = w; sz.height = h; }
    else { sz.width = 0; sz.height = 0; }
}

// ============================ 列表标记 ============================

void container_win32::draw_list_marker(litehtml::uint_ptr hdc, const litehtml::list_marker& marker)
{
    if (!m_hdc) return;
    // 简化：画一个实心圆点
    Gp::Graphics g(m_hdc);
    g.SetSmoothingMode(Gp::SmoothingModeAntiAlias);
    int size = 6;
    int x = marker.pos.x - size - 4;
    int y = marker.pos.y + marker.pos.height / 2 - size / 2;
    Gp::SolidBrush brush(ToGpColor(marker.color));
    g.FillEllipse(&brush, x, y, size, size);
}

// ============================ 裁剪 ============================

void container_win32::set_clip(const litehtml::position& pos, const litehtml::border_radiuses& bdr_radius)
{
    if (!m_hdc) return;
    Gp::Graphics g(m_hdc);
    Gp::GraphicsPath path;
    BuildRoundRectPath(path, pos.x, pos.y, pos.width, pos.height, bdr_radius);
    g.SetClip(&path, Gp::CombineModeIntersect);
}

void container_win32::del_clip()
{
    if (!m_hdc) return;
    Gp::Graphics g(m_hdc);
    g.ResetClip();
}

// ============================ 客户端区域 ============================

void container_win32::get_client_rect(litehtml::position& client) const
{
    client.x = m_clipX;
    client.y = m_clipY;
    client.width = m_clipW;
    client.height = m_clipH;
}

// ============================ 元素工厂 ============================

litehtml::element::ptr container_win32::create_element(const char* tag_name,
    const litehtml::string_map& attributes,
    const std::shared_ptr<litehtml::document>& doc)
{
    // 返回 NULL 让 litehtml 用默认实现
    return nullptr;
}

// ============================ 其它 ============================

int container_win32::pt_to_px(int pt) const { return pt * 96 / 72; }
int container_win32::get_default_font_size() const { return 16; }
const char* container_win32::get_default_font_name() const { return "微软雅黑"; }

void container_win32::set_caption(const char* caption) {}
void container_win32::set_base_url(const char* base_url) { if (base_url) m_basePath = base_url; }
void container_win32::link(const std::shared_ptr<litehtml::document>& doc,
    const litehtml::element::ptr& el) {}
void container_win32::on_anchor_click(const char* url, const litehtml::element::ptr& el) {}
void container_win32::set_cursor(const char* cursor) { if (cursor) m_cursor = cursor; }

void container_win32::transform_text(litehtml::string& text, litehtml::text_transform tt)
{
    // 简化：不做大小写转换
}

void container_win32::import_css(litehtml::string& text, const litehtml::string& url,
    litehtml::string& baseurl)
{
    // 不支持 @import
    text.clear();
}

void container_win32::get_media_features(litehtml::media_features& media) const
{
    media.type = litehtml::media_type_screen;
    media.width = m_clipW;
    media.height = m_clipH;
    media.device_width = m_clipW;
    media.device_height = m_clipH;
    media.color = 8;
    media.color_index = 0;
    media.monochrome = 0;
    media.resolution = 96;
}

void container_win32::get_language(litehtml::string& language, litehtml::string& culture) const
{
    language = "zh";
    culture = "CN";
}
