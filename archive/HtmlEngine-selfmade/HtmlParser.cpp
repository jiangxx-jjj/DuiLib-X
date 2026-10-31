#include "StdAfx.h"
#include "HtmlParser.h"

namespace HtmlEngine
{

// ============================ 实体解码 ============================

std::string HtmlParser::DecodeEntities(const std::string& s)
{
    std::string out;
    out.reserve(s.size());

    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] != '&') { out += s[i]; continue; }

        // 找分号
        size_t semi = s.find(';', i + 1);
        if (semi == std::string::npos || semi - i > 12) {
            out += s[i];
            continue;
        }

        std::string ent = s.substr(i + 1, semi - i - 1);
        std::string low = ToLower(ent);

        if (low == "amp")        out += '&';
        else if (low == "lt")    out += '<';
        else if (low == "gt")    out += '>';
        else if (low == "quot")  out += '"';
        else if (low == "apos")  out += '\'';
        else if (low == "nbsp")  out += ' ';
        else if (low == "ensp")  out += ' ';
        else if (low == "emsp")  out += ' ';
        else if (low == "thinsp") out += ' ';
        else if (low == "copy")  out += (char)0xA9;
        else if (low == "reg")   out += (char)0xAE;
        else if (low == "trade") out += (char)0x99;
        else if (low == "hellip") out += "...";
        else if (low == "mdash") out += '-';
        else if (low == "ndash") out += '-';
        else if (low == "lsquo" || low == "rsquo" || low == "ldquo" || low == "rdquo") out += '"';
        else if (low == "bull")  out += '*';
        else if (low[0] == '#') {
            // 数字实体：&#123; 或 &#x1F600;
            int code = 0;
            if (low.size() > 1 && (low[1] == 'x' || low[1] == 'X')) {
                code = (int)strtol(low.substr(2).c_str(), NULL, 16);
            } else {
                code = atoi(low.substr(1).c_str());
            }
            if (code > 0 && code < 128) {
                out += (char)code;
            } else if (code >= 128) {
                // 转 UTF-8
                if (code < 0x800) {
                    out += (char)(0xC0 | (code >> 6));
                    out += (char)(0x80 | (code & 0x3F));
                } else if (code < 0x10000) {
                    out += (char)(0xE0 | (code >> 12));
                    out += (char)(0x80 | ((code >> 6) & 0x3F));
                    out += (char)(0x80 | (code & 0x3F));
                } else {
                    out += (char)(0xF0 | (code >> 18));
                    out += (char)(0x80 | ((code >> 12) & 0x3F));
                    out += (char)(0x80 | ((code >> 6) & 0x3F));
                    out += (char)(0x80 | (code & 0x3F));
                }
            }
        } else {
            // 未知实体，原样保留
            out += s.substr(i, semi - i + 1);
        }
        i = semi;
    }
    return out;
}

// ============================ 属性解析 ============================

// 解析属性串：name="value" name='value' name=value name
static void ParseAttributes(const std::string& s, std::map<std::string, std::string>& attrs)
{
    size_t i = 0;
    while (i < s.size()) {
        // 跳过空白
        while (i < s.size() && isspace((unsigned char)s[i])) i++;
        if (i >= s.size()) break;

        // 属性名
        std::string name;
        while (i < s.size() && !isspace((unsigned char)s[i]) && s[i] != '=' && s[i] != '/' && s[i] != '>') {
            name += s[i];
            i++;
        }
        if (name.empty()) { i++; continue; }

        name = ToLower(name);

        // 跳过空白
        while (i < s.size() && isspace((unsigned char)s[i])) i++;

        std::string value;
        if (i < s.size() && s[i] == '=') {
            i++;
            while (i < s.size() && isspace((unsigned char)s[i])) i++;

            if (i < s.size() && (s[i] == '"' || s[i] == '\'')) {
                char quote = s[i];
                i++;
                while (i < s.size() && s[i] != quote) {
                    value += s[i];
                    i++;
                }
                if (i < s.size()) i++;   // 跳过结束引号
            } else {
                while (i < s.size() && !isspace((unsigned char)s[i]) && s[i] != '>') {
                    value += s[i];
                    i++;
                }
            }
        } else {
            // 无值属性（如 checked / disabled）
            value = "";
        }

        attrs[name] = HtmlParser::DecodeEntities(value);
    }
}

// ============================ 主解析 ============================

