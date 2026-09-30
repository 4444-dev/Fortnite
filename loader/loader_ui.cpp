#include "loader_ui.hpp"

#include "launch_target.hpp"

#include <Windows.h>

#include <thirdparty/imgui/imgui.h>

#include <algorithm>
#include <cfloat>
#include <cstring>
#include <string>

namespace loader {
namespace {

constexpr float kSidebarWidth = 214.0f;
constexpr float kTopBarHeight = 66.0f;
constexpr float kContentPadding = 24.0f;

constexpr ImU32 kBg = IM_COL32(8, 10, 18, 255);
constexpr ImU32 kPanel = IM_COL32(16, 20, 33, 248);
constexpr ImU32 kPanelAlt = IM_COL32(21, 26, 43, 248);
constexpr ImU32 kBorder = IM_COL32(39, 45, 69, 255);
constexpr ImU32 kPurple = IM_COL32(139, 61, 255, 255);
constexpr ImU32 kPurpleBright = IM_COL32(182, 92, 255, 255);
constexpr ImU32 kText = IM_COL32(242, 243, 249, 255);
constexpr ImU32 kMuted = IM_COL32(168, 175, 195, 255);
constexpr ImU32 kGreen = IM_COL32(57, 232, 121, 255);
constexpr ImU32 kRed = IM_COL32(255, 92, 112, 255);
constexpr ImU32 kAmber = IM_COL32(247, 185, 85, 255);
constexpr ImU32 kBlue = IM_COL32(108, 166, 255, 255);
constexpr ImU32 kPanelSoft = IM_COL32(18, 22, 37, 250);
constexpr ImU32 kPanelRaised = IM_COL32(23, 28, 47, 252);

void DrawLogo(ImDrawList* draw, const ImVec2& pos) {
	draw->AddText(
		ImGui::GetFont(),
		32.0f,
		ImVec2(pos.x + 1.0f, pos.y + 2.0f),
		IM_COL32(36, 20, 65, 210),
		"NEXUS"
	);

	draw->AddText(
		ImGui::GetFont(),
		32.0f,
		pos,
		IM_COL32(247, 248, 255, 255),
		"NE"
	);

	const float prefixWidth =
		ImGui::GetFont()->CalcTextSizeA(
			32.0f,
			1000.0f,
			0.0f,
			"NE"
		).x;

	draw->AddText(
		ImGui::GetFont(),
		32.0f,
		ImVec2(pos.x + prefixWidth, pos.y),
		kPurpleBright,
		"XUS"
	);

	draw->AddText(
		ImGui::GetFont(),
		11.0f,
		ImVec2(pos.x + 1.0f, pos.y + 38.0f),
		IM_COL32(101, 108, 137, 255),
		"DESKTOP CLIENT"
	);
}

void DrawStatusPill(
	const char* text,
	ImU32 dotColor,
	float width
) {
	const ImVec2 start = ImGui::GetCursorScreenPos();
	const float height = 34.0f;

	ImGui::PushID(text);
	ImGui::InvisibleButton(
		"##status_pill",
		ImVec2(width, height)
	);
	const bool hovered = ImGui::IsItemHovered();
	ImGui::PopID();

	ImDrawList* draw = ImGui::GetWindowDrawList();
	const ImVec2 end(start.x + width, start.y + height);

	draw->AddRectFilled(
		ImVec2(start.x - 2.0f, start.y - 2.0f),
		ImVec2(end.x + 2.0f, end.y + 2.0f),
		hovered
			? IM_COL32(112, 70, 170, 20)
			: IM_COL32(0, 0, 0, 18),
		12.0f
	);

	draw->AddRectFilled(
		start,
		end,
		hovered
			? IM_COL32(23, 27, 44, 248)
			: IM_COL32(17, 20, 33, 242),
		11.0f
	);
	draw->AddRect(
		start,
		end,
		hovered
			? IM_COL32(74, 67, 103, 240)
			: IM_COL32(39, 44, 67, 220),
		11.0f
	);

	draw->AddCircleFilled(
		ImVec2(start.x + 17.0f, start.y + 17.0f),
		8.0f,
		ImGui::ColorConvertFloat4ToU32(
			ImVec4(
				((dotColor >> IM_COL32_R_SHIFT) & 0xFF) / 255.0f,
				((dotColor >> IM_COL32_G_SHIFT) & 0xFF) / 255.0f,
				((dotColor >> IM_COL32_B_SHIFT) & 0xFF) / 255.0f,
				0.10f
			)
		)
	);
	draw->AddCircleFilled(
		ImVec2(start.x + 17.0f, start.y + 17.0f),
		4.5f,
		dotColor
	);
	draw->AddText(
		ImVec2(start.x + 31.0f, start.y + 8.0f),
		kText,
		text
	);
}

void DrawNavGlyph(
	ImDrawList* draw,
	const ImVec2& center,
	int index,
	ImU32 color
) {
	switch (index) {
	case 0:
		draw->AddTriangle(
			ImVec2(center.x - 8.0f, center.y),
			ImVec2(center.x, center.y - 7.0f),
			ImVec2(center.x + 8.0f, center.y),
			color,
			2.0f
		);
		draw->AddRect(
			ImVec2(center.x - 6.0f, center.y),
			ImVec2(center.x + 6.0f, center.y + 7.0f),
			color,
			1.0f,
			0,
			2.0f
		);
		break;
	case 1:
		draw->AddRect(
			ImVec2(center.x - 7.0f, center.y - 7.0f),
			ImVec2(center.x + 7.0f, center.y + 7.0f),
			color,
			3.0f,
			0,
			2.0f
		);
		draw->AddLine(
			ImVec2(center.x - 7.0f, center.y - 7.0f),
			ImVec2(center.x + 7.0f, center.y + 7.0f),
			color,
			1.5f
		);
		break;
	case 2:
		draw->AddCircle(center, 7.0f, color, 16, 2.0f);
		draw->AddCircle(center, 2.5f, color, 12, 2.0f);
		break;
	default:
		draw->AddCircle(center, 8.0f, color, 20, 2.0f);
		draw->AddText(
			ImVec2(center.x - 2.5f, center.y - 7.0f),
			color,
			"i"
		);
		break;
	}
}

bool NavItem(
	const char* label,
	int icon,
	bool selected
) {
	const ImVec2 start = ImGui::GetCursorScreenPos();
	const ImVec2 size(kSidebarWidth - 28.0f, 48.0f);

	ImGui::PushID(label);
	const bool pressed = ImGui::InvisibleButton("##nav", size);
	const bool hovered = ImGui::IsItemHovered();
	ImGui::PopID();

	ImDrawList* draw = ImGui::GetWindowDrawList();
	const ImVec2 end(start.x + size.x, start.y + size.y);

	if (selected || hovered) {
		draw->AddRectFilledMultiColor(
			start,
			end,
			selected
				? IM_COL32(117, 42, 235, 245)
				: IM_COL32(40, 31, 65, 180),
			selected
				? IM_COL32(142, 46, 255, 245)
				: IM_COL32(35, 28, 55, 180),
			selected
				? IM_COL32(105, 28, 220, 245)
				: IM_COL32(26, 24, 44, 160),
			selected
				? IM_COL32(91, 30, 205, 245)
				: IM_COL32(28, 24, 47, 160)
		);
		draw->AddRect(
			start,
			end,
			selected
				? IM_COL32(173, 83, 255, 255)
				: IM_COL32(52, 48, 78, 180),
			9.0f
		);
	}

	const ImU32 itemColor =
		selected ? IM_COL32(255, 255, 255, 255) : kMuted;

	DrawNavGlyph(
		draw,
		ImVec2(start.x + 22.0f, start.y + 24.0f),
		icon,
		itemColor
	);
	draw->AddText(
		ImVec2(start.x + 46.0f, start.y + 16.0f),
		itemColor,
		label
	);

	return pressed;
}

void DrawBadge(
	ImDrawList* draw,
	const ImVec2& pos,
	const char* text,
	ImU32 color
) {
	const float width =
		ImGui::CalcTextSize(text).x + 18.0f;
	const ImVec2 end(
		pos.x + width,
		pos.y + 24.0f
	);

	draw->AddRectFilled(
		pos,
		end,
		IM_COL32(13, 16, 27, 220),
		7.0f
	);
	draw->AddRect(
		pos,
		end,
		color,
		7.0f,
		0,
		1.0f
	);
	draw->AddText(
		ImVec2(pos.x + 9.0f, pos.y + 5.0f),
		color,
		text
	);
}

bool PrimaryActionButton(
	const char* label,
	const ImVec2& size,
	bool enabled
) {
	ImGui::BeginDisabled(!enabled);
	ImGui::PushID(label);

	const ImVec2 start =
		ImGui::GetCursorScreenPos();
	const bool pressed =
		ImGui::InvisibleButton(
			"##primary_action",
			size
		);
	const bool hovered =
		ImGui::IsItemHovered();
	const bool held =
		ImGui::IsItemActive();

	ImDrawList* draw =
		ImGui::GetWindowDrawList();
	const ImVec2 end(
		start.x + size.x,
		start.y + size.y
	);

	const ImU32 left =
		!enabled
			? IM_COL32(63, 58, 79, 210)
			: held
				? IM_COL32(92, 34, 184, 255)
				: hovered
					? IM_COL32(129, 48, 242, 255)
					: IM_COL32(109, 32, 232, 255);

	const ImU32 right =
		!enabled
			? IM_COL32(71, 63, 88, 210)
			: held
				? IM_COL32(137, 48, 210, 255)
				: hovered
					? IM_COL32(183, 74, 255, 255)
					: IM_COL32(164, 49, 255, 255);

	draw->AddRectFilled(
		ImVec2(start.x - 3.0f, start.y - 3.0f),
		ImVec2(end.x + 3.0f, end.y + 4.0f),
		enabled && hovered
			? IM_COL32(152, 72, 255, 35)
			: IM_COL32(0, 0, 0, 20),
		12.0f
	);
	draw->AddRectFilledMultiColor(
		start,
		end,
		left,
		right,
		right,
		left
	);
	draw->AddRect(
		start,
		end,
		enabled
			? IM_COL32(194, 122, 255, 210)
			: IM_COL32(94, 87, 110, 180),
		10.0f
	);

	const ImVec2 textSize =
		ImGui::CalcTextSize(label);
	draw->AddText(
		ImVec2(
			start.x + (size.x - textSize.x) * 0.5f,
			start.y + (size.y - textSize.y) * 0.5f
		),
		enabled
			? IM_COL32(255, 255, 255, 255)
			: IM_COL32(161, 157, 176, 255),
		label
	);

	ImGui::PopID();
	ImGui::EndDisabled();
	return pressed && enabled;
}

void DrawInfoCard(
	const char* id,
	const char* label,
	const char* value,
	const ImVec2& size,
	ImU32 valueColor = kText
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
		kPanelRaised,
		9.0f
	);
	draw->AddRect(
		start,
		end,
		IM_COL32(44, 51, 78, 230),
		9.0f
	);

