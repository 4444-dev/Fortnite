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
ImFont* font_title = nullptr;
int active_tab = 0;

namespace palette {
constexpr ImU32 background = IM_COL32(8, 10, 18, 248);
constexpr ImU32 panel = IM_COL32(16, 20, 33, 248);
constexpr ImU32 panel_alt = IM_COL32(21, 26, 43, 248);
constexpr ImU32 border = IM_COL32(39, 45, 69, 255);
constexpr ImU32 text = IM_COL32(244, 245, 250, 255);
constexpr ImU32 muted = IM_COL32(150, 158, 183, 255);
constexpr ImU32 success = IM_COL32(56, 228, 123, 255);
constexpr ImU32 warning = IM_COL32(247, 185, 85, 255);
constexpr ImU32 danger = IM_COL32(255, 92, 112, 255);
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

void DrawBrand() {
	ImDrawList* draw = ImGui::GetWindowDrawList();
	const ImVec2 pos = ImGui::GetCursorScreenPos();

	ImGui::PushFont(font_title);
	const ImVec2 neSize = ImGui::CalcTextSize("NE");
	ImGui::PopFont();

	draw->AddText(
		font_title,
		22.0f,
		pos,
		palette::text,
		"NE"
	);
	draw->AddText(
		font_title,
		22.0f,
		ImVec2(pos.x + neSize.x, pos.y),
		Accent(),
		"XUS"
	);

	ImGui::Dummy(ImVec2(96.0f, 26.0f));
}

bool Switch(const char* id, bool* value) {
	ImGui::PushID(id);

	const ImVec2 pos = ImGui::GetCursorScreenPos();
	const ImVec2 size(42.0f, 22.0f);
	const bool pressed =
		ImGui::InvisibleButton("##switch", size);

	if (pressed) {
		*value = !*value;
	}

	const bool hovered = ImGui::IsItemHovered();
	ImDrawList* draw = ImGui::GetWindowDrawList();

	const ImU32 track =
		*value
			? Accent(hovered ? 1.0f : 0.88f)
			: hovered
				? IM_COL32(66, 73, 100, 255)
				: IM_COL32(46, 52, 75, 255);

	draw->AddRectFilled(
		pos,
		pos + size,
		track,
		11.0f
	);

	const float knobX =
		*value
			? pos.x + size.x - 11.0f
			: pos.x + 11.0f;

	draw->AddCircleFilled(
		ImVec2(knobX, pos.y + 11.0f),
		7.0f,
		IM_COL32(245, 247, 255, 255)
	);

	ImGui::PopID();
	return pressed;
}

void SettingToggle(
	const char* label,
	bool* value,
	float* color = nullptr
) {
	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted(label);

	const float available =
		ImGui::GetContentRegionAvail().x;

	if (color) {
		ImGui::SameLine(
			ImGui::GetCursorPosX() +
			(std::max)(0.0f, available - 82.0f)
		);

		ImGui::SetNextItemWidth(30.0f);
		ImGui::ColorEdit4(
			"##color",
			color,
			ImGuiColorEditFlags_NoInputs |
			ImGuiColorEditFlags_NoLabel |
			ImGuiColorEditFlags_AlphaPreviewHalf
		);

		ImGui::SameLine(0.0f, 10.0f);
	} else {
		ImGui::SameLine(
			ImGui::GetCursorPosX() +
			(std::max)(0.0f, available - 42.0f)
		);
	}

	Switch(label, value);
	ImGui::Dummy(ImVec2(0.0f, 4.0f));
}

void ComboRow(
	const char* label,
	int* value,
	const char* const* items,
	int count
) {
	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted(label);

	ImGui::SameLine();
	const float width = 160.0f;
	ImGui::SetCursorPosX(
		ImGui::GetWindowContentRegionMax().x -
		width
	);
	ImGui::SetNextItemWidth(width);

	ImGui::PushID(label);
	ImGui::Combo(
		"##combo",
		value,
		items,
		count
	);
	ImGui::PopID();

	ImGui::Dummy(ImVec2(0.0f, 4.0f));
}

void BeginCard(
	const char* id,
	const ImVec2& size
) {
	ImGui::PushStyleVar(
		ImGuiStyleVar_ChildRounding,
		10.0f
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
		ImGui::ColorConvertU32ToFloat4(
			palette::panel
		)
	);
	ImGui::PushStyleColor(
		ImGuiCol_Border,
		ImGui::ColorConvertU32ToFloat4(
			palette::border
		)
	);

	ImGui::BeginChild(
		id,
		size,
		true,
		ImGuiWindowFlags_NoScrollbar
	);
}

void EndCard() {
	ImGui::EndChild();
	ImGui::PopStyleColor(2);
	ImGui::PopStyleVar(3);
}

void CardHeader(
	const char* title,
	const char* subtitle = nullptr
) {
	ImGui::PushFont(font_title);
	ImGui::TextUnformatted(title);
	ImGui::PopFont();

	if (subtitle && *subtitle) {
		ImGui::PushStyleColor(
			ImGuiCol_Text,
			ImGui::ColorConvertU32ToFloat4(
				palette::muted
			)
		);
		ImGui::TextWrapped("%s", subtitle);
		ImGui::PopStyleColor();
	}

	ImGui::Dummy(ImVec2(0.0f, 8.0f));
}

void StatusLine(
	const char* label,
	const char* value,
	ImU32 valueColor = palette::text
) {
	ImGui::TextDisabled("%s", label);
	ImGui::SameLine();

	const ImVec2 valueSize =
		ImGui::CalcTextSize(value);

	ImGui::SetCursorPosX(
		ImGui::GetWindowContentRegionMax().x -
		valueSize.x
	);

	ImGui::PushStyleColor(
		ImGuiCol_Text,
		ImGui::ColorConvertU32ToFloat4(valueColor)
	);
	ImGui::TextUnformatted(value);
	ImGui::PopStyleColor();
}

void KeyName(
	int key,
	char* out,
	size_t size
) {
	const char* name = "None";

	if (key != ImGuiKey_None) {
		name =
			ImGui::GetKeyName(
				static_cast<ImGuiKey>(key)
			);
	}

	size_t index = 0;
	for (
		;
		name[index] && index + 1 < size;
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

	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted(label);

	char keyText[48]{};
	KeyName(*key, keyText, sizeof(keyText));

	ImGui::SameLine();
	const float buttonWidth = 112.0f;
	ImGui::SetCursorPosX(
		ImGui::GetWindowContentRegionMax().x -
		buttonWidth
	);

	ImGui::PushID(label);
	const ImGuiID id =
		ImGui::GetID("##keybind");

	const bool waiting =
		waitingId == id;

	if (ImGui::Button(
		waiting ? "PRESS A KEY..." : keyText,
		ImVec2(buttonWidth, 0.0f)
	)) {
		waitingId = id;
		waitingFrame = ImGui::GetFrameCount();
	}

	bool changed = false;

	if (
		waiting &&
		ImGui::GetFrameCount() > waitingFrame
	) {
		if (ImGui::IsKeyPressed(
			ImGuiKey_Escape,
			false
		)) {
			waitingId = 0;
		} else {
			for (
				int candidate = ImGuiKey_NamedKey_BEGIN;
				candidate < ImGuiKey_NamedKey_END;
				++candidate
			) {
				if (
					candidate == ImGuiKey_MouseLeft ||
					candidate == ImGuiKey_MouseWheelX ||
					candidate == ImGuiKey_MouseWheelY
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
	ImGui::Dummy(ImVec2(0.0f, 4.0f));
	return changed;
}

bool TopTab(
	const char* label,
	int index
) {
	const bool selected =
		active_tab == index;

	if (selected) {
		ImGui::PushStyleColor(
			ImGuiCol_Button,
			ImGui::ColorConvertU32ToFloat4(
				Accent(0.88f)
			)
		);
		ImGui::PushStyleColor(
			ImGuiCol_ButtonHovered,
			ImGui::ColorConvertU32ToFloat4(
				Accent()
			)
		);
	} else {
		ImGui::PushStyleColor(
			ImGuiCol_Button,
			ImVec4(0, 0, 0, 0)
		);
		ImGui::PushStyleColor(
			ImGuiCol_ButtonHovered,
			ImGui::ColorConvertU32ToFloat4(
				palette::panel_alt
			)
		);
	}

	ImGui::PushStyleColor(
		ImGuiCol_ButtonActive,
		ImGui::ColorConvertU32ToFloat4(
			Accent(0.72f)
		)
	);

	const bool pressed =
		ImGui::Button(
			label,
			ImVec2(112.0f, 34.0f)
		);

	ImGui::PopStyleColor(3);

	if (pressed) {
		active_tab = index;
	}

	return pressed;
}

void RenderDisplayPage(
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

	BeginCard(
		"##display_primary",
		ImVec2(column, available.y)
	);
	CardHeader(
		"Display",
		"Customize the existing overlay presentation."
	);

	SettingToggle(
		"Bounding Box",
		&cfg.box,
		cfg.box_color
	);
	ComboRow(
		"Box Style",
		&cfg.box_style,
		boxStyles,
		IM_ARRAYSIZE(boxStyles)
	);
	SettingToggle(
		"Box Fill",
		&cfg.box_fill
	);
	SettingToggle(
		"Skeleton",
		&cfg.skeleton,
		cfg.skeleton_color
	);
	SettingToggle(
		"Distance",
		&cfg.distance
	);
	EndCard();

	ImGui::SameLine(0.0f, gap);

	BeginCard(
		"##display_secondary",
		ImVec2(
			available.x - column - gap,
			available.y
		)
	);
	CardHeader(
		"Lines",
		"Configure the existing line presentation."
	);

	SettingToggle(
		"Snaplines",
		&cfg.snaplines,
		cfg.snapline_color
	);
	ComboRow(
		"Origin",
		&cfg.snapline_origin,
		origins,
		IM_ARRAYSIZE(origins)
	);

	ImGui::Dummy(ImVec2(0.0f, 10.0f));
	ImGui::Separator();
	ImGui::Dummy(ImVec2(0.0f, 8.0f));

	ImGui::TextDisabled(
		"Changes are applied immediately."
	);
	EndCard();
}

void RenderSettingsPage(
	const ImVec2& available,
	const menu::RuntimeStatus& status
) {
	const float gap = 14.0f;
	const float column =
		(available.x - gap) * 0.5f;

	BeginCard(
		"##settings_primary",
		ImVec2(column, available.y)
	);
	CardHeader(
		"Interface",
		"Local Nexus interface preferences."
	);

	KeybindRow(
		"Menu Key",
		&cfg.menu_key
	);

	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted("Accent");
	ImGui::SameLine();
	ImGui::SetCursorPosX(
		ImGui::GetWindowContentRegionMax().x -
		160.0f
	);
	ImGui::SetNextItemWidth(160.0f);
	ImGui::ColorEdit4(
		"##accent",
		cfg.accent,
		ImGuiColorEditFlags_NoInputs |
		ImGuiColorEditFlags_AlphaPreviewHalf
	);

	ImGui::Dummy(ImVec2(0.0f, 16.0f));
	ImGui::Separator();
	ImGui::Dummy(ImVec2(0.0f, 12.0f));

	if (ImGui::Button(
		"SAVE",
		ImVec2(-1.0f, 36.0f)
	)) {
		(void)app_settings::Save(cfg);
	}

	if (ImGui::Button(
		"LOAD",
		ImVec2(-1.0f, 36.0f)
	)) {
		(void)app_settings::Load(cfg);
	}

	ImGui::PushStyleColor(
		ImGuiCol_Button,
		ImVec4(0.15f, 0.16f, 0.22f, 1.0f)
	);
	if (ImGui::Button(
		"RESET TO DEFAULTS",
		ImVec2(-1.0f, 36.0f)
	)) {
		cfg = cfg_default;
	}
	ImGui::PopStyleColor();

	EndCard();

	ImGui::SameLine(0.0f, gap);

	BeginCard(
		"##settings_status",
		ImVec2(
			available.x - column - gap,
			available.y
		)
	);
	CardHeader(
		"Runtime",
		"Live renderer and data diagnostics."
	);

	StatusLine(
		"Build",
		"1.0.5"
	);
	StatusLine(
		"Game",
		"Fortnite"
	);
	StatusLine(
		"World",
		status.world_valid ? "Ready" : "Unavailable",
		status.world_valid
			? palette::success
			: palette::warning
	);
	StatusLine(
		"Camera",
		status.camera_valid ? "Ready" : "Unavailable",
		status.camera_valid
			? palette::success
			: palette::warning
	);

	char buffer[64]{};

	std::snprintf(
		buffer,
		sizeof(buffer),
		"%d",
		status.actor_count
	);
	StatusLine("Actors", buffer);

	std::snprintf(
		buffer,
		sizeof(buffer),
		"%d",
		status.player_count
	);
	StatusLine("Players", buffer);

	std::snprintf(
		buffer,
		sizeof(buffer),
		"%.0f FPS",
		status.fps
	);
	StatusLine(
		"Renderer",
		buffer,
		status.fps >= 55.0f
			? palette::success
			: palette::warning
	);

	std::snprintf(
		buffer,
		sizeof(buffer),
		"%.2f ms",
		status.frame_ms
	);
	StatusLine("Frame Time", buffer);

	std::snprintf(
		buffer,
		sizeof(buffer),
		"%.3f ms",
		status.engine_ms
	);
	StatusLine("Engine", buffer);

	std::snprintf(
		buffer,
		sizeof(buffer),
		"%.3f ms",
		status.actors_ms
	);
	StatusLine("Actor Scan", buffer);

	std::snprintf(
		buffer,
		sizeof(buffer),
		"%.3f ms",
		status.players_ms
	);
	StatusLine("Player Cache", buffer);

	std::snprintf(
		buffer,
		sizeof(buffer),
		"%.2fx",
		status.dpi_scale
	);
	StatusLine("DPI Scale", buffer);

	EndCard();
}

void ApplyStyle() {
	ImGuiStyle& style = ImGui::GetStyle();

	style.WindowRounding = 12.0f;
	style.ChildRounding = 10.0f;
	style.FrameRounding = 8.0f;
	style.PopupRounding = 10.0f;
	style.GrabRounding = 8.0f;
	style.ScrollbarRounding = 8.0f;

	style.WindowPadding = ImVec2(18.0f, 16.0f);
	style.FramePadding = ImVec2(11.0f, 8.0f);
	style.ItemSpacing = ImVec2(10.0f, 10.0f);
	style.ItemInnerSpacing = ImVec2(8.0f, 6.0f);

	style.WindowBorderSize = 1.0f;
	style.ChildBorderSize = 1.0f;
	style.PopupBorderSize = 1.0f;
	style.FrameBorderSize = 1.0f;

	ImVec4* colors = style.Colors;

	colors[ImGuiCol_Text] =
		ImGui::ColorConvertU32ToFloat4(
			palette::text
		);
	colors[ImGuiCol_TextDisabled] =
		ImGui::ColorConvertU32ToFloat4(
			palette::muted
		);
	colors[ImGuiCol_WindowBg] =
		ImGui::ColorConvertU32ToFloat4(
			palette::background
		);
	colors[ImGuiCol_ChildBg] =
		ImGui::ColorConvertU32ToFloat4(
			palette::panel
		);
	colors[ImGuiCol_PopupBg] =
		ImGui::ColorConvertU32ToFloat4(
			palette::panel_alt
		);
	colors[ImGuiCol_Border] =
		ImGui::ColorConvertU32ToFloat4(
			palette::border
		);
	colors[ImGuiCol_FrameBg] =
		ImVec4(0.075f, 0.088f, 0.145f, 1.0f);
	colors[ImGuiCol_FrameBgHovered] =
		ImVec4(0.11f, 0.12f, 0.20f, 1.0f);
	colors[ImGuiCol_FrameBgActive] =
		ImVec4(0.13f, 0.14f, 0.24f, 1.0f);
	colors[ImGuiCol_Button] =
		ImVec4(0.42f, 0.12f, 0.86f, 1.0f);
	colors[ImGuiCol_ButtonHovered] =
		ImVec4(0.56f, 0.22f, 1.0f, 1.0f);
	colors[ImGuiCol_ButtonActive] =
		ImVec4(0.34f, 0.09f, 0.72f, 1.0f);
	colors[ImGuiCol_CheckMark] =
		ImVec4(0.72f, 0.36f, 1.0f, 1.0f);
	colors[ImGuiCol_SliderGrab] =
		ImVec4(0.58f, 0.27f, 1.0f, 1.0f);
	colors[ImGuiCol_SliderGrabActive] =
		ImVec4(0.72f, 0.40f, 1.0f, 1.0f);
	colors[ImGuiCol_Header] =
		ImVec4(0.35f, 0.16f, 0.62f, 0.75f);
	colors[ImGuiCol_HeaderHovered] =
		ImVec4(0.48f, 0.20f, 0.82f, 0.88f);
	colors[ImGuiCol_HeaderActive] =
		ImVec4(0.42f, 0.16f, 0.74f, 1.0f);
}

} // namespace

void menu::setup() {
	ImGuiIO& io = ImGui::GetIO();

	font_regular = io.Fonts->AddFontFromFileTTF(
		"C:\\Windows\\Fonts\\segoeui.ttf",
		14.5f,
		nullptr,
		io.Fonts->GetGlyphRangesDefault()
	);
	if (!font_regular) {
		font_regular =
			io.Fonts->AddFontDefault();
	}

	font_title = io.Fonts->AddFontFromFileTTF(
		"C:\\Windows\\Fonts\\seguisb.ttf",
		18.0f,
		nullptr,
		io.Fonts->GetGlyphRangesDefault()
	);
	if (!font_title) {
		font_title = font_regular;
	}

	io.FontDefault = font_regular;
	ApplyStyle();
}

void menu::render(
	const RuntimeStatus& status
) {
	const ImVec2 size(680.0f, 500.0f);

	ImGui::SetNextWindowPos(
		ImVec2(60.0f, 60.0f),
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

	draw->AddRect(
		windowPos,
		windowPos + windowSize,
		palette::border,
		12.0f,
		0,
		1.0f
	);

	draw->AddRectFilledMultiColor(
		ImVec2(
			windowPos.x + 1.0f,
			windowPos.y + 1.0f
		),
		ImVec2(
			windowPos.x + windowSize.x - 1.0f,
			windowPos.y + 5.0f
		),
		Accent(),
		Accent(0.85f),
		Accent(0.35f),
		Accent(0.65f)
	);

	ImGui::SetCursorPos(
		ImVec2(18.0f, 18.0f)
	);
	DrawBrand();

	ImGui::SameLine();
	ImGui::SetCursorPosY(23.0f);
	ImGui::TextDisabled(
		"Premium Interface"
	);

	ImGui::SameLine();
	const char* build = "BUILD 1.0.5";
	const ImVec2 buildSize =
		ImGui::CalcTextSize(build);
	ImGui::SetCursorPosX(
		windowSize.x -
		buildSize.x -
		20.0f
	);
	ImGui::TextDisabled("%s", build);

	ImGui::SetCursorPos(
		ImVec2(18.0f, 62.0f)
	);

	TopTab("Display", 0);
	ImGui::SameLine(0.0f, 8.0f);
	TopTab("Settings", 1);

	ImGui::SetCursorPos(
		ImVec2(18.0f, 110.0f)
	);

	const ImVec2 available(
		windowSize.x - 36.0f,
		windowSize.y - 128.0f
	);

	if (active_tab == 0) {
		RenderDisplayPage(available);
	} else {
		RenderSettingsPage(
			available,
			status
		);
	}

	ImGui::End();
}
