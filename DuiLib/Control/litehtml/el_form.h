#ifndef LH_EL_FORM_H
#define LH_EL_FORM_H

// ==========================================================================
//  el_form.h — litehtml 自定义表单控件元素（replaced element 方式）
//
//  实现 input/select/button/textarea 的自绘与交互。
//  通过 document_container::create_element 工厂注入 litehtml。
//
//  关键点：
//  · 继承 html_tag（litehtml 的通用元素）
//  · is_replaced() 返回 true —— 让布局引擎把它当"替换元素"处理（类似 <img>）
//  · get_content_size 返回固有小尺寸 —— 布局引擎据此分配宽高
//  · draw 里直接用 GDI+ 自绘控件外观（背景/边框由 litehtml 先画）
// ==========================================================================

#include "html_tag.h"
#include "render_item.h"

class container_win32;

namespace litehtml
{

// 表单控件基类：统一的值存取/焦点管理
class el_form_control : public html_tag
{
public:
    el_form_control(const document::ptr& doc, container_win32* container, const char* displayStyle);

    // 焦点
    void SetFocus(bool f) { m_focused = f; }
    bool IsFocused() const { return m_focused; }

    // 值存取（子类按需重写）
    virtual std::string GetValue() const { return ""; }
    virtual void SetValue(const std::string& v) {}
    virtual bool IsChecked() const { return false; }
    virtual void SetChecked(bool c) {}
    virtual int  GetSelectedIndex() const { return -1; }
    virtual void SetSelectedIndex(int idx) {}

    // 键盘
    virtual bool InputChar(wchar_t ch) { return false; }
    virtual bool InputKeyDown(int vkCode) { return false; }

    // 控件名（name 属性）
    std::string GetName() const
    {
        const char* n = get_attr("name");
        return n ? n : "";
    }

protected:
    container_win32* m_container;
    bool m_focused;

    // 绘制辅助：取绘制区（含 padding/border 的 border-box，绝对坐标）
    position GetDrawPos(int x, int y, const std::shared_ptr<render_item>& ri) const;

    // UTF-8 工具
    static size_t Utf8CharLen(const std::string& s, size_t bytePos);
    static size_t Utf8BytePos(const std::string& s, size_t charIdx);
    static size_t Utf8CharCount(const std::string& s);
};

// ============================ 文本输入框（text/password）============================

class el_input_text : public el_form_control
{
public:
    el_input_text(const document::ptr& doc, container_win32* container, const char* type);

    bool    is_replaced() const override { return true; }
    void    parse_attributes() override;
    void    draw(uint_ptr hdc, int x, int y, const position* clip,
                 const std::shared_ptr<render_item>& ri) override;
    void    get_content_size(size& sz, int max_width) override;
    void    on_click() override;

    std::string GetValue() const override { return m_value; }
    void SetValue(const std::string& v) override;
    bool InputChar(wchar_t ch) override;
    bool InputKeyDown(int vkCode) override;

protected:
    std::string m_type;         // text / password
    std::string m_value;
    size_t m_caretPos;
};

// ============================ 勾选框 / 单选框 ============================

class el_input_check : public el_form_control
{
public:
    el_input_check(const document::ptr& doc, container_win32* container, bool isRadio);

    bool    is_replaced() const override { return true; }
    void    draw(uint_ptr hdc, int x, int y, const position* clip,
                 const std::shared_ptr<render_item>& ri) override;
    void    get_content_size(size& sz, int max_width) override;
    void    on_click() override;

    bool IsChecked() const override { return m_checked; }
    void SetChecked(bool c) override { m_checked = c; }

protected:
    bool m_checked;
    bool m_isRadio;
};

// ============================ 按钮 ============================

class el_button : public el_form_control
{
public:
    el_button(const document::ptr& doc, container_win32* container);

    bool    is_replaced() const override { return true; }
    void    draw(uint_ptr hdc, int x, int y, const position* clip,
                 const std::shared_ptr<render_item>& ri) override;
    void    get_content_size(size& sz, int max_width) override;
    void    on_click() override;

    std::string GetValue() const override
    {
        const char* v = get_attr("value");
        return v ? v : "";
    }

protected:
    std::string m_label;    // 按钮文字
};

// ============================ 下拉框 ============================

class el_select : public el_form_control
{
public:
    el_select(const document::ptr& doc, container_win32* container);

    bool    is_replaced() const override { return true; }
    void    parse_attributes() override;
    void    draw(uint_ptr hdc, int x, int y, const position* clip,
                 const std::shared_ptr<render_item>& ri) override;
    void    get_content_size(size& sz, int max_width) override;
    void    on_click() override;

    int  GetSelectedIndex() const override { return m_selectedIndex; }
    void SetSelectedIndex(int idx) override;
    std::string GetValue() const override;

protected:
    int m_selectedIndex;
    std::vector<std::string> m_options;      // 选项文本列表
};

// ============================ 多行文本框 ============================

class el_textarea : public el_form_control
{
public:
    el_textarea(const document::ptr& doc, container_win32* container);