	draw->AddText(
		ImGui::GetFont(),
		12.0f,
		ImVec2(start.x + 14.0f, start.y + 10.0f),
		kMuted,
		label
	);
	draw->AddText(
		ImGui::GetFont(),
		16.0f,
		ImVec2(start.x + 14.0f, start.y + 31.0f),
		valueColor,
		value
	);

	ImGui::PopID();
}

bool ProductTile(
	const ProductDefinition& product,
	bool selected,
	const ImVec2& size
) {
	ImGui::PushID(product.Slug.data());

	const ImVec2 start =
		ImGui::GetCursorScreenPos();

	if (product.Configured) {
		ImGui::InvisibleButton(
			"##product",
			size
		);
	} else {
		ImGui::BeginDisabled();
		ImGui::InvisibleButton(
			"##product",
			size
		);
		ImGui::EndDisabled();
	}

	const bool hovered =
		ImGui::IsItemHovered();
	const bool pressed =
		ImGui::IsItemClicked();

	ImDrawList* draw =
		ImGui::GetWindowDrawList();
	const ImVec2 end(
		start.x + size.x,
		start.y + size.y
	);

	const bool fortnite =
		product.Id == ProductId::Fortnite;

	draw->AddRectFilled(
		ImVec2(start.x - 3.0f, start.y - 3.0f),
		ImVec2(end.x + 3.0f, end.y + 3.0f),
		selected
			? IM_COL32(146, 77, 255, 34)
			: hovered && product.Configured
				? IM_COL32(112, 72, 171, 24)
				: IM_COL32(0, 0, 0, 14),
		15.0f
	);

	draw->AddRectFilled(
		start,
		end,
		fortnite
			? IM_COL32(22, 17, 41, 255)
			: IM_COL32(35, 23, 29, 255),
		13.0f
	);

	draw->AddRectFilledMultiColor(
		ImVec2(start.x + 1.0f, start.y + 1.0f),
		ImVec2(end.x - 1.0f, start.y + size.y * 0.58f),
		fortnite
			? IM_COL32(68, 33, 124, 235)
			: IM_COL32(96, 43, 45, 225),
		fortnite
			? IM_COL32(31, 24, 62, 220)
			: IM_COL32(53, 28, 32, 220),
		fortnite
			? IM_COL32(25, 19, 46, 160)
			: IM_COL32(39, 24, 27, 160),
		fortnite
			? IM_COL32(47, 24, 82, 190)
			: IM_COL32(70, 33, 35, 190)
	);

	draw->AddText(
		ImGui::GetFont(),
		54.0f,
		ImVec2(start.x + 18.0f, start.y + 26.0f),
		fortnite
			? IM_COL32(219, 192, 255, 24)
			: IM_COL32(255, 190, 180, 22),
		fortnite ? "FN" : "APEX"
	);

	if (selected) {
		draw->AddRect(
			start,
			end,
			kPurpleBright,
			13.0f,
			0,
			2.0f
		);
	} else {
		draw->AddRect(
			start,
			end,
			hovered && product.Configured
				? IM_COL32(100, 83, 139, 255)
				: IM_COL32(47, 53, 78, 255),
			13.0f
		);
	}

	DrawBadge(
		draw,
		ImVec2(start.x + 14.0f, start.y + 14.0f),
		product.Configured
			? "READY"
			: "SETUP REQUIRED",
		product.Configured
			? kGreen
			: kAmber
	);

	if (selected) {
		draw->AddCircleFilled(
			ImVec2(end.x - 20.0f, start.y + 20.0f),
			9.0f,
			kPurpleBright
		);
		draw->AddCircleFilled(
			ImVec2(end.x - 20.0f, start.y + 20.0f),
			3.5f,
			IM_COL32(255, 255, 255, 255)
		);
	}

	draw->AddText(
		ImGui::GetFont(),
		18.0f,
		ImVec2(start.x + 16.0f, end.y - 62.0f),
		product.Configured
			? kText
			: IM_COL32(172, 173, 186, 255),
		product.DisplayName.data()
	);

	const std::string executable =
		product.Configured
			? std::string("Nexus-") +
				(product.Id == ProductId::Fortnite
					? "Fortnite.exe"
					: "Apex-Radar.exe")
			: !product.PackagedTargetRelativePath.empty()
				? "Radar Included - KeyAuth Required"
				: "Configuration Required";

	draw->AddText(
		ImGui::GetFont(),
		12.5f,
		ImVec2(start.x + 16.0f, end.y - 32.0f),
		product.Configured
			? kMuted
			: kAmber,
		executable.c_str()
	);

	ImGui::PopID();
	return pressed && product.Configured;
}

void DrawPanelHeader(
	const char* symbol,
	const char* title
) {
	ImGui::PushStyleColor(
		ImGuiCol_Text,
		ImVec4(0.94f, 0.95f, 0.99f, 1.0f)
	);
	ImGui::Text("%s", title);
	ImGui::PopStyleColor();

	ImGui::SameLine();
	ImGui::TextColored(
		ImVec4(0.55f, 0.36f, 0.92f, 0.9f),
		"%s",
		symbol
	);
}

ImVec4 StatusColor(const AuthSnapshot& snapshot) {
	if (snapshot.Authenticated) {
		return ImVec4(0.22f, 0.91f, 0.47f, 1.0f);
	}

	if (
		snapshot.State == AuthState::Error ||
		snapshot.State == AuthState::SessionInvalid
	) {
		return ImVec4(0.96f, 0.34f, 0.40f, 1.0f);
	}

	return ImVec4(0.64f, 0.67f, 0.78f, 1.0f);
}

} // namespace