bool HtmlParser::Parse(const std::string& html, DocumentTree& tree)
{
    tree.Clear();
    tree.root = new Node(NODE_DOCUMENT);

    // 当前打开的节点栈
    std::vector<Node*> stack;
    stack.push_back(tree.root);

    size_t i = 0;
    size_t n = html.size();

    std::string pendingText;

    // 把累积的文本刷成一个文本节点
    #define FLUSH_TEXT() \
        if (!pendingText.empty()) { \
            Node* tn = new Node(NODE_TEXT); \
            tn->text = DecodeEntities(pendingText); \
            stack.back()->AddChild(tn); \
            pendingText.clear(); \
        }

    while (i < n) {
        // ---- 注释 ----
        if (html[i] == '<' && i + 3 < n && html[i + 1] == '!' && html[i + 2] == '-' && html[i + 3] == '-') {
            FLUSH_TEXT();
            size_t end = html.find("-->", i + 4);
            if (end == std::string::npos) break;
            i = end + 3;
            continue;
        }

        // ---- DOCTYPE / CDATA / 其它 <! ----
        if (html[i] == '<' && i + 1 < n && html[i + 1] == '!') {
            FLUSH_TEXT();
            size_t end = html.find('>', i + 2);
            if (end == std::string::npos) break;
            i = end + 1;
            continue;
        }

        // ---- 结束标签 ----
        if (html[i] == '<' && i + 1 < n && html[i + 1] == '/') {
            FLUSH_TEXT();
            size_t end = html.find('>', i + 2);
            if (end == std::string::npos) break;

            std::string tag = Trim(html.substr(i + 2, end - i - 2));
            tag = ToLower(tag);

            // 找到匹配的节点并弹出
            for (size_t k = stack.size(); k > 1; k--) {
                if (stack[k - 1]->tagName == tag) {
                    while (stack.size() >= k) stack.pop_back();
                    break;
                }
            }
            i = end + 1;
            continue;
        }

        // ---- 开始标签 ----
        if (html[i] == '<' && i + 1 < n && (isalpha((unsigned char)html[i + 1]))) {
            FLUSH_TEXT();

            // 找到标签结束（注意属性值里可能有 >）
            size_t j = i + 1;
            int quoteState = 0;   // 0=无引号 1=双引号 2=单引号
            while (j < n) {
                char ch = html[j];
                if (quoteState == 0) {
                    if (ch == '"') quoteState = 1;
                    else if (ch == '\'') quoteState = 2;
                    else if (ch == '>') break;
                } else if (quoteState == 1 && ch == '"') {
                    quoteState = 0;
                } else if (quoteState == 2 && ch == '\'') {
                    quoteState = 0;
                }
                j++;
            }
            if (j >= n) break;

            std::string inner = html.substr(i + 1, j - i - 1);

            // 提取标签名
            size_t k = 0;
            std::string tag;
            while (k < inner.size() && !isspace((unsigned char)inner[k]) && inner[k] != '/') {
                tag += inner[k];
                k++;
            }
            tag = ToLower(tag);
            if (tag.empty()) { i = j + 1; continue; }

            bool selfClosing = (inner.size() > 0 && inner[inner.size() - 1] == '/');

            Node* node = new Node(NODE_ELEMENT);
            node->tagName = tag;

            // 解析属性
            ParseAttributes(inner.substr(k), node->attrs);

            // <style> / <script> 内容特殊处理（跳过内部所有 < >）
            if (tag == "style" || tag == "script") {
                stack.back()->AddChild(node);

                // 找到对应的结束标签
                std::string closeTag = "</" + tag;
                size_t closePos = i;
                while (true) {
                    closePos = ToLower(html).find(closeTag, closePos + 1);
                    if (closePos == std::string::npos) { closePos = n; break; }
                    break;
                }
                size_t contentStart = j + 1;
                size_t contentEnd = (closePos == n) ? n : closePos;

                if (tag == "style") {
                    node->text = html.substr(contentStart, contentEnd - contentStart);
                }

                // 跳到结束标签之后
                if (closePos != n) {
                    size_t gt = html.find('>', closePos);
                    i = (gt == std::string::npos) ? n : gt + 1;
                } else {
                    i = n;
                }
                continue;
            }

            stack.back()->AddChild(node);

            // 空元素不入栈
            if (!selfClosing && !IsVoidTag(tag)) {
                // 自动闭合：<p> 内不能再有 <p>，<li> 内不能再有 <li>
                if (tag == "p" || tag == "li") {
                    Node* top = stack.back();
                    if (top->tagName == tag) {
                        stack.pop_back();
                    }
                }
                stack.push_back(node);
            }

            i = j + 1;
            continue;
        }

        // ---- 普通文本 ----
        pendingText += html[i];
        i++;
    }

    FLUSH_TEXT();

    // 定位 html/head/body
    tree.htmlNode = tree.bodyNode = tree.headNode = NULL;
    for (size_t c = 0; c < tree.root->children.size(); c++) {
        Node* el = tree.root->children[c];
        if (!el->IsElement()) continue;
        if (el->tagName == "html") {
            tree.htmlNode = el;
            for (size_t cc = 0; cc < el->children.size(); cc++) {
                Node* sub = el->children[cc];
                if (!sub->IsElement()) continue;
                if (sub->tagName == "head") tree.headNode = sub;
                if (sub->tagName == "body") tree.bodyNode = sub;
            }
        }
    }

    // 没有 <html>/<body> 时，把顶层元素当作 body 内容
    if (tree.bodyNode == NULL) {
        // 造一个隐式 body，把 root 的所有元素子节点搬进去
        Node* body = new Node(NODE_ELEMENT);
        body->tagName = "body";

        std::vector<Node*> keep;
        std::vector<Node*> move;
        for (size_t c = 0; c < tree.root->children.size(); c++) {
            Node* el = tree.root->children[c];
            if (el->IsElement() && (el->tagName == "html" || el->tagName == "head" ||
                                    el->tagName == "title" || el->tagName == "style" ||
                                    el->tagName == "script" || el->tagName == "meta" ||
                                    el->tagName == "link")) {
                keep.push_back(el);
            } else {
                move.push_back(el);
            }
        }

        tree.root->children = keep;
        for (size_t c = 0; c < move.size(); c++) {
            move[c]->parent = body;
            body->children.push_back(move[c]);
        }
        tree.root->AddChild(body);
        tree.bodyNode = body;
    }

    // html 节点缺失时补一个
    if (tree.htmlNode == NULL && tree.bodyNode != NULL) {
        tree.htmlNode = tree.bodyNode;
    }

    return tree.bodyNode != NULL;
}

// ============================ 收集 CSS ============================

void HtmlParser::CollectStyles(Node* node, std::string& outCss)
{
    if (node == NULL) return;
    if (node->IsElement() && node->tagName == "style") {
        outCss += node->text;
        outCss += "\n";
    }
    for (size_t i = 0; i < node->children.size(); i++) {
        CollectStyles(node->children[i], outCss);
    }
}

} // namespace HtmlEngine
