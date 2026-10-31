#ifndef LH_EL_SCRIPT_H
#define LH_EL_SCRIPT_H

#include "html_tag.h"

namespace litehtml
{
	class el_script : public element
	{
		string m_text;
	public:
		explicit el_script(const std::shared_ptr<document>& doc);

		void parse_attributes() override;
		bool appendChild(const ptr &el) override;
		string_id tag() const override;
		const char*	get_tagName() const override;

		// 宿主获取脚本内容（DuiLib 的 JS 桥接用）
		const string& GetScriptText() const { return m_text; }
	};
}

#endif  // LH_EL_SCRIPT_H