UiController::~UiController() {
	SecureZeroMemory(
		m_License.data(),
		m_License.size()
	);
}

void ApplyLoaderStyle() {
	ImGuiStyle& style = ImGui::GetStyle();

	style.WindowRounding = 16.0f;
	style.ChildRounding = 12.0f;
	style.FrameRounding = 10.0f;
	style.PopupRounding = 12.0f;
	style.GrabRounding = 10.0f;
	style.WindowPadding = ImVec2(0.0f, 0.0f);
	style.FramePadding = ImVec2(13.0f, 10.0f);
	style.ItemSpacing = ImVec2(12.0f, 12.0f);
	style.ItemInnerSpacing = ImVec2(8.0f, 7.0f);
	style.WindowBorderSize = 1.0f;
	style.ChildBorderSize = 1.0f;
	style.FrameBorderSize = 1.0f;
	style.ScrollbarSize = 10.0f;

	ImVec4* colors = style.Colors;
	colors[ImGuiCol_WindowBg] =
		ImVec4(0.028f, 0.032f, 0.058f, 1.0f);
	colors[ImGuiCol_ChildBg] =
		ImVec4(0.047f, 0.054f, 0.095f, 0.96f);
	colors[ImGuiCol_Border] =
		ImVec4(0.16f, 0.18f, 0.28f, 1.0f);
	colors[ImGuiCol_FrameBg] =
		ImVec4(0.075f, 0.084f, 0.14f, 1.0f);
	colors[ImGuiCol_FrameBgHovered] =
		ImVec4(0.10f, 0.11f, 0.18f, 1.0f);
	colors[ImGuiCol_FrameBgActive] =
		ImVec4(0.12f, 0.13f, 0.21f, 1.0f);
	colors[ImGuiCol_Button] =
		ImVec4(0.46f, 0.13f, 0.89f, 1.0f);
	colors[ImGuiCol_ButtonHovered] =
		ImVec4(0.58f, 0.20f, 1.0f, 1.0f);
	colors[ImGuiCol_ButtonActive] =
		ImVec4(0.38f, 0.10f, 0.78f, 1.0f);
	colors[ImGuiCol_CheckMark] =
		ImVec4(0.72f, 0.30f, 1.0f, 1.0f);
	colors[ImGuiCol_Text] =
		ImVec4(0.94f, 0.95f, 0.98f, 1.0f);
	colors[ImGuiCol_TextDisabled] =
		ImVec4(0.55f, 0.58f, 0.70f, 1.0f);
}

