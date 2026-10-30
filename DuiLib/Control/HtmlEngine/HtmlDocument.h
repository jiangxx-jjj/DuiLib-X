#ifndef HTML_ENGINE_DOCUMENT_H
#define HTML_ENGINE_DOCUMENT_H

// ==========================================================================
//  HtmlDocument.h — 引擎门面
//
//  把 HTML 解析 → CSS 级联 → 布局 → 绘制 串成一个完整流程。
//  DuiLib 的 CHtmlBoxUI 只需调用这个类。
// ==========================================================================

#include "HtmlDom.h"
#include "HtmlParser.h"
#include "HtmlCss.h"
#include "HtmlLayout.h"
#include "HtmlPainter.h"
#include <string>
#include <vector>
#include <functional>

namespace HtmlEngine
{

class HtmlDocument
{
public:
    HtmlDocument();
    ~HtmlDocument();

    // ---- 加载 ----
    // 从 HTML 字符串加载
    bool LoadFromString(const std::string& html, const std::string& extraCss = "");
    // 从文件加载（UTF-8 / UTF-16 自动识别）
    bool LoadFromFile(const std::string& path, const std::string& extraCss = "");

    // 是否已成功加载
    bool IsLoaded() const { return m_bLoaded; }
    // 兼容别名
    bool Loaded() const { return m_bLoaded; }

    // 取 body 节点（供测试/高级用法）
    Node* GetBodyNode() const { return m_tree.bodyNode; }

    // ---- 排版 ----
    // 重新布局（宽度变化时调用），返回文档高度
    float Relayout(float viewportWidth);

    // 文档尺寸
    float GetWidth() const  { return m_docWidth; }
    float GetHeight() const { return m_docHeight; }

    // ---- 绘制 ----
    void Paint(HDC hdc, float scrollX, float scrollY, float clipW, float clipH);

    // ---- 交互 ----
    // 命中测试（文档坐标，y 需含滚动偏移）
    Node* HitTest(float x, float y);
    // 鼠标移动（更新 hover 状态），返回是否需要重绘
    bool OnMouseMove(float x, float y);
    // 鼠标离开
    void OnMouseLeave();
    // 点击（返回被点击的节点）
    Node* OnClick(float x, float y);

    // ---- 表单 ----
    // 按 name 找控件
    Node* FindFormByName(const std::string& name);
    // 读取控件值
    std::string GetFormValue(Node* node);
    // 设置控件值
    void SetFormValue(Node* node, const std::string& value);
    // 勾选状态
    bool GetFormChecked(Node* node);
    void SetFormChecked(Node* node, bool bChecked);
    // 下拉框选项
    int  GetFormSelectedIndex(Node* node);
    void SetFormSelectedIndex(Node* node, int idx);
    // 当前焦点控件
    Node* GetFocusNode() const;
    void  SetFocusNode(Node* node);

    // ---- 键盘 ----
    bool InputChar(wchar_t ch);
    bool InputKeyDown(int vkCode);

    // ---- 事件回调 ----
    // 链接点击
    void SetLinkClickCallback(std::function<void(const std::string& href)> cb)
    { m_fnLinkClick = cb; }
    // 按钮/可点击元素点击（wParam = name）
    void SetFormSubmitCallback(std::function<void(const std::string& name)> cb)
    { m_fnFormSubmit = cb; }
    // 表单值变化
    void SetFormChangedCallback(std::function<void(const std::string& name)> cb)
    { m_fnFormChanged = cb; }

    // ---- 图片加载回调（由 DuiLib 注入）----
    // 返回图片尺寸；bDraw 时把图片画到 hdc 的指定矩形
    void SetImageCallbacks(
        std::function<bool(const std::string& src, float& w, float& h)> fnSize,
        std::function<bool(HDC hdc, const std::string& src, float x, float y, float w, float h)> fnDraw)
    {
        m_fnImageSize = fnSize;
        m_fnImageDraw = fnDraw;
    }

    // ---- 基础路径（图片/css 相对路径基准）----
    void SetBasePath(const std::string& path) { m_sBasePath = path; }

private:
    DocumentTree    m_tree;
    CssParser       m_css;
    LayoutEngine    m_layout;
    HtmlPainter     m_painter;
    bool            m_bLoaded;

    float           m_docWidth;
    float           m_docHeight;
    float           m_viewportWidth;

    std::string     m_sBasePath;
    std::string     m_sExtraCss;

    std::function<void(const std::string&)> m_fnLinkClick;
    std::function<void(const std::string&)> m_fnFormSubmit;
    std::function<void(const std::string&)> m_fnFormChanged;
    std::function<bool(const std::string&, float&, float&)> m_fnImageSize;
    std::function<bool(HDC, const std::string&, float, float, float, float)> m_fnImageDraw;

    // 表单状态缓存（节点 → 值）
    struct FormValue {
        std::string value;
        bool        checked;
        int         selectedIndex;
    };
    std::map<Node*, FormValue> m_formValues;

    // 初始化节点表单状态
    void InitFormValues(Node* node);
    // 同步到 painter
    void SyncFormState(Node* node);
};

} // namespace HtmlEngine

#endif // HTML_ENGINE_DOCUMENT_H
