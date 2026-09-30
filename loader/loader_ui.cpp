#include "loader_ui.hpp"

#include "launch_target.hpp"

#include <Windows.h>

#include <thirdparty/imgui/imgui.h>

#include <algorithm>
#include <cstring>

namespace loader {
namespace {

void DrawWindowChrome(bool& requestClose) {
	ImDrawList* draw = ImGui::GetWindowDrawList();

	const ImVec2 min = ImGui::GetWindowPos();
	const ImVec2 windowSize = ImGui::GetWindowSize();
	const ImVec2 max(
		min.x + windowSize.x,
		min.y + windowSize.y
	);

	draw->AddRectFilled(
		min,
		ImVec2(max.x, min.y + 38.0f),
		IM_COL32(20, 20, 22, 255)
	);

	draw->AddLine(
		ImVec2(min.x, min.y + 38.0f),
		ImVec2(max.x, min.y + 38.0f),
		IM_COL32(63, 39, 90, 255)
	);

	ImGui::SetCursorPos(ImVec2(18.0f, 12.0f));
	ImGui::TextUnformatted("NEXUS // MULTI LOADER");

	ImGui::SetCursorPos(ImVec2(windowSize.x - 32.0f, 8.0f));

	if (ImGui::Button("X", ImVec2(24.0f, 22.0f))) {
		requestClose = true;
	}
}

bool ProductCard(
	const ProductDefinition& product,
	ImVec2 size
) {
	ImGui::PushID(product.Slug.data());

	const ImVec2 cursor = ImGui::GetCursorScreenPos();

	ImGui::BeginDisabled(!product.Configured);
	const bool pressed = ImGui::Button("##card", size);
	ImGui::EndDisabled();

	ImDrawList* draw = ImGui::GetWindowDrawList();

	const ImVec2 bottomRight(
		cursor.x + size.x,
		cursor.y + size.y
	);

	draw->AddRect(
		cursor,
		bottomRight,
		product.Configured
			? IM_COL32(89, 54, 128, 255)
			: IM_COL32(48, 48, 52, 255)
	);

	draw->AddText(
		ImVec2(cursor.x + 16.0f, cursor.y + 18.0f),
		product.Configured
			? IM_COL32(235, 235, 238, 255)
			: IM_COL32(120, 120, 125, 255),
		product.DisplayName.data()
	);

	draw->AddText(
		ImVec2(cursor.x + 16.0f, cursor.y + 46.0f),
		IM_COL32(140, 140, 148, 255),
		product.Subtitle.data()
	);

	draw->AddText(
		ImVec2(
			cursor.x + 16.0f,
			cursor.y + size.y - 30.0f
		),
		product.Configured
			? IM_COL32(171, 107, 242, 255)
			: IM_COL32(180, 100, 100, 255),
		product.Configured
			? "SELECT"
			: "CONFIGURATION REQUIRED"
	);

	ImGui::PopID();
	return pressed && product.Configured;
}

} // namespace

UiController::~UiController() {
	SecureZeroMemory(m_License.data(), m_License.size());
}

void ApplyLoaderStyle() {
	ImGuiStyle& style = ImGui::GetStyle();

	style.WindowRounding = 0.0f;
	style.ChildRounding = 0.0f;
	style.FrameRounding = 2.0f;
	style.PopupRounding = 0.0f;
	style.GrabRounding = 2.0f;
	style.WindowPadding = ImVec2(0.0f, 0.0f);
	style.FramePadding = ImVec2(10.0f, 7.0f);
	style.ItemSpacing = ImVec2(8.0f, 9.0f);
	style.WindowBorderSize = 1.0f;
	style.FrameBorderSize = 1.0f;

	ImVec4* colors = style.Colors;
	colors[ImGuiCol_WindowBg] =
		ImVec4(0.055f, 0.055f, 0.060f, 1.0f);
	colors[ImGuiCol_ChildBg] =
		ImVec4(0.075f, 0.075f, 0.082f, 1.0f);
	colors[ImGuiCol_Border] =
		ImVec4(0.17f, 0.17f, 0.19f, 1.0f);
	colors[ImGuiCol_FrameBg] =
		ImVec4(0.11f, 0.11f, 0.12f, 1.0f);
	colors[ImGuiCol_FrameBgHovered] =
		ImVec4(0.15f, 0.15f, 0.17f, 1.0f);
	colors[ImGuiCol_FrameBgActive] =
		ImVec4(0.17f, 0.17f, 0.19f, 1.0f);
	colors[ImGuiCol_Button] =
		ImVec4(0.36f, 0.18f, 0.62f, 1.0f);
	colors[ImGuiCol_ButtonHovered] =
		ImVec4(0.43f, 0.23f, 0.72f, 1.0f);
	colors[ImGuiCol_ButtonActive] =
		ImVec4(0.31f, 0.15f, 0.55f, 1.0f);
	colors[ImGuiCol_CheckMark] =
		ImVec4(0.67f, 0.42f, 0.95f, 1.0f);
	colors[ImGuiCol_Text] =
		ImVec4(0.88f, 0.88f, 0.90f, 1.0f);
	colors[ImGuiCol_TextDisabled] =
		ImVec4(0.48f, 0.48f, 0.52f, 1.0f);
}

void UiController::Tick() {
	if (!m_Auth) {
		return;
	}

	m_Auth->Tick();

	if (m_License[0] != '\0' && m_Auth->Snapshot().Authenticated) {
		SecureZeroMemory(m_License.data(), m_License.size());
	}
}

void UiController::LoadRememberedLicense(
	const ProductDefinition& product
) {
	m_License.fill('\0');

	std::string savedLicense;
	if (!license_store::Load(product.Slug, savedLicense)) {
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

void UiController::DrawProductSelection() {
	ImGui::SetCursorPos(ImVec2(34.0f, 67.0f));

	ImGui::BeginChild(
		"##products",
		ImVec2(-34.0f, 278.0f),
		true
	);

	ImGui::TextColored(
		ImVec4(0.67f, 0.42f, 0.95f, 1.0f),
		"SELECT PRODUCT"
	);

	ImGui::TextDisabled(
		"Each product uses its own independent license pool."
	);

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	constexpr float cardWidth = 250.0f;
	constexpr float cardHeight = 148.0f;
	const ImVec2 cardSize(cardWidth, cardHeight);
	const float spacing = ImGui::GetStyle().ItemSpacing.x;
	const float availableWidth = ImGui::GetContentRegionAvail().x;
	const std::size_t columns = (std::max)(
		std::size_t{1},
		static_cast<std::size_t>(
			(availableWidth + spacing) / (cardWidth + spacing)
		)
	);

	for (std::size_t index = 0; index < Products.size(); ++index) {
		const auto& product = Products[index];

		if (index > 0 && index % columns != 0) {
			ImGui::SameLine();
		}

		if (ProductCard(product, cardSize)) {
			m_SelectedProduct = &product;
			m_Screen = Screen::Authentication;
			m_Remember = true;
			m_LaunchStatus.clear();
			LoadRememberedLicense(product);
			m_Auth = std::make_unique<AuthController>(product);
			m_Auth->Initialize();
		}
	}

	ImGui::Spacing();
	ImGui::TextDisabled(
		"More projects can be added to product_registry.hpp."
	);

	ImGui::EndChild();
}

void UiController::DrawAuthentication(bool& requestClose) {
	if (!m_SelectedProduct || !m_Auth) {
		m_Screen = Screen::ProductSelect;
		return;
	}

	const ProductDefinition& product = *m_SelectedProduct;
	const AuthSnapshot snapshot = m_Auth->Snapshot();
	bool goBack = false;

	ImGui::SetCursorPos(ImVec2(34.0f, 61.0f));

	ImGui::BeginDisabled(snapshot.Busy);
	if (ImGui::Button("< PRODUCTS", ImVec2(112.0f, 28.0f))) {
		goBack = true;
	}
	ImGui::EndDisabled();

	ImGui::SetCursorPos(ImVec2(34.0f, 100.0f));

	ImGui::BeginChild(
		"##auth-card",
		ImVec2(-34.0f, 245.0f),
		true
	);

	ImGui::TextColored(
		ImVec4(0.67f, 0.42f, 0.95f, 1.0f),
		"%s // AUTHENTICATION",
		product.DisplayName.data()
	);

	ImGui::TextDisabled(
		"Independent KeyAuth application // API 1.3 // v%s",
		product.KeyAuthVersion.data()
	);

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	if (!snapshot.Authenticated) {
		ImGui::Text(
			"%s license key",
			product.DisplayName.data()
		);

		ImGui::BeginDisabled(snapshot.Busy);

		ImGui::SetNextItemWidth(-1.0f);
		const bool submitted = ImGui::InputTextWithHint(
			"##license",
			"XXXXXX-XXXXXX-XXXXXX",
			m_License.data(),
			m_License.size(),
			ImGuiInputTextFlags_Password |
				ImGuiInputTextFlags_EnterReturnsTrue
		);

		ImGui::Checkbox(
			"Remember this product key on this Windows account",
			&m_Remember
		);

		ImGui::Spacing();

		const bool authenticate = ImGui::Button(
			"AUTHENTICATE",
			ImVec2(-1.0f, 38.0f)
		);

		ImGui::EndDisabled();

		if (!snapshot.Busy && (submitted || authenticate)) {
			m_Auth->Authenticate(
				std::string(m_License.data()),
				m_Remember
			);
		}
	} else {
		ImGui::Text(
			"Product: %s",
			product.DisplayName.data()
		);

		ImGui::Text(
			"User: %s",
			snapshot.Username.empty()
				? "licensed user"
				: snapshot.Username.c_str()
		);

		if (!snapshot.Subscription.empty()) {
			ImGui::Text(
				"Subscription: %s",
				snapshot.Subscription.c_str()
			);
		}

		if (!snapshot.Expiry.empty()) {
			ImGui::Text(
				"Expiry: %s",
				snapshot.Expiry.c_str()
			);
		}

		ImGui::Spacing();

		ImGui::BeginDisabled(snapshot.Busy);
		if (ImGui::Button(
			"LAUNCH",
			ImVec2(-1.0f, 38.0f)
		)) {
			if (LaunchConfiguredTarget(product, m_LaunchStatus)) {
				requestClose = true;
			}
		}
		ImGui::EndDisabled();

		if (!m_LaunchStatus.empty()) {
			ImGui::TextWrapped(
				"%s",
				m_LaunchStatus.c_str()
			);
		}
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	const ImVec4 statusColor =
		snapshot.Authenticated
			? ImVec4(0.45f, 0.85f, 0.55f, 1.0f)
			: snapshot.State == AuthState::Error ||
			  snapshot.State == AuthState::SessionInvalid
				? ImVec4(0.95f, 0.38f, 0.38f, 1.0f)
				: ImVec4(0.70f, 0.70f, 0.74f, 1.0f);

	ImGui::TextColored(
		statusColor,
		"%s",
		snapshot.Status.c_str()
	);

	ImGui::EndChild();

	if (goBack) {
		m_Auth.reset();
		m_SelectedProduct = nullptr;
		m_License.fill('\0');
		m_LaunchStatus.clear();
		m_Screen = Screen::ProductSelect;
	}
}

void UiController::Draw(bool& requestClose) {
	ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
	ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);

	ImGui::Begin(
		"##loader",
		nullptr,
		ImGuiWindowFlags_NoDecoration |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoSavedSettings
	);

	DrawWindowChrome(requestClose);

	switch (m_Screen) {
	case Screen::ProductSelect:
		DrawProductSelection();
		break;
	case Screen::Authentication:
		DrawAuthentication(requestClose);
		break;
	}

	ImGui::End();
}

} // namespace loader