void UiController::EnsureSelection() {
	if (m_SelectedProduct && m_Auth) {
		return;
	}

	for (const auto& product : Products) {
		if (product.Configured) {
			SelectProduct(product);
			return;
		}
	}
}

void UiController::SelectProduct(
	const ProductDefinition& product
) {
	if (!product.Configured) {
		return;
	}

	if (
		m_SelectedProduct == &product &&
		m_Auth
	) {
		return;
	}

	m_Auth.reset();
	m_SelectedProduct = &product;
	m_LaunchStatus.clear();
	m_Remember = true;
	LoadRememberedLicense(product);

	m_Auth =
		std::make_unique<AuthController>(product);
	m_Auth->Initialize();
}

void UiController::Tick() {
	EnsureSelection();

	if (!m_Auth) {
		return;
	}

	m_Auth->Tick();

	if (
		m_License[0] != '\0' &&
		m_Auth->Snapshot().Authenticated
	) {
		SecureZeroMemory(
			m_License.data(),
			m_License.size()
		);
	}
}

void UiController::LoadRememberedLicense(
	const ProductDefinition& product
) {
	m_License.fill('\0');

	std::string savedLicense;
	if (!license_store::Load(
		product.Slug,
		savedLicense
	)) {
		return;
	}

	const std::size_t count = (std::min)(
		savedLicense.size(),
		m_License.size() - 1
	);

	std::memcpy(
		m_License.data(),
		savedLicense.data(),
		count
	);
	m_License[count] = '\0';
}

