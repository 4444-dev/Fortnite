#include <workspace/interface/menu.hpp>
#include <workspace/interface/settings_store.hpp>

#include <Windows.h>
#include <thirdparty/imgui/imgui.h>

#include <algorithm>
#include <cctype>
#include <cstdio>

using menu::cfg;

namespace {

const menu::Settings cfg_default{};

ImFont* font_regular = nullptr;
ImFont* font_semibold = nullptr;
ImFont* font_title = nullptr;
int active_tab = 0;

namespace palette {
constexpr ImU32 background = IM_COL32(8, 10, 18, 248);
constexpr ImU32 panel = IM_COL32(15, 19, 32, 250);
constexpr ImU32 panel_raised = IM_COL32(21, 26, 43, 252);
constexpr ImU32 border = IM_COL32(39, 45, 69, 255);
constexpr ImU32 border_hot = IM_COL32(86, 69, 120, 255);
constexpr ImU32 text = IM_COL32(244, 245, 250, 255);
constexpr ImU32 muted = IM_COL32(151, 160, 184, 255);
constexpr ImU32 subtle = IM_COL32(105, 113, 139, 255);
constexpr ImU32 success = IM_COL32(56, 228, 123, 255);
constexpr ImU32 warning = IM_COL32(247, 185, 85, 255);
constexpr ImU32 danger = IM_COL32(255, 92, 112, 255);
constexpr ImU32 blue = IM_COL32(108, 166, 255, 255);
}

ImU32 Accent(float alpha = 1.0f) {
	return ImGui::ColorConvertFloat4ToU32(
		ImVec4(
			cfg.accent[0],
			cfg.accent[1],
			cfg.accent[2],
			std::clamp(alpha, 0.0f, 1.0f)
		)
	);
}

ImVec4 ToVec4(ImU32 color) {
	return ImGui::ColorConvertU32ToFloat4(color);
}

void DrawBrand(
	ImDrawList* draw,
	const ImVec2& pos
) {
	draw->AddText(
		font_title,
		24.0f,
		pos,
		palette::text,
		"NE"
	);

	const float prefix =
		font_title->CalcTextSizeA(
			24.0f,
			1000.0f,
			0.0f,
			"NE"
		).x;

	draw->AddText(
		font_title,
		24.0f,
		ImVec2(pos.x + prefix, pos.y),
		Accent(),
		"XUS"
	);

	draw->AddText(
		font_regular,
		10.5f,
		ImVec2(pos.x, pos.y + 29.0f),
		palette::subtle,
		"PREMIUM INTERFACE"
	);
}

void DrawChip(
	ImDrawList* draw,
	const ImVec2& pos,
	const char* text,
	ImU32 color,
	bool withDot = true
) {
	const ImVec2 textSize =
		font_regular->CalcTextSizeA(
			11.5f,
			1000.0f,
			0.0f,
			text
		);

	const float width =
		textSize.x +
		(withDot ? 31.0f : 20.0f);

	const ImVec2 end(
		pos.x + width,
		pos.y + 27.0f
	);

	draw->AddRectFilled(
		pos,
		end,
		IM_COL32(19, 23, 38, 235),
		8.0f
	);
	draw->AddRect(
		pos,
		end,
		IM_COL32(48, 55, 82, 225),
		8.0f
	);

	float textX =
		pos.x + 10.0f;

	if (withDot) {
		draw->AddCircleFilled(
			ImVec2(
				pos.x + 13.0f,
				pos.y + 13.5f
			),
			3.5f,
			color
		);
		textX += 13.0f;
	}

	draw->AddText(
		font_regular,
		11.5f,
		ImVec2(textX, pos.y + 6.0f),
		color,
		text
	);
}

bool TabButton(
	const char* label,
	int index,
	float width
) {
	const bool selected =
		active_tab == index;

	ImGui::PushID(index);
	const ImVec2 start =
		ImGui::GetCursorScreenPos();

	const bool pressed =
		ImGui::InvisibleButton(
			"##tab",
			ImVec2(width, 38.0f)
		);
	const bool hovered =
		ImGui::IsItemHovered();

	ImDrawList* draw =
		ImGui::GetWindowDrawList();

	if (selected || hovered) {
		draw->AddRectFilled(
			start,
			ImVec2(
				start.x + width,
				start.y + 38.0f
			),
			selected
				? IM_COL32(72, 36, 119, 145)
				: IM_COL32(30, 34, 51, 180),
			9.0f
		);
	}

	const ImVec2 textSize =
		ImGui::CalcTextSize(label);
	draw->AddText(
		ImVec2(
			start.x +
				(width - textSize.x) * 0.5f,
			start.y + 10.0f
		),
		selected
			? palette::text
			: hovered
				? IM_COL32(214, 217, 232, 255)
				: palette::muted,
		label
	);

	if (selected) {
		draw->AddRectFilled(
			ImVec2(
				start.x + 18.0f,
				start.y + 35.0f
			),
			ImVec2(
				start.x + width - 18.0f,
				start.y + 38.0f
			),
			Accent(),
			1.5f
		);
	}

	ImGui::PopID();

	if (pressed) {
		active_tab = index;
	}

	return pressed;
}

void BeginPanel(
	const char* id,
	const ImVec2& size
) {
	ImGui::PushStyleVar(
		ImGuiStyleVar_ChildRounding,
		11.0f
	);
	ImGui::PushStyleVar(
		ImGuiStyleVar_ChildBorderSize,
		1.0f
	);
	ImGui::PushStyleVar(
		ImGuiStyleVar_WindowPadding,
		ImVec2(18.0f, 16.0f)
	);

	ImGui::PushStyleColor(
		ImGuiCol_ChildBg,
		ToVec4(palette::panel)
	);
	ImGui::PushStyleColor(
		ImGuiCol_Border,
		ToVec4(palette::border)
	);

	ImGui::BeginChild(
		id,
		size,
		true,
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoScrollWithMouse
	);
}

void EndPanel() {
	ImGui::EndChild();
	ImGui::PopStyleColor(2);
	ImGui::PopStyleVar(3);
}

void PanelHeader(
	const char* title,
	const char* subtitle
) {
	ImGui::PushFont(font_semibold);
	ImGui::TextUnformatted(title);
	ImGui::PopFont();

	if (subtitle && *subtitle) {
		ImGui::PushStyleColor(
			ImGuiCol_Text,
			ToVec4(palette::muted)
		);
		ImGui::TextWrapped("%s", subtitle);
		ImGui::PopStyleColor();
	}

	ImGui::Dummy(ImVec2(0.0f, 8.0f));
}

bool Switch(
	const char* id,
	bool* value
) {
	ImGui::PushID(id);

	const ImVec2 pos =
		ImGui::GetCursorScreenPos();
	const ImVec2 size(
		42.0f,
		22.0f
	);

	const bool pressed =
		ImGui::InvisibleButton(
			"##switch",
			size
		);
	const bool hovered =
		ImGui::IsItemHovered();

	if (pressed) {
		*value = !*value;
	}

	ImDrawList* draw =
		ImGui::GetWindowDrawList();

	draw->AddRectFilled(
		pos,
		ImVec2(
			pos.x + size.x,
			pos.y + size.y
		),
		*value
			? Accent(
				hovered ? 1.0f : 0.88f
			)
			: hovered
				? IM_COL32(67, 74, 102, 255)
				: IM_COL32(45, 51, 73, 255),
		11.0f
	);

	const float knobX =
		*value
			? pos.x + 31.0f
			: pos.x + 11.0f;

	draw->AddCircleFilled(
		ImVec2(
			knobX,
			pos.y + 11.0f
		),
		7.0f,
		IM_COL32(246, 247, 252, 255)
	);

	ImGui::PopID();
	return pressed;
}

void ToggleRow(
	const char* label,
	const char* hint,
	bool* value,
	float* color = nullptr
) {
	ImGui::PushID(label);

	ImGui::PushFont(font_regular);
	ImGui::TextUnformatted(label);
	ImGui::PopFont();

	if (hint && *hint) {
		ImGui::PushStyleColor(
			ImGuiCol_Text,
			ToVec4(palette::subtle)
		);
		ImGui::TextUnformatted(hint);
		ImGui::PopStyleColor();
	}

	const float right =
		ImGui::GetWindowContentRegionMax().x;

	const float y =
		hint && *hint
			? ImGui::GetCursorPosY() - 33.0f
			: ImGui::GetCursorPosY() - 20.0f;

	if (color) {
		ImGui::SetCursorPos(
			ImVec2(
				right - 87.0f,
				y
			)
		);
		ImGui::SetNextItemWidth(31.0f);
		ImGui::ColorEdit4(
			"##color",
			color,
			ImGuiColorEditFlags_NoInputs |
			ImGuiColorEditFlags_NoLabel |
			ImGuiColorEditFlags_AlphaPreviewHalf
		);

		ImGui::SameLine(
			0.0f,
			10.0f
		);
	} else {
		ImGui::SetCursorPos(
			ImVec2(
				right - 42.0f,
				y
			)
		);
	}

	Switch("toggle", value);

	ImGui::SetCursorPosY(
		(std::max)(
			ImGui::GetCursorPosY(),
			y + 30.0f
		)
	);
	ImGui::Separator();
	ImGui::Dummy(ImVec2(0.0f, 4.0f));

	ImGui::PopID();
}

void ComboRow(
	const char* label,
	const char* hint,
	int* value,
	const char* const* items,
	int count
) {
	ImGui::PushID(label);

	ImGui::TextUnformatted(label);
	if (hint && *hint) {
		ImGui::TextDisabled("%s", hint);
	}

	const float width = 150.0f;
	const float right =
		ImGui::GetWindowContentRegionMax().x;
	const float y =
		hint && *hint
			? ImGui::GetCursorPosY() - 38.0f
			: ImGui::GetCursorPosY() - 25.0f;

	ImGui::SetCursorPos(
		ImVec2(
			right - width,
			y
		)
	);
	ImGui::SetNextItemWidth(width);
	ImGui::Combo(
		"##combo",
		value,
		items,
		count
	);

	ImGui::SetCursorPosY(
		(std::max)(
			ImGui::GetCursorPosY(),
			y + 40.0f
		)
	);
	ImGui::Separator();
	ImGui::Dummy(ImVec2(0.0f, 4.0f));

	ImGui::PopID();
}

void StatusRow(
	const char* label,
	const char* value,
	ImU32 valueColor = palette::text
) {
	ImGui::TextDisabled("%s", label);

	const ImVec2 valueSize =
		ImGui::CalcTextSize(value);

	ImGui::SameLine();
	ImGui::SetCursorPosX(
		ImGui::GetWindowContentRegionMax().x -
		valueSize.x
	);

	ImGui::PushStyleColor(
		ImGuiCol_Text,
		ToVec4(valueColor)
	);
	ImGui::TextUnformatted(value);
	ImGui::PopStyleColor();

	ImGui::Dummy(ImVec2(0.0f, 3.0f));
}

void MetricCard(
	const char* id,
	const char* label,
	const char* value,
	ImU32 color,
	const ImVec2& size
) {
	ImGui::PushID(id);

	const ImVec2 start =
		ImGui::GetCursorScreenPos();

	ImGui::Dummy(size);

	ImDrawList* draw =
		ImGui::GetWindowDrawList();

	const ImVec2 end(
		start.x + size.x,
		start.y + size.y
	);

	draw->AddRectFilled(
		start,
		end,
		palette::panel_raised,
		9.0f
	);
	draw->AddRect(
		start,
		end,
		palette::border,
		9.0f
	);
	draw->AddCircleFilled(
		ImVec2(
			start.x + 15.0f,
			start.y + 17.0f
		),
		4.0f,
		color
	);

	draw->AddText(
		font_regular,
		11.5f,
		ImVec2(
			start.x + 27.0f,
			start.y + 10.0f
		),
		palette::muted,
		label
	);
	draw->AddText(
		font_semibold,
		17.0f,
		ImVec2(
			start.x + 14.0f,
			start.y + 34.0f
		),
		color,
		value
	);

	ImGui::PopID();
}

void KeyName(
	int key,
	char* out,
	size_t size
) {
	const char* name = "NONE";

	if (key != ImGuiKey_None) {
		name =
			ImGui::GetKeyName(
				static_cast<ImGuiKey>(key)
			);
	}

	size_t index = 0;
	for (
		;
		name[index] &&
		index + 1 < size;
		++index
	) {
		out[index] =
			static_cast<char>(
				std::toupper(
					static_cast<unsigned char>(
						name[index]
					)
				)
			);
	}
	out[index] = '\0';
}

bool KeybindRow(
	const char* label,
	int* key
) {
	static ImGuiID waitingId = 0;
	static int waitingFrame = 0;

	ImGui::TextUnformatted(label);
	ImGui::TextDisabled(
		"Click the key field to rebind."
	);

	char keyText[48]{};
	KeyName(
		*key,
		keyText,
		sizeof(keyText)
	);

	const float width = 128.0f;
	const float right =
		ImGui::GetWindowContentRegionMax().x;

	ImGui::SetCursorPos(
		ImVec2(
			right - width,
			ImGui::GetCursorPosY() - 39.0f
		)
	);

	ImGui::PushID(label);
	const ImGuiID id =
		ImGui::GetID("##keybind");

	const bool waiting =
		waitingId == id;

	char buttonLabel[96]{};
	std::snprintf(
		buttonLabel,
		sizeof(buttonLabel),
		"%s###keybind_button",
		waiting
			? "PRESS A KEY..."
			: keyText
	);

	if (ImGui::Button(
		buttonLabel,
		ImVec2(width, 34.0f)
	)) {
		waitingId = id;
		waitingFrame =
			ImGui::GetFrameCount();
	}

	bool changed = false;

	if (
		waiting &&
		ImGui::GetFrameCount() >
			waitingFrame
	) {
		if (ImGui::IsKeyPressed(
			ImGuiKey_Escape,
			false
		)) {
			waitingId = 0;
		} else {
			for (
				int candidate =
					ImGuiKey_NamedKey_BEGIN;
				candidate <
					ImGuiKey_NamedKey_END;
				++candidate
			) {
				if (
					candidate ==
						ImGuiKey_MouseLeft ||
					candidate ==
						ImGuiKey_MouseWheelX ||
					candidate ==
						ImGuiKey_MouseWheelY
				) {
					continue;
				}

				if (ImGui::IsKeyPressed(
					static_cast<ImGuiKey>(
						candidate
					),
					false
				)) {
					*key = candidate;
					waitingId = 0;
					changed = true;
					break;
				}
			}
		}
	}

	ImGui::PopID();

	ImGui::SetCursorPosY(
		ImGui::GetCursorPosY() + 6.0f
	);
	ImGui::Separator();
	ImGui::Dummy(ImVec2(0.0f, 6.0f));

	return changed;
}

void RenderOverview(
	const ImVec2& available,
	const menu::RuntimeStatus& status
) {
	const float gap = 14.0f;
	const float leftWidth =
		available.x * 0.55f;
	const float rightWidth =
		available.x -
		leftWidth -
		gap;

	BeginPanel(
		"##overview_session",
		ImVec2(
			leftWidth,
			available.y
		)
	);

	PanelHeader(
		"Session",
		"Live status for the current Nexus overlay session."
	);

	const float metricGap = 10.0f;
	const float metricWidth =
		(
			ImGui::GetContentRegionAvail().x -
			metricGap
		) * 0.5f;

	char players[24]{};
	std::snprintf(
		players,
		sizeof(players),
		"%d",
		status.player_count
	);

	char actors[24]{};
	std::snprintf(
		actors,
		sizeof(actors),
		"%d",
		status.actor_count
	);

	MetricCard(
		"world",
		"WORLD",
		status.world_valid
			? "READY"
			: "WAITING",
		status.world_valid
			? palette::success
			: palette::warning,
		ImVec2(
			metricWidth,
			70.0f
		)
	);

	ImGui::SameLine(
		0.0f,
		metricGap
	);
	MetricCard(
		"camera",
		"CAMERA",
		status.camera_valid
			? "READY"
			: "WAITING",
		status.camera_valid
			? palette::success
			: palette::warning,
		ImVec2(
			metricWidth,
			70.0f
		)
	);

	ImGui::Dummy(ImVec2(0.0f, 10.0f));

	MetricCard(
		"players",
		"PLAYERS",
		players,
		palette::blue,
		ImVec2(
			metricWidth,
			70.0f
		)
	);

	ImGui::SameLine(
		0.0f,
		metricGap
	);
	MetricCard(
		"actors",
		"ACTORS",
		actors,
		Accent(),
		ImVec2(
			metricWidth,
			70.0f
		)
	);

	ImGui::Dummy(ImVec2(0.0f, 14.0f));
	ImGui::TextDisabled(
		"Overlay Window"
	);
	ImGui::TextUnformatted(
		"Bound to the active Fortnite monitor."
	);

	EndPanel();

	ImGui::SameLine(
		0.0f,
		gap
	);

	BeginPanel(
		"##overview_performance",
		ImVec2(
			rightWidth,
			available.y
		)
	);

	PanelHeader(
		"Performance",
		"Renderer timing and UI scale."
	);

	char value[64]{};

	std::snprintf(
		value,
		sizeof(value),
		"%.0f FPS",
		status.fps
	);
	StatusRow(
		"Renderer",
		value,
		status.fps >= 55.0f
			? palette::success
			: palette::warning
	);

	std::snprintf(
		value,
		sizeof(value),
		"%.2f ms",
		status.frame_ms
	);
	StatusRow(
		"Frame Time",
		value
	);

	std::snprintf(
		value,
		sizeof(value),
		"%.3f ms",
		status.engine_ms
	);
	StatusRow(
		"Engine",
		value
	);

	std::snprintf(
		value,
		sizeof(value),
		"%.3f ms",
		status.actors_ms
	);
	StatusRow(
		"Actor Scan",
		value
	);

	std::snprintf(
		value,
		sizeof(value),
		"%.3f ms",
		status.players_ms
	);
	StatusRow(
		"Player Cache",
		value
	);

	std::snprintf(
		value,
		sizeof(value),
		"%.2fx",
		status.dpi_scale
	);
	StatusRow(
		"DPI Scale",
		value
	);

	ImGui::Dummy(ImVec2(0.0f, 10.0f));
	ImGui::Separator();
	ImGui::Dummy(ImVec2(0.0f, 8.0f));

	StatusRow(
		"Build",
		"1.0.6",
		Accent()
	);
	StatusRow(
		"Game",
		"Fortnite",
		palette::text
	);

	EndPanel();
}

void RenderDisplay(
	const ImVec2& available
) {
	static const char* boxStyles[] = {
		"Full",
		"Corner"
	};
	static const char* origins[] = {
		"Bottom",
		"Center",
		"Top"
	};

	const float gap = 14.0f;
	const float column =
		(available.x - gap) * 0.5f;

	BeginPanel(
		"##display_primary",
		ImVec2(
			column,
			available.y
		)
	);

	PanelHeader(
		"Player Overlay",
		"Presentation controls for the existing overlay."
	);

	ToggleRow(
		"Bounding Box",
		"Show the configured box style.",
		&cfg.box,
		cfg.box_color
	);

	ComboRow(
		"Box Style",
		"Choose the box presentation.",
		&cfg.box_style,
		boxStyles,
		IM_ARRAYSIZE(boxStyles)
	);

	ToggleRow(
		"Box Fill",
		"Add a subtle fill inside the box.",
		&cfg.box_fill
	);

	ToggleRow(
		"Skeleton",
		"Show the configured skeleton overlay.",
		&cfg.skeleton,
		cfg.skeleton_color
	);

	ToggleRow(
		"Distance",
		"Show the existing distance label.",
		&cfg.distance
	);

	EndPanel();

	ImGui::SameLine(
		0.0f,
		gap
	);

	BeginPanel(
		"##display_secondary",
		ImVec2(
			available.x -
				column -
				gap,
			available.y
		)
	);

	PanelHeader(
		"Lines",
		"Line presentation and color controls."
	);

	ToggleRow(
		"Snaplines",
		"Show the configured line overlay.",
		&cfg.snaplines,
		cfg.snapline_color
	);

	ComboRow(
		"Origin",
		"Choose where lines start.",
		&cfg.snapline_origin,
		origins,
		IM_ARRAYSIZE(origins)
	);

	ImGui::Dummy(ImVec2(0.0f, 10.0f));

	ImGui::PushStyleColor(
		ImGuiCol_Text,
		ToVec4(palette::muted)
	);
	ImGui::TextWrapped(
		"All visual changes are applied immediately. "
		"Use Settings to save them for the next session."
	);
	ImGui::PopStyleColor();

	EndPanel();
}

void RenderSettings(
	const ImVec2& available
) {
	const float gap = 14.0f;
	const float column =
		(available.x - gap) * 0.5f;

	BeginPanel(
		"##settings_interface",
		ImVec2(
			column,
			available.y
		)
	);

	PanelHeader(
		"Interface",
		"Customize the local Nexus menu."
	);

	KeybindRow(
		"Menu Key",
		&cfg.menu_key
	);

	ImGui::TextUnformatted(
		"Accent Color"
	);
	ImGui::TextDisabled(
		"Used across active controls and highlights."
	);

	const float colorWidth = 160.0f;
	const float right =
		ImGui::GetWindowContentRegionMax().x;

	ImGui::SetCursorPos(
		ImVec2(
			right - colorWidth,
			ImGui::GetCursorPosY() - 39.0f
		)
	);
	ImGui::SetNextItemWidth(
		colorWidth
	);
	ImGui::ColorEdit4(
		"##accent",
		cfg.accent,
		ImGuiColorEditFlags_NoInputs |
		ImGuiColorEditFlags_AlphaPreviewHalf
	);

	ImGui::SetCursorPosY(
		ImGui::GetCursorPosY() + 8.0f
	);

	EndPanel();

	ImGui::SameLine(
		0.0f,
		gap
	);

	BeginPanel(
		"##settings_config",
		ImVec2(
			available.x -
				column -
				gap,
			available.y
		)
	);

	PanelHeader(
		"Configuration",
		"Save or restore local interface settings."
	);

	if (ImGui::Button(
		"SAVE CONFIGURATION",
		ImVec2(
			-1.0f,
			40.0f
		)
	)) {
		(void)app_settings::Save(cfg);
	}

	if (ImGui::Button(
		"LOAD CONFIGURATION",
		ImVec2(
			-1.0f,
			40.0f
		)
	)) {
		(void)app_settings::Load(cfg);
	}

	ImGui::PushStyleColor(
		ImGuiCol_Button,
		ImVec4(
			0.15f,
			0.16f,
			0.22f,
			1.0f
		)
	);
	ImGui::PushStyleColor(
		ImGuiCol_ButtonHovered,
		ImVec4(
			0.20f,
			0.21f,
			0.29f,
			1.0f
		)
	);

	if (ImGui::Button(
		"RESET TO DEFAULTS",
		ImVec2(
			-1.0f,
			40.0f
		)
	)) {
		cfg = cfg_default;
	}

	ImGui::PopStyleColor(2);

	ImGui::Dummy(
		ImVec2(
			0.0f,
			12.0f
		)
	);
	ImGui::Separator();
	ImGui::Dummy(
		ImVec2(
			0.0f,
			8.0f
		)
	);
	ImGui::TextDisabled(
		"Settings are stored locally under Nexus."
	);

	EndPanel();
}

void ApplyStyle() {
	ImGuiStyle& style =
		ImGui::GetStyle();

	style.WindowRounding = 13.0f;
	style.ChildRounding = 11.0f;
	style.FrameRounding = 9.0f;
	style.PopupRounding = 10.0f;
	style.GrabRounding = 9.0f;
	style.ScrollbarRounding = 9.0f;

	style.WindowPadding =
		ImVec2(0.0f, 0.0f);
	style.FramePadding =
		ImVec2(11.0f, 8.0f);
	style.ItemSpacing =
		ImVec2(10.0f, 10.0f);
	style.ItemInnerSpacing =
		ImVec2(8.0f, 6.0f);

	style.WindowBorderSize = 0.0f;
	style.ChildBorderSize = 1.0f;
	style.PopupBorderSize = 1.0f;
	style.FrameBorderSize = 1.0f;

	ImVec4* colors =
		style.Colors;

	colors[ImGuiCol_Text] =
		ToVec4(palette::text);
	colors[ImGuiCol_TextDisabled] =
		ToVec4(palette::muted);
	colors[ImGuiCol_WindowBg] =
		ToVec4(palette::background);
	colors[ImGuiCol_ChildBg] =
		ToVec4(palette::panel);
	colors[ImGuiCol_PopupBg] =
		ToVec4(palette::panel_raised);
	colors[ImGuiCol_Border] =
		ToVec4(palette::border);
	colors[ImGuiCol_FrameBg] =
		ImVec4(
			0.075f,
			0.086f,
			0.14f,
			1.0f
		);
	colors[ImGuiCol_FrameBgHovered] =
		ImVec4(
			0.11f,
			0.12f,
			0.20f,
			1.0f
		);
	colors[ImGuiCol_FrameBgActive] =
		ImVec4(
			0.13f,
			0.14f,
			0.24f,
			1.0f
		);
	colors[ImGuiCol_Button] =
		ImVec4(
			0.43f,
			0.13f,
			0.87f,
			1.0f
		);
	colors[ImGuiCol_ButtonHovered] =
		ImVec4(
			0.58f,
			0.22f,
			1.0f,
			1.0f
		);
	colors[ImGuiCol_ButtonActive] =
		ImVec4(
			0.35f,
			0.09f,
			0.73f,
			1.0f
		);
	colors[ImGuiCol_CheckMark] =
		ImVec4(
			0.72f,
			0.36f,
			1.0f,
			1.0f
		);
	colors[ImGuiCol_Header] =
		ImVec4(
			0.34f,
			0.15f,
			0.61f,
			0.75f
		);
	colors[ImGuiCol_HeaderHovered] =
		ImVec4(
			0.47f,
			0.20f,
			0.82f,
			0.88f
		);
	colors[ImGuiCol_HeaderActive] =
		ImVec4(
			0.42f,
			0.16f,
			0.74f,
			1.0f
		);
}

} // namespace

