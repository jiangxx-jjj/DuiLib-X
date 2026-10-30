#ifndef HTML_ENGINE_LAYOUT_H
#define HTML_ENGINE_LAYOUT_H

// ==========================================================================
//  HtmlLayout.h — 自研布局引擎
//
//  实现：
//    · Block 流式布局（垂直堆叠，宽度撑满）
//    · Inline 流式布局（水平排列，自动换行）
//    · Flex 布局（row/column、wrap、justify-content、align-items）
//    · 绝对定位（position:absolute/relative/fixed）
//    · 浮动（float:left/right）
//    · 表格布局（table/tr/td）
//    · 列表项标记
// ==========================================================================

#include "HtmlDom.h"
#include <vector>

namespace HtmlEngine
{

class LayoutEngine
{
public:
    LayoutEngine();

    // 设置字体测量回调（由绘制层提供）
    typedef float (*MeasureTextFn)(const std::string& text, const std::string& fontFamily,
                                   float fontSize, bool bBold, bool bItalic);
    typedef float (*MeasureLineHeightFn)(const std::string& fontFamily, float fontSize,
                                         bool bBold, bool bItalic);

    void SetMeasureCallbacks(MeasureTextFn fnText, MeasureLineHeightFn fnLineHeight);

    // 布局整棵树，返回文档总高度
    float Layout(DocumentTree& tree, float viewportWidth);

    // 查找指定坐标命中的最深层元素（文档坐标）
    Node* HitTest(DocumentTree& tree, float x, float y);

    // 收集所有可点击元素（name 属性）的矩形
    struct HitBox { Node* node; float x, y, w, h; };
    void CollectHitBoxes(DocumentTree& tree, std::vector<HitBox>& out);

private:
    MeasureTextFn     m_fnMeasureText;
    MeasureLineHeightFn m_fnMeasureLineHeight;

    // 布局上下文
    struct LayoutContext {
        float  containerWidth;    // 可用宽度
        float  containerHeight;   // 可用高度（0=不限）
        float  x, y;              // 当前内容区起点（绝对坐标）
        Node*  parent;
        float  floatLeftUsed;     // 左侧浮动占用的高度
    };

    // 主布局入口（返回内容高度）
    float LayoutNode(Node* node, float x, float y, float availWidth);

    // 块级布局（垂直堆叠）
    float LayoutBlockChildren(Node* node, float x, float y, float availWidth);

    // 行内布局（水平排列 + 换行）
    float LayoutInlineChildren(Node* node, float x, float y, float availWidth);

    // Flex 布局
    float LayoutFlex(Node* node, float x, float y, float availWidth);

    // 表格布局
    float LayoutTable(Node* node, float x, float y, float availWidth);

    // 计算节点自身尺寸（不含子内容）
    void ComputeNodeSize(Node* node, float availWidth, float parentHeight);

    // 测量文本
    float MeasureText(const std::string& text, const ComputedStyle& style);
    float GetLineHeight(const ComputedStyle& style);

public:
    // 供外部布局辅助函数使用
    float MeasureTextPublic(const std::string& text, const ComputedStyle& style) { return MeasureText(text, style); }
    float GetLineHeightPublic(const ComputedStyle& style) { return GetLineHeight(style); }

private:

    // 判断节点是否参与正常流
    bool IsInNormalFlow(Node* node) const;

    // 收集行内级子节点（含文本）
    void CollectInlineRuns(Node* node, std::vector<Node*>& out);

    // 递归设置绝对坐标
    void ApplyAbsolutePos(Node* node, float baseX, float baseY);
};

} // namespace HtmlEngine

#endif // HTML_ENGINE_LAYOUT_H