void UiController::DrawSidebar() {
	ImDrawList* draw =
		ImGui::GetWindowDrawList();
	const ImVec2 origin =
		ImGui::GetWindowPos();
	const ImVec2 size =
		ImGui::GetWindowSize();

	draw->AddRectFilledMultiColor(
		origin,
		ImVec2(
			origin.x + kSidebarWidth,
			origin.y + size.y
		),
		IM_COL32(10, 12, 24, 255),
		IM_COL32(10, 12, 24, 255),
		IM_COL32(26, 15, 59, 255),
		IM_COL32(15, 12, 37, 255)
	);

	draw->AddLine(
		ImVec2(
			origin.x + kSidebarWidth,
			origin.y
		),
		ImVec2(
			origin.x + kSidebarWidth,
			origin.y + size.y
		),
		IM_COL32(42, 45, 71, 230)
	);

	DrawLogo(
		draw,
		ImVec2(origin.x + 24.0f, origin.y + 22.0f)
	);

	ImGui::SetCursorPos(
		ImVec2(14.0f, 105.0f)
	);

	if (NavItem(
		"Home",
		0,
		m_Page == Page::Home
	)) {
		m_Page = Page::Home;
	}

	ImGui::SetCursorPosX(14.0f);
	if (NavItem(
		"Products",
		1,
		m_Page == Page::Products
	)) {
		m_Page = Page::Products;
	}

	ImGui::SetCursorPosX(14.0f);
	if (NavItem(
		"Settings",
		2,
		m_Page == Page::Settings
	)) {
		m_Page = Page::Settings;
	}

	ImGui::SetCursorPosX(14.0f);
	if (NavItem(
		"About",
		3,
		m_Page == Page::About
	)) {
		m_Page = Page::About;
	}

	draw->AddText(
		ImVec2(
			origin.x + 22.0f,
			origin.y + size.y - 54.0f
		),
		IM_COL32(106, 111, 141, 255),
		"NEXUS CLIENT"
	);
	draw->AddText(
		ImVec2(
			origin.x + 22.0f,
			origin.y + size.y - 34.0f
		),
		IM_COL32(84, 90, 114, 255),
		"BUILD 1.0.5"
	);
}

void UiController::DrawHeader(
	bool& requestClose,
	bool& requestMinimize
) {
	const ImVec2 origin =
		ImGui::GetWindowPos();
	const ImVec2 windowSize =
		ImGui::GetWindowSize();

	ImDrawList* draw =
		ImGui::GetWindowDrawList();

	draw->AddRectFilled(
		ImVec2(
			origin.x + kSidebarWidth,
			origin.y
		),
		ImVec2(
			origin.x + windowSize.x,
			origin.y + kTopBarHeight
		),
		IM_COL32(11, 13, 25, 248)
	);

	draw->AddLine(
		ImVec2(
			origin.x + kSidebarWidth,
			origin.y + kTopBarHeight
		),
		ImVec2(
			origin.x + windowSize.x,
			origin.y + kTopBarHeight
		),
		IM_COL32(36, 40, 62, 220)
	);

	AuthSnapshot snapshot{};
	if (m_Auth) {
		snapshot = m_Auth->Snapshot();
	}

	const bool serviceOnline =
		m_SelectedProduct &&
		m_SelectedProduct->Configured;

	const char* keyAuthText =
		snapshot.Authenticated
			? "KeyAuth Connected"
			: snapshot.State == AuthState::Error ||
			  snapshot.State == AuthState::SessionInvalid
				? "KeyAuth Error"
				: snapshot.State == AuthState::Connecting
					? "Connecting..."
					: "KeyAuth Ready";

	ImGui::SetCursorPos(
		ImVec2(
			windowSize.x - 446.0f,
			16.0f
		)
	);
	DrawStatusPill(
		serviceOnline
			? "Service Online"
			: "Service Offline",
		serviceOnline ? kGreen : kRed,
		142.0f
	);

	ImGui::SameLine();
	DrawStatusPill(
		keyAuthText,
		snapshot.State == AuthState::Error ||
		snapshot.State == AuthState::SessionInvalid
			? kRed
			: kPurpleBright,
		178.0f
	);

	ImGui::SameLine();
	if (ImGui::Button(
		"_",
		ImVec2(36.0f, 34.0f)
	)) {
		requestMinimize = true;
	}

	ImGui::SameLine();
	if (ImGui::Button(
		"X",
		ImVec2(36.0f, 34.0f)
	)) {
		requestClose = true;
	}
}

