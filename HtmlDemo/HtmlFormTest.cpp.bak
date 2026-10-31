// ==========================================================================
//  HtmlFormTest.cpp — 自研 HTML 引擎无窗口自测
//
//  不依赖 GUI，直接验证 HtmlEngine::HtmlDocument 的完整链路：
//    1. HTML 解析 + CSS 级联 + 布局
//    2. 按 name 查找表单控件
//    3. 读取初始值（value / selected / checked）
//    4. 写入值、切换勾选、切换下拉项
//    5. 键盘输入（中英文、退格按字符处理）
//    6. 命中测试（点击坐标 → 节点）
//
//  结果输出到 stdout 与 form_test.log。
// ==========================================================================

#include "StdAfx.h"
#include "UIlib.h"
#include "../DuiLib/Control/HtmlEngine/HtmlDocument.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "GdiPlus.lib")
#pragma comment(lib, "msimg32.lib")

using namespace HtmlEngine;

static int g_pass = 0;
static int g_fail = 0;
static FILE* g_log = NULL;

static void Check(bool cond, const char* what)
{
    if (cond) { g_pass++; printf("[PASS] %s\n", what); }
    else      { g_fail++; printf("[FAIL] %s\n", what); }
    if (g_log) fprintf(g_log, "%s %s\n", cond ? "[PASS]" : "[FAIL]", what);
}

static std::string WToUtf8(const std::wstring& w)
{
    return WideToUtf8(w);
}

