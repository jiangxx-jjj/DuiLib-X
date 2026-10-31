#ifndef HTML_ENGINE_PARSER_H
#define HTML_ENGINE_PARSER_H

// ==========================================================================
//  HtmlParser.h — 自研 HTML5 解析器
//
//  容错式解析：不要求严格闭合，自动补全、自动闭合，
//  能正确处理 <br> <img> 等空元素、注释、CDATA、实体。
// ==========================================================================

#include "HtmlDom.h"
#include <string>
#include <vector>

namespace HtmlEngine
{

class HtmlParser
{
public:
    // 解析 HTML 字符串，结果写入 tree
    // 返回是否成功
    static bool Parse(const std::string& html, DocumentTree& tree);

    // 解码 HTML 实体：&amp; &lt; &gt; &quot; &#39; &nbsp; 等
    static std::string DecodeEntities(const std::string& s);

public:
    // 从 <style> 标签里提取 CSS（供 CSS 解析器使用）
    static void CollectStyles(Node* node, std::string& outCss);
};

} // namespace HtmlEngine

#endif // HTML_ENGINE_PARSER_H