void UiController::DrawHome(
	bool& requestClose
) {
	EnsureSelection();

	const ImVec2 windowSize =
		ImGui::GetWindowSize();
	const float contentX =
		kSidebarWidth + kContentPadding;
	const float contentWidth =
		windowSize.x -
		contentX -
		kContentPadding;

	ImDrawList* draw =
		ImGui::GetWindowDrawList();
	const ImVec2 origin =
		ImGui::GetWindowPos();

	draw->AddCircleFilled(
		ImVec2(
			origin.x + windowSize.x - 160.0f,
			origin.y + 150.0f
		),
		170.0f,
		IM_COL32(115, 36, 212, 18),
		64
	);

	draw->AddText(
		ImGui::GetFont(),
		86.0f,
		ImVec2(
			origin.x + windowSize.x - 400.0f,
			origin.y + 84.0f
		),
		IM_COL32(154, 74, 238, 18),
		"NEXUS"
	);

	for (int index = 0; index < 4; ++index) {
		const float offset =
			static_cast<float>(index) * 44.0f;
		draw->AddLine(
			ImVec2(
				origin.x + windowSize.x - 310.0f + offset,
				origin.y + 72.0f
			),
			ImVec2(
				origin.x + windowSize.x - 410.0f + offset,
				origin.y + 172.0f
			),
			IM_COL32(164, 76, 255, 22),
			8.0f
		);
	}

	const char* welcomePrefix = "Welcome to";
	const ImVec2 welcomePos(
		origin.x + contentX,
		origin.y + 96.0f
	);
	draw->AddText(
		ImGui::GetFont(),
		34.0f,
		welcomePos,
		kText,
		welcomePrefix
	);

	const float welcomeWidth =
		ImGui::GetFont()->CalcTextSizeA(
			34.0f,
			FLT_MAX,
			0.0f,
			welcomePrefix
		).x;

	draw->AddText(
		ImGui::GetFont(),
		34.0f,
		ImVec2(
			welcomePos.x + welcomeWidth + 10.0f,
			welcomePos.y
		),
		kPurpleBright,
		"Nexus"
	);

	draw->AddText(
		ImVec2(
			origin.x + contentX,
			origin.y + 138.0f
		),
		kMuted,
		"Authenticate, choose your product, and launch instantly."
	);

	const float top = 184.0f;
	const float gap = 18.0f;
	const float authWidth =
		contentWidth * 0.40f;
	const float productsWidth =
		contentWidth - authWidth - gap;
	const float cardHeight = 292.0f;

	ImGui::SetCursorPos(
		ImVec2(contentX, top)
	);
	ImGui::BeginChild(
		"##auth_panel",
		ImVec2(authWidth, cardHeight),
		true
	);

	ImGui::SetCursorPos(
		ImVec2(18.0f, 17.0f)
	);
	DrawPanelHeader("[+]", "Authentication");

	AuthSnapshot snapshot{};
	if (m_Auth) {
		snapshot = m_Auth->Snapshot();
	}

	ImGui::SetCursorPos(
		ImVec2(18.0f, 62.0f)
	);

	if (!snapshot.Authenticated) {
		ImGui::BeginDisabled(
			!m_Auth ||
			snapshot.Busy
		);

		ImGui::SetNextItemWidth(-18.0f);
		const bool submitted =
			ImGui::InputTextWithHint(
				"##license",
				"Enter your activation key...",
				m_License.data(),
				m_License.size(),
				ImGuiInputTextFlags_Password |
				ImGuiInputTextFlags_EnterReturnsTrue
			);

		ImGui::Spacing();

		const bool authenticate =
			ImGui::Button(
				"SIGN IN",
				ImVec2(-18.0f, 42.0f)
			);

		ImGui::Spacing();
		ImGui::Checkbox(
			"Remember my key",
			&m_Remember
		);

		ImGui::EndDisabled();

		if (
			m_Auth &&
			!snapshot.Busy &&
			(submitted || authenticate)
		) {
			m_Auth->Authenticate(
				std::string(m_License.data()),
				m_Remember
			);
		}
	} else {
		ImGui::TextColored(
			ImVec4(0.22f, 0.91f, 0.47f, 1.0f),
			"Authenticated"
		);
		ImGui::TextDisabled(
			"Your license is active."
		);

		ImGui::Spacing();
		if (!snapshot.Username.empty()) {
			ImGui::Text(
				"Account: %s",
				snapshot.Username.c_str()
			);
		}
		if (!snapshot.Subscription.empty()) {
			ImGui::Text(
				"Subscription: %s",
				snapshot.Subscription.c_str()
			);
		}
		if (!snapshot.Expiry.empty()) {
			ImGui::Text(
				"Expires: %s",
				snapshot.Expiry.c_str()
			);
		}
	}

	ImGui::SetCursorPos(
		ImVec2(18.0f, cardHeight - 74.0f)
	);
	ImGui::BeginChild(
		"##auth_status",
		ImVec2(-18.0f, 56.0f),
		true
	);
	ImGui::TextColored(
		StatusColor(snapshot),
		"%s",
		snapshot.Authenticated
			? "Authenticated"
			: snapshot.Status.c_str()
	);
	ImGui::EndChild();
	ImGui::EndChild();

	ImGui::SetCursorPos(
		ImVec2(
			contentX + authWidth + gap,
			top
		)
	);
	ImGui::BeginChild(
		"##products_panel",
		ImVec2(productsWidth, cardHeight),
		true
	);

	ImGui::SetCursorPos(
		ImVec2(18.0f, 17.0f)
	);
	DrawPanelHeader(
		"[ ]",
		"Products Available"
	);

	const float tileGap = 14.0f;
	const float tileWidth =
		(productsWidth - 36.0f - tileGap) /
		2.0f;

	ImGui::SetCursorPos(
		ImVec2(18.0f, 60.0f)
	);

	for (
		std::size_t index = 0;
		index < Products.size();
		++index
	) {
		const auto& product = Products[index];

		if (index > 0) {
			ImGui::SameLine(
				0.0f,
				tileGap
			);
		}

		if (ProductTile(
			product,
			m_SelectedProduct == &product,
			ImVec2(tileWidth, 205.0f)
		)) {
			SelectProduct(product);
		}
	}

	ImGui::EndChild();

	const float launchTop =
		top + cardHeight + 18.0f;
	const float launchHeight =
		windowSize.y -
		launchTop -
		20.0f;

	ImGui::SetCursorPos(
		ImVec2(contentX, launchTop)
	);
	ImGui::BeginChild(
		"##launch_panel",
		ImVec2(contentWidth, launchHeight),
		true
	);

	ImGui::SetCursorPos(
		ImVec2(20.0f, 18.0f)
	);
	DrawPanelHeader(
		">",
		"Launch"
	);

	const char* productName =
		m_SelectedProduct
			? m_SelectedProduct->DisplayName.data()
			: "product";

	ImGui::SetCursorPos(
		ImVec2(20.0f, 50.0f)
	);
	ImGui::TextDisabled(
		snapshot.Authenticated
			? "Ready to launch %s with your Nexus configuration."
			: "Authenticate first to enable launching %s.",
		productName
	);

	const float buttonWidth = 220.0f;
	ImGui::SetCursorPos(
		ImVec2(
			contentWidth - buttonWidth - 20.0f,
			18.0f
		)
	);

	ImGui::BeginDisabled(
		!snapshot.Authenticated ||
		snapshot.Busy ||
		!m_SelectedProduct
	);

	if (ImGui::Button(
		"LAUNCH",
		ImVec2(buttonWidth, 54.0f)
	)) {
		if (
			m_SelectedProduct &&
			LaunchConfiguredTarget(
				*m_SelectedProduct,
				m_LaunchStatus
			)
		) {
			requestClose = true;
		}
	}

	ImGui::EndDisabled();

	ImGui::SetCursorPos(
		ImVec2(20.0f, 92.0f)
	);

	const float infoWidth =
		(contentWidth - 56.0f) /
		3.0f;

	ImGui::BeginChild(
		"##game_info",
		ImVec2(infoWidth, 62.0f),
		true
	);
	ImGui::TextDisabled("Product");
	ImGui::Text(
		"%s",
		m_SelectedProduct
			? m_SelectedProduct->DisplayName.data()
			: "None"
	);
	ImGui::EndChild();

	ImGui::SameLine();
	ImGui::BeginChild(
		"##config_info",
		ImVec2(infoWidth, 62.0f),
		true
	);
	ImGui::TextDisabled("Configuration");
	ImGui::Text(
		"%s",
		m_SelectedProduct &&
		m_SelectedProduct->Configured
			? "Ready"
			: "Required"
	);
	ImGui::EndChild();

	ImGui::SameLine();
	ImGui::BeginChild(
		"##status_info",
		ImVec2(infoWidth, 62.0f),
		true
	);
	ImGui::TextDisabled("Status");
	ImGui::TextColored(
		snapshot.Authenticated
			? ImVec4(0.22f, 0.91f, 0.47f, 1.0f)
			: ImVec4(0.64f, 0.67f, 0.78f, 1.0f),
		"%s",
		snapshot.Authenticated
			? "Operational"
			: "Waiting"
	);
	ImGui::EndChild();

	if (!m_LaunchStatus.empty()) {
		ImGui::SetCursorPos(
			ImVec2(20.0f, launchHeight - 28.0f)
		);
		ImGui::TextDisabled(
			"%s",
			m_LaunchStatus.c_str()
		);
	}

	ImGui::EndChild();
}

