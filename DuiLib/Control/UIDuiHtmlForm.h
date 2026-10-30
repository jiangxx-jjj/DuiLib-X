#ifndef __UIDUIHTMLFORM_H__
#define __UIDUIHTMLFORM_H__

// ==========================================================================
// CHtmlFormElement — litehtml 表单控件元素
//
// litehtml 自身不实现 <input>/<select>/<button>/<textarea>（master CSS 里
// 只给了 display:inline-block），因此由本类接管，按"替换元素(replaced
// element)"处理：尺寸由 get_content_size 给出，绘制由 draw 自绘，
// 交互（点击/键盘）回调到 CDuiHtmlContainer。
//
// 这样 HTML 里的表单标签就能变成真正可用的输入框、复选框、下拉框、按钮。
// ==========================================================================

#include "StdAfx.h"
// litehtml 内部用 std::min/std::max，包含前解除 windows.h 的宏
#pragma push_macro("min")
#pragma push_macro("max")
#undef min
#undef max
#include "litehtml.h"
#include "render_image.h"
#pragma pop_macro("min")
#pragma pop_macro("max")

#include <string>
#include <vector>

namespace DuiLib
{

class CDuiHtmlContainer;

// 控件类型
enum HtmlFormType
{
    HtmlFormText = 0,       // 文本输入框
    HtmlFormPassword,       // 密码框
    HtmlFormCheckbox,       // 复选框
    HtmlFormRadio,          // 单选框
    HtmlFormButton,         // 按钮
    HtmlFormSelect,         // 下拉框
    HtmlFormTextArea,       // 多行文本框
    HtmlFormHidden,         // 隐藏域（不绘制）
};

class CHtmlFormElement : public litehtml::html_tag
{
public:
    explicit CHtmlFormElement(const std::shared_ptr<litehtml::document>& doc);

    // ---- litehtml::element 覆盖 ----
    bool is_replaced(void) const override;
    void parse_attributes(void) override;
    void get_content_size(litehtml::size& sz, int max_width) override;
    void draw(litehtml::uint_ptr hdc, int x, int y, const litehtml::position* clip,
              const std::shared_ptr<litehtml::render_item>& ri) override;
    bool on_lbutton_down(void) override;
    void on_click(void) override;
    std::shared_ptr<litehtml::render_item> create_render_item(
        const std::shared_ptr<litehtml::render_item>& parent_ri) override;

    // ---- 供容器读写 ----
    HtmlFormType GetFormType(void) const { return m_iFormType; }
    const std::string& GetFormName(void) const { return m_sName; }
    std::string GetFormValue(void) const;
    void SetFormValue(const std::string& val);
    bool IsFormChecked(void) const { return m_bChecked; }
    void SetFormChecked(bool bChecked);
    const std::vector<std::pair<std::string, std::string> >& GetOptions(void) const { return m_options; }
    int GetSelectedIndex(void) const { return m_iSelIndex; }
    void SetSelectedIndex(int idx);

    // ---- 文本编辑（供容器键盘输入调用）----
    void InsertChar(wchar_t ch);
    void Backspace(void);
    void DeleteChar(void);
    int  GetCaretPos(void) const { return m_iCaret; }
    void SetCaretPos(int pos);
    bool IsEditable(void) const;

private:
    // 取该元素所属的容器（本项目里 document_container 就是 CDuiHtmlContainer）
    CDuiHtmlContainer* GetHostContainer(void) const;

    int  GetFontHeight(void) const;                  // 字体行高
    int  MeasureText(const std::string& text) const; // 文本宽度

    void ParseInputType(const char* type);
    void ParseSelectOptions(void);

private:
    HtmlFormType        m_iFormType;
    std::string         m_sName;        // name 属性
    std::string         m_sValue;       // 当前值（UTF-8）
    std::string         m_sLabel;       // 按钮文字
    bool                m_bChecked;     // checkbox/radio 选中态
    int                 m_iSize;        // size 属性（字符数）
    int                 m_iCols;        // textarea 列数
    int                 m_iRows;        // textarea 行数
    int                 m_iCaret;       // 光标位置（字符偏移）
    std::vector<std::pair<std::string, std::string> > m_options;  // (value, text)
    int                 m_iSelIndex;    // select 当前项
};

} // namespace DuiLib

#endif // __UIDUIHTMLFORM_H__
