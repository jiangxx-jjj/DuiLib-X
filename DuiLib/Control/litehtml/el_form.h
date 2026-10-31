#ifndef LH_EL_FORM_H
#define LH_EL_FORM_H

// ==========================================================================
//  el_form.h — litehtml 自定义表单控件元素
//
//  实现 input/select/button/textarea 的自绘与交互。
//  通过 document_container::create_element 工厂注入 litehtml。
// ==========================================================================

#include "html_tag.h"

class container_win32;

namespace litehtml
{

// 前置声明
class document;

// ============================ 输入框（text/password）============================

class el_input_text : public html_tag
{
public:
    explicit el_input_text(const std::shared_ptr<document>& doc, container_win32* container);

    void draw(uint_ptr hdc, int x, int y, const position* clip,
              const std::shared_ptr<render_item>& ri) override;
    void on_click() override;
    bool on_lbutton_down() override;

    // 键盘输入
    bool InputChar(wchar_t ch);
    bool InputKeyDown(int vkCode);

    // 值访问
    std::string GetValue() const { return m_value; }
    void SetValue(const std::string& v) { m_value = v; m_caretPos = Utf8CharCount(v); }

    // 焦点
    void SetFocus(bool f) { m_focused = f; }
    bool IsFocused() const { return m_focused; }

protected:
    container_win32* m_container;
    std::string m_value;
    size_t m_caretPos;      // 字符索引
    bool m_focused;

    // UTF-8 工具
    static size_t Utf8CharCount(const std::string& s);
    static size_t Utf8BytePos(const std::string& s, size_t charIdx);
    static size_t Utf8CharLen(const std::string& s, size_t bytePos);
};

// ============================ 勾选框 / 单选框 ============================

class el_input_check : public html_tag
{
public:
    explicit el_input_check(const std::shared_ptr<document>& doc, container_win32* container, bool isRadio);

    void draw(uint_ptr hdc, int x, int y, const position* clip,
              const std::shared_ptr<render_item>& ri) override;
    void on_click() override;

    bool IsChecked() const { return m_checked; }
    void SetChecked(bool c) { m_checked = c; }

protected:
    container_win32* m_container;
    bool m_checked;
    bool m_isRadio;
};

// ============================ 按钮 ============================

class el_button : public html_tag
{
public:
    explicit el_button(const std::shared_ptr<document>& doc, container_win32* container);

    void draw(uint_ptr hdc, int x, int y, const position* clip,
              const std::shared_ptr<render_item>& ri) override;
    void on_click() override;

protected:
    container_win32* m_container;
};

// ============================ 下拉框 ============================

class el_select : public html_tag
{
public:
    explicit el_select(const std::shared_ptr<document>& doc, container_win32* container);

    void draw(uint_ptr hdc, int x, int y, const position* clip,
              const std::shared_ptr<render_item>& ri) override;
    void on_click() override;

    int GetSelectedIndex() const { return m_selectedIndex; }
    void SetSelectedIndex(int idx);

    std::string GetValue() const;

protected:
    container_win32* m_container;
    int m_selectedIndex;
};

// ============================ 多行文本框 ============================

class el_textarea : public html_tag
{
public:
    explicit el_textarea(const std::shared_ptr<document>& doc, container_win32* container);

    void draw(uint_ptr hdc, int x, int y, const position* clip,
              const std::shared_ptr<render_item>& ri) override;
    void on_click() override;
    bool on_lbutton_down() override;

    bool InputChar(wchar_t ch);
    bool InputKeyDown(int vkCode);

    std::string GetValue() const { return m_value; }
    void SetValue(const std::string& v);

    void SetFocus(bool f) { m_focused = f; }
    bool IsFocused() const { return m_focused; }

protected:
    container_win32* m_container;
    std::string m_value;
    size_t m_caretPos;
    bool m_focused;

    static size_t Utf8CharCount(const std::string& s);
    static size_t Utf8BytePos(const std::string& s, size_t charIdx);
    static size_t Utf8CharLen(const std::string& s, size_t bytePos);
};

} // namespace litehtml

#endif // LH_EL_FORM_H
