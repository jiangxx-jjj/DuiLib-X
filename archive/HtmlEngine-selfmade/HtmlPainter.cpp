#include "StdAfx.h"
#include "HtmlPainter.h"

// GDI+ 头文件（必须在 windows.h 之后）
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

namespace HtmlEngine
{

// HtmlEngine::Color → Gdiplus::Color（带透明度缩放）
static inline Gdiplus::Color ToGpColor(const Color& c, float opacity = 1.0f)
{
    int a = (int)(c.a * opacity);
    if (a < 0) a = 0;
    if (a > 255) a = 255;
    return Gdiplus::Color((BYTE)a, c.r, c.g, c.b);
}

// ============================ GDI+ 生命周期 ============================

static ULONG_PTR g_gdiplusToken = 0;
static int g_gdiplusRefCount = 0;

bool HtmlPainter::InitGdiPlus()
{
    if (g_gdiplusRefCount > 0) {
        g_gdiplusRefCount++;
        return true;
    }
    GdiplusStartupInput input;
    input.GdiplusVersion = 1;
    input.DebugEventCallback = NULL;
    input.SuppressBackgroundThread = FALSE;
    input.SuppressExternalCodecs = FALSE;

    if (GdiplusStartup(&g_gdiplusToken, &input, NULL) != Status::Ok) {
        g_gdiplusToken = 0;
        return false;
    }
    g_gdiplusRefCount = 1;
    return true;
}

void HtmlPainter::ShutdownGdiPlus()
{
    if (g_gdiplusRefCount <= 0) return;
    g_gdiplusRefCount--;
    if (g_gdiplusRefCount == 0 && g_gdiplusToken != 0) {
        GdiplusShutdown(g_gdiplusToken);
        g_gdiplusToken = 0;
    }
}

// ============================ 字体测量 ============================

// 字体缓存：key = family|size|bold|italic
struct FontCacheEntry
{
    Gdiplus::Font*       pFont;
    float       height;
};

static std::map<std::string, FontCacheEntry> g_fontCache;

static Gdiplus::Font* GetFont(const std::string& fontFamily, float fontSize, bool bBold, bool bItalic)
{
    std::string key = fontFamily + "|" + FloatToString(fontSize) +
                      (bBold ? "|B" : "|N") + (bItalic ? "|I" : "|R");

    std::map<std::string, FontCacheEntry>::iterator it = g_fontCache.find(key);
    if (it != g_fontCache.end()) return it->second.pFont;

    std::wstring wFamily = Utf8ToWide(fontFamily);
    if (wFamily.empty()) wFamily = L"微软雅黑";

    Gdiplus::FontStyle style = Gdiplus::FontStyleRegular;
    if (bBold && bItalic) style = Gdiplus::FontStyleBoldItalic;
    else if (bBold)       style = Gdiplus::FontStyleBold;
    else if (bItalic)     style = Gdiplus::FontStyleItalic;

    Gdiplus::Font* pFont = new Gdiplus::Font(wFamily.c_str(), fontSize, style, Gdiplus::UnitPixel);
    if (pFont == NULL || pFont->GetLastStatus() != Gdiplus::Status::Ok) {
        delete pFont;
        pFont = new Gdiplus::Font(L"微软雅黑", fontSize, style, Gdiplus::UnitPixel);
    }

    FontCacheEntry entry;
    entry.pFont = pFont;
    entry.height = fontSize * 1.4f;
    g_fontCache[key] = entry;
    return pFont;
}

float HtmlPainter::MeasureText(const std::string& text, const std::string& fontFamily,
                               float fontSize, bool bBold, bool bItalic)
{
    if (text.empty()) return 0.0f;

    InitGdiPlus();

    Gdiplus::Font* pFont = GetFont(fontFamily, fontSize, bBold, bItalic);
    if (pFont == NULL) return (float)text.size() * fontSize * 0.55f;

    std::wstring wText = Utf8ToWide(text);
    if (wText.empty()) return 0.0f;

    // 用 GDI+ 测量
    Bitmap bmp(1, 1);
    Graphics g(&bmp);
    g.SetTextRenderingHint(TextRenderingHintAntiAlias);

    RectF layoutRect(0, 0, 100000.0f, 100000.0f);
    RectF boundRect;
    StringFormat fmt;
    fmt.SetFormatFlags(StringFormatFlagsMeasureTrailingSpaces | StringFormatFlagsNoWrap);
    fmt.SetTrimming(StringTrimmingNone);

    g.MeasureString(wText.c_str(), (INT)wText.size(), pFont, layoutRect, &fmt, &boundRect);
    return boundRect.Width;
}

float HtmlPainter::MeasureLineHeight(const std::string& fontFamily, float fontSize,
                                     bool bBold, bool bItalic)
{
    InitGdiPlus();

    Gdiplus::Font* pFont = GetFont(fontFamily, fontSize, bBold, bItalic);
    if (pFont == NULL) return fontSize * 1.4f;

    Gdiplus::FontFamily family;
    pFont->GetFamily(&family);
    float emHeight = (float)family.GetEmHeight(pFont->GetStyle());
    float lineSpacing = (float)family.GetLineSpacing(pFont->GetStyle());
    if (emHeight <= 0.0f) return fontSize * 1.4f;

    return fontSize * lineSpacing / emHeight;
}

// ============================ 构造 ============================

HtmlPainter::HtmlPainter()
    : m_hoverNode(NULL)
    , m_focusNode(NULL)
    , m_hdc(NULL)
    , m_scrollX(0.0f)
    , m_scrollY(0.0f)
{
    InitGdiPlus();
}

HtmlPainter::~HtmlPainter()
{
    ShutdownGdiPlus();
}

void HtmlPainter::SetFormState(Node* node, const FormState& state)
{
    m_formStates[node] = state;
}

HtmlPainter::FormState* HtmlPainter::GetFormState(Node* node)
{
    std::map<Node*, FormState>::iterator it = m_formStates.find(node);
    return (it == m_formStates.end()) ? NULL : &it->second;
}

// ============================ 圆角路径 ============================

void HtmlPainter::BuildRoundRectPath(GraphicsPath& path, float x, float y,
                                     float w, float h, const BorderRadius& br)
{
    float tl = br.tl, tr = br.tr, brr = br.br, bl = br.bl;
    float tlv = br.tlv > 0 ? br.tlv : tl;
    float trv = br.trv > 0 ? br.trv : tr;
    float brv = br.brv > 0 ? br.brv : brr;
    float blv = br.blv > 0 ? br.blv : bl;

    // 半径不能超过一半
    if (tl > w / 2) tl = w / 2;    if (tlv > h / 2) tlv = h / 2;
    if (tr > w / 2) tr = w / 2;    if (trv > h / 2) trv = h / 2;
    if (brr > w / 2) brr = w / 2;  if (brv > h / 2) brv = h / 2;
    if (bl > w / 2) bl = w / 2;    if (blv > h / 2) blv = h / 2;

    path.Reset();

    if (tl <= 0 && tr <= 0 && brr <= 0 && bl <= 0) {
        path.AddRectangle(RectF(x, y, w, h));
        return;
    }

    // 用 4 段贝塞尔近似圆角
    const float k = 0.5522847498f;   // 四分之一圆的贝塞尔控制点系数

    // 左上
    if (tl > 0 || tlv > 0) {
        path.AddBezier(PointF(x + tl, y),
                       PointF(x + tl - tl * k, y),
                       PointF(x, y + tlv - tlv * k),
                       PointF(x, y + tlv));
    } else {
        path.AddLine(PointF(x, y), PointF(x, y));
    }

    // 上边
    path.AddLine(PointF(x + tl, y), PointF(x + w - tr, y));

    // 右上
    if (tr > 0 || trv > 0) {
        path.AddBezier(PointF(x + w - tr, y),
                       PointF(x + w - tr + tr * k, y),
                       PointF(x + w, y + trv - trv * k),
                       PointF(x + w, y + trv));
    }

    // 右边
    path.AddLine(PointF(x + w, y + trv), PointF(x + w, y + h - brv));

    // 右下
    if (brr > 0 || brv > 0) {
        path.AddBezier(PointF(x + w, y + h - brv),
                       PointF(x + w, y + h - brv + brv * k),
                       PointF(x + w - brr + brr * k, y + h),
                       PointF(x + w - brr, y + h));
    }

    // 下边
    path.AddLine(PointF(x + w - brr, y + h), PointF(x + bl, y + h));

    // 左下
    if (bl > 0 || blv > 0) {
        path.AddBezier(PointF(x + bl, y + h),
                       PointF(x + bl - bl * k, y + h),
                       PointF(x, y + h - blv + blv * k),
                       PointF(x, y + h - blv));
    }

    // 左边
    path.AddLine(PointF(x, y + h - blv), PointF(x, y + tlv));

    path.CloseFigure();
}

// ============================ 阴影绘制 ============================

void HtmlPainter::PaintShadows(Node* node, float x, float y, float w, float h, float opacity)
{
    if (node == NULL || node->style.boxShadows.empty()) return;

    for (size_t i = 0; i < node->style.boxShadows.size(); i++) {
        const Shadow& sh = node->style.boxShadows[i];

        float sx = x + sh.x - sh.spread;
        float sy = y + sh.y - sh.spread;
        float sw = w + sh.spread * 2;
        float sh2 = h + sh.spread * 2;
        if (sw <= 0 || sh2 <= 0) continue;

        int a = (int)(sh.color.a * opacity);
        if (a <= 0) continue;

        // 简化：用多层半透明圆角矩形模拟模糊
        int layers = (int)(sh.blur / 2.0f);
        if (layers < 1) layers = 1;
        if (layers > 10) layers = 10;

        for (int l = layers; l >= 1; l--) {
            float inflate = (float)(sh.blur * l / layers);
            float rx = sx - inflate;
            float ry = sy - inflate;
            float rw = sw + inflate * 2;
            float rh = sh2 + inflate * 2;

            int la = a / layers;
            if (la <= 0) la = 1;

            GraphicsPath path;
            BorderRadius br = node->style.borderRadius;
            br.tl += inflate; br.tr += inflate; br.br += inflate; br.bl += inflate;
            br.tlv += inflate; br.trv += inflate; br.brv += inflate; br.blv += inflate;
            BuildRoundRectPath(path, rx, ry, rw, rh, br);

            Graphics g(m_hdc);
            g.SetSmoothingMode(SmoothingModeAntiAlias);
            SolidBrush brush(ToGpColor(sh.color, (float)la / 255.0f));
            g.FillPath(&brush, &path);
        }
    }
}

// ============================ 背景绘制 ============================

void HtmlPainter::PaintBackground(Node* node, float x, float y, float w, float h, float opacity)
{
    if (node == NULL) return;
    if (w <= 0 || h <= 0) return;

    const ComputedStyle& st = node->style;

    Graphics g(m_hdc);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetPixelOffsetMode(PixelOffsetModeHalf);

    GraphicsPath path;
    BuildRoundRectPath(path, x, y, w, h, st.borderRadius);

    // ---- 渐变背景 ----
    if (st.backgroundImage.type == BackgroundImage::GRADIENT &&
        !st.backgroundImage.gradient.IsEmpty()) {
        const Gradient& grad = st.backgroundImage.gradient;
        int stopCount = (int)grad.stops.size();
        if (stopCount >= 2) {
            Gdiplus::Color* colors = new Gdiplus::Color[stopCount];
            REAL* positions = new REAL[stopCount];
            for (int i = 0; i < stopCount; i++) {
                const GradientStop& s = grad.stops[i];
                colors[i] = ToGpColor(s.color, opacity);
                positions[i] = (REAL)s.pos;
            }

            if (grad.radial) {
                // 径向渐变
                float cx = x + w / 2;
                float cy = y + h / 2;
                float maxR = (w > h ? w : h) / 2;
                GraphicsPath circlePath;
                circlePath.AddEllipse(cx - maxR, cy - maxR, maxR * 2, maxR * 2);
                PathGradientBrush pgb(&circlePath);
                pgb.SetCenterPoint(PointF(cx, cy));
                pgb.SetCenterColor(colors[0]);

                int surroundCount = stopCount - 1;
                if (surroundCount > 0) {
                    Gdiplus::Color* surround = new Gdiplus::Color[surroundCount];
                    for (int i = 0; i < surroundCount; i++) surround[i] = colors[i + 1];
                    pgb.SetSurroundColors(surround, &surroundCount);
                    delete[] surround;
                }

                // 用路径裁剪后填充
                g.SetClip(&path, CombineModeReplace);
                g.FillEllipse(&pgb, cx - maxR, cy - maxR, maxR * 2, maxR * 2);
                g.ResetClip();
            } else {
                // 线性渐变：按角度计算起点/终点
                float rad = grad.angle * 3.14159265f / 180.0f;
                float dx = cosf(rad);
                float dy = sinf(rad);

                float cx = x + w / 2;
                float cy = y + h / 2;
                float halfLen = (fabsf(dx) * w + fabsf(dy) * h) / 2;

                PointF p1(cx - dx * halfLen, cy - dy * halfLen);
                PointF p2(cx + dx * halfLen, cy + dy * halfLen);

                LinearGradientBrush lgb(p1, p2, colors[0], colors[stopCount - 1]);
                if (stopCount > 2) {
                    lgb.SetInterpolationColors(colors, positions, stopCount);
                }

                g.FillPath(&lgb, &path);
            }

            delete[] colors;
            delete[] positions;
            return;
        }
    }

    // ---- 背景图片 ----
    if (st.backgroundImage.type == BackgroundImage::URL && !st.backgroundImage.url.empty()) {
        // 交给外部（DuiLib 图片加载），这里用占位绘制
        // 实际图片由 CHtmlBoxUI 注入回调处理
    }

    // ---- 纯色背景 ----
    if (st.backgroundColor.IsVisible()) {
        int a = (int)(st.backgroundColor.a * opacity);
        if (a > 0) {
            SolidBrush brush(ToGpColor(st.backgroundColor, opacity));
            g.FillPath(&brush, &path);
        }
    }
}

// ============================ 边框绘制 ============================

void HtmlPainter::PaintBorders(Node* node, float x, float y, float w, float h, float opacity)
{
    if (node == NULL) return;
    if (w <= 0 || h <= 0) return;

    const ComputedStyle& st = node->style;
    if (!st.borderTop.IsVisible() && !st.borderRight.IsVisible() &&
        !st.borderBottom.IsVisible() && !st.borderLeft.IsVisible()) {
        return;
    }

    Graphics g(m_hdc);
    g.SetSmoothingMode(SmoothingModeAntiAlias);

    // 简化：四边同色同宽时用一条描边路径
    float bt = st.borderTop.IsVisible() ? st.borderTop.width : 0;
    float br = st.borderRight.IsVisible() ? st.borderRight.width : 0;
    float bb = st.borderBottom.IsVisible() ? st.borderBottom.width : 0;
    float bl = st.borderLeft.IsVisible() ? st.borderLeft.width : 0;

    bool bSame = st.borderTop.IsVisible() &&
                 st.borderTop.color == st.borderRight.color &&
                 st.borderTop.color == st.borderBottom.color &&
                 st.borderTop.color == st.borderLeft.color &&
                 st.borderTop.width == st.borderRight.width &&
                 st.borderTop.width == st.borderBottom.width &&
                 st.borderTop.width == st.borderLeft.width;

    if (bSame) {
        GraphicsPath path;
        BuildRoundRectPath(path, x + bt / 2, y + bt / 2, w - bt, h - bt, st.borderRadius);

        int a = (int)(st.borderTop.color.a * opacity);
        if (a > 0) {
            Pen pen(Gdiplus::Color((BYTE)a, st.borderTop.color.r, st.borderTop.color.g, st.borderTop.color.b),
                    st.borderTop.width);
            switch (st.borderTop.style) {
            case BORDER_DASHED: pen.SetDashStyle(DashStyleDash); break;
            case BORDER_DOTTED: pen.SetDashStyle(DashStyleDot); break;
            default: pen.SetDashStyle(DashStyleSolid); break;
            }
            pen.SetLineJoin(LineJoinRound);
            g.DrawPath(&pen, &path);
        }
        return;
    }

    // 四边不同：分别绘制
    if (st.borderTop.IsVisible() && bt > 0) {
        int a = (int)(st.borderTop.color.a * opacity);
        if (a > 0) {
            SolidBrush brush(Gdiplus::Color((BYTE)a, st.borderTop.color.r, st.borderTop.color.g, st.borderTop.color.b));
            g.FillRectangle(&brush, x, y, w, bt);
        }
    }
    if (st.borderBottom.IsVisible() && bb > 0) {
        int a = (int)(st.borderBottom.color.a * opacity);
        if (a > 0) {
            SolidBrush brush(Gdiplus::Color((BYTE)a, st.borderBottom.color.r, st.borderBottom.color.g, st.borderBottom.color.b));
            g.FillRectangle(&brush, x, y + h - bb, w, bb);
        }
    }
    if (st.borderLeft.IsVisible() && bl > 0) {
        int a = (int)(st.borderLeft.color.a * opacity);
        if (a > 0) {
            SolidBrush brush(Gdiplus::Color((BYTE)a, st.borderLeft.color.r, st.borderLeft.color.g, st.borderLeft.color.b));
            g.FillRectangle(&brush, x, y, bl, h);
        }
    }
    if (st.borderRight.IsVisible() && br > 0) {
        int a = (int)(st.borderRight.color.a * opacity);
        if (a > 0) {
            SolidBrush brush(Gdiplus::Color((BYTE)a, st.borderRight.color.r, st.borderRight.color.g, st.borderRight.color.b));
            g.FillRectangle(&brush, x + w - br, y, br, h);
        }
    }
}

// ============================ 文本绘制 ============================

void HtmlPainter::PaintTextNode(Node* node, float x, float y, float opacity)
{
    if (node == NULL || !node->IsText()) return;

    std::string text = node->displayText;
    if (text.empty()) text = node->text;
    if (text.empty()) return;

    const ComputedStyle& st = node->style;
    if (!st.color.IsVisible()) return;

    Graphics g(m_hdc);
    g.SetTextRenderingHint(TextRenderingHintAntiAlias);

    Gdiplus::Font* pFont = GetFont(st.fontFamily, st.fontSize,
                          st.fontWeight >= FONT_WEIGHT_BOLD,
                          st.fontStyle == FONT_ITALIC);
    if (pFont == NULL) return;

    std::wstring wText = Utf8ToWide(text);
    if (wText.empty()) return;

    // 文字阴影
    for (size_t i = 0; i < st.textShadows.size(); i++) {
        const Shadow& sh = st.textShadows[i];
        int a = (int)(sh.color.a * opacity);
        if (a <= 0) continue;
        SolidBrush shadowBrush(Gdiplus::Color((BYTE)a, sh.color.r, sh.color.g, sh.color.b));
        g.DrawString(wText.c_str(), (INT)wText.size(), pFont,
                     PointF(x + sh.x, y + sh.y), &shadowBrush);
    }

    int a = (int)(st.color.a * opacity);
    if (a <= 0) return;

    SolidBrush brush(Gdiplus::Color((BYTE)a, st.color.r, st.color.g, st.color.b));
    g.DrawString(wText.c_str(), (INT)wText.size(), pFont, PointF(x, y), &brush);

    // 下划线 / 删除线
    if (st.underline || st.lineThrough) {
        float textW = node->width;
        float lh = st.fontSize;
        Pen pen(Gdiplus::Color((BYTE)a, st.color.r, st.color.g, st.color.b), 1.0f);

        if (st.underline) {
            g.DrawLine(&pen, x, y + lh + 1.0f, x + textW, y + lh + 1.0f);
        }
        if (st.lineThrough) {
            g.DrawLine(&pen, x, y + lh * 0.55f, x + textW, y + lh * 0.55f);
        }
    }
}

// ============================ 列表标记 ============================

void HtmlPainter::PaintListMarker(Node* node, float x, float y, float h, float opacity)
{
    if (node == NULL) return;
    ListStyleType lst = node->style.listStyleType;
    if (lst == LIST_NONE) return;

    Graphics g(m_hdc);
    g.SetSmoothingMode(SmoothingModeAntiAlias);

    float size = node->style.fontSize * 0.42f;
    if (size < 3.0f) size = 3.0f;
    if (size > 8.0f) size = 8.0f;

    float cx = x - size - 8.0f;
    float cy = y + h / 2.0f;

    int a = (int)(node->style.color.a * opacity);
    if (a <= 0) a = 255;
    SolidBrush brush(Gdiplus::Color((BYTE)a, node->style.color.r, node->style.color.g, node->style.color.b));

    if (lst == LIST_DISC || lst == LIST_CIRCLE) {
        g.FillEllipse(&brush, cx - size / 2, cy - size / 2, size, size);
    } else if (lst == LIST_SQUARE) {
        g.FillRectangle(&brush, cx - size / 2, cy - size / 2, size, size);
    } else if (lst == LIST_DECIMAL) {
        Gdiplus::Font* pFont = GetFont(node->style.fontFamily, node->style.fontSize, false, false);
        std::wstring num = Utf8ToWide(std::to_string(node->listIndex) + ".");
        RectF rc(cx - 24.0f, y, 24.0f, h);
        StringFormat fmt;
        fmt.SetAlignment(StringAlignmentFar);
        fmt.SetLineAlignment(StringAlignmentCenter);
        g.DrawString(num.c_str(), (INT)num.size(), pFont, rc, &fmt, &brush);
    }
}

// ============================ 图片绘制 ============================

void HtmlPainter::PaintImage(Node* node, float x, float y, float w, float h, float opacity)
{
    // 图片由外部注入（DuiLib 的图片加载回调）
    // 这里画一个占位框（有边框的浅灰矩形）
    if (node == NULL || w <= 0 || h <= 0) return;

    Graphics g(m_hdc);
    SolidBrush brush(Gdiplus::Color(200, 230, 230, 230));
    g.FillRectangle(&brush, x, y, w, h);
    Pen pen(Gdiplus::Color(255, 180, 180, 180), 1.0f);
    g.DrawRectangle(&pen, x, y, w, h);
}

// ============================ 表单控件 ============================

void HtmlPainter::PaintFormControl(Node* node, float x, float y, float w, float h, float opacity)
{
    if (node == NULL || w <= 0 || h <= 0) return;

    const ComputedStyle& st = node->style;
    std::string tag = node->tagName;
    std::string type = ToLower(node->GetAttr("type", "text"));

    Graphics g(m_hdc);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetTextRenderingHint(TextRenderingHintAntiAlias);

    FormState* pState = GetFormState(node);

    if (tag == "input" && (type == "checkbox" || type == "radio")) {
        bool bChecked = (pState != NULL) ? pState->checked : node->HasAttr("checked");

        float size = (w < h) ? w : h;
        float cx = x + (w - size) / 2;
        float cy = y + (h - size) / 2;

        if (type == "radio") {
            // 圆
            SolidBrush bg(Gdiplus::Color(255, 255, 255, 255));
            g.FillEllipse(&bg, cx, cy, size, size);
            Pen border(Gdiplus::Color(255, 160, 160, 160), 1.0f);
            g.DrawEllipse(&border, cx, cy, size, size);
            if (bChecked) {
                SolidBrush dot(Gdiplus::Color(255, 45, 91, 215));
                float d = size * 0.6f;
                g.FillEllipse(&dot, cx + (size - d) / 2, cy + (size - d) / 2, d, d);
            }
        } else {
            // 方
            SolidBrush bg(Gdiplus::Color(255, 255, 255, 255));
            g.FillRectangle(&bg, cx, cy, size, size);
            Pen border(Gdiplus::Color(255, 160, 160, 160), 1.0f);
            g.DrawRectangle(&border, cx, cy, size, size);
            if (bChecked) {
                Pen tick(Gdiplus::Color(255, 45, 91, 215), 2.0f);
                g.DrawLine(&tick, cx + size * 0.2f, cy + size * 0.5f,
                           cx + size * 0.42f, cy + size * 0.72f);
                g.DrawLine(&tick, cx + size * 0.42f, cy + size * 0.72f,
                           cx + size * 0.8f, cy + size * 0.28f);
            }
        }
        return;
    }

    if (tag == "select") {
        // 当前选中项文字
        std::string selText;
        if (pState != NULL) {
            // 从 option 子节点取
            int idx = 0;
            for (size_t i = 0; i < node->children.size(); i++) {
                Node* opt = node->children[i];
                if (!opt->IsElement() || opt->tagName != "option") continue;
                // 取文本
                std::string t;
                for (size_t k = 0; k < opt->children.size(); k++) {
                    if (opt->children[k]->IsText()) t += opt->children[k]->text;
                }
                if (idx == pState->selectedIndex) { selText = Trim(t); break; }
                idx++;
            }
        }

        Gdiplus::Font* pFont = GetFont(st.fontFamily, st.fontSize,
                              st.fontWeight >= FONT_WEIGHT_BOLD, false);
        if (pFont != NULL && !selText.empty()) {
            SolidBrush txt(Gdiplus::Color(255, st.color.r, st.color.g, st.color.b));
            std::wstring wText = Utf8ToWide(selText);
            RectF rc(x + 6, y, w - 26, h);
            StringFormat fmt;
            fmt.SetLineAlignment(StringAlignmentCenter);
            g.DrawString(wText.c_str(), (INT)wText.size(), pFont, rc, &fmt, &txt);
        }

        // 下拉箭头
        SolidBrush arrow(Gdiplus::Color(255, 110, 120, 140));
        float ax = x + w - 18;
        float ay = y + h / 2 - 2;
        PointF pts[3] = {
            PointF(ax, ay), PointF(ax + 9, ay), PointF(ax + 4.5f, ay + 6)
        };
        g.FillPolygon(&arrow, pts, 3);
        return;
    }

    if (tag == "textarea") {
        // 多行文本
        Gdiplus::Font* pFont = GetFont(st.fontFamily, st.fontSize,
                              st.fontWeight >= FONT_WEIGHT_BOLD, false);
        if (pFont == NULL) return;

        std::string text;
        if (pState != NULL) text = pState->value;
        else {
            for (size_t i = 0; i < node->children.size(); i++) {
                if (node->children[i]->IsText()) text += node->children[i]->text;
            }
            text = Trim(text);
        }

        if (!text.empty()) {
            SolidBrush txt(Gdiplus::Color(255, st.color.r, st.color.g, st.color.b));
            std::wstring wText = Utf8ToWide(text);
            RectF rc(x + 5, y + 3, w - 10, h - 6);
            g.DrawString(wText.c_str(), (INT)wText.size(), pFont, rc, NULL, &txt);
        }
        return;
    }

    // ---- 文本框 / 密码框 / 按钮 ----
    std::string displayText;
    bool bPassword = (type == "password");
    bool bButton = (tag == "button" || type == "submit" || type == "button" || type == "reset");

    if (bButton) {
        // 按钮文字
        std::string label = node->GetAttr("value");
        if (label.empty()) {
            for (size_t i = 0; i < node->children.size(); i++) {
                if (node->children[i]->IsText()) label += node->children[i]->text;
            }
            label = Trim(label);
        }
        if (label.empty()) {
            if (type == "submit") label = "Submit";
            else if (type == "reset") label = "Reset";
        }
        displayText = label;
    } else {
        if (pState != NULL) {
            displayText = pState->value;
        } else {
            displayText = node->GetAttr("value");
        }
        if (bPassword && !displayText.empty()) {
            displayText.assign(displayText.size(), '*');
        }
    }

    Gdiplus::Font* pFont = GetFont(st.fontFamily, st.fontSize,
                          st.fontWeight >= FONT_WEIGHT_BOLD, false);
    if (pFont == NULL) return;

    SolidBrush txt(Gdiplus::Color(255, st.color.r, st.color.g, st.color.b));
    std::wstring wText = Utf8ToWide(displayText);

    if (bButton) {
        RectF rc(x, y, w, h);
        StringFormat fmt;
        fmt.SetAlignment(StringAlignmentCenter);
        fmt.SetLineAlignment(StringAlignmentCenter);
        g.DrawString(wText.c_str(), (INT)wText.size(), pFont, rc, &fmt, &txt);
    } else {
        RectF rc(x + 5, y, w - 8, h);
        StringFormat fmt;
        fmt.SetLineAlignment(StringAlignmentCenter);
        fmt.SetTrimming(StringTrimmingEllipsisCharacter);
        g.DrawString(wText.c_str(), (INT)wText.size(), pFont, rc, &fmt, &txt);

        // 焦点光标：按字符索引定位，支持中文等宽字符
        if (pState != NULL && pState->focused) {
            size_t caretChar = (pState->caretPos <= (size_t)INT_MAX) ? pState->caretPos : 0;
            // 把字符索引转成字节索引
            size_t caretByte = 0;
            size_t c = 0;
            while (caretByte < displayText.size() && c < caretChar) {
                unsigned char uc = (unsigned char)displayText[caretByte];
                if ((uc & 0x80) == 0) caretByte += 1;
                else if ((uc & 0xE0) == 0xC0) caretByte += 2;
                else if ((uc & 0xF0) == 0xE0) caretByte += 3;
                else caretByte += 4;
                c++;
            }
            std::string beforeCaret = displayText.substr(0, caretByte);
            float caretX = x + 5 + MeasureText(beforeCaret, st.fontFamily,
                                               st.fontSize, st.fontWeight >= FONT_WEIGHT_BOLD, false);
            Pen caret(Gdiplus::Color(255, 0, 0, 0), 1.0f);
            g.DrawLine(&caret, caretX, y + 3, caretX, y + h - 3);
        }
    }
}

// ============================ 节点绘制 ============================

void HtmlPainter::PaintNode(Node* node, float baseX, float baseY,
                            float clipX, float clipY, float clipW, float clipH,
                            float opacity)
{
    if (node == NULL) return;
    if (node->style.display == DISPLAY_NONE) return;
    if (node->style.visibility != VIS_VISIBLE) return;

    float absX = baseX + node->x;
    float absY = baseY + node->y;

    // 累计透明度
    float nodeOpacity = opacity * node->style.opacity;

    // ---- 文本节点 ----
    if (node->IsText()) {
        float drawY = absY;
        // 垂直对齐（在行内流里由布局决定基线，这里简单处理）
        PaintTextNode(node, absX, drawY, nodeOpacity);
        return;
    }

    // ---- 元素 ----
    float w = node->width;
    float h = node->height;

    // 裁剪判断（视口外不画）
    if (clipW > 0 && clipH > 0) {
        if (absX > clipX + clipW || absY > clipY + clipH ||
            absX + w < clipX || absY + h < clipY) {
            // 允许部分越界（阴影/圆角溢出）
        }
    }

    // 1. 阴影（在背景之前）
    PaintShadows(node, absX, absY, w, h, nodeOpacity);

    // 2. 背景
    PaintBackground(node, absX, absY, w, h, nodeOpacity);

    // 3. 边框
    PaintBorders(node, absX, absY, w, h, nodeOpacity);

    // 4. 图片
    if (node->tagName == "img") {
        PaintImage(node, absX, absY, w, h, nodeOpacity);
        return;
    }

    // 5. 表单控件
    if (node->IsFormControl()) {
        PaintFormControl(node, absX, absY, w, h, nodeOpacity);
        return;
    }

    // 6. 列表标记
    if (node->style.display == DISPLAY_LIST_ITEM) {
        PaintListMarker(node, absX, absY, h, nodeOpacity);
    }

    // 6.5 行内块/行内元素里的文本子节点：直接绘制（不递归到 padding 计算）
    if ((node->style.display == DISPLAY_INLINE_BLOCK ||
         node->style.display == DISPLAY_INLINE ||
         node->style.display == DISPLAY_INLINE_FLEX) &&
        node->children.size() > 0) {
        // 行内元素内部可能还有文本，走正常子节点绘制流程
    }

    // 7. 子节点
    // 内容区偏移（含 margin + border + padding，与布局时的 baseX/baseY 保持一致）
    const float rootFontSize = 16.0f;
    float mt = node->style.marginTop.IsAuto() ? 0.0f
              : node->style.marginTop.Resolve(w, node->style.fontSize, rootFontSize);
    float ml = node->style.marginLeft.IsAuto() ? 0.0f
              : node->style.marginLeft.Resolve(w, node->style.fontSize, rootFontSize);
    float pt = node->style.paddingTop.Resolve(w, node->style.fontSize, rootFontSize);
    float pl = node->style.paddingLeft.Resolve(w, node->style.fontSize, rootFontSize);
    float bt = node->style.borderTop.IsVisible() ? node->style.borderTop.width : 0;
    float bl = node->style.borderLeft.IsVisible() ? node->style.borderLeft.width : 0;

    float contentBaseX = absX + ml + bl + pl;
    float contentBaseY = absY + mt + bt + pt;

    // overflow: hidden 时裁剪子内容
    bool bClipChildren = (node->style.overflowX == OVERFLOW_HIDDEN ||
                          node->style.overflowY == OVERFLOW_HIDDEN ||
                          node->style.overflowX == OVERFLOW_AUTO ||
                          node->style.overflowY == OVERFLOW_AUTO);

    if (bClipChildren && w > 0 && h > 0) {
        Graphics g(m_hdc);
        GraphicsPath clipPath;
        BuildRoundRectPath(clipPath, absX, absY, w, h, node->style.borderRadius);
        g.SetClip(&clipPath, CombineModeIntersect);

        for (size_t i = 0; i < node->children.size(); i++) {
            PaintNode(node->children[i], contentBaseX, contentBaseY,
                      clipX, clipY, clipW, clipH, nodeOpacity);
        }

        g.ResetClip();
    } else {
        for (size_t i = 0; i < node->children.size(); i++) {
            PaintNode(node->children[i], contentBaseX, contentBaseY,
                      clipX, clipY, clipW, clipH, nodeOpacity);
        }
    }
}

// ============================ 入口 ============================

void HtmlPainter::Paint(HDC hdc, DocumentTree& tree, float scrollX, float scrollY,
                        float clipW, float clipH)
{
    if (hdc == NULL || tree.bodyNode == NULL) return;

    m_hdc = hdc;
    m_scrollX = scrollX;
    m_scrollY = scrollY;

    Graphics g(hdc);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetTextRenderingHint(TextRenderingHintAntiAlias);
    g.SetPixelOffsetMode(PixelOffsetModeHalf);
    g.SetInterpolationMode(InterpolationModeHighQualityBicubic);

    // 视口裁剪
    g.SetClip(RectF(0, 0, clipW, clipH), CombineModeIntersect);

    // 用世界变换做滚动
    Matrix m;
    m.Translate(-scrollX, -scrollY);
    g.SetTransform(&m);

    // 从 body 开始画
    Node* body = tree.bodyNode;
    PaintNode(body, 0, 0, scrollX, scrollY, clipW, clipH, 1.0f);

    g.ResetTransform();
    g.ResetClip();
}

} // namespace HtmlEngine