    bool    is_replaced() const override { return true; }
    void    parse_attributes() override;
    void    draw(uint_ptr hdc, int x, int y, const position* clip,
                 const std::shared_ptr<render_item>& ri) override;
    void    get_content_size(size& sz, int max_width) override;
    void    on_click() override;

    std::string GetValue() const override { return m_value; }
    void SetValue(const std::string& v) override;
    bool InputChar(wchar_t ch) override;
    bool InputKeyDown(int vkCode) override;

protected:
    std::string m_value;
    size_t m_caretPos;
};

// ============================ 媒体标签占位 ============================
// iframe/svg/embed/object 的降级占位：灰块 + 提示文字

class el_media_placeholder : public el_form_control
{
public:
    el_media_placeholder(const document::ptr& doc, container_win32* container, const char* tag);

    bool    is_replaced() const override { return true; }
    void    draw(uint_ptr hdc, int x, int y, const position* clip,
                 const std::shared_ptr<render_item>& ri) override;
    void    get_content_size(size& sz, int max_width) override;

protected:
    std::string m_tagName;
};

// ============================ 音频 ============================
// <audio> 播放控件：播放/暂停按钮 + 进度条（用 Win32 MCI 播放本地文件）

class el_audio : public el_form_control
{
public:
    el_audio(const document::ptr& doc, container_win32* container);

    bool    is_replaced() const override { return true; }
    void    parse_attributes() override;
    void    draw(uint_ptr hdc, int x, int y, const position* clip,
                 const std::shared_ptr<render_item>& ri) override;
    void    get_content_size(size& sz, int max_width) override;
    void    on_click() override;
    bool    on_lbutton_down() override;

    // 点击命中区域判断
    enum HitZone { HIT_NONE, HIT_PLAY, HIT_BAR };
    HitZone HitTest(int x, int y, const position& pos) const;

protected:
    std::string m_src;
    bool m_playing;
    // MCI 别名（每个控件唯一）
    std::string m_mciAlias;
    void Play();
    void Pause();
    void Stop();
};

// ============================ 画布 ============================
// <canvas> 位图画布：GDI+ 内存位图，JS 通过桥接 API 绘制

class el_canvas : public el_form_control
{
public:
    el_canvas(const document::ptr& doc, container_win32* container);

    bool    is_replaced() const override { return true; }
    void    parse_attributes() override;
    void    draw(uint_ptr hdc, int x, int y, const position* clip,
                 const std::shared_ptr<render_item>& ri) override;
    void    get_content_size(size& sz, int max_width) override;
    void    on_click() override;

    // 按属性初始化画布尺寸（width/height 属性，默认 300x150）
    int  Width() const { return m_width; }
    int  Height() const { return m_height; }

    // ---- JS 桥接绘制 API（由 JsBridge 调用，按 name 找到画布）----
    bool InitBitmap(int w, int h);                          // 创建/重建内存位图
    bool Clear(const char* color);                          // 填充背景色
    bool FillRect(int x, int y, int w, int h, const char* color);
    bool DrawRect(int x, int y, int w, int h, const char* color, float lineWidth);
    bool DrawLine(int x1, int y1, int x2, int y2, const char* color, float lineWidth);
    bool DrawCircle(int cx, int cy, float r, const char* color, bool fill);
    bool DrawText(const char* text, int x, int y, const char* color, float fontSize);
    // 画布内容是否变化（重绘标记）
    void SetDirty() { m_dirty = true; }

protected:
    int  m_width;
    int  m_height;
    bool m_dirty;
    HBITMAP m_hBitmap;      // 内存位图（GDI+ 绘制目标）
    int     m_bitmapW;
    int     m_bitmapH;
};

// ============================ 内嵌框架 ============================
// <iframe src="page.html">：内嵌子文档渲染（本地 HTML）

class el_iframe : public el_form_control
{
public:
    el_iframe(const document::ptr& doc, container_win32* container);

    bool    is_replaced() const override { return true; }
    void    parse_attributes() override;
    void    draw(uint_ptr hdc, int x, int y, const position* clip,
                 const std::shared_ptr<render_item>& ri) override;
    void    get_content_size(size& sz, int max_width) override;
    void    on_click() override;

protected:
    std::string m_src;
    // 子文档（懒加载，随宿主文档一起绘制）
    std::shared_ptr<document> m_childDoc;
    std::shared_ptr<render_item> m_childRender;
    bool m_childLoaded;
};

// ============================ 矢量图 ============================
// <svg>：简化渲染——支持 rect/circle/line/text 基本形状（内嵌子元素）

class el_svg : public el_form_control
{
public:
    el_svg(const document::ptr& doc, container_win32* container);

    bool    is_replaced() const override { return true; }
    void    parse_attributes() override;
    void    draw(uint_ptr hdc, int x, int y, const position* clip,
                 const std::shared_ptr<render_item>& ri) override;
    void    get_content_size(size& sz, int max_width) override;
    void    on_click() override;

protected:
    int m_width;
    int m_height;
};

} // namespace litehtml

#endif // LH_EL_FORM_H
