#include "StdAfx.h"
#include "HtmlCss.h"
#include <algorithm>

namespace HtmlEngine
{

// ============================ 默认样式表 ============================

// 浏览器默认样式（相当于 UA stylesheet）
void CssParser::ApplyDefaultStyle(Node* node, ComputedStyle& style)
{
    if (node == NULL || !node->IsElement()) return;

    std::string tag = node->tagName;

    // ---- 默认 display ----
    if (IsBlockTag(tag)) style.display = DISPLAY_BLOCK;
    else if (IsInlineTag(tag)) style.display = DISPLAY_INLINE;

    // 表单控件默认 inline-block
    if (tag == "input" || tag == "select" || tag == "button" || tag == "textarea") {
        style.display = DISPLAY_INLINE_BLOCK;
    }
    if (tag == "button") {
        style.display = DISPLAY_INLINE_BLOCK;
        style.textAlign = ALIGN_CENTER;
        style.paddingLeft = Length(12, Length::PX);
        style.paddingRight = Length(12, Length::PX);
        style.paddingTop = Length(4, Length::PX);
        style.paddingBottom = Length(4, Length::PX);
        style.backgroundColor = Color(240, 240, 240, 255);
        style.borderTop = BorderSide();
        style.borderTop.width = 1; style.borderTop.style = BORDER_SOLID;
        style.borderTop.color = Color(180, 180, 180, 255);
        style.borderRight = style.borderTop;
        style.borderBottom = style.borderTop;
        style.borderLeft = style.borderTop;
        style.borderRadius.tl = style.borderRadius.tr = 3;
        style.borderRadius.br = style.borderRadius.bl = 3;
        style.borderRadius.tlv = style.borderRadius.trv = 3;
        style.borderRadius.brv = style.borderRadius.blv = 3;
    }
    if (tag == "input") {
        style.paddingLeft = Length(4, Length::PX);
        style.paddingRight = Length(4, Length::PX);
        style.backgroundColor = Color(255, 255, 255, 255);
        style.borderTop = BorderSide();
        style.borderTop.width = 1; style.borderTop.style = BORDER_SOLID;
        style.borderTop.color = Color(170, 170, 170, 255);
        style.borderRight = style.borderTop;
        style.borderBottom = style.borderTop;
        style.borderLeft = style.borderTop;
        style.borderRadius.tl = style.borderRadius.tr = 2;
        style.borderRadius.br = style.borderRadius.bl = 2;
        style.borderRadius.tlv = style.borderRadius.trv = 2;
        style.borderRadius.brv = style.borderRadius.blv = 2;
    }
    if (tag == "select") {
        style.paddingLeft = Length(6, Length::PX);
        style.paddingRight = Length(6, Length::PX);
        style.backgroundColor = Color(255, 255, 255, 255);
        style.borderTop = BorderSide();
        style.borderTop.width = 1; style.borderTop.style = BORDER_SOLID;
        style.borderTop.color = Color(170, 170, 170, 255);
        style.borderRight = style.borderTop;
        style.borderBottom = style.borderTop;
        style.borderLeft = style.borderTop;
        style.borderRadius.tl = style.borderRadius.tr = 3;
        style.borderRadius.br = style.borderRadius.bl = 3;
        style.borderRadius.tlv = style.borderRadius.trv = 3;
        style.borderRadius.brv = style.borderRadius.blv = 3;
    }
    if (tag == "textarea") {
        style.paddingLeft = Length(4, Length::PX);
        style.paddingTop = Length(3, Length::PX);
        style.backgroundColor = Color(255, 255, 255, 255);
        style.borderTop = BorderSide();
        style.borderTop.width = 1; style.borderTop.style = BORDER_SOLID;
        style.borderTop.color = Color(170, 170, 170, 255);
        style.borderRight = style.borderTop;
        style.borderBottom = style.borderTop;
        style.borderLeft = style.borderTop;
    }
    if (tag == "input") {
        std::string type = ToLower(node->GetAttr("type", "text"));
        if (type == "hidden") style.display = DISPLAY_NONE;
        if (type == "checkbox" || type == "radio") {
            style.width = Length(13, Length::PX);
            style.height = Length(13, Length::PX);
            style.paddingLeft = Length(0, Length::PX);
            style.paddingRight = Length(0, Length::PX);
            style.backgroundColor = Color(255, 255, 255, 255);
        }
    }

    // ---- 标题默认字号/字重 ----
    if (tag == "h1") { style.fontSize = 28.0f; style.fontWeight = FONT_WEIGHT_BOLD;
                       style.marginTop = Length(18, Length::PX); style.marginBottom = Length(18, Length::PX); }
    if (tag == "h2") { style.fontSize = 22.0f; style.fontWeight = FONT_WEIGHT_BOLD;
                       style.marginTop = Length(16, Length::PX); style.marginBottom = Length(16, Length::PX); }
    if (tag == "h3") { style.fontSize = 18.0f; style.fontWeight = FONT_WEIGHT_BOLD;
                       style.marginTop = Length(14, Length::PX); style.marginBottom = Length(14, Length::PX); }
    if (tag == "h4") { style.fontSize = 16.0f; style.fontWeight = FONT_WEIGHT_BOLD;
                       style.marginTop = Length(12, Length::PX); style.marginBottom = Length(12, Length::PX); }
    if (tag == "h5") { style.fontSize = 14.0f; style.fontWeight = FONT_WEIGHT_BOLD;
                       style.marginTop = Length(10, Length::PX); style.marginBottom = Length(10, Length::PX); }
    if (tag == "h6") { style.fontSize = 12.0f; style.fontWeight = FONT_WEIGHT_BOLD;
                       style.marginTop = Length(10, Length::PX); style.marginBottom = Length(10, Length::PX); }

    if (tag == "p") {
        style.marginTop = Length(12, Length::PX);
        style.marginBottom = Length(12, Length::PX);
    }

    // ---- 行内语义标签 ----
    if (tag == "b" || tag == "strong") style.fontWeight = FONT_WEIGHT_BOLD;
    if (tag == "i" || tag == "em" || tag == "cite" || tag == "dfn" || tag == "var") style.fontStyle = FONT_ITALIC;
    if (tag == "u" || tag == "ins") style.underline = true;
    if (tag == "s" || tag == "del" || tag == "strike") style.lineThrough = true;
    if (tag == "small") style.fontSize = 12.0f;
    if (tag == "big")   style.fontSize = 17.0f;
    if (tag == "code" || tag == "kbd" || tag == "samp" || tag == "pre" || tag == "tt") {
        style.fontFamily = "Consolas";
    }
    if (tag == "mark") {
        style.backgroundColor = Color(255, 255, 0, 255);
    }
    if (tag == "a") {
        style.color = Color(0, 0, 238, 255);
        style.underline = true;
        style.cursor = "pointer";
    }
    if (tag == "sub") { style.fontSize = 11.0f; style.verticalAlign = VALIGN_BOTTOM; }
    if (tag == "sup") { style.fontSize = 11.0f; style.verticalAlign = VALIGN_TOP; }

    // ---- 块级元素外边距 ----
    if (tag == "body") {
        style.marginTop = Length(8, Length::PX);
        style.marginRight = Length(8, Length::PX);
        style.marginBottom = Length(8, Length::PX);
        style.marginLeft = Length(8, Length::PX);
    }
    if (tag == "ul" || tag == "ol") {
        style.marginTop = Length(12, Length::PX);
        style.marginBottom = Length(12, Length::PX);
        style.paddingLeft = Length(36, Length::PX);
        style.display = DISPLAY_BLOCK;
    }
    if (tag == "li") {
        style.display = DISPLAY_LIST_ITEM;
        style.listStyleType = LIST_DISC;
        style.marginTop = Length(2, Length::PX);
        style.marginBottom = Length(2, Length::PX);
    }
    if (tag == "ol") style.listStyleType = LIST_DECIMAL;

    if (tag == "blockquote") {
        style.marginTop = Length(12, Length::PX);
        style.marginBottom = Length(12, Length::PX);
        style.marginLeft = Length(36, Length::PX);
        style.marginRight = Length(36, Length::PX);
    }
    if (tag == "pre") {
        style.marginTop = Length(12, Length::PX);
        style.marginBottom = Length(12, Length::PX);
        style.whiteSpace = WS_PRE;
        style.backgroundColor = Color(245, 245, 245, 255);
        style.paddingLeft = Length(10, Length::PX);
        style.paddingTop = Length(8, Length::PX);
        style.paddingRight = Length(10, Length::PX);
        style.paddingBottom = Length(8, Length::PX);
        style.borderRadius.tl = style.borderRadius.tr = 4;
        style.borderRadius.br = style.borderRadius.bl = 4;
        style.borderRadius.tlv = style.borderRadius.trv = 4;
        style.borderRadius.brv = style.borderRadius.blv = 4;
    }

    if (tag == "hr") {
        style.display = DISPLAY_BLOCK;
        style.marginTop = Length(8, Length::PX);
        style.marginBottom = Length(8, Length::PX);
        style.borderTop = BorderSide();
        style.borderTop.width = 1;
        style.borderTop.style = BORDER_SOLID;
        style.borderTop.color = Color(200, 200, 200, 255);
        style.height = Length(0, Length::PX);
    }

    // ---- 表格 ----
    if (tag == "table") {
        style.display = DISPLAY_TABLE;
        style.borderCollapse = 0;
        style.marginTop = Length(0, Length::PX);
        style.marginBottom = Length(0, Length::PX);
    }
    if (tag == "tr") style.display = DISPLAY_TABLE_ROW;
    if (tag == "td" || tag == "th") {
        style.display = DISPLAY_TABLE_CELL;
        style.paddingLeft = Length(6, Length::PX);
        style.paddingRight = Length(6, Length::PX);
        style.paddingTop = Length(4, Length::PX);
        style.paddingBottom = Length(4, Length::PX);
    }
    if (tag == "th") style.fontWeight = FONT_WEIGHT_BOLD;
    if (tag == "thead" || tag == "tbody" || tag == "tfoot") {
        style.display = DISPLAY_TABLE_ROW_GROUP;
    }
    if (tag == "caption") style.textAlign = ALIGN_CENTER;

    // ---- 其它块 ----
    if (tag == "header" || tag == "footer" || tag == "nav" ||
        tag == "section" || tag == "article" || tag == "aside" || tag == "main" ||
        tag == "figure" || tag == "figcaption" || tag == "fieldset" ||
        tag == "legend" || tag == "details" || tag == "summary" ||
        tag == "address" || tag == "dl" || tag == "dt" || tag == "dd") {
        style.display = DISPLAY_BLOCK;
    }
    if (tag == "dd") style.marginLeft = Length(40, Length::PX);
    if (tag == "fieldset") {
        style.marginTop = Length(12, Length::PX);
        style.marginBottom = Length(12, Length::PX);
        style.paddingLeft = Length(10, Length::PX);
        style.paddingRight = Length(10, Length::PX);
        style.paddingTop = Length(8, Length::PX);
        style.paddingBottom = Length(8, Length::PX);
        style.borderTop = BorderSide();
        style.borderTop.width = 1;
        style.borderTop.style = BORDER_SOLID;
        style.borderTop.color = Color(200, 200, 200, 255);
        style.borderRight = style.borderTop;
        style.borderBottom = style.borderTop;
        style.borderLeft = style.borderTop;
        style.borderRadius.tl = style.borderRadius.tr = 4;
        style.borderRadius.br = style.borderRadius.bl = 4;
        style.borderRadius.tlv = style.borderRadius.trv = 4;
        style.borderRadius.brv = style.borderRadius.blv = 4;
    }

    // ---- 隐藏元素 ----
    if (tag == "head" || tag == "title" || tag == "meta" || tag == "link" ||
        tag == "style" || tag == "script" || tag == "base" || tag == "template") {
        style.display = DISPLAY_NONE;
    }

    // ---- img 默认 ----
    if (tag == "img") {
        style.display = DISPLAY_INLINE_BLOCK;
    }

    // ---- br ----
    if (tag == "br") {
        style.display = DISPLAY_INLINE;
    }
}

// ============================ 选择器解析 ============================

// 解析单个简单选择器，如 "div#main.card:hover"
static bool ParseSimpleSelector(const std::string& s, SimpleSelector& out)
{
    size_t i = 0;
    size_t n = s.size();

    // 标签名
    while (i < n && (isalnum((unsigned char)s[i]) || s[i] == '-' || s[i] == '_' || s[i] == '*')) {
        out.tag += s[i];
        i++;
    }
    if (out.tag == "*") out.tag.clear();

    while (i < n) {
        if (s[i] == '#') {
            i++;
            while (i < n && (isalnum((unsigned char)s[i]) || s[i] == '-' || s[i] == '_')) {
                out.id += s[i];
                i++;
            }
        }
        else if (s[i] == '.') {
            i++;
            std::string cls;
            while (i < n && (isalnum((unsigned char)s[i]) || s[i] == '-' || s[i] == '_')) {
                cls += s[i];
                i++;
            }
            if (!cls.empty()) out.classes.push_back(cls);
        }
        else if (s[i] == '[') {
            // 属性选择器
            size_t end = s.find(']', i);
            if (end == std::string::npos) break;
            std::string inner = Trim(s.substr(i + 1, end - i - 1));

            SimpleSelector::AttrCond cond;
            size_t opPos = inner.find_first_of("~^$*=");
            if (opPos == std::string::npos) {
                cond.name = ToLower(inner);
            } else {
                cond.name = ToLower(Trim(inner.substr(0, opPos)));
                size_t vp = opPos;
                std::string op;
                if (inner[opPos] == '=') { op = "="; vp = opPos + 1; }
                else if (opPos + 1 < inner.size() && inner[opPos + 1] == '=') {
                    op = inner.substr(opPos, 2);
                    vp = opPos + 2;
                } else { op = "="; vp = opPos + 1; }
                std::string val = Trim(inner.substr(vp));
                // 去掉引号
                if (val.size() >= 2 && (val[0] == '"' || val[0] == '\'')) {
                    val = val.substr(1, val.size() - 2);
                }
                cond.op = op;
                cond.value = val;
            }
            out.attrs.push_back(cond);
            i = end + 1;
        }
        else if (s[i] == ':') {
            i++;
            if (i < n && s[i] == ':') { i++; out.hasPseudoElement = true; }

            std::string pc;
            while (i < n && (isalnum((unsigned char)s[i]) || s[i] == '-' || s[i] == '_')) {
                pc += s[i];
                i++;
            }
            std::string pcLow = ToLower(pc);

            // 带参数的伪类
            if (i < n && s[i] == '(') {
                size_t end = s.find(')', i);
                if (end != std::string::npos) {
                    std::string arg = Trim(s.substr(i + 1, end - i - 1));
                    i = end + 1;

                    if (pcLow == "nth-child" || pcLow == "nth-of-type") {
                        std::string a = ToLower(arg);
                        if (a == "odd")       out.nthChild.push_back(-2);
                        else if (a == "even") out.nthChild.push_back(-1);
                        else {
                            // 支持 "2n+1" 形式，简化：取数字
                            size_t plus = a.find('+');
                            if (plus != std::string::npos) {
                                out.nthChild.push_back(atoi(a.substr(plus + 1).c_str()));
                            } else {
                                out.nthChild.push_back(atoi(a.c_str()));
                            }
                        }
                    }
                    else if (pcLow == "not") {
                        // :not(.cls) 简化处理：当作 class 排除
                        out.pseudoClasses.push_back("not:" + arg);
                    }
                    continue;
                }
            }

            if (!pcLow.empty()) out.pseudoClasses.push_back(pcLow);
        }
        else {
            i++;
        }
    }

    return true;
}

// 解析完整选择器串（含组合符），如 "div > .card .title"
static ComplexSelector ParseComplexSelector(const std::string& s)
{
    ComplexSelector sel;

    // 按空白和 > + ~ 切分，保留组合符
    std::vector<std::string> tokens;
    std::vector<Combinator> combs;

    std::string cur;
    size_t i = 0;
    bool lastWasSpace = false;

    while (i < s.size()) {
        char ch = s[i];
        if (isspace((unsigned char)ch)) {
            if (!cur.empty()) {
                tokens.push_back(cur);
                cur.clear();
                lastWasSpace = true;
            }
            i++;
            continue;
        }
        if (ch == '>' || ch == '+' || ch == '~') {
            if (!cur.empty()) {
                tokens.push_back(cur);
                cur.clear();
            }
            if (tokens.empty()) { i++; continue; }
            // 上一个 token 与这一个之间是组合符
            if (combs.size() < tokens.size()) {
                combs.push_back(ch == '>' ? COMB_CHILD : (ch == '+' ? COMB_ADJACENT : COMB_SIBLING));
            }
            lastWasSpace = false;
            i++;
            continue;
        }
        if (lastWasSpace && !cur.empty()) {
            // 空白作为后代组合符
        }
        if (lastWasSpace && cur.empty() && !tokens.empty()) {
            if (combs.size() < tokens.size()) combs.push_back(COMB_DESCENDANT);
        }
        cur += ch;
        lastWasSpace = false;
        i++;
    }
    if (!cur.empty()) tokens.push_back(cur);

    // 组合符补齐（默认后代）
    while (combs.size() < (tokens.size() > 0 ? tokens.size() - 1 : 0)) {
        combs.push_back(COMB_DESCENDANT);
    }

    for (size_t k = 0; k < tokens.size(); k++) {
        SimpleSelector ss;
        ParseSimpleSelector(tokens[k], ss);
        sel.parts.push_back(ss);
    }
    sel.combinators = combs;
    sel.specificity = CssParser::CalcSpecificity(sel);
    sel.order = 0;
    return sel;
}

// 计算特异性
int CssParser::CalcSpecificity(const ComplexSelector& sel)
{
    int a = 0, b = 0, c = 0;   // a=id b=class/attr/pseudo c=tag
    for (size_t i = 0; i < sel.parts.size(); i++) {
        const SimpleSelector& ss = sel.parts[i];
        if (!ss.id.empty()) a++;
        b += (int)ss.classes.size();
        b += (int)ss.attrs.size();
        b += (int)ss.pseudoClasses.size();
        if (!ss.tag.empty()) c++;
    }
    return a * 10000 + b * 100 + c;
}

// ============================ 样式表解析 ============================

CssParser::CssParser()
{
}

void CssParser::Clear()
{
    m_rules.clear();
}

void CssParser::ParseStylesheet(const std::string& css)
{
    std::string s = css;

    // 去掉注释 /* ... */
    std::string clean;
    size_t i = 0;
    while (i < s.size()) {
        if (i + 1 < s.size() && s[i] == '/' && s[i + 1] == '*') {
            size_t end = s.find("*/", i + 2);
            if (end == std::string::npos) break;
            i = end + 2;
            continue;
        }
        clean += s[i];
        i++;
    }
    s = clean;

    size_t pos = 0;
    while (pos < s.size()) {
        // 跳过空白
        while (pos < s.size() && isspace((unsigned char)s[pos])) pos++;
        if (pos >= s.size()) break;

        // @media 处理
        if (s[pos] == '@') {
            size_t braceOpen = s.find('{', pos);
            if (braceOpen == std::string::npos) break;

            std::string atRule = Trim(s.substr(pos, braceOpen - pos));

            // 找到匹配的 }
            int depth = 1;
            size_t k = braceOpen + 1;
            while (k < s.size() && depth > 0) {
                if (s[k] == '{') depth++;
                else if (s[k] == '}') depth--;
                k++;
            }
            std::string inner = s.substr(braceOpen + 1, (k > 0 ? k - 1 : k) - braceOpen - 1);

            std::string atLow = ToLower(atRule);
            // 只处理 screen / all / 无媒体条件的
            bool bApply = true;
            if (atLow.find("@media") == 0) {
                if (atLow.find("print") != std::string::npos &&
                    atLow.find("screen") == std::string::npos) {
                    bApply = false;
                }
            } else if (atLow.find("@supports") == 0 || atLow.find("@keyframes") == 0) {
                bApply = false;
            }

            if (bApply) ParseStylesheet(inner);

            pos = k;
            continue;
        }

        // 普通规则：selector { decls }
        size_t braceOpen = s.find('{', pos);
        if (braceOpen == std::string::npos) break;

        std::string selectorText = Trim(s.substr(pos, braceOpen - pos));

        int depth = 1;
        size_t k = braceOpen + 1;
        while (k < s.size() && depth > 0) {
            if (s[k] == '{') depth++;
            else if (s[k] == '}') depth--;
            k++;
        }
        std::string body = s.substr(braceOpen + 1, (k > 0 ? k - 1 : k) - braceOpen - 1);
        pos = k;

        if (selectorText.empty()) continue;

        Rule rule;

        // 选择器列表（逗号分隔）
        std::vector<std::string> selParts = SplitTopLevel(selectorText, ',');
        for (size_t si = 0; si < selParts.size(); si++) {
            std::string one = Trim(selParts[si]);
            if (one.empty()) continue;
            ComplexSelector cs = ParseComplexSelector(one);
            if (!cs.parts.empty()) rule.selectors.push_back(cs);
        }

        // 声明列表
        std::vector<std::string> declParts = SplitTopLevel(body, ';');
        for (size_t di = 0; di < declParts.size(); di++) {
            std::string d = Trim(declParts[di]);
            if (d.empty()) continue;

            size_t colon = d.find(':');
            if (colon == std::string::npos) continue;

            Declaration decl;
            decl.property = ToLower(Trim(d.substr(0, colon)));
            decl.value = Trim(d.substr(colon + 1));
            decl.important = false;

            // !important
            std::string lowVal = ToLower(decl.value);
            size_t imp = lowVal.find("!important");
            if (imp != std::string::npos) {
                decl.important = true;
                decl.value = Trim(decl.value.substr(0, imp));
            }

            if (!decl.property.empty() && !decl.value.empty()) {
                rule.declarations.push_back(decl);
            }
        }

        if (!rule.selectors.empty() && !rule.declarations.empty()) {
            for (size_t ri = 0; ri < rule.selectors.size(); ri++) {
                rule.selectors[ri].order = (int)m_rules.size();
            }
            m_rules.push_back(rule);
        }
    }
}

// ============================ 选择器匹配 ============================

bool CssParser::MatchesSimple(Node* node, const SimpleSelector& sel, bool bHover, bool bActive)
{
    if (node == NULL || !node->IsElement()) return false;

    // 伪元素不匹配普通元素
    if (sel.hasPseudoElement) return false;

    // 标签
    if (!sel.tag.empty() && node->tagName != sel.tag) return false;

    // ID
    if (!sel.id.empty() && node->GetId() != sel.id) return false;

    // class
    if (!sel.classes.empty()) {
        std::vector<std::string> nodeClasses = node->GetClasses();
        for (size_t i = 0; i < sel.classes.size(); i++) {
            bool bFound = false;
            for (size_t k = 0; k < nodeClasses.size(); k++) {
                if (nodeClasses[k] == sel.classes[i]) { bFound = true; break; }
            }
            if (!bFound) return false;
        }
    }

    // 属性
    for (size_t i = 0; i < sel.attrs.size(); i++) {
        const SimpleSelector::AttrCond& cond = sel.attrs[i];
        if (!node->HasAttr(cond.name)) return false;
        std::string v = node->GetAttr(cond.name);

        if (cond.op == "=") {
            if (v != cond.value) return false;
        } else if (cond.op == "~=") {
            std::vector<std::string> words = Split(v, ' ');
            bool bFound = false;
            for (size_t k = 0; k < words.size(); k++) {
                if (words[k] == cond.value) { bFound = true; break; }
            }
            if (!bFound) return false;
        } else if (cond.op == "^=") {
            if (v.size() < cond.value.size() || v.substr(0, cond.value.size()) != cond.value) return false;
        } else if (cond.op == "$=") {
            if (v.size() < cond.value.size() || v.substr(v.size() - cond.value.size()) != cond.value) return false;
        } else if (cond.op == "*=") {
            if (v.find(cond.value) == std::string::npos) return false;
        }
    }

    // 伪类
    for (size_t i = 0; i < sel.pseudoClasses.size(); i++) {
        std::string pc = sel.pseudoClasses[i];

        if (pc == "hover")  { if (!bHover) return false; }
        else if (pc == "active") { if (!bActive) return false; }
        else if (pc == "first-child") {
            if (node->parent == NULL) return false;
            bool bFirst = true;
            for (size_t k = 0; k < node->parent->children.size(); k++) {
                if (node->parent->children[k] == node) break;
                if (node->parent->children[k]->IsElement()) { bFirst = false; break; }
            }
            if (!bFirst) return false;
        }
        else if (pc == "last-child") {
            if (node->parent == NULL) return false;
            bool bLast = true;
            for (size_t k = node->parent->children.size(); k > 0; k--) {
                if (node->parent->children[k - 1] == node) break;
                if (node->parent->children[k - 1]->IsElement()) { bLast = false; break; }
            }
            if (!bLast) return false;
        }
        else if (pc == "only-child") {
            if (node->parent == NULL) return false;
            int count = 0;
            for (size_t k = 0; k < node->parent->children.size(); k++) {
                if (node->parent->children[k]->IsElement()) count++;
            }
            if (count != 1) return false;
        }
        else if (pc == "first-of-type" || pc == "last-of-type") {
            if (node->parent == NULL) return false;
            int idx = 0, total = 0, myIdx = 0;
            for (size_t k = 0; k < node->parent->children.size(); k++) {
                Node* sib = node->parent->children[k];
                if (!sib->IsElement() || sib->tagName != node->tagName) continue;
                total++;
                if (sib == node) myIdx = total;
            }
            if (pc == "first-of-type" && myIdx != 1) return false;
            if (pc == "last-of-type" && myIdx != total) return false;
        }
        else if (pc == "link") {
            if (node->tagName != "a" || !node->HasAttr("href")) return false;
        }
        else if (pc == "checked") {
            if (!node->HasAttr("checked")) return false;
        }
        else if (pc == "disabled") {
            if (!node->HasAttr("disabled")) return false;
        }
        else if (pc == "enabled") {
            if (node->HasAttr("disabled")) return false;
            if (!node->IsFormControl()) return false;
        }
        else if (pc == "focus") {
            // 简化：不跟踪焦点
            return false;
        }
        else if (pc == "root") {
            if (node->tagName != "html") return false;
        }
        else if (pc == "empty") {
            if (!node->children.empty()) return false;
        }
        else if (pc.find("not:") == 0) {
            std::string inner = pc.substr(4);
            std::string innerLow = ToLower(Trim(inner));
            if (innerLow[0] == '.') {
                std::string cls = innerLow.substr(1);
                std::vector<std::string> nodeClasses = node->GetClasses();
                for (size_t k = 0; k < nodeClasses.size(); k++) {
                    if (nodeClasses[k] == cls) return false;
                }
            }
        }
        else {
            // 未识别伪类：忽略（不影响匹配）
        }
    }

    // nth-child
    for (size_t i = 0; i < sel.nthChild.size(); i++) {
        if (node->parent == NULL) return false;
        int idx = 0;
        for (size_t k = 0; k < node->parent->children.size(); k++) {
            if (node->parent->children[k] == node) break;
            if (node->parent->children[k]->IsElement()) idx++;
        }
        int myIdx = idx + 1;   // 1-based
        int param = sel.nthChild[i];
        if (param == -2) { if (myIdx % 2 != 1) return false; }        // odd
        else if (param == -1) { if (myIdx % 2 != 0) return false; }   // even
        else if (param > 0) { if (myIdx != param) return false; }
    }

    return true;
}

bool CssParser::MatchesComplex(Node* node, const ComplexSelector& sel, bool bHover, bool bActive)
{
    if (sel.parts.empty()) return false;

    // 从右往左匹配
    Node* cur = node;
    size_t pi = sel.parts.size();

    if (!MatchesSimple(cur, sel.parts[pi - 1], bHover, bActive)) return false;
    pi--;

    while (pi > 0) {
        Combinator comb = (pi - 1 < sel.combinators.size()) ? sel.combinators[pi - 1] : COMB_DESCENDANT;
        const SimpleSelector& target = sel.parts[pi - 1];

        if (comb == COMB_CHILD) {
            cur = cur->parent;
            if (cur == NULL || !MatchesSimple(cur, target, bHover, bActive)) return false;
        }
        else if (comb == COMB_ADJACENT) {
            if (cur->parent == NULL) return false;
            Node* prev = NULL;
            for (size_t k = 0; k < cur->parent->children.size(); k++) {
                if (cur->parent->children[k] == cur) break;
                if (cur->parent->children[k]->IsElement()) prev = cur->parent->children[k];
            }
            if (prev == NULL || !MatchesSimple(prev, target, bHover, bActive)) return false;
            cur = prev;
        }
        else if (comb == COMB_SIBLING) {
            if (cur->parent == NULL) return false;
            Node* prev = NULL;
            bool bFound = false;
            for (size_t k = 0; k < cur->parent->children.size(); k++) {
                if (cur->parent->children[k] == cur) break;
                if (cur->parent->children[k]->IsElement()) prev = cur->parent->children[k];
            }
            while (prev != NULL) {
                if (MatchesSimple(prev, target, bHover, bActive)) { bFound = true; break; }
                // 再往前
                Node* pp = NULL;
                for (size_t k = 0; k < prev->parent->children.size(); k++) {
                    if (prev->parent->children[k] == prev) break;
                    if (prev->parent->children[k]->IsElement()) pp = prev->parent->children[k];
                }
                prev = pp;
            }
            if (!bFound) return false;
            cur = prev;
        }
        else {
            // 后代：向上找任意祖先
            Node* p = cur->parent;
            bool bFound = false;
            while (p != NULL) {
                if (MatchesSimple(p, target, bHover, bActive)) { bFound = true; cur = p; break; }
                p = p->parent;
            }
            if (!bFound) return false;
        }

        pi--;
    }

    return true;
}

// ============================ 级联 ============================

void CssParser::CollectMatched(Node* node, std::vector<Declaration>& out, bool bHover, bool bActive)
{
    // 收集所有匹配的声明，带上特异性与顺序用于排序
    struct Matched {
        Declaration decl;
        int spec;
        int order;
    };
    std::vector<Matched> matched;

    for (size_t r = 0; r < m_rules.size(); r++) {
        const Rule& rule = m_rules[r];
        int bestSpec = -1;
        int bestOrder = -1;

        for (size_t s = 0; s < rule.selectors.size(); s++) {
            if (MatchesComplex(node, rule.selectors[s], bHover, bActive)) {
                if (rule.selectors[s].specificity > bestSpec) {
                    bestSpec = rule.selectors[s].specificity;
                    bestOrder = rule.selectors[s].order;
                }
            }
        }
        if (bestSpec < 0) continue;

        for (size_t d = 0; d < rule.declarations.size(); d++) {
            Matched m;
            m.decl = rule.declarations[d];
            m.spec = bestSpec;
            m.order = bestOrder;
            matched.push_back(m);
        }
    }

    // 按 (important, spec, order) 升序排序，后者覆盖前者
    for (size_t i = 0; i < matched.size(); i++) {
        for (size_t k = i + 1; k < matched.size(); k++) {
            bool bSwap = false;
            if (matched[k].decl.important != matched[i].decl.important) {
                bSwap = (matched[i].decl.important && !matched[k].decl.important);
            } else if (matched[k].spec != matched[i].spec) {
                bSwap = (matched[k].spec < matched[i].spec);
            } else if (matched[k].order != matched[i].order) {
                bSwap = (matched[k].order < matched[i].order);
            }
            if (bSwap) {
                Matched tmp = matched[i];
                matched[i] = matched[k];
                matched[k] = tmp;
            }
        }
    }

    for (size_t i = 0; i < matched.size(); i++) {
        out.push_back(matched[i].decl);
    }
}

void CssParser::ComputeNode(Node* node, Node* parent, float parentFontSize, float rootFontSize)
{
    if (node == NULL) return;

    // ---- 继承属性从父节点复制 ----
    if (parent != NULL && parent->IsElement()) {
        const ComputedStyle& ps = parent->style;

        node->style.fontFamily   = ps.fontFamily;
        node->style.fontSize     = ps.fontSize;
        node->style.fontWeight   = ps.fontWeight;
        node->style.fontStyle    = ps.fontStyle;
        node->style.color        = ps.color;
        node->style.textAlign    = ps.textAlign;
        node->style.lineHeight   = ps.lineHeight;
        node->style.whiteSpace   = ps.whiteSpace;
        node->style.letterSpacing = ps.letterSpacing;
        node->style.wordSpacing  = ps.wordSpacing;
        node->style.textIndent   = ps.textIndent;
        node->style.visibility   = ps.visibility;
        node->style.cursor       = ps.cursor;
        node->style.listStyleType = ps.listStyleType;
        node->style.listStyleInside = ps.listStyleInside;
        node->style.borderCollapse = ps.borderCollapse;
        node->style.borderSpacing  = ps.borderSpacing;
        node->style.underline    = ps.underline;
        node->style.lineThrough  = ps.lineThrough;
        node->style.overline     = ps.overline;
    }

    // 文本节点：只继承字体信息，不需要布局
    if (node->IsText()) {
        node->style.fontFamily = node->style.fontFamily;
        for (size_t i = 0; i < node->children.size(); i++) {
            ComputeNode(node->children[i], node, node->style.fontSize, rootFontSize);
        }
        return;
    }

    if (!node->IsElement()) {
        for (size_t i = 0; i < node->children.size(); i++) {
            ComputeNode(node->children[i], node, parentFontSize, rootFontSize);
        }
        return;
    }

    // ---- 应用默认样式 ----
    ApplyDefaultStyle(node, node->style);

    // 注意：ApplyDefaultStyle 会重置继承来的值，这里重新补回
    if (parent != NULL && parent->IsElement()) {
        const ComputedStyle& ps = parent->style;
        // 默认样式里没设的继承属性，用父级
        if (node->style.fontFamily == "微软雅黑" && ps.fontFamily != "微软雅黑")
            node->style.fontFamily = ps.fontFamily;
        if (node->style.fontSize == 14.0f && ps.fontSize != 14.0f)
            node->style.fontSize = ps.fontSize;
        if (node->style.color == Color(0, 0, 0, 255) && ps.color != Color(0, 0, 0, 255))
            node->style.color = ps.color;
        if (node->style.textAlign == ALIGN_LEFT)
            node->style.textAlign = ps.textAlign;
        if (node->style.whiteSpace == WS_NORMAL)
            node->style.whiteSpace = ps.whiteSpace;
        if (node->style.cursor == "auto")
            node->style.cursor = ps.cursor;
    }

    // ---- 应用匹配的 CSS 规则 ----
    std::vector<Declaration> decls;
    CollectMatched(node, decls, false, false);
    for (size_t i = 0; i < decls.size(); i++) {
        ApplyDeclaration(decls[i].property, decls[i].value, node->style,
                         parentFontSize, rootFontSize);
    }

    // ---- 应用内联 style ----
    ApplyInlineStyle(node, node->style, parentFontSize, rootFontSize);

    // ---- 递归子节点 ----
    for (size_t i = 0; i < node->children.size(); i++) {
        ComputeNode(node->children[i], node, node->style.fontSize, rootFontSize);
    }
}

void CssParser::ApplyToTree(DocumentTree& tree)
{
    if (tree.root == NULL) return;

    // 根字号
    float rootFontSize = 16.0f;

    // 从 body 开始计算（跳过 html 的继承噪声）
    Node* start = tree.bodyNode != NULL ? tree.bodyNode : tree.root;

    if (tree.bodyNode != NULL) {
        // body 自身：父级是 html
        ComputeNode(tree.bodyNode, tree.htmlNode, rootFontSize, rootFontSize);
    } else {
        for (size_t i = 0; i < tree.root->children.size(); i++) {
            ComputeNode(tree.root->children[i], NULL, rootFontSize, rootFontSize);
        }
    }
}

// ============================ 声明应用 ============================

// 解析边框简写：1px solid #ccc
static void ParseBorderShorthand(const std::string& val, BorderSide& side, float fontSize, float rootFontSize)
{
    std::vector<std::string> tokens;
    std::string cur;
    int depth = 0;
    for (size_t i = 0; i < val.size(); i++) {
        char ch = val[i];
        if (ch == '(') depth++;
        else if (ch == ')') depth--;
        if (isspace((unsigned char)ch) && depth == 0) {
            if (!cur.empty()) { tokens.push_back(cur); cur.clear(); }
        } else {
            cur += ch;
        }
    }
    if (!cur.empty()) tokens.push_back(cur);

    for (size_t i = 0; i < tokens.size(); i++) {
        std::string t = ToLower(tokens[i]);
        if (t == "none" || t == "hidden") { side.style = BORDER_NONE; side.width = 0; }
        else if (t == "solid")  side.style = BORDER_SOLID;
        else if (t == "dashed") side.style = BORDER_DASHED;
        else if (t == "dotted") side.style = BORDER_DOTTED;
        else if (t == "double") side.style = BORDER_DOUBLE;
        else if (t == "groove") side.style = BORDER_GROOVE;
        else if (t == "ridge")  side.style = BORDER_RIDGE;
        else if (Color::IsColorString(tokens[i])) side.color = Color::FromString(tokens[i]);
        else if (t == "thin")   side.width = 1.0f;
        else if (t == "medium") side.width = 3.0f;
        else if (t == "thick")  side.width = 5.0f;
        else {
            Length len = Length::FromString(tokens[i]);
            if (!len.IsAuto()) side.width = len.Resolve(0, fontSize, rootFontSize);
        }
    }
    if (side.width > 0 && side.style == BORDER_NONE) side.style = BORDER_SOLID;
    if (side.style != BORDER_NONE && side.color.a == 0) side.color = Color(0, 0, 0, 255);
}

// 解析四值简写：margin/padding
static void ParseFourLengths(const std::string& val, Length& top, Length& right,
                             Length& bottom, Length& left)
{
    std::vector<std::string> parts;
    std::string cur;
    for (size_t i = 0; i < val.size(); i++) {
        if (isspace((unsigned char)val[i])) {
            if (!cur.empty()) { parts.push_back(cur); cur.clear(); }
        } else cur += val[i];
    }
    if (!cur.empty()) parts.push_back(cur);

    if (parts.size() == 1) {
        top = right = bottom = left = Length::FromString(parts[0]);
    } else if (parts.size() == 2) {
        top = bottom = Length::FromString(parts[0]);
        right = left = Length::FromString(parts[1]);
    } else if (parts.size() == 3) {
        top = Length::FromString(parts[0]);
        right = left = Length::FromString(parts[1]);
        bottom = Length::FromString(parts[2]);
    } else if (parts.size() >= 4) {
        top = Length::FromString(parts[0]);
        right = Length::FromString(parts[1]);
        bottom = Length::FromString(parts[2]);
        left = Length::FromString(parts[3]);
    }
}

// 解析 box-shadow / text-shadow
static void ParseShadows(const std::string& val, std::vector<Shadow>& out,
                         float fontSize, float rootFontSize)
{
    out.clear();
    std::vector<std::string> groups = SplitTopLevel(val, ',');

    for (size_t g = 0; g < groups.size(); g++) {
        std::string group = Trim(groups[g]);
        if (group.empty()) continue;

        std::vector<std::string> tokens;
        std::string cur;
        int depth = 0;
        for (size_t i = 0; i < group.size(); i++) {
            char ch = group[i];
            if (ch == '(') depth++;
            else if (ch == ')') depth--;
            if (isspace((unsigned char)ch) && depth == 0) {
                if (!cur.empty()) { tokens.push_back(cur); cur.clear(); }
            } else cur += ch;
        }
        if (!cur.empty()) tokens.push_back(cur);

        Shadow sh;
        int numIdx = 0;
        for (size_t i = 0; i < tokens.size(); i++) {
            std::string t = tokens[i];
            std::string tl = ToLower(t);
            if (tl == "inset") { sh.inset = true; continue; }
            if (Color::IsColorString(t)) { sh.color = Color::FromString(t); continue; }

            Length len = Length::FromString(t);
            float v = len.Resolve(0, fontSize, rootFontSize);
            if (numIdx == 0) sh.x = v;
            else if (numIdx == 1) sh.y = v;
            else if (numIdx == 2) sh.blur = v;
            else if (numIdx == 3) sh.spread = v;
            numIdx++;
        }
        if (numIdx >= 2) out.push_back(sh);
    }
}

// 解析渐变：linear-gradient(...) / radial-gradient(...)
static bool ParseGradient(const std::string& val, Gradient& out)
{
    std::string low = ToLower(val);
    bool bRadial = false;

    if (low.find("linear-gradient") != std::string::npos) bRadial = false;
    else if (low.find("radial-gradient") != std::string::npos) bRadial = true;
    else return false;

    size_t p1 = val.find('(');
    size_t p2 = val.rfind(')');
    if (p1 == std::string::npos || p2 == std::string::npos || p2 <= p1) return false;

    std::string inner = val.substr(p1 + 1, p2 - p1 - 1);
    std::vector<std::string> parts = SplitTopLevel(inner, ',');

    out.Clear();
    out.radial = bRadial;

    size_t startIdx = 0;

    // 第一段可能是角度或方向
    if (!parts.empty()) {
        std::string first = ToLower(Trim(parts[0]));
        bool bAngle = false;

        if (first.find("deg") != std::string::npos) {
            out.angle = (float)atof(first.c_str());
            bAngle = true;
        } else if (first.find("rad") != std::string::npos) {
            out.angle = (float)atof(first.c_str()) * 180.0f / 3.14159265f;
            bAngle = true;
        } else if (first.find("turn") != std::string::npos) {
            out.angle = (float)atof(first.c_str()) * 360.0f;
            bAngle = true;
        } else if (first.find("to ") != std::string::npos) {
            bAngle = true;
            if (first.find("right") != std::string::npos) out.angle = 90.0f;
            else if (first.find("left") != std::string::npos) out.angle = 270.0f;
            else if (first.find("top") != std::string::npos) out.angle = 0.0f;
            else out.angle = 180.0f;

            // "to bottom right" 之类
            if (first.find("right") != std::string::npos && first.find("bottom") != std::string::npos) out.angle = 135.0f;
            if (first.find("left") != std::string::npos && first.find("bottom") != std::string::npos) out.angle = 225.0f;
            if (first.find("right") != std::string::npos && first.find("top") != std::string::npos) out.angle = 45.0f;
            if (first.find("left") != std::string::npos && first.find("top") != std::string::npos) out.angle = 315.0f;
        } else if (bRadial && (first.find("circle") != std::string::npos ||
                               first.find("ellipse") != std::string::npos)) {
            bAngle = true;
        }

        if (bAngle) startIdx = 1;
    }

    for (size_t i = startIdx; i < parts.size(); i++) {
        std::string token = Trim(parts[i]);
        if (token.empty()) continue;

        GradientStop stop;
        stop.hasPos = false;
        stop.pos = 0.0f;

        // 找百分比位置
        size_t pct = token.find('%');
        std::string colorPart = token;

        if (pct != std::string::npos) {
            size_t numStart = pct;
            while (numStart > 0 && (isdigit((unsigned char)token[numStart - 1]) ||
                                    token[numStart - 1] == '.' || token[numStart - 1] == '-')) {
                numStart--;
            }
            stop.pos = (float)atof(token.substr(numStart, pct - numStart).c_str()) / 100.0f;
            stop.hasPos = true;
            colorPart = Trim(token.substr(0, numStart));
        }

        if (Color::IsColorString(colorPart)) {
            stop.color = Color::FromString(colorPart);
            out.stops.push_back(stop);
        }
    }

    if (out.stops.size() < 2) { out.Clear(); return false; }

    // 自动分配位置
    float lastPos = 0.0f;
    for (size_t i = 0; i < out.stops.size(); i++) {
        if (!out.stops[i].hasPos) {
            out.stops[i].pos = (i == 0) ? 0.0f :
                ((i == out.stops.size() - 1) ? 1.0f : (float)i / (float)(out.stops.size() - 1));
        }
        if (out.stops[i].pos < lastPos) out.stops[i].pos = lastPos;
        lastPos = out.stops[i].pos;
    }

    return true;
}

// 解析 border-radius
static void ParseBorderRadius(const std::string& val, BorderRadius& br, float fontSize, float rootFontSize)
{
    std::vector<std::string> groups = SplitTopLevel(val, '/');
    if (groups.size() > 2) return;

    // 水平半径
    std::vector<std::string> parts;
    std::string cur;
    for (size_t i = 0; i < groups[0].size(); i++) {
        if (isspace((unsigned char)groups[0][i])) {
            if (!cur.empty()) { parts.push_back(cur); cur.clear(); }
        } else cur += groups[0][i];
    }
    if (!cur.empty()) parts.push_back(cur);
    if (parts.empty()) return;

    float h[4] = { 0, 0, 0, 0 };
    for (size_t i = 0; i < parts.size() && i < 4; i++) {
        Length len = Length::FromString(parts[i]);
        h[i] = len.Resolve(0, fontSize, rootFontSize);
    }
    if (parts.size() == 1) h[1] = h[2] = h[3] = h[0];
    else if (parts.size() == 2) { h[1] = h[0]; h[2] = h[0]; h[3] = h[1]; }
    else if (parts.size() == 3) { h[3] = h[1]; }

    // 垂直半径
    float v[4] = { h[0], h[1], h[2], h[3] };
    if (groups.size() == 2) {
        std::vector<std::string> vparts;
        cur.clear();
        for (size_t i = 0; i < groups[1].size(); i++) {
            if (isspace((unsigned char)groups[1][i])) {
                if (!cur.empty()) { vparts.push_back(cur); cur.clear(); }
            } else cur += groups[1][i];
        }
        if (!cur.empty()) vparts.push_back(cur);

        for (size_t i = 0; i < vparts.size() && i < 4; i++) {
            Length len = Length::FromString(vparts[i]);
            v[i] = len.Resolve(0, fontSize, rootFontSize);
        }
        if (vparts.size() == 1) v[1] = v[2] = v[3] = v[0];
        else if (vparts.size() == 2) { v[1] = v[0]; v[2] = v[0]; v[3] = v[1]; }
        else if (vparts.size() == 3) { v[3] = v[1]; }
    }

    br.tl = h[0]; br.tr = h[1]; br.br = h[2]; br.bl = h[3];
    br.tlv = v[0]; br.trv = v[1]; br.brv = v[2]; br.blv = v[3];
}

void CssParser::ApplyDeclaration(const std::string& property, const std::string& value,
                                 ComputedStyle& style, float parentFontSize, float rootFontSize)
{
    std::string prop = ToLower(Trim(property));
    std::string val = Trim(value);
    std::string low = ToLower(val);

    // ============ 显示与定位 ============
    if (prop == "display") {
        if (low == "none")               style.display = DISPLAY_NONE;
        else if (low == "block")         style.display = DISPLAY_BLOCK;
        else if (low == "inline")        style.display = DISPLAY_INLINE;
        else if (low == "inline-block")  style.display = DISPLAY_INLINE_BLOCK;
        else if (low == "flex")          style.display = DISPLAY_FLEX;
        else if (low == "inline-flex")   style.display = DISPLAY_INLINE_FLEX;
        else if (low == "list-item")     style.display = DISPLAY_LIST_ITEM;
        else if (low == "table")         style.display = DISPLAY_TABLE;
        else if (low == "table-row")     style.display = DISPLAY_TABLE_ROW;
        else if (low == "table-cell")    style.display = DISPLAY_TABLE_CELL;
        else if (low == "table-row-group") style.display = DISPLAY_TABLE_ROW_GROUP;
        else if (low == "contents")      style.display = DISPLAY_INLINE;
        else if (low == "grid" || low == "inline-grid") style.display = DISPLAY_BLOCK;
    }
    else if (prop == "position") {
        if (low == "static")        style.position = POS_STATIC;
        else if (low == "relative") style.position = POS_RELATIVE;
        else if (low == "absolute") style.position = POS_ABSOLUTE;
        else if (low == "fixed")    style.position = POS_FIXED;
    }
    else if (prop == "float") {
        if (low == "left")       style.floatType = FLOAT_LEFT;
        else if (low == "right") style.floatType = FLOAT_RIGHT;
        else                     style.floatType = FLOAT_NONE;
    }
    else if (prop == "clear") {
        if (low == "left")       style.clearType = CLEAR_LEFT;
        else if (low == "right") style.clearType = CLEAR_RIGHT;
        else if (low == "both")  style.clearType = CLEAR_BOTH;
        else                     style.clearType = CLEAR_NONE;
    }
    else if (prop == "visibility") {
        if (low == "visible")       style.visibility = VIS_VISIBLE;
        else if (low == "hidden")   style.visibility = VIS_HIDDEN;
        else if (low == "collapse") style.visibility = VIS_COLLAPSE;
    }
    else if (prop == "z-index") {
        if (low == "auto") style.zIndex = 0;
        else style.zIndex = atoi(val.c_str());
    }
    else if (prop == "box-sizing") {
        if (low == "border-box") style.boxSizing = BOX_BORDER;
        else style.boxSizing = BOX_CONTENT;
    }

    // ============ 尺寸 ============
    else if (prop == "width")     style.width = Length::FromString(val);
    else if (prop == "height")    style.height = Length::FromString(val);
    else if (prop == "min-width") style.minWidth = Length::FromString(val);
    else if (prop == "min-height") style.minHeight = Length::FromString(val);
    else if (prop == "max-width") style.maxWidth = Length::FromString(val);
    else if (prop == "max-height") style.maxHeight = Length::FromString(val);
    else if (prop == "top")    style.top = Length::FromString(val);
    else if (prop == "right")  style.right = Length::FromString(val);
    else if (prop == "bottom") style.bottom = Length::FromString(val);
    else if (prop == "left")   style.left = Length::FromString(val);

    // ============ 盒模型 ============
    else if (prop == "margin") {
        ParseFourLengths(val, style.marginTop, style.marginRight, style.marginBottom, style.marginLeft);
    }
    else if (prop == "margin-top")    style.marginTop = Length::FromString(val);
    else if (prop == "margin-right")  style.marginRight = Length::FromString(val);
    else if (prop == "margin-bottom") style.marginBottom = Length::FromString(val);
    else if (prop == "margin-left")   style.marginLeft = Length::FromString(val);

    else if (prop == "padding") {
        ParseFourLengths(val, style.paddingTop, style.paddingRight, style.paddingBottom, style.paddingLeft);
    }
    else if (prop == "padding-top")    style.paddingTop = Length::FromString(val);
    else if (prop == "padding-right")  style.paddingRight = Length::FromString(val);
    else if (prop == "padding-bottom") style.paddingBottom = Length::FromString(val);
    else if (prop == "padding-left")   style.paddingLeft = Length::FromString(val);

    // ============ 边框 ============
    else if (prop == "border") {
        BorderSide side;
        ParseBorderShorthand(val, side, style.fontSize, rootFontSize);
        style.borderTop = style.borderRight = style.borderBottom = style.borderLeft = side;
    }
    else if (prop == "border-top")    ParseBorderShorthand(val, style.borderTop, style.fontSize, rootFontSize);
    else if (prop == "border-right")  ParseBorderShorthand(val, style.borderRight, style.fontSize, rootFontSize);
    else if (prop == "border-bottom") ParseBorderShorthand(val, style.borderBottom, style.fontSize, rootFontSize);
    else if (prop == "border-left")   ParseBorderShorthand(val, style.borderLeft, style.fontSize, rootFontSize);

    else if (prop == "border-width") {
        Length t, r, b, l;
        ParseFourLengths(val, t, r, b, l);
        style.borderTop.width = t.Resolve(0, style.fontSize, rootFontSize);
        style.borderRight.width = r.Resolve(0, style.fontSize, rootFontSize);
        style.borderBottom.width = b.Resolve(0, style.fontSize, rootFontSize);
        style.borderLeft.width = l.Resolve(0, style.fontSize, rootFontSize);
        if (style.borderTop.width > 0 && style.borderTop.style == BORDER_NONE) style.borderTop.style = BORDER_SOLID;
        if (style.borderRight.width > 0 && style.borderRight.style == BORDER_NONE) style.borderRight.style = BORDER_SOLID;
        if (style.borderBottom.width > 0 && style.borderBottom.style == BORDER_NONE) style.borderBottom.style = BORDER_SOLID;
        if (style.borderLeft.width > 0 && style.borderLeft.style == BORDER_NONE) style.borderLeft.style = BORDER_SOLID;
    }
    else if (prop == "border-style") {
        std::vector<std::string> parts;
        std::string cur;
        for (size_t i = 0; i < val.size(); i++) {
            if (isspace((unsigned char)val[i])) { if (!cur.empty()) { parts.push_back(cur); cur.clear(); } }
            else cur += val[i];
        }
        if (!cur.empty()) parts.push_back(cur);
        BorderStyle st = BORDER_SOLID;
        if (!parts.empty()) {
            std::string p = ToLower(parts[0]);
            if (p == "solid") st = BORDER_SOLID;
            else if (p == "dashed") st = BORDER_DASHED;
            else if (p == "dotted") st = BORDER_DOTTED;
            else if (p == "double") st = BORDER_DOUBLE;
            else if (p == "none") st = BORDER_NONE;
        }
        style.borderTop.style = style.borderRight.style = style.borderBottom.style = style.borderLeft.style = st;
    }
    else if (prop == "border-color") {
        std::vector<std::string> parts;
        std::string cur;
        for (size_t i = 0; i < val.size(); i++) {
            if (isspace((unsigned char)val[i])) { if (!cur.empty()) { parts.push_back(cur); cur.clear(); } }
            else cur += val[i];
        }
        if (!cur.empty()) parts.push_back(cur);
        Color c = parts.empty() ? Color::Black() : Color::FromString(parts[0]);
        style.borderTop.color = style.borderRight.color = style.borderBottom.color = style.borderLeft.color = c;
    }
    else if (prop == "border-top-width")    style.borderTop.width = Length::FromString(val).Resolve(0, style.fontSize, rootFontSize);
    else if (prop == "border-right-width")  style.borderRight.width = Length::FromString(val).Resolve(0, style.fontSize, rootFontSize);
    else if (prop == "border-bottom-width") style.borderBottom.width = Length::FromString(val).Resolve(0, style.fontSize, rootFontSize);
    else if (prop == "border-left-width")   style.borderLeft.width = Length::FromString(val).Resolve(0, style.fontSize, rootFontSize);
    else if (prop == "border-top-color")    style.borderTop.color = Color::FromString(val);
    else if (prop == "border-right-color")  style.borderRight.color = Color::FromString(val);
    else if (prop == "border-bottom-color") style.borderBottom.color = Color::FromString(val);
    else if (prop == "border-left-color")   style.borderLeft.color = Color::FromString(val);
    else if (prop == "border-top-style") {
        std::string p = low;
        style.borderTop.style = (p == "none") ? BORDER_NONE : (p == "dashed" ? BORDER_DASHED :
            (p == "dotted" ? BORDER_DOTTED : (p == "double" ? BORDER_DOUBLE : BORDER_SOLID)));
    }
    else if (prop == "border-right-style") {
        std::string p = low;
        style.borderRight.style = (p == "none") ? BORDER_NONE : (p == "dashed" ? BORDER_DASHED :
            (p == "dotted" ? BORDER_DOTTED : (p == "double" ? BORDER_DOUBLE : BORDER_SOLID)));
    }
    else if (prop == "border-bottom-style") {
        std::string p = low;
        style.borderBottom.style = (p == "none") ? BORDER_NONE : (p == "dashed" ? BORDER_DASHED :
            (p == "dotted" ? BORDER_DOTTED : (p == "double" ? BORDER_DOUBLE : BORDER_SOLID)));
    }
    else if (prop == "border-left-style") {
        std::string p = low;
        style.borderLeft.style = (p == "none") ? BORDER_NONE : (p == "dashed" ? BORDER_DASHED :
            (p == "dotted" ? BORDER_DOTTED : (p == "double" ? BORDER_DOUBLE : BORDER_SOLID)));
    }

    else if (prop == "border-radius") {
        ParseBorderRadius(val, style.borderRadius, style.fontSize, rootFontSize);
    }
    else if (prop == "border-top-left-radius") {
        Length len = Length::FromString(val);
        style.borderRadius.tl = len.Resolve(0, style.fontSize, rootFontSize);
        style.borderRadius.tlv = style.borderRadius.tl;
    }
    else if (prop == "border-top-right-radius") {
        Length len = Length::FromString(val);
        style.borderRadius.tr = len.Resolve(0, style.fontSize, rootFontSize);
        style.borderRadius.trv = style.borderRadius.tr;
    }
    else if (prop == "border-bottom-right-radius") {
        Length len = Length::FromString(val);
        style.borderRadius.br = len.Resolve(0, style.fontSize, rootFontSize);
        style.borderRadius.brv = style.borderRadius.br;
    }
    else if (prop == "border-bottom-left-radius") {
        Length len = Length::FromString(val);
        style.borderRadius.bl = len.Resolve(0, style.fontSize, rootFontSize);
        style.borderRadius.blv = style.borderRadius.bl;
    }

    // ============ 背景 ============
    else if (prop == "background-color") {
        style.backgroundColor = Color::FromString(val);
        style.backgroundImage.Clear();
    }
    else if (prop == "background-image") {
        if (low.find("gradient") != std::string::npos) {
            Gradient g;
            if (ParseGradient(val, g)) {
                style.backgroundImage.type = BackgroundImage::GRADIENT;
                style.backgroundImage.gradient = g;
            }
        } else if (low.find("url(") != std::string::npos) {
            size_t p1 = val.find('(');
            size_t p2 = val.find(')');
            if (p1 != std::string::npos && p2 != std::string::npos) {
                std::string url = Trim(val.substr(p1 + 1, p2 - p1 - 1));
                if (url.size() >= 2 && (url[0] == '"' || url[0] == '\'')) {
                    url = url.substr(1, url.size() - 2);
                }
                style.backgroundImage.type = BackgroundImage::URL;
                style.backgroundImage.url = url;
            }
        } else if (low == "none") {
            style.backgroundImage.Clear();
        }
    }
    else if (prop == "background") {
        // 简写：可能含颜色、渐变、url
        if (low.find("gradient") != std::string::npos) {
            Gradient g;
            if (ParseGradient(val, g)) {
                style.backgroundImage.type = BackgroundImage::GRADIENT;
                style.backgroundImage.gradient = g;
            }
            // 同时提取颜色（渐变之外的部分）
            std::vector<std::string> parts = SplitTopLevel(val, ' ');
            for (size_t i = 0; i < parts.size(); i++) {
                if (Color::IsColorString(parts[i]) && low.find(ToLower(parts[i])) != std::string::npos) {
                    // 避免把渐变色标当背景色：只在渐变关键字前出现才设置
                    size_t posColor = low.find(ToLower(parts[i]));
                    size_t posGrad = low.find("gradient");
                    if (posColor < posGrad) {
                        style.backgroundColor = Color::FromString(parts[i]);
                        break;
                    }
                }
            }
        }
        else if (low.find("url(") != std::string::npos) {
            size_t p1 = val.find('(');
            size_t p2 = val.find(')');
            if (p1 != std::string::npos && p2 != std::string::npos) {
                std::string url = Trim(val.substr(p1 + 1, p2 - p1 - 1));
                if (url.size() >= 2 && (url[0] == '"' || url[0] == '\'')) url = url.substr(1, url.size() - 2);
                style.backgroundImage.type = BackgroundImage::URL;
                style.backgroundImage.url = url;
            }
            std::vector<std::string> parts = SplitTopLevel(val, ' ');
            for (size_t i = 0; i < parts.size(); i++) {
                if (Color::IsColorString(parts[i])) { style.backgroundColor = Color::FromString(parts[i]); break; }
            }
        }
        else if (low == "none" || low == "transparent") {
            style.backgroundColor = Color(0, 0, 0, 0);
            style.backgroundImage.Clear();
        }
        else {
            style.backgroundColor = Color::FromString(val);
            style.backgroundImage.Clear();
        }
    }
    else if (prop == "background-repeat") {
        style.backgroundImage.repeatX = (low.find("no-repeat") == std::string::npos);
        style.backgroundImage.repeatY = (low.find("repeat-y") != std::string::npos ||
                                         (low.find("no-repeat") == std::string::npos &&
                                          low.find("repeat-x") == std::string::npos));
        if (low.find("repeat-x") != std::string::npos) { style.backgroundImage.repeatX = true; style.backgroundImage.repeatY = false; }
    }

    // ============ 文字 ============
    else if (prop == "color") {
        style.color = Color::FromString(val);
    }
    else if (prop == "font-size") {
        std::string l = low;
        if (l == "xx-small") style.fontSize = 9.0f;
        else if (l == "x-small") style.fontSize = 10.0f;
        else if (l == "small") style.fontSize = 13.0f;
        else if (l == "medium") style.fontSize = 16.0f;
        else if (l == "large") style.fontSize = 18.0f;
        else if (l == "x-large") style.fontSize = 24.0f;
        else if (l == "xx-large") style.fontSize = 32.0f;
        else if (l == "smaller") style.fontSize = style.fontSize * 0.83f;
        else if (l == "larger") style.fontSize = style.fontSize * 1.2f;
        else {
            Length len = Length::FromString(val);
            style.fontSize = len.Resolve(parentFontSize, style.fontSize, rootFontSize);
        }
        if (style.fontSize < 1.0f) style.fontSize = 1.0f;
    }
    else if (prop == "font-family") {
        // 去掉引号，取第一个字体
        std::vector<std::string> parts = SplitTopLevel(val, ',');
        std::string first = Trim(parts[0]);
        if (first.size() >= 2 && (first[0] == '"' || first[0] == '\'')) {
            first = first.substr(1, first.size() - 2);
        }
        if (!first.empty()) style.fontFamily = first;
    }
    else if (prop == "font-weight") {
        if (low == "bold" || low == "bolder") style.fontWeight = FONT_WEIGHT_BOLD;
        else if (low == "normal" || low == "lighter") style.fontWeight = FONT_WEIGHT_NORMAL;
        else {
            int w = atoi(val.c_str());
            style.fontWeight = (w >= 600) ? FONT_WEIGHT_BOLD : FONT_WEIGHT_NORMAL;
        }
    }
    else if (prop == "font-style") {
        if (low == "italic" || low == "oblique") style.fontStyle = FONT_ITALIC;
        else style.fontStyle = FONT_NORMAL;
    }
    else if (prop == "font") {
        // 简写：font: bold 14px/1.5 "微软雅黑"
        std::vector<std::string> parts;
        std::string cur;
        for (size_t i = 0; i < val.size(); i++) {
            char ch = val[i];
            if (isspace((unsigned char)ch)) { if (!cur.empty()) { parts.push_back(cur); cur.clear(); } }
            else cur += ch;
        }
        if (!cur.empty()) parts.push_back(cur);

        for (size_t i = 0; i < parts.size(); i++) {
            std::string p = ToLower(parts[i]);
            if (p == "bold" || p == "bolder") style.fontWeight = FONT_WEIGHT_BOLD;
            else if (p == "normal") { /* 保持 */ }
            else if (p == "italic" || p == "oblique") style.fontStyle = FONT_ITALIC;
            else if (p.find("px") != std::string::npos || p.find("pt") != std::string::npos ||
                     p.find("em") != std::string::npos || p.find("rem") != std::string::npos) {
                // 可能带 /line-height
                size_t slash = p.find('/');
                std::string sizeStr = (slash == std::string::npos) ? p : p.substr(0, slash);
                Length len = Length::FromString(sizeStr);
                style.fontSize = len.Resolve(parentFontSize, style.fontSize, rootFontSize);
                if (slash != std::string::npos) {
                    std::string lh = p.substr(slash + 1);
                    if (lh.find("px") == std::string::npos && lh.find("%") == std::string::npos) {
                        style.lineHeight = style.fontSize * (float)atof(lh.c_str());
                    } else {
                        style.lineHeight = Length::FromString(lh).Resolve(parentFontSize, style.fontSize, rootFontSize);
                    }
                }
            }
            else if (p.size() > 1 && (parts[i][0] == '"' || parts[i][0] == '\'')) {
                style.fontFamily = parts[i].substr(1, parts[i].size() - 2);
            }
        }
    }
    else if (prop == "line-height") {
        if (low == "normal") style.lineHeight = 0.0f;
        else {
            Length len = Length::FromString(val);
            if (len.unit == Length::PERCENT) style.lineHeight = style.fontSize * len.value / 100.0f;
            else if (len.unit == Length::PX || len.unit == Length::PT) style.lineHeight = len.Resolve(0, style.fontSize, rootFontSize);
            else if (len.IsAuto()) style.lineHeight = style.fontSize * 1.5f;
            else style.lineHeight = style.fontSize * len.value;   // 纯数字 = 倍数
        }
    }
    else if (prop == "text-align") {
        if (low == "left" || low == "start") style.textAlign = ALIGN_LEFT;
        else if (low == "center") style.textAlign = ALIGN_CENTER;
        else if (low == "right" || low == "end") style.textAlign = ALIGN_RIGHT;
        else if (low == "justify") style.textAlign = ALIGN_JUSTIFY;
    }
    else if (prop == "text-decoration" || prop == "text-decoration-line") {
        style.underline = (low.find("underline") != std::string::npos);
        style.lineThrough = (low.find("line-through") != std::string::npos);
        style.overline = (low.find("overline") != std::string::npos);
    }
    else if (prop == "text-indent") {
        style.textIndent = Length::FromString(val);
    }
    else if (prop == "text-transform") {
        // 由绘制层处理（这里保存到 whiteSpace 旁边的临时标记不合适，简化：忽略）
    }
    else if (prop == "white-space") {
        if (low == "normal")   style.whiteSpace = WS_NORMAL;
        else if (low == "nowrap") style.whiteSpace = WS_NOWRAP;
        else if (low == "pre")    style.whiteSpace = WS_PRE;
        else if (low == "pre-wrap") style.whiteSpace = WS_PRE_WRAP;
        else if (low == "pre-line") style.whiteSpace = WS_PRE_LINE;
    }
    else if (prop == "vertical-align") {
        if (low == "top") style.verticalAlign = VALIGN_TOP;
        else if (low == "middle") style.verticalAlign = VALIGN_MIDDLE;
        else if (low == "bottom") style.verticalAlign = VALIGN_BOTTOM;
        else style.verticalAlign = VALIGN_BASELINE;
    }
    else if (prop == "letter-spacing") style.letterSpacing = Length::FromString(val);
    else if (prop == "word-spacing")   style.wordSpacing = Length::FromString(val);

    // ============ 效果 ============
    else if (prop == "opacity") {
        float o = (float)atof(val.c_str());
        if (o < 0.0f) o = 0.0f;
        if (o > 1.0f) o = 1.0f;
        style.opacity = o;
    }
    else if (prop == "box-shadow") {
        ParseShadows(val, style.boxShadows, style.fontSize, rootFontSize);
    }
    else if (prop == "text-shadow") {
        ParseShadows(val, style.textShadows, style.fontSize, rootFontSize);
    }
    else if (prop == "filter") {
        // blur() / grayscale() / brightness()
        size_t bp = low.find("blur(");
        if (bp != std::string::npos) {
            size_t e = low.find(')', bp);
            if (e != std::string::npos) {
                std::string num = low.substr(bp + 5, e - bp - 5);
                style.blurRadius = Length::FromString(num).Resolve(0, style.fontSize, rootFontSize);
            }
        }
        size_t gp = low.find("grayscale(");
        if (gp != std::string::npos) {
            size_t e = low.find(')', gp);
            if (e != std::string::npos) {
                std::string num = low.substr(gp + 10, e - gp - 10);
                float g = (float)atof(num.c_str());
                if (num.find('%') != std::string::npos) g /= 100.0f;
                style.grayscale = (g < 0) ? 0 : (g > 1 ? 1 : g);
            }
        }
        size_t brp = low.find("brightness(");
        if (brp != std::string::npos) {
            size_t e = low.find(')', brp);
            if (e != std::string::npos) {
                std::string num = low.substr(brp + 11, e - brp - 11);
                float b = (float)atof(num.c_str());
                if (num.find('%') != std::string::npos) b /= 100.0f;
                style.brightness = b;
            }
        }
    }

    // ============ 列表 ============
    else if (prop == "list-style-type") {
        if (low == "none") style.listStyleType = LIST_NONE;
        else if (low == "disc") style.listStyleType = LIST_DISC;
        else if (low == "circle") style.listStyleType = LIST_CIRCLE;
        else if (low == "square") style.listStyleType = LIST_SQUARE;
        else if (low == "decimal") style.listStyleType = LIST_DECIMAL;
    }
    else if (prop == "list-style-position") {
        style.listStyleInside = (low == "inside");
    }
    else if (prop == "list-style") {
        if (low.find("none") != std::string::npos) style.listStyleType = LIST_NONE;
        else if (low.find("disc") != std::string::npos) style.listStyleType = LIST_DISC;
        else if (low.find("circle") != std::string::npos) style.listStyleType = LIST_CIRCLE;
        else if (low.find("square") != std::string::npos) style.listStyleType = LIST_SQUARE;
        else if (low.find("decimal") != std::string::npos) style.listStyleType = LIST_DECIMAL;
        if (low.find("inside") != std::string::npos) style.listStyleInside = true;
    }

    // ============ Flex ============
    else if (prop == "flex-direction") {
        if (low == "row") style.flexDirection = FLEX_ROW;
        else if (low == "row-reverse") style.flexDirection = FLEX_ROW_REVERSE;
        else if (low == "column") style.flexDirection = FLEX_COLUMN;
        else if (low == "column-reverse") style.flexDirection = FLEX_COLUMN_REVERSE;
    }
    else if (prop == "flex-wrap") {
        if (low == "nowrap") style.flexWrap = WRAP_NOWRAP;
        else if (low == "wrap") style.flexWrap = WRAP_WRAP;
        else if (low == "wrap-reverse") style.flexWrap = WRAP_WRAP_REVERSE;
    }
    else if (prop == "justify-content") {
        if (low == "flex-start" || low == "start") style.justifyContent = JUSTIFY_START;
        else if (low == "flex-end" || low == "end") style.justifyContent = JUSTIFY_END;
        else if (low == "center") style.justifyContent = JUSTIFY_CENTER;
        else if (low == "space-between") style.justifyContent = JUSTIFY_SPACE_BETWEEN;
        else if (low == "space-around") style.justifyContent = JUSTIFY_SPACE_AROUND;
        else if (low == "space-evenly") style.justifyContent = JUSTIFY_SPACE_EVENLY;
    }
    else if (prop == "align-items") {
        if (low == "stretch") style.alignItems = ALIGN_ITEMS_STRETCH;
        else if (low == "flex-start" || low == "start") style.alignItems = ALIGN_ITEMS_START;
        else if (low == "flex-end" || low == "end") style.alignItems = ALIGN_ITEMS_END;
        else if (low == "center") style.alignItems = ALIGN_ITEMS_CENTER;
        else if (low == "baseline") style.alignItems = ALIGN_ITEMS_BASELINE;
    }
    else if (prop == "align-self") {
        if (low == "auto") style.alignSelf = ALIGN_ITEMS_STRETCH;
        else if (low == "stretch") style.alignSelf = ALIGN_ITEMS_STRETCH;
        else if (low == "flex-start" || low == "start") style.alignSelf = ALIGN_ITEMS_START;
        else if (low == "flex-end" || low == "end") style.alignSelf = ALIGN_ITEMS_END;
        else if (low == "center") style.alignSelf = ALIGN_ITEMS_CENTER;
    }
    else if (prop == "flex-grow")   style.flexGrow = (float)atof(val.c_str());
    else if (prop == "flex-shrink") style.flexShrink = (float)atof(val.c_str());
    else if (prop == "flex-basis")  style.flexBasis = Length::FromString(val);
    else if (prop == "flex") {
        // flex: 1 / flex: 1 1 auto / flex: none / flex: auto
        if (low == "none") { style.flexGrow = 0; style.flexShrink = 0; style.flexBasis = Length(0, Length::AUTO); }
        else if (low == "auto") { style.flexGrow = 1; style.flexShrink = 1; style.flexBasis = Length(0, Length::AUTO); }
        else {
            std::vector<std::string> parts;
            std::string cur;
            for (size_t i = 0; i < val.size(); i++) {
                if (isspace((unsigned char)val[i])) { if (!cur.empty()) { parts.push_back(cur); cur.clear(); } }
                else cur += val[i];
            }
            if (!cur.empty()) parts.push_back(cur);

            int numIdx = 0;
            for (size_t i = 0; i < parts.size(); i++) {
                std::string p = ToLower(parts[i]);
                if (p.find("px") != std::string::npos || p.find('%') != std::string::npos ||
                    p.find("em") != std::string::npos || p == "auto") {
                    style.flexBasis = Length::FromString(p);
                } else {
                    float f = (float)atof(p.c_str());
                    if (numIdx == 0) style.flexGrow = f;
                    else if (numIdx == 1) style.flexShrink = f;
                    numIdx++;
                }
            }
        }
    }
    else if (prop == "gap" || prop == "grid-gap") {
        style.gap = Length::FromString(val).Resolve(0, style.fontSize, rootFontSize);
    }
    else if (prop == "row-gap" || prop == "column-gap") {
        style.gap = Length::FromString(val).Resolve(0, style.fontSize, rootFontSize);
    }

    // ============ 溢出 ============
    else if (prop == "overflow") {
        if (low == "visible") { style.overflowX = style.overflowY = OVERFLOW_VISIBLE; }
        else if (low == "hidden") { style.overflowX = style.overflowY = OVERFLOW_HIDDEN; }
        else if (low == "auto") { style.overflowX = style.overflowY = OVERFLOW_AUTO; }
        else if (low == "scroll") { style.overflowX = style.overflowY = OVERFLOW_SCROLL; }
    }
    else if (prop == "overflow-x") {
        if (low == "visible") style.overflowX = OVERFLOW_VISIBLE;
        else if (low == "hidden") style.overflowX = OVERFLOW_HIDDEN;
        else if (low == "auto") style.overflowX = OVERFLOW_AUTO;
        else if (low == "scroll") style.overflowX = OVERFLOW_SCROLL;
    }
    else if (prop == "overflow-y") {
        if (low == "visible") style.overflowY = OVERFLOW_VISIBLE;
        else if (low == "hidden") style.overflowY = OVERFLOW_HIDDEN;
        else if (low == "auto") style.overflowY = OVERFLOW_AUTO;
        else if (low == "scroll") style.overflowY = OVERFLOW_SCROLL;
    }

    // ============ 表格 ============
    else if (prop == "border-collapse") {
        style.borderCollapse = (low == "collapse") ? 1 : 0;
    }
    else if (prop == "border-spacing") {
        style.borderSpacing = Length::FromString(val).Resolve(0, style.fontSize, rootFontSize);
    }

    // ============ 其它 ============
    else if (prop == "cursor") {
        style.cursor = low;
    }
    else if (prop == "transform" || prop == "transition" || prop == "animation" ||
             prop == "will-change" || prop == "backface-visibility" ||
             prop == "perspective" || prop == "transform-origin") {
        // 解析但不应用（保留 CSS 兼容性，避免整条规则失效）
    }
}

void CssParser::ApplyInlineStyle(Node* node, ComputedStyle& style, float parentFontSize, float rootFontSize)
{
    if (node == NULL || !node->IsElement()) return;

    std::string inlineStyle = node->GetAttr("style");
    if (inlineStyle.empty()) return;

    std::vector<std::string> decls = SplitTopLevel(inlineStyle, ';');
    for (size_t i = 0; i < decls.size(); i++) {
        std::string d = Trim(decls[i]);
        if (d.empty()) continue;
        size_t colon = d.find(':');
        if (colon == std::string::npos) continue;

        std::string prop = Trim(d.substr(0, colon));
        std::string val = Trim(d.substr(colon + 1));

        // 去掉 !important
        std::string lowVal = ToLower(val);
        size_t imp = lowVal.find("!important");
        if (imp != std::string::npos) val = Trim(val.substr(0, imp));

        ApplyDeclaration(prop, val, style, parentFontSize, rootFontSize);
    }
}

} // namespace HtmlEngine
