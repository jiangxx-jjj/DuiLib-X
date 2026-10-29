#include "StdAfx.h"
#include "UIDuiHtmlContainer.h"
// UTF-8 ↔ TSTR 转换（litehtml 桥接专用，显式 Win32 API 实现避免编码依赖）
static char* DuiW2Utf8(LPCTSTR wstr)
{
    if (wstr == NULL) return NULL;
    int n = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
    if (n <= 0) return NULL;
    char* p = new char[n];
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, p, n, NULL, NULL);
    return p;
}
static wchar_t* DuiUtf8ToW(const char* ustr)
{
    if (ustr == NULL) return NULL;
    int n = MultiByteToWideChar(CP_UTF8, 0, ustr, -1, NULL, 0);
    if (n <= 0) return NULL;
    wchar_t* p = new wchar_t[n];
    MultiByteToWideChar(CP_UTF8, 0, ustr, -1, p, n);
    return p;
}

namespace DuiLib
{

CDuiHtmlContainer::CDuiHtmlContainer(void)
    : m_scrollY(0)
    , m_iDPI(96)
    , m_iDefaultFontSize(14)
    , m_nextFontSlot(1)
{
    m_sDefaultFontName = _T("微软雅黑");
    m_rcClient.left = m_rcClient.top = m_rcClient.right = m_rcClient.bottom = 0;
}

CDuiHtmlContainer::~CDuiHtmlContainer(void)
{
    DestroyDocument();
}

void CDuiHtmlContainer::DestroyDocument(void)
{
    m_doc.reset();
    // 清理字体缓存
    for (auto& pair : m_fontCache) {
        if (pair.second) ::DeleteObject(pair.second);
    }
    m_fontCache.clear();
    // 清理图片缓存
    for (auto& pair : m_imageCache) {
        if (pair.second.pInfo) CRenderEngine::FreeImage(pair.second.pInfo);
    }
    m_imageCache.clear();
    // 清理裁剪栈
    for (auto hRgn : m_clipStack) { if (hRgn) ::DeleteObject(hRgn); }
    m_clipStack.clear();
}

bool CDuiHtmlContainer::CreateDocument(LPCTSTR html, LPCTSTR css)
{
    DestroyDocument();

    // TSTR → UTF-8（litehtml 内部使用 UTF-8）
    int nLen = WideCharToMultiByte(CP_UTF8, 0, html, -1, NULL, 0, NULL, NULL);
    if (nLen <= 0) return false;
    char* pUtf8 = new char[nLen];
    WideCharToMultiByte(CP_UTF8, 0, html, -1, pUtf8, nLen, NULL, NULL);

    // v0.9 API：master_styles 用默认内置 CSS，user_styles 传入自定义样式
    char* pUserCss = (css != NULL && css[0] != _T('\0')) ? DuiW2Utf8(css) : NULL;
    m_doc = litehtml::document::createFromString(pUtf8, this, litehtml::master_css, pUserCss ? pUserCss : "");
    if (pUserCss) delete[] pUserCss;
    delete[] pUtf8;

    return m_doc != nullptr;
}

int CDuiHtmlContainer::Render(int maxWidth)
{
    if (!m_doc) return 0;
    m_doc->render(maxWidth);
    return m_doc->height();
}

int CDuiHtmlContainer::GetDocumentWidth(void) const
{
    return m_doc ? m_doc->width() : 0;
}

int CDuiHtmlContainer::GetDocumentHeight(void) const
{
    return m_doc ? m_doc->height() : 0;
}

void CDuiHtmlContainer::Draw(HDC hdc, int scrollY, const RECT& rcClient)
{
    if (!m_doc) return;
    m_scrollY = scrollY;
    m_rcClient = rcClient;
    litehtml::position clip(rcClient.left, rcClient.top - scrollY,
                            rcClient.right - rcClient.left, rcClient.bottom - rcClient.top);
    m_doc->draw((litehtml::uint_ptr)hdc, 0, -scrollY, &clip);
}

void CDuiHtmlContainer::OnMouseMove(POINT pt)
{
    if (m_doc) { litehtml::position::vector redraw; m_doc->on_mouse_over(pt.x, pt.y + m_scrollY, pt.x, pt.y, redraw); }
}

void CDuiHtmlContainer::OnLeftButtonDown(POINT pt)
{
    if (m_doc) { litehtml::position::vector redraw; m_doc->on_lbutton_down(pt.x, pt.y + m_scrollY, pt.x, pt.y, redraw); }
}

void CDuiHtmlContainer::OnLeftButtonUp(POINT pt)
{
    if (m_doc) { litehtml::position::vector redraw; m_doc->on_lbutton_up(pt.x, pt.y + m_scrollY, pt.x, pt.y, redraw); }
}

void CDuiHtmlContainer::SetScrollOffset(int scrollY)
{
    m_scrollY = scrollY;
    // v0.9 无 set_scroll_top API，滚动偏移由 Draw 时的 clip/y 参数控制
}

// ============================ 字体 ============================

litehtml::uint_ptr CDuiHtmlContainer::create_font(const char* faceName, int size, int weight, litehtml::font_style italic, unsigned int decoration, litehtml::font_metrics* fm)
{
    // faceName（UTF-8）→ TSTR；空则用默认字体
    wchar_t* wFace = DuiUtf8ToW(faceName);
    CDuiString sFace = wFace ? (LPCTSTR)wFace : m_sDefaultFontName;
    if (wFace) delete[] wFace;
    if (sFace.IsEmpty()) sFace = m_sDefaultFontName;

    int iSize = size > 0 ? size : m_iDefaultFontSize;

    LOGFONT lf = { 0 };
    _tcsncpy(lf.lfFaceName, sFace.GetData(), LF_FACESIZE - 1);
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfHeight = -iSize;
    lf.lfWeight = weight;
    lf.lfItalic = (italic == litehtml::font_style_italic) ? TRUE : FALSE;
    lf.lfUnderline = (decoration & litehtml::font_decoration_underline) ? TRUE : FALSE;
    lf.lfStrikeOut = (decoration & litehtml::font_decoration_linethrough) ? TRUE : FALSE;
    lf.lfQuality = CLEARTYPE_QUALITY;

    HFONT hFont = ::CreateFontIndirect(&lf);
    if (hFont == NULL) return 0;

    litehtml::uint_ptr slot = (litehtml::uint_ptr)m_nextFontSlot++;
    m_fontCache[slot] = hFont;

    // 填充字体度量（litehtml 排版需要）
    if (fm != NULL) {
        HDC hdc = ::GetDC(NULL);
        HFONT hOld = (HFONT)::SelectObject(hdc, hFont);
        TEXTMETRIC tm;
        ::GetTextMetrics(hdc, &tm);
        fm->ascent = tm.tmAscent;
        fm->descent = tm.tmDescent;
        fm->height = tm.tmHeight;
        fm->x_height = tm.tmHeight / 2;
        ::SelectObject(hdc, hOld);
        ::ReleaseDC(NULL, hdc);
    }
    return slot;
}

void CDuiHtmlContainer::delete_font(litehtml::uint_ptr hFont)
{
    auto it = m_fontCache.find(hFont);
    if (it != m_fontCache.end()) {
        if (it->second) ::DeleteObject(it->second);
        m_fontCache.erase(it);
    }
}

HFONT CDuiHtmlContainer::GetFontFromCache(int slot)
{
    auto it = m_fontCache.find((litehtml::uint_ptr)slot);
    return it != m_fontCache.end() ? it->second : NULL;
}

int CDuiHtmlContainer::text_width(const char* text, litehtml::uint_ptr hFont)
{
    HFONT hFontReal = GetFontFromCache((int)hFont);
    if (hFontReal == NULL) return 0;

    wchar_t* wText = DuiUtf8ToW(text);
    if (wText == NULL) return 0;

    HDC hdc = ::GetDC(NULL);
    HFONT hOld = (HFONT)::SelectObject(hdc, hFontReal);
    SIZE sz = { 0, 0 };
    ::GetTextExtentPoint32W(hdc, wText, (int)wcslen(wText), &sz);
    ::SelectObject(hdc, hOld);
    ::ReleaseDC(NULL, hdc);
    delete[] wText;
    return sz.cx;
}

void CDuiHtmlContainer::draw_text(litehtml::uint_ptr hdc, const char* text, litehtml::uint_ptr hFont, litehtml::web_color color, const litehtml::position& pos)
{
    HDC hDC = (HDC)hdc;
    HFONT hFontReal = GetFontFromCache((int)hFont);
    if (hFontReal == NULL || text == NULL) return;

    wchar_t* wText = DuiUtf8ToW(text);
    if (wText == NULL) return;

    HFONT hOld = (HFONT)::SelectObject(hDC, hFontReal);
    ::SetBkMode(hDC, TRANSPARENT);
    ::SetTextColor(hDC, RGB(color.red, color.green, color.blue));
    ::TextOutW(hDC, pos.x, pos.y, wText, (int)wcslen(wText));
    ::SelectObject(hDC, hOld);
    delete[] wText;
}

// ============================ 工具 ============================

COLORREF CDuiHtmlContainer::ToCOLORREF(const litehtml::web_color& c) const
{
    return RGB(c.red, c.green, c.blue);
}

void CDuiHtmlContainer::FillSolidRect(HDC hdc, const RECT& rc, COLORREF clr)
{
    COLORREF old = ::SetBkColor(hdc, clr);
    ::ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
    ::SetBkColor(hdc, old);
}

// ============================ 常规回调 ============================

int CDuiHtmlContainer::pt_to_px(int pt) const
{
    HDC hdc = ::GetDC(NULL);
    int px = ::GetDeviceCaps(hdc, LOGPIXELSY);
    ::ReleaseDC(NULL, hdc);
    return MulDiv(pt, px, 72);
}

int CDuiHtmlContainer::get_default_font_size() const
{
    return m_iDefaultFontSize;
}

const char* CDuiHtmlContainer::get_default_font_name() const
{
    // 默认字体名缓存为 UTF-8（构造时转换一次足够——这里简单转换）
    static char szName[256] = { 0 };
    if (szName[0] == '\0') {
        wchar_t* a = (wchar_t*)m_sDefaultFontName.GetData();
        char* p = DuiW2Utf8((LPCTSTR)a);
        if (p) { strncpy(szName, p, 255); delete[] p; }
    }
    return szName;
}

void CDuiHtmlContainer::draw_list_marker(litehtml::uint_ptr hdc, const litehtml::list_marker& marker)
{
    HDC hDC = (HDC)hdc;
    COLORREF clr = ToCOLORREF(marker.color);

    switch (marker.marker_type)
    {
    case litehtml::list_style_type_circle:
    case litehtml::list_style_type_disc:
    case litehtml::list_style_type_square:
    {
        RECT rc = { marker.pos.x, marker.pos.y,
                    marker.pos.x + marker.pos.width, marker.pos.y + marker.pos.height };
        HBRUSH hBrush = ::CreateSolidBrush(clr);
        HBRUSH hOld = (HBRUSH)::SelectObject(hDC, hBrush);
        HPEN hPen = ::CreatePen(PS_SOLID, 1, clr);
        HPEN hOldPen = (HPEN)::SelectObject(hDC, hPen);
        if (marker.marker_type == litehtml::list_style_type_square)
            ::Rectangle(hDC, rc.left, rc.top, rc.right, rc.bottom);
        else
            ::Ellipse(hDC, rc.left, rc.top, rc.right, rc.bottom);
        ::SelectObject(hDC, hOld);
        ::SelectObject(hDC, hOldPen);
        ::DeleteObject(hBrush);
        ::DeleteObject(hPen);
        break;
    }
    default:
    {
        // 数字/字母标记：绘制文本
        if (marker.image.empty() && marker.pos.width > 0) {
            CDuiString sText;
            if (marker.marker_type >= litehtml::list_style_type_decimal) {
                sText.Format(_T("%d."), marker.index);
            }
            if (!sText.IsEmpty()) {
                HFONT hFont = GetFontFromCache((int)marker.font);
                if (hFont) {
                    wchar_t* wText = (wchar_t*)sText.GetData();
                    HFONT hOld = (HFONT)::SelectObject(hDC, hFont);
                    ::SetBkMode(hDC, TRANSPARENT);
                    ::SetTextColor(hDC, clr);
                    ::TextOutW(hDC, marker.pos.x, marker.pos.y, wText, sText.GetLength());
                    ::SelectObject(hDC, hOld);
                }
            }
        }
        break;
    }
    }
}

// ============================ 图片 ============================

bool CDuiHtmlContainer::LoadImageFile(LPCTSTR src, TImageInfo** ppImageInfo)
{
    if (ppImageInfo) *ppImageInfo = NULL;
    if (src == NULL || src[0] == '\0') return false;

    CDuiString sKey = src;
    auto it = m_imageCache.find(sKey);
    if (it != m_imageCache.end()) {
        if (it->second.bFailed) return false;
        if (ppImageInfo) *ppImageInfo = it->second.pInfo;
        return true;
    }

    // 拼接基础路径
    CDuiString sPath = src;
    if (!m_sBasePath.IsEmpty() && sPath.Find(_T(':')) < 0 && sPath.Left(2) != _T("\\\\")) {
        sPath = m_sBasePath + sPath;
    }

    TImageInfo* pInfo = (TImageInfo*)CRenderEngine::LoadImage(sPath.GetData(), NULL, 0);
    if (pInfo == NULL) {
        // 尝试 GDI+ 加载
        pInfo = (TImageInfo*)CRenderEngine::GdiplusLoadImage(sPath.GetData(), NULL, 0);
    }

    ImageEntry entry;
    entry.pInfo = pInfo;
    entry.bFailed = (pInfo == NULL);
    m_imageCache[sKey] = entry;

    if (ppImageInfo) *ppImageInfo = pInfo;
    return pInfo != NULL;
}

void CDuiHtmlContainer::load_image(const char* src, const char* baseurl, bool redraw_on_ready)
{
    TImageInfo* pInfo = NULL;
    wchar_t* wSrc = DuiUtf8ToW(src);
    if (wSrc) {
        LoadImageFile((LPCTSTR)wSrc, &pInfo);
        delete[] wSrc;
    }
}

void CDuiHtmlContainer::get_image_size(const char* src, const char* baseurl, litehtml::size& sz)
{
    TImageInfo* pInfo = NULL;
    wchar_t* wSrc = DuiUtf8ToW(src);
    if (wSrc) {
        LoadImageFile((LPCTSTR)wSrc, &pInfo);
        delete[] wSrc;
    }
    if (pInfo) {
        sz.width = pInfo->nX;
        sz.height = pInfo->nY;
    } else {
        sz.width = 0;
        sz.height = 0;
    }
}

void CDuiHtmlContainer::draw_background(litehtml::uint_ptr hdc, const std::vector<litehtml::background_paint>& bg)
{
    if (bg.empty()) return;
    HDC hDC = (HDC)hdc;

    // 按 CSS 顺序绘制（最后一个最靠近用户）
    for (const auto& b : bg)
    {
        // 背景色
        if (b.color.alpha > 0) {
            RECT rc = { b.clip_box.x, b.clip_box.y,
                        b.clip_box.x + b.clip_box.width, b.clip_box.y + b.clip_box.height };
            HBRUSH hBrush = ::CreateSolidBrush(ToCOLORREF(b.color));
            HBRUSH hOld = (HBRUSH)::SelectObject(hDC, hBrush);
            ::FillRect(hDC, &rc, hBrush);
            ::SelectObject(hDC, hOld);
            ::DeleteObject(hBrush);
        }

        // 背景图片
        if (!b.image.empty() && b.image[0] != '\0') {
            wchar_t* wSrc = DuiUtf8ToW(b.image.c_str());
            TImageInfo* pInfo = NULL;
            if (wSrc) {
                LoadImageFile((LPCTSTR)wSrc, &pInfo);
                delete[] wSrc;
            }
            if (pInfo != NULL && pInfo->hBitmap != NULL) {
                HDC hMemDC = ::CreateCompatibleDC(hDC);
                HBITMAP hOld = (HBITMAP)::SelectObject(hMemDC, pInfo->hBitmap);

                // 简单实现：拉伸到 clip_box（repeat 平铺可作为后续增强）
                ::SetStretchBltMode(hDC, COLORONCOLOR);
                ::StretchBlt(hDC,
                    b.clip_box.x, b.clip_box.y, b.clip_box.width, b.clip_box.height,
                    hMemDC, 0, 0, pInfo->nX, pInfo->nY, SRCCOPY);

                ::SelectObject(hMemDC, hOld);
                ::DeleteDC(hMemDC);
            }
        }
    }
}

void CDuiHtmlContainer::draw_borders(litehtml::uint_ptr hdc, const litehtml::borders& borders, const litehtml::position& draw_pos, bool root)
{
    HDC hDC = (HDC)hdc;

    int left   = draw_pos.x;
    int top    = draw_pos.y;
    int right  = draw_pos.x + draw_pos.width;
    int bottom = draw_pos.y + draw_pos.height;

    struct BorderLine { int w; litehtml::web_color c; RECT rc; };
    BorderLine lines[4] = {
        { borders.left.width,   borders.left.color,   { left, top, left + borders.left.width, bottom } },
        { borders.top.width,    borders.top.color,    { left, top, right, top + borders.top.width } },
        { borders.right.width,  borders.right.color,  { right - borders.right.width, top, right, bottom } },
        { borders.bottom.width, borders.bottom.color, { left, bottom - borders.bottom.width, right, bottom } },
    };

    for (int i = 0; i < 4; i++) {
        if (lines[i].w > 0 && lines[i].c.alpha > 0) {
            HBRUSH hBrush = ::CreateSolidBrush(ToCOLORREF(lines[i].c));
            HBRUSH hOld = (HBRUSH)::SelectObject(hDC, hBrush);
            ::FillRect(hDC, &lines[i].rc, hBrush);
            ::SelectObject(hDC, hOld);
            ::DeleteObject(hBrush);
        }
    }
}

// ============================ 杂项 ============================

void CDuiHtmlContainer::set_caption(const char* caption)
{
    wchar_t* w = DuiUtf8ToW(caption);
    if (w) { m_sCaption = (LPCTSTR)w; delete[] w; }
}

void CDuiHtmlContainer::set_base_url(const char* base_url)
{
    wchar_t* w = DuiUtf8ToW(base_url);
    if (w) { m_sBaseUrl = (LPCTSTR)w; delete[] w; }
}

void CDuiHtmlContainer::link(const std::shared_ptr<litehtml::document>& doc, const litehtml::element::ptr& el)
{
    // 链接注册（可用于后续处理）
}

void CDuiHtmlContainer::on_anchor_click(const char* url, const litehtml::element::ptr& el)
{
    wchar_t* w = DuiUtf8ToW(url);
    if (w && m_fnLinkClick) {
        m_fnLinkClick((LPCTSTR)w);
    }
    if (w) delete[] w;
}

void CDuiHtmlContainer::set_cursor(const char* cursor)
{
    if (cursor && strcmp(cursor, "pointer") == 0) {
        ::SetCursor(::LoadCursor(NULL, IDC_HAND));
    } else {
        ::SetCursor(::LoadCursor(NULL, IDC_ARROW));
    }
}

void CDuiHtmlContainer::transform_text(litehtml::string& text, litehtml::text_transform tt)
{
    // uppercase/lowercase/capitalize 变换
    if (text.empty()) return;
    wchar_t* w = DuiUtf8ToW(text.c_str());
    if (!w) return;
    if (tt == litehtml::text_transform_uppercase) {
        ::CharUpperBuffW(w, (DWORD)wcslen(w));
    } else if (tt == litehtml::text_transform_lowercase) {
        ::CharLowerBuffW(w, (DWORD)wcslen(w));
    } else if (tt == litehtml::text_transform_capitalize) {
        bool bNewWord = true;
        for (size_t i = 0; w[i]; i++) {
            if (iswalpha(w[i])) {
                if (bNewWord) { w[i] = towupper(w[i]); bNewWord = false; }
            } else bNewWord = true;
        }
    }
    char* a = DuiW2Utf8((LPCTSTR)w);
    if (a) { text = a; delete[] a; }
    delete[] w;
}

void CDuiHtmlContainer::import_css(litehtml::string& text, const litehtml::string& url, litehtml::string& baseurl)
{
    // 外部 CSS 导入：从基础路径读取
    wchar_t* wUrl = DuiUtf8ToW(url.c_str());
    if (!wUrl) return;
    CDuiString sPath = (LPCTSTR)wUrl;
    delete[] wUrl;
    if (!m_sBasePath.IsEmpty() && sPath.Find(_T(':')) < 0) {
        sPath = m_sBasePath + sPath;
    }

    HANDLE hFile = ::CreateFile(sPath.GetData(), GENERIC_READ, FILE_SHARE_READ, NULL,
                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return;
    DWORD dwSize = ::GetFileSize(hFile, NULL);
    if (dwSize == 0 || dwSize == INVALID_FILE_SIZE) { ::CloseHandle(hFile); return; }

    BYTE* pBuf = new BYTE[dwSize + 1];
    DWORD dwRead = 0;
    ::ReadFile(hFile, pBuf, dwSize, &dwRead, NULL);
    ::CloseHandle(hFile);
    pBuf[dwRead] = 0;

    char* pUtf8 = (char*)pBuf;
    // BOM 检测
    if (dwRead >= 3 && pBuf[0] == 0xEF && pBuf[1] == 0xBB && pBuf[2] == 0xBF) pUtf8 += 3;
    text = pUtf8;
    delete[] pBuf;
}

void CDuiHtmlContainer::set_clip(const litehtml::position& pos, const litehtml::border_radiuses& bdr_radius)
{
    HRGN hRgn = ::CreateRectRgn(pos.x, pos.y - m_scrollY, pos.x + pos.width, pos.y + pos.height - m_scrollY);
    ::SelectClipRgn((HDC)0, NULL); // 占位——实际 DC 在 draw 时传入
    m_clipStack.push_back(hRgn);
}

void CDuiHtmlContainer::del_clip()
{
    if (!m_clipStack.empty()) {
        HRGN hRgn = m_clipStack.back();
        if (hRgn) ::DeleteObject(hRgn);
        m_clipStack.pop_back();
    }
}

void CDuiHtmlContainer::get_client_rect(litehtml::position& client) const
{
    client.x = m_rcClient.left;
    client.y = m_rcClient.top;
    client.width = m_rcClient.right - m_rcClient.left;
    client.height = m_rcClient.bottom - m_rcClient.top;
}

litehtml::element::ptr CDuiHtmlContainer::create_element(const char* tag_name, const litehtml::string_map& attributes, const std::shared_ptr<litehtml::document>& doc)
{
    return nullptr;   // 使用 litehtml 默认元素
}

void CDuiHtmlContainer::get_media_features(litehtml::media_features& media) const
{
    HDC hdc = ::GetDC(NULL);
    media.type = litehtml::media_type_screen;
    media.width = ::GetDeviceCaps(hdc, HORZRES);
    media.height = ::GetDeviceCaps(hdc, VERTRES);
    media.device_width = ::GetDeviceCaps(hdc, HORZRES);
    media.device_height = ::GetDeviceCaps(hdc, VERTRES);
    media.color = 8;
    media.monochrome = 0;
    media.color_index = ::GetDeviceCaps(hdc, NUMCOLORS);
    media.resolution = ::GetDeviceCaps(hdc, LOGPIXELSX);
    ::ReleaseDC(NULL, hdc);
}

void CDuiHtmlContainer::get_language(litehtml::string& language, litehtml::string& culture) const
{
    language = "zh";
    culture = "zh-CN";
}

} // namespace DuiLib
