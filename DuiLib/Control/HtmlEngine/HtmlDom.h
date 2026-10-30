#ifndef HTML_ENGINE_DOM_H
#define HTML_ENGINE_DOM_H

// ==========================================================================
//  HtmlDom.h — DOM 节点树与计算样式
//
//  HTML 解析后得到 Node 树；CSS 级联后每个 Node 上挂一份 ComputedStyle。
// ==========================================================================

#include "HtmlTypes.h"
#include <vector>
#include <string>
#include <memory>

namespace HtmlEngine
{

class Node;

// ============================ 计算后的样式 ============================

struct ComputedStyle
{
    // ---- 显示与定位 ----
    DisplayType     display;
    PositionType    position;
    FloatType       floatType;
    ClearType       clearType;
    Visibility      visibility;
    int             zIndex;
    BoxSizing       boxSizing;

    // ---- 尺寸 ----
    Length  width, height;
    Length  minWidth, minHeight;
    Length  maxWidth, maxHeight;
    Length  top, right, bottom, left;

    // ---- 盒模型 ----
    Length  marginTop, marginRight, marginBottom, marginLeft;
    Length  paddingTop, paddingRight, paddingBottom, paddingLeft;
    BorderSide borderTop, borderRight, borderBottom, borderLeft;
    BorderRadius borderRadius;

    // ---- 背景 ----
    Color               backgroundColor;
    BackgroundImage     backgroundImage;
    bool                backgroundClipText;   // background-clip: text

    // ---- 文字 ----
    std::string fontFamily;
    float       fontSize;           // px
    FontWeight  fontWeight;
    FontStyle   fontStyle;
    Color       color;
    TextAlign   textAlign;
    float       lineHeight;         // px（0 = 用字体默认行高）
    Length      textIndent;
    bool        underline;
    bool        lineThrough;
    bool        overline;
    WhiteSpaceType whiteSpace;
    VerticalAlign  verticalAlign;

    // ---- 效果 ----
    float       opacity;            // 0~1
    std::vector<Shadow> boxShadows;
    std::vector<Shadow> textShadows;
    float       blurRadius;         // filter: blur()
    float       grayscale;          // filter: grayscale()
    float       brightness;         // filter: brightness()

    // ---- 列表 ----
    ListStyleType listStyleType;
    bool          listStyleInside;

    // ---- Flex ----
    FlexDirection   flexDirection;
    FlexWrap        flexWrap;
    JustifyContent  justifyContent;
    AlignItems      alignItems;
    AlignItems      alignSelf;
    float           flexGrow;
    float           flexShrink;
    Length          flexBasis;
    float           gap;

    // ---- 表格 ----
    int   borderCollapse;       // 0=separate 1=collapse
    float borderSpacing;

    // ---- 溢出 ----
    OverflowType overflowX, overflowY;

    // ---- 其它 ----
    std::string cursor;
    Length      letterSpacing;
    Length      wordSpacing;

    ComputedStyle();
};

// ============================ DOM 节点 ============================

enum NodeType
{
    NODE_DOCUMENT,
    NODE_ELEMENT,
    NODE_TEXT,
    NODE_COMMENT,
};

class Node : public std::enable_shared_from_this<Node>
{
public:
    NodeType                    type;
    std::string                 tagName;        // 元素标签名（小写）
    std::string                 text;           // 文本节点内容
    std::map<std::string, std::string> attrs;   // 元素属性
    std::vector<Node*>          children;
    Node*                       parent;
    ComputedStyle               style;
    int                         depth;

    // 布局结果（由布局引擎填写）
    float   x, y;               // 相对父容器内容区左上角
    float   width, height;      // 边框盒尺寸
    float   contentX, contentY; // 内容区左上角（绝对，相对文档）
    float   contentW, contentH;

    // 辅助标记
    bool    bLaidOut;
    bool    bIsInline;          // 是否行内级
    int     listIndex;          // 列表项序号

    // 文本测量缓存
    float   cachedTextWidth;
    std::string displayText;    // 清理后的实际绘制文本（white-space 处理后）

    Node(NodeType t) : type(t), parent(NULL), depth(0),
        x(0), y(0), width(0), height(0),
        contentX(0), contentY(0), contentW(0), contentH(0),
        bLaidOut(false), bIsInline(false), listIndex(0), cachedTextWidth(-1.0f) {}

    ~Node()
    {
        for (size_t i = 0; i < children.size(); i++) delete children[i];
        children.clear();
    }

    // 属性访问
    bool HasAttr(const std::string& name) const
    {
        return attrs.find(name) != attrs.end();
    }
    std::string GetAttr(const std::string& name, const std::string& def = "") const
    {
        std::map<std::string, std::string>::const_iterator it = attrs.find(name);
        return (it == attrs.end()) ? def : it->second;
    }
    int GetAttrInt(const std::string& name, int def = 0) const
    {
        std::map<std::string, std::string>::const_iterator it = attrs.find(name);
        return (it == attrs.end()) ? def : atoi(it->second.c_str());
    }

    void AddChild(Node* child)
    {
        child->parent = this;
        child->depth = depth + 1;
        children.push_back(child);
    }

    bool IsElement() const { return type == NODE_ELEMENT; }
    bool IsText() const { return type == NODE_TEXT; }
    bool IsDocument() const { return type == NODE_DOCUMENT; }

    std::string GetId() const { return GetAttr("id"); }
    std::string GetClass() const { return GetAttr("class"); }
    std::vector<std::string> GetClasses() const;

    // 递归查找（用于 getElementById 之类）
    Node* FindById(const std::string& id);
    Node* FindByAttr(const std::string& attrName, const std::string& value);

    // 计算内容区宽度（width 属性 or 撑满）
    float GetContentWidth() const;
    float GetContentHeight() const;

    // 元素类型判断（用于默认样式）
    bool IsBlockByDefault() const;
    bool IsInlineByDefault() const;
    bool IsVoidElement() const;     // <br> <img> <input> 等自闭合标签
    bool IsFormControl() const;     // input/select/button/textarea
};

// ============================ 文档树 ============================

class DocumentTree
{
public:
    Node* root;         // 根节点（NODE_DOCUMENT）
    Node* htmlNode;
    Node* headNode;
    Node* bodyNode;

    DocumentTree();
    ~DocumentTree();

    void Clear();

    // 递归查找元素
    Node* FindById(const std::string& id) const;
    Node* FindByTag(const std::string& tag) const;
    // 收集所有表单控件（按 name 属性）
    Node* FindFormByName(const std::string& name) const;
};

// ============================ 工具 ============================

// 标签是否块级
bool IsBlockTag(const std::string& tag);
// 标签是否行内
bool IsInlineTag(const std::string& tag);
// 标签是否空元素（无需闭合）
bool IsVoidTag(const std::string& tag);

} // namespace HtmlEngine

#endif // HTML_ENGINE_DOM_H