void UiController::DrawProducts() {
	const ImVec2 windowSize =
		ImGui::GetWindowSize();
	const float contentX =
		kSidebarWidth + kContentPadding;
	const float width =
		windowSize.x -
		contentX -
		kContentPadding;

	ImGui::SetCursorPos(
		ImVec2(contentX, 96.0f)
	);
	ImGui::Text("Products");
	ImGui::TextDisabled(
		"Manage the products available through Nexus."
	);

	ImGui::SetCursorPos(
		ImVec2(contentX, 148.0f)
	);
	ImGui::BeginChild(
		"##products_page",
		ImVec2(width, 300.0f),
		true
	);

	const float gap = 18.0f;
	const float tileWidth =
		(width - 54.0f - gap) / 2.0f;

	ImGui::SetCursorPos(
		ImVec2(18.0f, 18.0f)
	);

	for (
		std::size_t index = 0;
		index < Products.size();
		++index
	) {
		const auto& product = Products[index];

		if (index > 0) {
			ImGui::SameLine(
				0.0f,
				gap
			);
		}

		if (ProductTile(
			product,
			m_SelectedProduct == &product,
			ImVec2(tileWidth, 245.0f)
		)) {
			SelectProduct(product);
			m_Page = Page::Home;
		}
	}

	ImGui::EndChild();
}

