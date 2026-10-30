#ifndef HTML_ENGINE_PAINTER_H
#define HTML_ENGINE_PAINTER_H

// ==========================================================================
//  HtmlPainter.h — GDI+ 绘制层
//
//  用 GDI+ 绘制（原生支持抗锯齿、圆角、渐变、alpha），
//  把布局结果画到目标 HDC。
// ==========================================================================

#include "HtmlDom.h"
#include "HtmlLayout.h"
#include <string>
#include <vector>

namespace HtmlEngine
{

class HtmlPainter
{
public:
    HtmlPainter();
    ~HtmlPainter();

    // 初始化 GDI+（进程内只需一次，内部有引用计数）
    static bool InitGdiPlus();
    static void ShutdownGdiPlus();

    // 字体测量回调（供布局引擎使用）
    static float MeasureText(const std::string& text, const std::string& fontFamily,
                             float fontSize, bool bBold, bool bItalic);
    static float MeasureLineHeight(const std::string& fontFamily, float fontSize,
                                   bool bBold, bool bItalic);

    // 绘制整棵文档树
    // hdc: 目标 DC；scrollY: 垂直滚动偏移；clipW/clipH: 可视区尺寸
    void Paint(HDC hdc, DocumentTree& tree, float scrollX, float scrollY,
               float clipW, float clipH);

    // 交互状态
    void SetHoverNode(Node* node) { m_hoverNode = node; }
    Node* GetHoverNode() const { return m_hoverNode; }

    // 表单控件状态（供绘制）
    struct FormState {
        std::string value;      // 当前文本值
        bool        checked;
        int         selectedIndex;
        bool        focused;
    };
    // 设置某个表单控件的状态
    void SetFormState(Node* node, const FormState& state);
    FormState* GetFormState(Node* node);

    // 当前焦点控件
    void SetFocusNode(Node* node) { m_focusNode = node; }
    Node* GetFocusNode() const { return m_focusNode; }

private:
    Node* m_hoverNode;
    Node* m_focusNode;
    std::map<Node*, FormState> m_formStates;

    // 绘制节点
    void PaintNode(Node* node, float baseX, float baseY, float clipX, float clipY,
                   float clipW, float clipH, float opacity);

    // 绘制背景（颜色 + 渐变 + 图片）
    void PaintBackground(Node* node, float x, float y, float w, float h, float opacity);
    // 绘制边框
    void PaintBorders(Node* node, float x, float y, float w, float h, float opacity);
    // 绘制阴影
    void PaintShadows(Node* node, float x, float y, float w, float h, float opacity);
    // 绘制文本节点
    void PaintTextNode(Node* node, float x, float y, float opacity);
    // 绘制表单控件
    void PaintFormControl(Node* node, float x, float y, float w, float h, float opacity);
    // 绘制列表标记
    void PaintListMarker(Node* node, float x, float y, float h, float opacity);
    // 绘制图片
    void PaintImage(Node* node, float x, float y, float w, float h, float opacity);

    // 当前绘制目标
    HDC m_hdc;
    float m_scrollX;
    float m_scrollY;

    // 圆角路径构建
    static void BuildRoundRectPath(Gdiplus::GraphicsPath& path, float x, float y,
                                   float w, float h, const BorderRadius& br);
};

} // namespace HtmlEngine

#endif // HTML_ENGINE_PAINTER_H