int _tmain(int argc, _TCHAR* argv[])
{
    ::CoInitialize(NULL);
    g_log = fopen("form_test.log", "w");

    static const char* HTML =
        "<!DOCTYPE html><html><head><style>"
        "body { font-family: 微软雅黑; font-size: 14px; color: #333; margin: 0; }"
        ".card { background: #FFF; border: 1px solid #D9E0EA; border-radius: 10px;"
        "        box-shadow: 0 2px 10px rgba(45,91,215,0.08); padding: 16px; }"
        "input[type=text], input[type=password] { width: 200px; height: 28px; }"
        "</style></head><body>"
        "<div class='card'>"
        "<input type='text' name='user' value='张三'/>"
        "<input type='password' name='pwd' value='123456'/>"
        "<select name='city'>"
        "  <option>北京</option><option selected>上海</option><option>深圳</option>"
        "</select>"
        "<input type='checkbox' name='remember' checked/>"
        "<button name='do_login'>登 录</button>"
        "</div></body></html>";

    HtmlDocument doc;

    // ---- 1. 加载 + 排版 ----
    bool ok = doc.LoadFromString(HTML);
    Check(ok, "LoadFromString 返回 true");

    float docH = doc.Relayout(760.0f);
    Check(docH > 0.0f, "Relayout 返回正高度");
    printf("      文档尺寸: w=%.0f h=%.0f\n", doc.GetWidth(), docH);
    if (g_log) fprintf(g_log, "doc w=%.0f h=%.0f\n", doc.GetWidth(), docH);

    // ---- 2. 按 name 查找 ----
    Node* pUser = doc.FindFormByName("user");
    Node* pPwd  = doc.FindFormByName("pwd");
    Node* pCity = doc.FindFormByName("city");
    Node* pRem  = doc.FindFormByName("remember");
    Node* pBtn  = doc.FindFormByName("do_login");
    Check(pUser != NULL, "找到 name=user");
    Check(pPwd  != NULL, "找到 name=pwd");
    Check(pCity != NULL, "找到 name=city");
    Check(pRem  != NULL, "找到 name=remember");
    Check(pBtn  != NULL, "找到 name=do_login");
    Check(doc.FindFormByName("nope") == NULL, "不存在的 name 返回 NULL");

    // ---- 3. 读初始值 ----
    if (pUser) {
        std::string v = doc.GetFormValue(pUser);
        printf("      user 值 = %s\n", v.c_str());
        Check(v == "\xE5\xBC\xA0\xE4\xB8\x89", "user 初始值为 张三（UTF-8 正确）");
    }
    if (pPwd) {
        Check(doc.GetFormValue(pPwd) == "123456", "pwd 初始值为 123456");
    }
    if (pCity) {
        int idx = doc.GetFormSelectedIndex(pCity);
        std::string v = doc.GetFormValue(pCity);
        printf("      city 选中索引=%d 值=%s\n", idx, v.c_str());
        Check(idx == 1, "city 默认选中第 2 项（上海）");
        Check(v == "\xE4\xB8\x8A\xE6\xB5\xB7", "city 值为 上海（UTF-8 正确）");
    }
    if (pRem) {
        Check(doc.GetFormChecked(pRem), "remember 初始为勾选");
    }

    // ---- 4. 写值 ----
    if (pUser) {
        doc.SetFormValue(pUser, "李四");
        Check(doc.GetFormValue(pUser) == "李四", "SetFormValue 生效");
    }
    if (pCity) {
        doc.SetFormSelectedIndex(pCity, 2);
        Check(doc.GetFormSelectedIndex(pCity) == 2, "SetFormSelectedIndex 生效");
        Check(doc.GetFormValue(pCity) == "\xE6\xB7\xB1\xE5\x9C\xB3", "city 值变为 深圳（UTF-8 正确）");
    }
    if (pRem) {
        doc.SetFormChecked(pRem, false);
        Check(!doc.GetFormChecked(pRem), "SetFormChecked(false) 生效");
    }

    // ---- 5. 键盘输入 ----
    if (pUser) {
        doc.SetFormValue(pUser, "");
        doc.SetFocusNode(pUser);

        Check(doc.InputChar(L'a'), "InputChar('a') 返回 true");
        Check(doc.InputChar(L'b'), "InputChar('b') 返回 true");
        Check(doc.InputChar(L'c'), "InputChar('c') 返回 true");
        Check(doc.GetFormValue(pUser) == "abc", "键盘输入 abc 正确");

        Check(doc.InputKeyDown(VK_BACK), "InputKeyDown(VK_BACK) 返回 true");
        Check(doc.GetFormValue(pUser) == "ab", "Backspace 删除一个字符");

        // 中文输入
        Check(doc.InputChar(L'中'), "输入中文 中");
        Check(doc.InputChar(L'文'), "输入中文 文");
        std::string v = doc.GetFormValue(pUser);
        printf("      中文输入结果 = %s\n", v.c_str());
        Check(v == "ab" "\xE4\xB8\xAD" "\xE6\x96\x87", "中文输入正确（ab中文）");

        // 中文退格：按字符而非字节
        doc.InputKeyDown(VK_BACK);
        std::string v2 = doc.GetFormValue(pUser);
        Check(v2 == "ab" "\xE4\xB8\xAD", "中文退格正确（ab中，按字符删除）");
    }

    // ---- 6. 命中测试 ----
    if (pUser) {
        // 取 user 输入框的中心点
        float cx = pUser->x + pUser->width / 2.0f;
        float cy = pUser->y + pUser->height / 2.0f;
        Node* pHit = doc.HitTest(cx, cy);
        Check(pHit != NULL, "命中测试返回非空节点");
        if (pHit != NULL && g_log) {
            fprintf(g_log, "hit at (%.0f,%.0f) -> tag=%s name=%s\n",
                    cx, cy, pHit->tagName.c_str(), pHit->GetAttr("name").c_str());
        }
    }

    // ---- 7. CSS3 解析验证 ----
    if (doc.Loaded()) {
        Node* pCard = NULL;
        // 找 .card（第一个带 class=card 的 div）
        // 通过遍历 body 的子节点
        Node* body = doc.GetBodyNode();
        if (body != NULL) {
            for (size_t i = 0; i < body->children.size(); i++) {
                Node* c = body->children[i];
                if (c->IsElement() && c->GetAttr("class") == "card") { pCard = c; break; }
            }
        }
        if (pCard != NULL) {
            Check(pCard->style.borderRadius.tl > 0, "border-radius 解析生效");
            Check(pCard->style.backgroundColor.IsVisible(), "background-color 解析生效");
            Check(!pCard->style.boxShadows.empty(), "box-shadow 解析生效");
        }
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
