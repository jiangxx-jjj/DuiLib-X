#include "StdAfx.h"
#include "HtmlLayout.h"
#include <cmath>

namespace HtmlEngine
{

LayoutEngine::LayoutEngine()
    : m_fnMeasureText(NULL)
    , m_fnMeasureLineHeight(NULL)
{
}

void LayoutEngine::SetMeasureCallbacks(MeasureTextFn fnText, MeasureLineHeightFn fnLineHeight)
{
    m_fnMeasureText = fnText;
    m_fnMeasureLineHeight = fnLineHeight;
}

// ============================ 测量 ============================

float LayoutEngine::MeasureText(const std::string& text, const ComputedStyle& style)
{
    if (text.empty()) return 0.0f;
    if (m_fnMeasureText != NULL) {
        return m_fnMeasureText(text, style.fontFamily, style.fontSize,
                               style.fontWeight >= FONT_WEIGHT_BOLD,
                               style.fontStyle == FONT_ITALIC);
    }
    // 兜底：按字号估算
    return (float)text.size() * style.fontSize * 0.55f;
}

float LayoutEngine::GetLineHeight(const ComputedStyle& style)
{
    if (style.lineHeight > 0.0f) return style.lineHeight;
    if (m_fnMeasureLineHeight != NULL) {
        return m_fnMeasureLineHeight(style.fontFamily, style.fontSize,
                                     style.fontWeight >= FONT_WEIGHT_BOLD,
                                     style.fontStyle == FONT_ITALIC);
    }
    return style.fontSize * 1.4f;
}

// ============================ 辅助 ============================

bool LayoutEngine::IsInNormalFlow(Node* node) const
{
    if (node == NULL) return false;
    if (node->style.display == DISPLAY_NONE) return false;
    if (node->style.position == POS_ABSOLUTE || node->style.position == POS_FIXED) return false;
    return true;
}

// 解析外边距/内边距为像素
static float ResolveMargin(const Length& len, float base, float fontSize, float rootFontSize)
{
    if (len.IsAuto()) return 0.0f;
    return len.Resolve(base, fontSize, rootFontSize);
}

// 判断节点是否是行内级
static bool IsInlineLevel(Node* node)
{
    if (node == NULL) return false;
    if (node->IsText()) return true;
    if (!node->IsElement()) return false;

    DisplayType d = node->style.display;
    return d == DISPLAY_INLINE || d == DISPLAY_INLINE_BLOCK || d == DISPLAY_INLINE_FLEX;
}

// ============================ 尺寸计算 ============================

void LayoutEngine::ComputeNodeSize(Node* node, float availWidth, float parentHeight)
{
    if (node == NULL) return;

    ComputedStyle& st = node->style;
    float fontSize = st.fontSize;
    const float rootFontSize = 16.0f;

    // ---- 内外边距 ----
    float mt = ResolveMargin(st.marginTop, availWidth, fontSize, rootFontSize);
    float mr = ResolveMargin(st.marginRight, availWidth, fontSize, rootFontSize);
    float mb = ResolveMargin(st.marginBottom, availWidth, fontSize, rootFontSize);
    float ml = ResolveMargin(st.marginLeft, availWidth, fontSize, rootFontSize);

    float pt = ResolveMargin(st.paddingTop, availWidth, fontSize, rootFontSize);
    float pr = ResolveMargin(st.paddingRight, availWidth, fontSize, rootFontSize);
    float pb = ResolveMargin(st.paddingBottom, availWidth, fontSize, rootFontSize);
    float pl = ResolveMargin(st.paddingLeft, availWidth, fontSize, rootFontSize);

    // ---- 边框 ----
    float bt = st.borderTop.IsVisible() ? st.borderTop.width : 0.0f;
    float br = st.borderRight.IsVisible() ? st.borderRight.width : 0.0f;
    float bb = st.borderBottom.IsVisible() ? st.borderBottom.width : 0.0f;
    float bl = st.borderLeft.IsVisible() ? st.borderLeft.width : 0.0f;

    // ---- 宽度 ----
    float contentWidth = 0.0f;
    bool bWidthAuto = st.width.IsAuto();

    if (!bWidthAuto) {
        float w = st.width.Resolve(availWidth, fontSize, rootFontSize);
        if (st.boxSizing == BOX_BORDER) {
            contentWidth = w - pl - pr - bl - br;
        } else {
            contentWidth = w;
        }
    } else {
        // auto：块级撑满，行内由内容决定
        if (st.display == DISPLAY_BLOCK || st.display == DISPLAY_FLEX ||
            st.display == DISPLAY_LIST_ITEM || st.display == DISPLAY_TABLE ||
            st.display == DISPLAY_TABLE_ROW_GROUP) {
            contentWidth = availWidth - ml - mr - pl - pr - bl - br;
        } else {
            contentWidth = 0.0f;   // 由内容决定（行内/行内块）
        }
    }
    if (contentWidth < 0) contentWidth = 0;

    // min/max width
    if (!st.minWidth.IsAuto()) {
        float mw = st.minWidth.Resolve(availWidth, fontSize, rootFontSize);
        if (contentWidth < mw) contentWidth = mw;
    }
    if (!st.maxWidth.IsAuto()) {
        float mw = st.maxWidth.Resolve(availWidth, fontSize, rootFontSize);
        if (mw > 0 && contentWidth > mw) contentWidth = mw;
    }

    // ---- 高度 ----
    float contentHeight = 0.0f;
    bool bHeightAuto = st.height.IsAuto();

    if (!bHeightAuto) {
        float h = st.height.Resolve(parentHeight > 0 ? parentHeight : 0, fontSize, rootFontSize);
        if (st.boxSizing == BOX_BORDER) {
            contentHeight = h - pt - pb - bt - bb;
        } else {
            contentHeight = h;
        }
        if (contentHeight < 0) contentHeight = 0;
    }

    if (!st.minHeight.IsAuto()) {
        float mh = st.minHeight.Resolve(0, fontSize, rootFontSize);
        if (contentHeight < mh) contentHeight = mh;
    }
    if (!st.maxHeight.IsAuto()) {
        float mh = st.maxHeight.Resolve(0, fontSize, rootFontSize);
        if (mh > 0 && contentHeight > mh) contentHeight = mh;
    }

    node->contentW = contentWidth;
    node->contentH = contentHeight;
    node->width  = contentWidth + pl + pr + bl + br;
    node->height = contentHeight + pt + pb + bt + bb;
}

// ============================ 文本节点布局 ============================

// 文本节点：测量宽高
static void LayoutTextNode(Node* node, LayoutEngine* engine, float availWidth)
{
    if (node == NULL || !node->IsText()) return;

    const ComputedStyle& st = node->style;

    // 清理文本：合并空白（white-space: normal）
    std::string text = node->text;
    if (st.whiteSpace == WS_NORMAL || st.whiteSpace == WS_NOWRAP) {
        std::string cleaned;
        bool lastSpace = true;
        for (size_t i = 0; i < text.size(); i++) {
            char ch = text[i];
            if (ch == '\r' || ch == '\n' || ch == '\t') ch = ' ';
            if (ch == ' ') {
                if (!lastSpace) { cleaned += ' '; lastSpace = true; }
            } else {
                cleaned += ch;
                lastSpace = false;
            }
        }
        text = Trim(cleaned);
    }

    if (text.empty()) {
        node->width = 0;
        node->height = 0;
        node->contentW = 0;
        node->contentH = 0;
        node->displayText.clear();
        return;
    }

    node->displayText = text;

    float w = 0.0f;
    if (engine != NULL) {
        w = engine->MeasureTextPublic(text, st);
    } else {
        w = (float)text.size() * st.fontSize * 0.55f;
    }

    float lh = st.fontSize * 1.4f;
    if (st.lineHeight > 0.0f) lh = st.lineHeight;

    node->width = w;
    node->height = lh;
    node->contentW = w;
    node->contentH = lh;
    node->cachedTextWidth = w;
}

// ============================ 行内布局 ============================

float LayoutEngine::LayoutInlineChildren(Node* node, float x, float y, float availWidth)
{
    if (node == NULL) return 0.0f;

    // 收集所有行内级子节点（文本节点 + 行内元素）
    std::vector<Node*> runs;
    for (size_t i = 0; i < node->children.size(); i++) {
        Node* child = node->children[i];
        if (!IsInNormalFlow(child)) continue;
        if (child->style.display == DISPLAY_NONE) continue;

        if (child->IsText()) {
            runs.push_back(child);
        } else if (IsInlineLevel(child)) {
            runs.push_back(child);
        } else {
            // 块级子节点混在行内流里：先处理行内，再单独布局块级
            runs.push_back(child);
        }
    }

    float lineHeight = GetLineHeight(node->style);
    float curX = 0.0f;
    float curY = 0.0f;
    float lineMaxH = lineHeight;
    float maxRight = 0.0f;
    float maxBottom = 0.0f;

    // 行内元素尺寸缓存
    struct InlineItem { Node* node; float w; float h; bool bBlock; };
    std::vector<InlineItem> items;

    for (size_t i = 0; i < runs.size(); i++) {
        Node* child = runs[i];

        if (child->IsText()) {
            LayoutTextNode(child, this, availWidth);
            InlineItem it;
            it.node = child;
            it.w = child->width;
            it.h = child->height;
            it.bBlock = false;
            items.push_back(it);
        } else if (IsInlineLevel(child)) {
            // 行内块元素：先按内容布局内部，再取实际尺寸
            if (child->style.display == DISPLAY_INLINE_BLOCK ||
                child->style.display == DISPLAY_INLINE_FLEX ||
                child->style.display == DISPLAY_INLINE) {
                ComputeNodeSize(child, availWidth, 0);

                // 宽度 auto：先按最大可用宽度布局内部，再收缩到内容宽度
                float innerAvail = child->contentW > 0 ? child->contentW : availWidth;
                LayoutNode(child, 0, 0, innerAvail);

                if (child->style.width.IsAuto()) {
                    // 用内容实际宽度收缩
                    float contentNeed = 0.0f;
                    for (size_t k = 0; k < child->children.size(); k++) {
                        Node* gc = child->children[k];
                        if (gc->style.display == DISPLAY_NONE) continue;
                        float right = gc->x + gc->width;
                        if (right > contentNeed) contentNeed = right;
                    }
                    if (contentNeed > 0) {
                        child->contentW = contentNeed;
                        child->width = contentNeed +
                            child->style.paddingLeft.Resolve(availWidth, child->style.fontSize, 16.0f) +
                            child->style.paddingRight.Resolve(availWidth, child->style.fontSize, 16.0f) +
                            (child->style.borderLeft.IsVisible() ? child->style.borderLeft.width : 0) +
                            (child->style.borderRight.IsVisible() ? child->style.borderRight.width : 0);
                    }
                }
            }
            InlineItem it;
            it.node = child;
            it.w = child->width;
            it.h = child->height;
            it.bBlock = false;
            items.push_back(it);
        } else {
            // 块级：单独处理
            InlineItem it;
            it.node = child;
            it.w = 0;
            it.h = 0;
            it.bBlock = true;
            items.push_back(it);
        }
    }

    // 排版：逐项放入行
    float blockY = 0.0f;

    // 行内对齐（text-align）
    TextAlign align = node->style.textAlign;

    // 记录本容器所有已放置的行内元素（用于对齐后处理）
    std::vector<Node*> lineItems;
    float lineEndX = 0.0f;

    for (size_t i = 0; i < items.size(); i++) {
        InlineItem& it = items[i];
        Node* child = it.node;

        if (it.bBlock) {
            // 块级：先换行，再占满宽度
            if (curX > 0.0f) {
                curY += lineMaxH;
                curX = 0.0f;
                lineMaxH = lineHeight;
            }
            float blockAvail = availWidth;
            ComputeNodeSize(child, blockAvail, 0);
            LayoutNode(child, 0, curY, blockAvail);
            child->x = 0;
            child->y = curY;
            curY += child->height;
            blockY = curY;
            continue;
        }

        // 行内：检查是否需要换行
        bool bNeedWrap = false;
        if (it.node->IsText() && node->style.whiteSpace != WS_NOWRAP &&
            node->style.whiteSpace != WS_PRE) {
            if (curX + it.w > availWidth && curX > 0.0f) bNeedWrap = true;
        } else if (!it.node->IsText() && node->style.whiteSpace != WS_NOWRAP) {
            if (curX + it.w > availWidth && curX > 0.0f) bNeedWrap = true;
        }

        if (bNeedWrap) {
            curY += lineMaxH;
            curX = 0.0f;
            lineMaxH = lineHeight;
        }

        // 放置
        child->x = curX;
        child->y = curY;

        curX += it.w;
        if (it.h > lineMaxH) lineMaxH = it.h;
        if (curX > maxRight) maxRight = curX;

        // 记录该行已放置的元素（用于对齐后处理）
        lineItems.push_back(child);
        lineEndX = curX;
    }

    // ---- 按行应用 text-align ----
    if (align != ALIGN_LEFT && !lineItems.empty()) {
        // 简化：把整个最后一行按对齐重排
        // 收集所有行的起止
        struct LineInfo { float startY; std::vector<Node*> items; float endX; };
        std::vector<LineInfo> lines;

        for (size_t i = 0; i < lineItems.size(); i++) {
            Node* c = lineItems[i];
            if (lines.empty() || c->y > lines.back().startY + 1.0f) {
                LineInfo li;
                li.startY = c->y;
                li.endX = 0.0f;
                lines.push_back(li);
            }
            lines.back().items.push_back(c);
            float right = c->x + c->width;
            if (right > lines.back().endX) lines.back().endX = right;
        }

        for (size_t li = 0; li < lines.size(); li++) {
            float offset = 0.0f;
            if (align == ALIGN_CENTER) {
                offset = (availWidth - lines[li].endX) / 2.0f;
            } else if (align == ALIGN_RIGHT) {
                offset = availWidth - lines[li].endX;
            } else if (align == ALIGN_JUSTIFY && lines[li].items.size() > 1) {
                // 两端对齐：把剩余空间平均分配到间隙
                float gap = (availWidth - lines[li].endX) / (float)(lines[li].items.size() - 1);
                float acc = 0.0f;
                for (size_t k = 0; k < lines[li].items.size(); k++) {
                    lines[li].items[k]->x += acc;
                    acc += gap;
                }
                continue;
            }
            if (offset <= 0.0f) continue;
            for (size_t k = 0; k < lines[li].items.size(); k++) {
                lines[li].items[k]->x += offset;
            }
        }
    }

    float totalH = curY + lineMaxH;
    if (blockY > totalH) totalH = blockY;

    return totalH;
}

// ============================ 块级布局 ============================

float LayoutEngine::LayoutBlockChildren(Node* node, float x, float y, float availWidth)
{
    if (node == NULL) return 0.0f;

    float curY = y;
    float maxBottom = y;

    // 列表项序号
    int listCounter = 1;

    for (size_t i = 0; i < node->children.size(); i++) {
        Node* child = node->children[i];

        if (child->style.display == DISPLAY_NONE) continue;
        if (child->style.visibility == VIS_HIDDEN) {
            // 占位但不绘制
        }

        // 绝对定位：脱离流
        if (child->style.position == POS_ABSOLUTE || child->style.position == POS_FIXED) {
            ComputeNodeSize(child, availWidth, 0);

            float cx = x;
            float cy = y;

            const float rootFontSize = 16.0f;
            if (!child->style.left.IsAuto()) {
                cx = x + child->style.left.Resolve(availWidth, child->style.fontSize, rootFontSize);
            } else if (!child->style.right.IsAuto()) {
                float r = child->style.right.Resolve(availWidth, child->style.fontSize, rootFontSize);
                cx = x + availWidth - child->width - r;
            }
            if (!child->style.top.IsAuto()) {
                cy = y + child->style.top.Resolve(0, child->style.fontSize, rootFontSize);
            } else if (!child->style.bottom.IsAuto()) {
                cy = y + child->style.bottom.Resolve(0, child->style.fontSize, rootFontSize);
            }



            LayoutNode(child, cx, cy, child->contentW > 0 ? child->contentW : availWidth);
            child->x = cx - x;
            child->y = cy - y;
            continue;
        }

        // 文本节点：单独成行
        if (child->IsText()) {
            LayoutTextNode(child, this, availWidth);
            if (child->width <= 0 && child->height <= 0) continue;

            child->x = 0;
            child->y = curY - y;
            curY += child->height;
            if (curY > maxBottom) maxBottom = curY;
            continue;
        }

        // 列表项计数
        if (child->style.display == DISPLAY_LIST_ITEM) {
            child->listIndex = listCounter++;
        }

        // 浮动元素：简化处理（占一行）
        if (child->style.floatType != FLOAT_NONE) {
            ComputeNodeSize(child, availWidth, 0);
            LayoutNode(child, 0, curY, child->contentW > 0 ? child->contentW : availWidth);
            child->x = (child->style.floatType == FLOAT_RIGHT)
                     ? (availWidth - child->width) : 0;
            child->y = curY - y;
            curY += child->height;
            if (curY > maxBottom) maxBottom = curY;
            continue;
        }

        // 行内级元素（inline / inline-block）出现在块级流里：包成一行
        if (IsInlineLevel(child)) {
            // 把连续的同行内元素合并成一次行内布局
            std::vector<Node*> runGroup;
            size_t k = i;
            while (k < node->children.size()) {
                Node* c = node->children[k];
                if (c->style.display == DISPLAY_NONE) { k++; continue; }
                if (!IsInlineLevel(c)) break;
                if (c->style.position == POS_ABSOLUTE || c->style.position == POS_FIXED) break;
                runGroup.push_back(c);
                k++;
            }

            if (!runGroup.empty()) {
                // 用一个临时虚拟容器做行内布局
                float lineY = curY;
                float curX = 0.0f;
                float lineMaxH = GetLineHeight(node->style);
                float maxRight = 0.0f;
                int lineStartIdx = 0;
                std::vector<Node*> lineGroup;

                for (size_t r = 0; r < runGroup.size(); r++) {
                    Node* c = runGroup[r];

                    if (c->IsText()) {
                        LayoutTextNode(c, this, availWidth);
                    } else {
                        ComputeNodeSize(c, availWidth, 0);
                        float innerAvail = c->contentW > 0 ? c->contentW : availWidth;
                        LayoutNode(c, 0, 0, innerAvail);
                    }

                    float w = c->width;
                    float h = c->height;

                    bool bWrap = (node->style.whiteSpace != WS_NOWRAP) &&
                                 (curX + w > availWidth) && (curX > 0.0f);
                    if (bWrap) {
                        curY += lineMaxH;
                        curX = 0.0f;
                        lineMaxH = GetLineHeight(node->style);
                        lineY = curY;
                    }

                    c->x = curX;
                    c->y = curY - y;
                    curX += w;
                    if (h > lineMaxH) lineMaxH = h;
                    if (curX > maxRight) maxRight = curX;

                    // 记录该行元素（供 text-align 后处理）
                    if (lineStartIdx == 0) lineStartIdx = (int)lineGroup.size();
                    lineGroup.push_back(c);
                }

                // ---- 应用 text-align ----
                TextAlign align = node->style.textAlign;
                if (align != ALIGN_LEFT && !lineGroup.empty()) {
                    // 按 y 分行
                    struct LineInfo { float startY; std::vector<Node*> items; float endX; };
                    std::vector<LineInfo> lines;
                    for (size_t q = 0; q < lineGroup.size(); q++) {
                        Node* c = lineGroup[q];
                        if (lines.empty() || c->y > lines.back().startY + 1.0f) {
                            LineInfo li;
                            li.startY = c->y;
                            li.endX = 0.0f;
                            lines.push_back(li);
                        }
                        lines.back().items.push_back(c);
                        float right = c->x + c->width;
                        if (right > lines.back().endX) lines.back().endX = right;
                    }
                    for (size_t q = 0; q < lines.size(); q++) {
                        float offset = 0.0f;
                        if (align == ALIGN_CENTER) offset = (availWidth - lines[q].endX) / 2.0f;
                        else if (align == ALIGN_RIGHT) offset = availWidth - lines[q].endX;
                        else if (align == ALIGN_JUSTIFY && lines[q].items.size() > 1) {
                            float gap = (availWidth - lines[q].endX) / (float)(lines[q].items.size() - 1);
                            float acc = 0.0f;
                            for (size_t z = 0; z < lines[q].items.size(); z++) {
                                lines[q].items[z]->x += acc;
                                acc += gap;
                            }
                            continue;
                        }
                        if (offset <= 0.0f) continue;
                        for (size_t z = 0; z < lines[q].items.size(); z++) {
                            lines[q].items[z]->x += offset;
                        }
                    }
                }

                curY += lineMaxH;
                if (curY > maxBottom) maxBottom = curY;

                i = k - 1;
                continue;
            }
        }

        // ---- 普通块级元素 ----
        ComputeNodeSize(child, availWidth, 0);

        // 外边距折叠（简化：上下相邻取较大值）
        float mt = ResolveMargin(child->style.marginTop, availWidth,
                                 child->style.fontSize, 16.0f);
        curY += mt;

        float innerAvail = child->contentW > 0 ? child->contentW : availWidth;
        float childContentH = LayoutNode(child, 0, curY, innerAvail);

        // 如果高度是 auto，用内容高度
        if (child->style.height.IsAuto()) {
            float pt = ResolveMargin(child->style.paddingTop, availWidth, child->style.fontSize, 16.0f);
            float pb = ResolveMargin(child->style.paddingBottom, availWidth, child->style.fontSize, 16.0f);
            float bt = child->style.borderTop.IsVisible() ? child->style.borderTop.width : 0;
            float bb = child->style.borderBottom.IsVisible() ? child->style.borderBottom.width : 0;
            child->contentH = childContentH;
            child->height = childContentH + pt + pb + bt + bb;
        }

        child->x = 0;
        child->y = curY - y;

        curY += child->height;
        float mb = ResolveMargin(child->style.marginBottom, availWidth,
                                 child->style.fontSize, 16.0f);
        curY += mb;

        if (curY > maxBottom) maxBottom = curY;
    }

    return maxBottom - y;
}

// ============================ Flex 布局 ============================

float LayoutEngine::LayoutFlex(Node* node, float x, float y, float availWidth)
{
    if (node == NULL) return 0.0f;

    const ComputedStyle& st = node->style;
    bool bRow = (st.flexDirection == FLEX_ROW || st.flexDirection == FLEX_ROW_REVERSE);
    bool bReverse = (st.flexDirection == FLEX_ROW_REVERSE || st.flexDirection == FLEX_COLUMN_REVERSE);

    // 收集 flex 子项
    std::vector<Node*> items;
    for (size_t i = 0; i < node->children.size(); i++) {
        Node* c = node->children[i];
        if (c->style.display == DISPLAY_NONE) continue;
        if (c->style.position == POS_ABSOLUTE || c->style.position == POS_FIXED) continue;
        if (c->IsText()) {
            // 纯空白文本跳过
            std::string t = Trim(c->text);
            if (t.empty()) continue;
        }
        items.push_back(c);
    }
    if (items.empty()) return 0.0f;
    if (bReverse) std::reverse(items.begin(), items.end());

    float gap = st.gap;
    float totalMain = 0.0f;
    float maxCross = 0.0f;

    // ---- 第一遍：测量各子项 ----
    for (size_t i = 0; i < items.size(); i++) {
        Node* c = items[i];

        if (c->IsText()) {
            LayoutTextNode(c, this, availWidth);
        } else {
            ComputeNodeSize(c, availWidth, 0);

            // flex 容器 / inline-block 且宽度 auto：先布局内部，再按内容收缩
            if (c->style.width.IsAuto() &&
                (c->style.display == DISPLAY_FLEX ||
                 c->style.display == DISPLAY_INLINE_BLOCK ||
                 c->style.display == DISPLAY_INLINE_FLEX)) {
                float innerAvail = availWidth;
                LayoutNode(c, 0, 0, innerAvail);

                float need = 0.0f;
                for (size_t k = 0; k < c->children.size(); k++) {
                    Node* gc = c->children[k];
                    if (gc->style.display == DISPLAY_NONE) continue;
                    float right = gc->x + gc->width;
                    if (right > need) need = right;
                }
                if (need > 0) {
                    c->contentW = need;
                    c->width = need +
                        c->style.paddingLeft.Resolve(availWidth, c->style.fontSize, 16.0f) +
                        c->style.paddingRight.Resolve(availWidth, c->style.fontSize, 16.0f) +
                        (c->style.borderLeft.IsVisible() ? c->style.borderLeft.width : 0) +
                        (c->style.borderRight.IsVisible() ? c->style.borderRight.width : 0);
                }
            }
        }

        float mainSize = bRow ? c->width : c->height;
        float crossSize = bRow ? c->height : c->width;

        totalMain += mainSize + (i > 0 ? gap : 0.0f);
        if (crossSize > maxCross) maxCross = crossSize;
    }

    // ---- 第二遍：分配剩余空间（flex-grow）----
    float availMain = bRow ? availWidth : (node->contentH > 0 ? node->contentH : 0.0f);
    float freeSpace = availMain - totalMain;

    if (freeSpace > 0.0f && !bRow && availMain <= 0.0f) {
        freeSpace = 0.0f;   // column 且高度未定：不拉伸
    }

    float totalGrow = 0.0f;
    for (size_t i = 0; i < items.size(); i++) {
        totalGrow += items[i]->style.flexGrow;
    }

    if (freeSpace > 0.0f && totalGrow > 0.0f) {
        for (size_t i = 0; i < items.size(); i++) {
            Node* c = items[i];
            if (c->style.flexGrow <= 0.0f) continue;
            float extra = freeSpace * (c->style.flexGrow / totalGrow);
            if (bRow) {
                c->width += extra;
                c->contentW += extra;
            } else {
                c->height += extra;
                c->contentH += extra;
            }
        }
    }

    // ---- 第三遍：定位 ----
    float startOffset = 0.0f;
    float between = 0.0f;
    float count = (float)items.size();

    if (freeSpace > 0.0f && totalGrow <= 0.0f) {
        switch (st.justifyContent) {
        case JUSTIFY_END:
            startOffset = freeSpace;
            break;
        case JUSTIFY_CENTER:
            startOffset = freeSpace / 2.0f;
            break;
        case JUSTIFY_SPACE_BETWEEN:
            if (count > 1.0f) between = freeSpace / (count - 1.0f);
            break;
        case JUSTIFY_SPACE_AROUND:
            if (count > 0.0f) {
                between = freeSpace / count;
                startOffset = between / 2.0f;
            }
            break;
        case JUSTIFY_SPACE_EVENLY:
            if (count > 0.0f) {
                between = freeSpace / (count + 1.0f);
                startOffset = between;
            }
            break;
        default:
            break;
        }
    }

    float mainPos = startOffset;
    float maxMainExtent = 0.0f;

    for (size_t i = 0; i < items.size(); i++) {
        Node* c = items[i];

        float mainSize = bRow ? c->width : c->height;
        float crossSize = bRow ? c->height : c->width;

        // 交叉轴对齐
        float crossOffset = 0.0f;
        AlignItems align = c->style.alignSelf;
        if (align == ALIGN_ITEMS_STRETCH && st.alignItems != ALIGN_ITEMS_STRETCH) {
            align = st.alignItems;
        } else if (align == ALIGN_ITEMS_STRETCH) {
            align = st.alignItems;
        }

        float crossAvail = bRow ? (node->contentH > 0 ? node->contentH : maxCross) : availWidth;

        if (align == ALIGN_ITEMS_STRETCH && crossAvail > crossSize && crossAvail > 0.0f) {
            // 拉伸
            if (bRow) {
                c->height = crossAvail;
                c->contentH = crossAvail;
            } else {
                c->width = crossAvail;
                c->contentW = crossAvail;
            }
            crossSize = crossAvail;
        }
        else if (align == ALIGN_ITEMS_CENTER) {
            crossOffset = (crossAvail - crossSize) / 2.0f;
        }
        else if (align == ALIGN_ITEMS_END) {
            crossOffset = crossAvail - crossSize;
        }

        float cx, cy;
        if (bRow) {
            cx = x + mainPos;
            cy = y + crossOffset;
        } else {
            cx = x + crossOffset;
            cy = y + mainPos;
        }

        // 布局子项内容
        float innerW = c->contentW > 0 ? c->contentW : availWidth;
        if (!c->IsText()) {
            float innerH = LayoutNode(c, cx, cy, innerW);
            if (c->style.height.IsAuto()) {
                float pt = ResolveMargin(c->style.paddingTop, innerW, c->style.fontSize, 16.0f);
                float pb = ResolveMargin(c->style.paddingBottom, innerW, c->style.fontSize, 16.0f);
                c->contentH = innerH;
                c->height = innerH + pt + pb +
                    (c->style.borderTop.IsVisible() ? c->style.borderTop.width : 0) +
                    (c->style.borderBottom.IsVisible() ? c->style.borderBottom.width : 0);
            }
        }

        c->x = cx - x;
        c->y = cy - y;

        mainPos += mainSize + between + gap;
        if (mainPos > maxMainExtent) maxMainExtent = mainPos;
    }

    float totalCross = 0.0f;
    if (bRow) {
        totalCross = node->contentH > 0 ? node->contentH : maxCross;
    } else {
        totalCross = maxMainExtent;
    }

    return bRow ? (totalCross > 0 ? totalCross : maxCross) : maxMainExtent;
}

// ============================ 表格布局 ============================

float LayoutEngine::LayoutTable(Node* node, float x, float y, float availWidth)
{
    if (node == NULL) return 0.0f;

    // 收集行
    std::vector<Node*> rows;
    for (size_t i = 0; i < node->children.size(); i++) {
        Node* c = node->children[i];
        if (c->style.display == DISPLAY_NONE) continue;
        if (c->tagName == "tr" || c->style.display == DISPLAY_TABLE_ROW) {
            rows.push_back(c);
        } else if (c->tagName == "thead" || c->tagName == "tbody" || c->tagName == "tfoot") {
            for (size_t k = 0; k < c->children.size(); k++) {
                Node* r = c->children[k];
                if (r->tagName == "tr" || r->style.display == DISPLAY_TABLE_ROW) rows.push_back(r);
            }
        }
    }
    if (rows.empty()) return 0.0f;

    // 计算列数
    int colCount = 0;
    for (size_t i = 0; i < rows.size(); i++) {
        int cnt = 0;
        for (size_t k = 0; k < rows[i]->children.size(); k++) {
            if (rows[i]->children[k]->IsElement()) {
                cnt += rows[i]->children[k]->GetAttrInt("colspan", 1);
            }
        }
        if (cnt > colCount) colCount = cnt;
    }
    if (colCount <= 0) colCount = 1;

    // 每列宽度：按内容最小需求平均分配
    std::vector<float> colWidths(colCount, 0.0f);
    for (size_t i = 0; i < rows.size(); i++) {
        int colIdx = 0;
        for (size_t k = 0; k < rows[i]->children.size(); k++) {
            Node* cell = rows[i]->children[k];
            if (!cell->IsElement()) continue;
            int span = cell->GetAttrInt("colspan", 1);
            if (span < 1) span = 1;

            ComputeNodeSize(cell, availWidth / colCount, 0);
            float w = cell->width;
            float perCol = w / span;
            for (int s = 0; s < span && colIdx + s < colCount; s++) {
                if (perCol > colWidths[colIdx + s]) colWidths[colIdx + s] = perCol;
            }
            colIdx += span;
        }
    }

    // 撑满可用宽度
    float totalW = 0.0f;
    for (int i = 0; i < colCount; i++) totalW += colWidths[i];
    if (totalW < availWidth && totalW > 0.0f) {
        float scale = availWidth / totalW;
        for (int i = 0; i < colCount; i++) colWidths[i] *= scale;
    }

    // 逐行布局
    float curY = 0.0f;
    for (size_t i = 0; i < rows.size(); i++) {
        Node* row = rows[i];
        float rowH = 0.0f;
        float curX = 0.0f;
        int colIdx = 0;

        for (size_t k = 0; k < row->children.size(); k++) {
            Node* cell = row->children[k];
            if (!cell->IsElement()) continue;

            int span = cell->GetAttrInt("colspan", 1);
            if (span < 1) span = 1;
            float cellW = 0.0f;
            for (int s = 0; s < span && colIdx + s < colCount; s++) cellW += colWidths[colIdx + s];

            cell->contentW = cellW;
            LayoutNode(cell, curX, curY, cellW);

            float h = cell->contentH;
            if (cell->style.height.IsAuto()) {
                float pt = ResolveMargin(cell->style.paddingTop, cellW, cell->style.fontSize, 16.0f);
                float pb = ResolveMargin(cell->style.paddingBottom, cellW, cell->style.fontSize, 16.0f);
                cell->contentH = h;
                cell->height = h + pt + pb;
            }
            cell->x = curX;
            cell->y = curY;

            if (cell->height > rowH) rowH = cell->height;
            curX += cellW;
            colIdx += span;
        }

        // 行高统一
        for (size_t k = 0; k < row->children.size(); k++) {
            Node* cell = row->children[k];
            if (cell->IsElement()) cell->height = rowH;
        }
        row->height = rowH;
        row->x = 0;
        row->y = curY;
        curY += rowH;
    }

    return curY;
}

// ============================ 主布局 ============================

float LayoutEngine::LayoutNode(Node* node, float x, float y, float availWidth)
{
    if (node == NULL) return 0.0f;
    if (node->style.display == DISPLAY_NONE) {
        node->width = node->height = 0;
        node->contentW = node->contentH = 0;
        return 0.0f;
    }

    node->bLaidOut = true;

    // 绝对定位基准
    float baseX = x + ResolveMargin(node->style.marginLeft, availWidth, node->style.fontSize, 16.0f)
                      + (node->style.borderLeft.IsVisible() ? node->style.borderLeft.width : 0)
                      + ResolveMargin(node->style.paddingLeft, availWidth, node->style.fontSize, 16.0f);
    float baseY = y + ResolveMargin(node->style.marginTop, availWidth, node->style.fontSize, 16.0f)
                      + (node->style.borderTop.IsVisible() ? node->style.borderTop.width : 0)
                      + ResolveMargin(node->style.paddingTop, availWidth, node->style.fontSize, 16.0f);

    node->contentX = baseX;
    node->contentY = baseY;

    // 按 display 类型分发
    DisplayType disp = node->style.display;

    if (disp == DISPLAY_FLEX || disp == DISPLAY_INLINE_FLEX) {
        float h = LayoutFlex(node, baseX, baseY, node->contentW > 0 ? node->contentW : availWidth);
        if (node->style.height.IsAuto()) {
            node->contentH = h;
            node->height = h +
                ResolveMargin(node->style.paddingTop, availWidth, node->style.fontSize, 16.0f) +
                ResolveMargin(node->style.paddingBottom, availWidth, node->style.fontSize, 16.0f) +
                (node->style.borderTop.IsVisible() ? node->style.borderTop.width : 0) +
                (node->style.borderBottom.IsVisible() ? node->style.borderBottom.width : 0);
        }
        return node->contentH;
    }

    if (disp == DISPLAY_TABLE) {
        float h = LayoutTable(node, baseX, baseY, node->contentW > 0 ? node->contentW : availWidth);
        if (node->style.height.IsAuto()) {
            node->contentH = h;
            node->height = h +
                ResolveMargin(node->style.paddingTop, availWidth, node->style.fontSize, 16.0f) +
                ResolveMargin(node->style.paddingBottom, availWidth, node->style.fontSize, 16.0f) +
                (node->style.borderTop.IsVisible() ? node->style.borderTop.width : 0) +
                (node->style.borderBottom.IsVisible() ? node->style.borderBottom.width : 0);
        }
        return node->contentH;
    }

    // 表格行/单元格：由 LayoutTable 驱动
    if (disp == DISPLAY_TABLE_ROW || disp == DISPLAY_TABLE_CELL ||
        disp == DISPLAY_TABLE_ROW_GROUP) {
        float innerW = node->contentW > 0 ? node->contentW : availWidth;
        float h = LayoutBlockChildren(node, baseX, baseY, innerW);
        if (node->style.height.IsAuto() && node->contentH <= 0) {
            node->contentH = h;
            node->height = h +
                ResolveMargin(node->style.paddingTop, innerW, node->style.fontSize, 16.0f) +
                ResolveMargin(node->style.paddingBottom, innerW, node->style.fontSize, 16.0f);
        }
        return node->contentH;
    }

    // 块级 / 列表项 / 行内块：判断子内容类型
    if (disp == DISPLAY_BLOCK || disp == DISPLAY_LIST_ITEM ||
        disp == DISPLAY_INLINE_BLOCK) {

        float innerW = node->contentW > 0 ? node->contentW : availWidth;

        // 判断子节点是否全是行内级
        bool bAllInline = true;
        bool bHasContent = false;
        for (size_t i = 0; i < node->children.size(); i++) {
            Node* c = node->children[i];
            if (c->style.display == DISPLAY_NONE) continue;
            if (c->IsText()) {
                if (!Trim(c->text).empty()) bHasContent = true;
                continue;
            }
            bHasContent = true;
            if (!IsInlineLevel(c)) { bAllInline = false; break; }
        }

        // 行内块：内部行内布局不换行（按内容自然宽度）
        if (disp == DISPLAY_INLINE_BLOCK && bAllInline) {
            innerW = 100000.0f;
        }

        float h = 0.0f;
        if (bAllInline && bHasContent) {
            h = LayoutInlineChildren(node, baseX, baseY, innerW);
        } else {
            h = LayoutBlockChildren(node, baseX, baseY, innerW);
        }

        // 行内块宽度 auto 时，收缩到内容实际宽度
        if (disp == DISPLAY_INLINE_BLOCK && node->style.width.IsAuto()) {
            float need = 0.0f;
            for (size_t i = 0; i < node->children.size(); i++) {
                Node* c = node->children[i];
                if (c->style.display == DISPLAY_NONE) continue;
                float right = c->x + c->width;
                if (right > need) need = right;
            }
            if (need > 0) {
                node->contentW = need;
                node->width = need +
                    node->style.paddingLeft.Resolve(innerW, node->style.fontSize, 16.0f) +
                    node->style.paddingRight.Resolve(innerW, node->style.fontSize, 16.0f) +
                    (node->style.borderLeft.IsVisible() ? node->style.borderLeft.width : 0) +
                    (node->style.borderRight.IsVisible() ? node->style.borderRight.width : 0);
            }
        }

        if (node->style.height.IsAuto()) {
            node->contentH = h;
            node->height = h +
                ResolveMargin(node->style.paddingTop, innerW, node->style.fontSize, 16.0f) +
                ResolveMargin(node->style.paddingBottom, innerW, node->style.fontSize, 16.0f) +
                (node->style.borderTop.IsVisible() ? node->style.borderTop.width : 0) +
                (node->style.borderBottom.IsVisible() ? node->style.borderBottom.width : 0);
        }
        return node->contentH;
    }

    // 行内元素：递归布局子内容
    {
        float innerW = node->contentW > 0 ? node->contentW : availWidth;
        float h = LayoutBlockChildren(node, baseX, baseY, innerW);
        if (node->style.height.IsAuto() && node->contentH <= 0) {
            node->contentH = h;
            node->height = h;
        }
    }
}

// ============================ 入口 ============================

float LayoutEngine::Layout(DocumentTree& tree, float viewportWidth)
{
    if (tree.bodyNode == NULL) return 0.0f;

    Node* body = tree.bodyNode;

    // body 尺寸
    ComputeNodeSize(body, viewportWidth, 0);
    body->width = viewportWidth;
    body->contentW = viewportWidth
        - ResolveMargin(body->style.marginLeft, viewportWidth, body->style.fontSize, 16.0f)
        - ResolveMargin(body->style.marginRight, viewportWidth, body->style.fontSize, 16.0f)
        - ResolveMargin(body->style.paddingLeft, viewportWidth, body->style.fontSize, 16.0f)
        - ResolveMargin(body->style.paddingRight, viewportWidth, body->style.fontSize, 16.0f)
        - (body->style.borderLeft.IsVisible() ? body->style.borderLeft.width : 0)
        - (body->style.borderRight.IsVisible() ? body->style.borderRight.width : 0);
    if (body->contentW < 0) body->contentW = 0;

    body->x = 0;
    body->y = 0;

    LayoutNode(body, 0, 0, body->contentW);

    return body->height;
}

// ============================ 命中测试 ============================

// 递归命中测试
static Node* HitTestRec(Node* node, float px, float py, float baseX, float baseY)
{
    if (node == NULL) return NULL;
    if (node->style.display == DISPLAY_NONE) return NULL;
    if (node->style.visibility != VIS_VISIBLE) return NULL;

    float absX = baseX + node->x;
    float absY = baseY + node->y;

    // 从后往前（后面的元素在上层）
    for (size_t i = node->children.size(); i > 0; i--) {
        Node* child = node->children[i - 1];
        Node* r = HitTestRec(child, px, py, absX, absY);
        if (r != NULL) return r;
    }

    // 自身
    if (px >= absX && px < absX + node->width &&
        py >= absY && py < absY + node->height) {
        return node;
    }
    return NULL;
}

Node* LayoutEngine::HitTest(DocumentTree& tree, float x, float y)
{
    if (tree.bodyNode == NULL) return NULL;
    return HitTestRec(tree.bodyNode, x, y, 0, 0);
}

// ============================ 收集可点击区 ============================

static void CollectHitBoxRec(Node* node, float baseX, float baseY, std::vector<LayoutEngine::HitBox>& out)
{
    if (node == NULL) return;
    if (node->style.display == DISPLAY_NONE) return;

    float absX = baseX + node->x;
    float absY = baseY + node->y;

    if (node->IsElement() && node->HasAttr("name") && node->width > 0 && node->height > 0) {
        LayoutEngine::HitBox hb;
        hb.node = node;
        hb.x = absX;
        hb.y = absY;
        hb.w = node->width;
        hb.h = node->height;
        out.push_back(hb);
    }

    for (size_t i = 0; i < node->children.size(); i++) {
        CollectHitBoxRec(node->children[i], absX, absY, out);
    }
}

void LayoutEngine::CollectHitBoxes(DocumentTree& tree, std::vector<HitBox>& out)
{
    out.clear();
    if (tree.bodyNode == NULL) return;
    CollectHitBoxRec(tree.bodyNode, 0, 0, out);
}

} // namespace HtmlEngine