void UiController::DrawSettings() {
	const ImVec2 windowSize =
		ImGui::GetWindowSize();
	const float contentX =
		kSidebarWidth + kContentPadding;
	const float width =
		windowSize.x -
		contentX -
		kContentPadding;

	ImGui::SetCursorPos(
		ImVec2(contentX, 96.0f)
	);
	ImGui::Text("Settings");
	ImGui::TextDisabled(
		"Local preferences for the Nexus client."
	);

	ImGui::SetCursorPos(
		ImVec2(contentX, 148.0f)
	);
	ImGui::BeginChild(
		"##settings_page",
		ImVec2(width, 210.0f),
		true
	);

	ImGui::SetCursorPos(
		ImVec2(20.0f, 20.0f)
	);
	ImGui::Checkbox(
		"Remember my key on this device",
		&m_Remember
	);

	ImGui::Spacing();
	ImGui::TextDisabled(
		"Stored keys are protected locally with Windows DPAPI."
	);
	ImGui::TextDisabled(
		"Local data: %%LOCALAPPDATA%%\\Nexus"
	);

	if (m_SelectedProduct) {
		ImGui::Spacing();
		ImGui::Text(
			"Active product: %s",
			m_SelectedProduct->DisplayName.data()
		);
	}

	ImGui::EndChild();
}

void UiController::DrawAbout() {
	const ImVec2 windowSize =
		ImGui::GetWindowSize();
	const float contentX =
		kSidebarWidth + kContentPadding;
	const float width =
		windowSize.x -
		contentX -
		kContentPadding;

	ImGui::SetCursorPos(
		ImVec2(contentX, 96.0f)
	);
	ImGui::Text("About");
	ImGui::TextDisabled(
		"Nexus Client"
	);

	ImGui::SetCursorPos(
		ImVec2(contentX, 148.0f)
	);
	ImGui::BeginChild(
		"##about_page",
		ImVec2(width, 230.0f),
		true
	);

	ImGui::SetCursorPos(
		ImVec2(20.0f, 20.0f)
	);
	ImGui::TextColored(
		ImVec4(0.72f, 0.30f, 1.0f, 1.0f),
		"NEXUS"
	);
	ImGui::Spacing();
	ImGui::TextWrapped(
		"Multi-product desktop client with isolated product licensing, "
		"KeyAuth authentication, and installed or portable launch support."
	);
	ImGui::Spacing();
	ImGui::TextDisabled(
		"Products without authentication configuration remain intentionally disabled."
	);

	ImGui::EndChild();
}

void UiController::Draw(
	bool& requestClose,
	bool& requestMinimize
) {
	EnsureSelection();

	ImGui::SetNextWindowPos(
		ImVec2(0.0f, 0.0f)
	);
	ImGui::SetNextWindowSize(
		ImGui::GetIO().DisplaySize
	);

	ImGui::Begin(
		"##nexus_loader",
		nullptr,
		ImGuiWindowFlags_NoDecoration |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoBringToFrontOnFocus
	);

	ImDrawList* draw =
		ImGui::GetWindowDrawList();
	const ImVec2 origin =
		ImGui::GetWindowPos();
	const ImVec2 size =
		ImGui::GetWindowSize();

	draw->AddRectFilled(
		origin,
		ImVec2(
			origin.x + size.x,
			origin.y + size.y
		),
		kBg,
		14.0f
	);

	draw->AddRectFilledMultiColor(
		ImVec2(
			origin.x + kSidebarWidth,
			origin.y + kTopBarHeight
		),
		ImVec2(
			origin.x + size.x,
			origin.y + size.y
		),
		IM_COL32(8, 9, 17, 0),
		IM_COL32(16, 11, 31, 30),
		IM_COL32(40, 16, 78, 75),
		IM_COL32(17, 12, 39, 50)
	);

	DrawSidebar();
	DrawHeader(
		requestClose,
		requestMinimize
	);

	switch (m_Page) {
	case Page::Home:
		DrawHome(requestClose);
		break;
	case Page::Products:
		DrawProducts();
		break;
	case Page::Settings:
		DrawSettings();
		break;
	case Page::About:
		DrawAbout();
		break;
	}

	ImGui::End();
}

} // namespace loader
