// ==========================================================================
//  HtmlFormTest.cpp — 表单链路无窗口自测
//
// 目的：不依赖 GUI 交互，直接验证 CDuiHtmlContainer 的表单能力：
//   1. 文档创建与排版
//   2. 按 name 查找控件
//   3. 读取初始值（value / selected / checked）
//   4. 写入值、切换勾选
//   5. 键盘输入（InsertChar / Backspace）
//
// 结果写到 stdout 与 form_test.log。
// ==========================================================================

#include "StdAfx.h"
#include "UIlib.h"
#include "../DuiLib/Control/UIDuiHtmlContainer.h"
#include "../DuiLib/Control/UIDuiHtmlForm.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "GdiPlus.lib")

using namespace DuiLib;

static int g_pass = 0;
static int g_fail = 0;
static FILE* g_log = NULL;

static void Check(bool cond, const char* what)
{
    if (cond) { g_pass++; printf("[PASS] %s\n", what); }
    else      { g_fail++; printf("[FAIL] %s\n", what); }
    if (g_log) fprintf(g_log, "%s %s\n", cond ? "[PASS]" : "[FAIL]", what);
}

// UTF-8 → 宽字符（输出用）
static std::wstring ToW(const std::string& s)
{
    std::wstring w;
    if (s.empty()) return w;
    int n = ::MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), NULL, 0);
    w.resize(n);
    ::MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &w[0], n);
    return w;
}

int _tmain(int argc, _TCHAR* argv[])
{
    ::CoInitialize(NULL);
    g_log = fopen("form_test.log", "w");

    static LPCTSTR HTML =
        _T("<html><body style='margin:0;font-family:微软雅黑;font-size:14px;color:#333'>")
        _T("<div style='background-color:#FFFFFF;border:1px solid #D9E0EA;padding:14px'>")
        _T("<input type='text' name='user' value='张三'/>")
        _T("<input type='password' name='pwd' value='123456'/>")
        _T("<select name='city'><option>北京</option><option selected>上海</option><option>深圳</option></select>")
        _T("<input type='checkbox' name='remember' checked/>")
        _T("<button name='do_login'>登 录</button>")
        _T("</div></body></html>");

    CDuiHtmlContainer container;

    // ---- 1. 创建文档 ----
    bool ok = container.CreateDocument(HTML, NULL);
    Check(ok, "CreateDocument 返回 true");

    // ---- 2. 排版 ----
    int docH = container.Render(760);
    Check(docH > 0, "Render 返回正高度");
    printf("      文档尺寸: w=%d h=%d\n", container.GetDocumentWidth(), docH);
    if (g_log) fprintf(g_log, "doc w=%d h=%d\n", container.GetDocumentWidth(), docH);

    // ---- 3. 按 name 查找 ----
    CHtmlFormElement* pUser = container.FindFormElement(_T("user"));
    CHtmlFormElement* pPwd  = container.FindFormElement(_T("pwd"));
    CHtmlFormElement* pCity = container.FindFormElement(_T("city"));
    CHtmlFormElement* pRem  = container.FindFormElement(_T("remember"));
    CHtmlFormElement* pBtn  = container.FindFormElement(_T("do_login"));
    Check(pUser != NULL, "找到 name=user");
    Check(pPwd  != NULL, "找到 name=pwd");
    Check(pCity != NULL, "找到 name=city");
    Check(pRem  != NULL, "找到 name=remember");
    Check(pBtn  != NULL, "找到 name=do_login");
    Check(container.FindFormElement(_T("nope")) == NULL, "不存在的 name 返回 NULL");

    // ---- 4. 读初始值 ----
    if (pUser) {
        std::wstring w = ToW(pUser->GetFormValue());
        printf("      user 值 = %ws\n", w.c_str());
        Check(pUser->GetFormValue() == "\xE5\xBC\xA0\xE4\xB8\x89", "user 初始值为 张三（UTF-8 正确）");
    }
    if (pPwd) {
        Check(pPwd->GetFormValue() == "123456", "pwd 初始值为 123456");
        Check(pPwd->GetFormType() == HtmlFormPassword, "pwd 是密码框类型");
    }
    if (pCity) {
        std::wstring w = ToW(pCity->GetFormValue());
        printf("      city 值 = %ws (index=%d, 选项数=%d)\n",
               w.c_str(), pCity->GetSelectedIndex(), (int)pCity->GetOptions().size());
        Check(pCity->GetSelectedIndex() == 1, "city 默认选中第 2 项（上海）");
        Check(pCity->GetOptions().size() == 3, "city 有 3 个选项");
    }
    if (pRem) {
        Check(pRem->IsFormChecked(), "remember 初始为勾选");
        Check(pRem->GetFormType() == HtmlFormCheckbox, "remember 是复选框类型");
    }
    if (pBtn) {
        Check(pBtn->GetFormType() == HtmlFormButton, "do_login 是按钮类型");
    }

    // ---- 5. 写值 ----
    if (pUser) {
        pUser->SetFormValue("李四");
        Check(pUser->GetFormValue() == "李四", "SetFormValue 生效");
    }
    if (pCity) {
        pCity->SetSelectedIndex(2);
        Check(pCity->GetSelectedIndex() == 2, "SetSelectedIndex 生效");
        Check(pCity->GetFormValue() == "\xE6\xB7\xB1\xE5\x9C\xB3", "city 值变为 深圳（UTF-8 正确）");
    }
    if (pRem) {
        pRem->SetFormChecked(false);
        Check(!pRem->IsFormChecked(), "SetFormChecked(false) 生效");
    }

    // ---- 6. 键盘输入 ----
    if (pUser) {
        pUser->SetFormValue("abc");
        pUser->SetCaretPos(3);
        pUser->InsertChar(L'd');
        Check(pUser->GetFormValue() == "abcd", "InsertChar 追加字符");
        pUser->Backspace();
        Check(pUser->GetFormValue() == "abc", "Backspace 删除字符");
        pUser->SetCaretPos(1);
        pUser->DeleteChar();
        Check(pUser->GetFormValue() == "ac", "DeleteChar 删除光标处字符");
    }

    // ---- 7. 中文输入（UTF-8 多字节）----
    if (pUser) {
        pUser->SetFormValue("");
        pUser->SetCaretPos(0);
        pUser->InsertChar(L'中');
        pUser->InsertChar(L'文');
        std::wstring w = ToW(pUser->GetFormValue());
        printf("      中文输入结果 = %ws\n", w.c_str());
        Check(w == L"中文", "中文输入正确");
        pUser->Backspace();
        Check(ToW(pUser->GetFormValue()) == L"中", "中文退格正确（按字符而非字节）");
    }

    // ---- 汇总 ----
    printf("\n===== 结果: %d 通过, %d 失败 =====\n", g_pass, g_fail);
    if (g_log) {
        fprintf(g_log, "\n===== %d passed, %d failed =====\n", g_pass, g_fail);
        fclose(g_log);
    }

    ::CoUninitialize();
    return g_fail == 0 ? 0 : 1;
}
