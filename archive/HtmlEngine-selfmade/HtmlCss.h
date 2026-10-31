#ifndef HTML_ENGINE_CSS_H
#define HTML_ENGINE_CSS_H

// ==========================================================================
//  HtmlCss.h — 自研 CSS3 解析器 + 选择器匹配 + 级联
//
//  支持：
//    · 选择器：标签 / .class / #id / [attr] / [attr=val] / * /
//              后代(空格) / 子(>) / 相邻(+) / 兄弟(~)
//              伪类 :hover :active :first-child :last-child :nth-child(n)
//                    :not() :link :visited :focus :checked :disabled :enabled
//    · @media (screen/print/all)
//    · 属性：见 ApplyDeclaration()
// ==========================================================================

#include "HtmlDom.h"
#include <string>
#include <vector>

namespace HtmlEngine
{

// 简单选择器（复合选择器的一段）
struct SimpleSelector
{
    std::string tag;                    // 标签名（空 = 任意）
    std::string id;                     // #id
    std::vector<std::string> classes;   // .class
    struct AttrCond {
        std::string name;
        std::string op;      // "" = 仅存在; "=" "~=" "^=" "$=" "*="
        std::string value;
    };
    std::vector<AttrCond> attrs;
    std::vector<std::string> pseudoClasses;
    std::vector<int> nthChild;          // nth-child 参数（偶数=-1，奇数=-2）
    bool hasPseudoElement;              // ::before / ::after

    SimpleSelector() : hasPseudoElement(false) {}
};

// 组合符
enum Combinator { COMB_NONE, COMB_DESCENDANT, COMB_CHILD, COMB_ADJACENT, COMB_SIBLING };

// 完整选择器 = 简单选择器序列 + 组合符
struct ComplexSelector
{
    std::vector<SimpleSelector> parts;
    std::vector<Combinator>     combinators;   // parts.size()-1 个

    int specificity;            // 特异性（用于级联排序）
    int order;                 // 出现顺序
};

// 一条声明
struct Declaration
{
    std::string property;
    std::string value;
    bool        important;
};

// 一条规则
struct Rule
{
    std::vector<ComplexSelector> selectors;
    std::vector<Declaration>     declarations;
};

// CSS 解析器
class CssParser
{
public:
    CssParser();

    // 解析样式表文本，追加到内部规则表
    void ParseStylesheet(const std::string& css);

    // 清空
    void Clear();

    // 给整棵 DOM 树计算样式
    void ApplyToTree(DocumentTree& tree);

    // 基础默认样式（浏览器默认样式表）
    static void ApplyDefaultStyle(Node* node, ComputedStyle& style);

    // 应用一条声明到样式（公开，供内联 style 使用）
    static void ApplyDeclaration(const std::string& property, const std::string& value,
                                 ComputedStyle& style, float parentFontSize, float rootFontSize);

    // 从元素的 style="..." 属性应用内联样式
    static void ApplyInlineStyle(Node* node, ComputedStyle& style, float parentFontSize, float rootFontSize);

    // 设置/查询当前交互节点（供 :hover :active 伪类匹配）
    static void SetInteractiveNodes(Node* hover, Node* active);
    static Node* GetHoverNode();
    static Node* GetActiveNode();

private:
    std::vector<Rule> m_rules;

    // 计算样式：递归
    void ComputeNode(Node* node, Node* parent, float parentFontSize, float rootFontSize);

    // 匹配并收集该节点命中的所有声明（按特异性排序）
    void CollectMatched(Node* node, std::vector<Declaration>& out, bool bHover, bool bActive);

    // 匹配单个复杂选择器
    bool MatchesComplex(Node* node, const ComplexSelector& sel, bool bHover, bool bActive);

    // 匹配单个简单选择器
    bool MatchesSimple(Node* node, const SimpleSelector& sel, bool bHover, bool bActive);

    // 计算特异性（公开，供选择器解析使用）
public:
    static int CalcSpecificity(const ComplexSelector& sel);

private:
};

} // namespace HtmlEngine

#endif // HTML_ENGINE_CSS_H
