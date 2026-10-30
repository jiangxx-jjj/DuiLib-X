#include "StdAfx.h"
#include "HtmlDom.h"

namespace HtmlEngine
{

// ============================ ComputedStyle ============================

ComputedStyle::ComputedStyle()
    : display(DISPLAY_INLINE)
    , position(POS_STATIC)
    , floatType(FLOAT_NONE)
    , clearType(CLEAR_NONE)
    , visibility(VIS_VISIBLE)
    , zIndex(0)
    , boxSizing(BOX_CONTENT)
    , width(0, Length::AUTO)
    , height(0, Length::AUTO)
    , minWidth(0, Length::AUTO)
    , minHeight(0, Length::AUTO)
    , maxWidth(0, Length::NONE)
    , maxHeight(0, Length::NONE)
    , top(0, Length::AUTO)
    , right(0, Length::AUTO)
    , bottom(0, Length::AUTO)
    , left(0, Length::AUTO)
    , marginTop(0, Length::PX)
    , marginRight(0, Length::PX)
    , marginBottom(0, Length::PX)
    , marginLeft(0, Length::PX)
    , paddingTop(0, Length::PX)
    , paddingRight(0, Length::PX)
    , paddingBottom(0, Length::PX)
    , paddingLeft(0, Length::PX)
    , backgroundColor(0, 0, 0, 0)
    , backgroundClipText(false)
    , fontFamily("微软雅黑")
    , fontSize(14.0f)
    , fontWeight(FONT_WEIGHT_NORMAL)
    , fontStyle(FONT_NORMAL)
    , color(0, 0, 0, 255)
    , textAlign(ALIGN_LEFT)
    , lineHeight(0.0f)
    , textIndent(0, Length::PX)
    , underline(false)
    , lineThrough(false)
    , overline(false)
    , whiteSpace(WS_NORMAL)
    , verticalAlign(VALIGN_BASELINE)
    , opacity(1.0f)
    , blurRadius(0.0f)
    , grayscale(0.0f)
    , brightness(1.0f)
    , listStyleType(LIST_NONE)
    , listStyleInside(false)
    , flexDirection(FLEX_ROW)
    , flexWrap(WRAP_NOWRAP)
    , justifyContent(JUSTIFY_START)
    , alignItems(ALIGN_ITEMS_STRETCH)
    , alignSelf(ALIGN_ITEMS_STRETCH)
    , flexGrow(0.0f)
    , flexShrink(1.0f)
    , flexBasis(0, Length::AUTO)
    , gap(0.0f)
    , borderCollapse(0)
    , borderSpacing(0.0f)
    , overflowX(OVERFLOW_VISIBLE)
    , overflowY(OVERFLOW_VISIBLE)
    , cursor("auto")
    , letterSpacing(0, Length::PX)
    , wordSpacing(0, Length::PX)
{
}

// ============================ Node ============================

std::vector<std::string> Node::GetClasses() const
{
    std::vector<std::string> out;
    std::string cls = GetAttr("class");
    if (cls.empty()) return out;

    // 按空白切分
    std::string cur;
    for (size_t i = 0; i < cls.size(); i++) {
        if (isspace((unsigned char)cls[i])) {
            if (!cur.empty()) { out.push_back(cur); cur.clear(); }
        } else {
            cur += cls[i];
        }
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}

Node* Node::FindById(const std::string& id)
{
    if (IsElement() && GetId() == id) return this;
    for (size_t i = 0; i < children.size(); i++) {
        Node* r = children[i]->FindById(id);
        if (r != NULL) return r;
    }
    return NULL;
}

Node* Node::FindByAttr(const std::string& attrName, const std::string& value)
{
    if (IsElement() && GetAttr(attrName) == value) return this;
    for (size_t i = 0; i < children.size(); i++) {
        Node* r = children[i]->FindByAttr(attrName, value);
        if (r != NULL) return r;
    }
    return NULL;
}

float Node::GetContentWidth() const
{
    return contentW;
}

float Node::GetContentHeight() const
{
    return contentH;
}

bool Node::IsBlockByDefault() const
{
    return IsBlockTag(tagName);
}

bool Node::IsInlineByDefault() const
{
    return IsInlineTag(tagName);
}

bool Node::IsVoidElement() const
{
    return IsVoidTag(tagName);
}

bool Node::IsFormControl() const
{
    if (!IsElement()) return false;
    return tagName == "input" || tagName == "select" ||
           tagName == "button" || tagName == "textarea";
}

// ============================ DocumentTree ============================

DocumentTree::DocumentTree()
    : root(NULL), htmlNode(NULL), headNode(NULL), bodyNode(NULL)
{
    root = new Node(NODE_DOCUMENT);
}

DocumentTree::~DocumentTree()
{
    Clear();
}

void DocumentTree::Clear()
{
    if (root != NULL) {
        delete root;
        root = NULL;
    }
    htmlNode = headNode = bodyNode = NULL;
}

Node* DocumentTree::FindById(const std::string& id) const
{
    if (root == NULL) return NULL;
    return root->FindById(id);
}

Node* DocumentTree::FindByTag(const std::string& tag) const
{
    if (root == NULL) return NULL;
    return root->FindByAttr("__tag__", tag);
}

Node* DocumentTree::FindFormByName(const std::string& name) const
{
    if (root == NULL || name.empty()) return NULL;
    return root->FindByAttr("name", name);
}

// ============================ 标签分类 ============================

bool IsBlockTag(const std::string& tag)
{
    static const char* kBlocks[] = {
        "html", "body", "div", "p", "h1", "h2", "h3", "h4", "h5", "h6",
        "ul", "ol", "li", "dl", "dt", "dd", "blockquote", "pre", "hr",
        "header", "footer", "nav", "section", "article", "aside", "main",
        "figure", "figcaption", "form", "fieldset", "legend", "address",
        "table", "thead", "tbody", "tfoot", "tr", "caption",
        "details", "summary", "center", "dir", "menu", "search",
        "canvas", "video", "audio", "iframe", "noscript", "output",
        "hgroup", "optgroup", "dialog", "template", "head", "title", "style", "script", "meta", "link"
    };
    for (size_t i = 0; i < sizeof(kBlocks) / sizeof(kBlocks[0]); i++) {
        if (tag == kBlocks[i]) return true;
    }
    return false;
}

bool IsInlineTag(const std::string& tag)
{
    static const char* kInlines[] = {
        "a", "span", "b", "i", "u", "s", "em", "strong", "small", "big",
        "code", "kbd", "samp", "var", "cite", "dfn", "abbr", "acronym",
        "mark", "sub", "sup", "q", "strike", "tt", "font", "label",
        "br", "img", "input", "select", "button", "textarea", "output",
        "progress", "meter", "time", "data", "bdi", "bdo", "wbr", "ruby", "rt", "rp",
        "object", "embed", "param", "source", "track", "map", "area",
        "del", "ins", "picture"
    };
    for (size_t i = 0; i < sizeof(kInlines) / sizeof(kInlines[0]); i++) {
        if (tag == kInlines[i]) return true;
    }
    return false;
}

bool IsVoidTag(const std::string& tag)
{
    static const char* kVoid[] = {
        "area", "base", "br", "col", "embed", "hr", "img", "input",
        "link", "meta", "param", "source", "track", "wbr"
    };
    for (size_t i = 0; i < sizeof(kVoid) / sizeof(kVoid[0]); i++) {
        if (tag == kVoid[i]) return true;
    }
    return false;
}

} // namespace HtmlEngine