void menu::setup() {
	ImGuiIO& io =
		ImGui::GetIO();

	font_regular =
		io.Fonts->AddFontFromFileTTF(
			"C:\\Windows\\Fonts\\segoeui.ttf",
			14.0f,
			nullptr,
			io.Fonts->GetGlyphRangesDefault()
		);

	if (!font_regular) {
		font_regular =
			io.Fonts->AddFontDefault();
	}

	font_semibold =
		io.Fonts->AddFontFromFileTTF(
			"C:\\Windows\\Fonts\\seguisb.ttf",
			16.0f,
			nullptr,
			io.Fonts->GetGlyphRangesDefault()
		);

	if (!font_semibold) {
		font_semibold =
			font_regular;
	}

	font_title =
		io.Fonts->AddFontFromFileTTF(
			"C:\\Windows\\Fonts\\seguisb.ttf",
			22.0f,
			nullptr,
			io.Fonts->GetGlyphRangesDefault()
		);

	if (!font_title) {
		font_title =
			font_semibold;
	}

	io.FontDefault =
		font_regular;

	ApplyStyle();
}

void menu::render(
	const RuntimeStatus& status
) {
	const ImVec2 size(
		720.0f,
		520.0f
	);

	ImGui::SetNextWindowPos(
		ImVec2(
			60.0f,
			60.0f
		),
		ImGuiCond_FirstUseEver
	);
	ImGui::SetNextWindowSize(size);

	ImGui::Begin(
		"##nexus_overlay_menu",
		nullptr,
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoScrollWithMouse |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoCollapse
	);

	ImDrawList* draw =
		ImGui::GetWindowDrawList();

	const ImVec2 windowPos =
		ImGui::GetWindowPos();
	const ImVec2 windowSize =
		ImGui::GetWindowSize();

	const ImVec2 windowEnd(
		windowPos.x + windowSize.x,
		windowPos.y + windowSize.y
	);

	draw->AddRectFilled(
		windowPos,
		windowEnd,
		palette::background,
		13.0f
	);
	draw->AddRect(
		windowPos,
		windowEnd,
		palette::border,
		13.0f,
		0,
		1.0f
	);

	// Purple premium accent.
	draw->AddRectFilledMultiColor(
		ImVec2(
			windowPos.x + 1.0f,
			windowPos.y + 1.0f
		),
		ImVec2(
			windowEnd.x - 1.0f,
			windowPos.y + 5.0f
		),
		Accent(),
		Accent(0.78f),
		Accent(0.30f),
		Accent(0.64f)
	);

	draw->AddCircleFilled(
		ImVec2(
			windowEnd.x - 92.0f,
			windowPos.y + 82.0f
		),
		105.0f,
		IM_COL32(132, 62, 230, 14),
		64
	);

	DrawBrand(
		draw,
		ImVec2(
			windowPos.x + 20.0f,
			windowPos.y + 17.0f
		)
	);

	DrawChip(
		draw,
		ImVec2(
			windowEnd.x - 265.0f,
			windowPos.y + 20.0f
		),
		"FORTNITE",
		palette::blue,
		false
	);

	DrawChip(
		draw,
		ImVec2(
			windowEnd.x - 158.0f,
			windowPos.y + 20.0f
		),
		status.world_valid
			? "SESSION READY"
			: "WAITING",
		status.world_valid
			? palette::success
			: palette::warning,
		true
	);

	draw->AddLine(
		ImVec2(
			windowPos.x + 18.0f,
			windowPos.y + 66.0f
		),
		ImVec2(
			windowEnd.x - 18.0f,
			windowPos.y + 66.0f
		),
		IM_COL32(42, 48, 72, 210)
	);

	ImGui::SetCursorPos(
		ImVec2(
			18.0f,
			78.0f
		)
	);

	const float tabGap =
		8.0f;
	const float tabWidth =
		(
			windowSize.x -
			36.0f -
			tabGap * 2.0f
		) / 3.0f;

	TabButton(
		"Overview",
		0,
		tabWidth
	);
	ImGui::SameLine(
		0.0f,
		tabGap
	);
	TabButton(
		"Display",
		1,
		tabWidth
	);
	ImGui::SameLine(
		0.0f,
		tabGap
	);
	TabButton(
		"Settings",
		2,
		tabWidth
	);

	ImGui::SetCursorPos(
		ImVec2(
			18.0f,
			130.0f
		)
	);

	const ImVec2 available(
		windowSize.x - 36.0f,
		windowSize.y - 166.0f
	);

	if (active_tab == 0) {
		RenderOverview(
			available,
			status
		);
	} else if (active_tab == 1) {
		RenderDisplay(
			available
		);
	} else {
		RenderSettings(
			available
		);
	}

	draw->AddText(
		font_regular,
		11.0f,
		ImVec2(
			windowPos.x + 20.0f,
			windowEnd.y - 22.0f
		),
		palette::subtle,
		"INSERT  -  Toggle Nexus"
	);

	draw->AddText(
		font_regular,
		11.0f,
		ImVec2(
			windowEnd.x - 84.0f,
			windowEnd.y - 22.0f
		),
		palette::subtle,
		"BUILD 1.0.6"
	);

	ImGui::End();
}
