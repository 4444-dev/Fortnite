#include "auth_controller.hpp"
#include "launch_target.hpp"
#include "license_store.hpp"
#include "loader_renderer.hpp"
#include "loader_window.hpp"
#include "product_registry.hpp"


#include <thirdparty/imgui/imgui.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <memory>
#include <string>

namespace {


enum class Screen {
	ProductSelect,
	Authentication
};

using LicenseBuffer =
	std::array<char, loader::license_store::kMaxLicenseLength + 1>;

void ApplyStyle() {
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
	ImGui::TextUnformatted("LUVKRIMES // MULTI LOADER");

	ImGui::SetCursorPos(ImVec2(588.0f, 8.0f));

	if (ImGui::Button("X", ImVec2(24.0f, 22.0f))) {
		requestClose = true;
	}
}

bool ProductCard(
	const loader::ProductDefinition& product,
	ImVec2 size
) {
	ImGui::PushID(product.Slug.data());

	const ImVec2 cursor = ImGui::GetCursorScreenPos();

	ImGui::BeginDisabled(!product.Configured);

	const bool pressed =
		ImGui::Button("##card", size);

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

void DrawProductSelection(
	const loader::ProductDefinition*& selectedProduct,
	Screen& screen
) {
	ImGui::SetCursorPos(ImVec2(34.0f, 67.0f));

	ImGui::BeginChild(
		"##products",
		ImVec2(552.0f, 278.0f),
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

	const ImVec2 cardSize(250.0f, 148.0f);

	for (std::size_t index = 0; index < loader::Products.size(); ++index) {
		const auto& product = loader::Products[index];

		if (index > 0) {
			ImGui::SameLine();
		}

		if (ProductCard(product, cardSize)) {
			selectedProduct = &product;
			screen = Screen::Authentication;
		}
	}

	ImGui::Spacing();
	ImGui::TextDisabled(
		"More projects can be added to product_registry.hpp."
	);

	ImGui::EndChild();
}

void LoadRememberedLicense(
	const loader::ProductDefinition& product,
	LicenseBuffer& license
) {
	license.fill('\0');

	std::string savedLicense;

	if (!loader::license_store::Load(product.Slug, savedLicense)) {
		return;
	}

	const std::size_t count = (std::min)(
		savedLicense.size(),
		license.size() - 1
	);

	std::memcpy(
		license.data(),
		savedLicense.data(),
		count
	);

	license[count] = '\0';
}

void DrawAuthentication(
	const loader::ProductDefinition& product,
	loader::AuthController& auth,
	LicenseBuffer& license,
	bool& remember,
	bool& goBack,
	bool& requestClose,
	std::string& launchStatus
) {
	const loader::AuthSnapshot snapshot = auth.Snapshot();

	ImGui::SetCursorPos(ImVec2(34.0f, 61.0f));

	ImGui::BeginDisabled(snapshot.Busy);
	if (ImGui::Button("< PRODUCTS", ImVec2(112.0f, 28.0f))) {
		goBack = true;
	}
	ImGui::EndDisabled();

	ImGui::SetCursorPos(ImVec2(34.0f, 100.0f));

	ImGui::BeginChild(
		"##auth-card",
		ImVec2(552.0f, 245.0f),
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

		ImGui::SetNextItemWidth(-1.0f);

		ImGui::InputTextWithHint(
			"##license",
			"XXXXXX-XXXXXX-XXXXXX",
			license.data(),
			license.size(),
			ImGuiInputTextFlags_Password
		);

		ImGui::Checkbox(
			"Remember this product key on this Windows account",
			&remember
		);

		ImGui::Spacing();

		ImGui::BeginDisabled(snapshot.Busy);

		if (ImGui::Button(
			"AUTHENTICATE",
			ImVec2(-1.0f, 38.0f)
		)) {
			auth.Authenticate(
				std::string(license.data()),
				remember
			);
		}

		ImGui::EndDisabled();
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

		if (ImGui::Button(
			"LAUNCH",
			ImVec2(-1.0f, 38.0f)
		)) {
			if (loader::LaunchConfiguredTarget(product, launchStatus)) {
				requestClose = true;
			}
		}

		if (!launchStatus.empty()) {
			ImGui::TextWrapped(
				"%s",
				launchStatus.c_str()
			);
		}
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	const ImVec4 statusColor =
		snapshot.Authenticated
			? ImVec4(0.45f, 0.85f, 0.55f, 1.0f)
			: snapshot.State == loader::AuthState::Error ||
			  snapshot.State == loader::AuthState::SessionInvalid
				? ImVec4(0.95f, 0.38f, 0.38f, 1.0f)
				: ImVec4(0.70f, 0.70f, 0.74f, 1.0f);

	ImGui::TextColored(
		statusColor,
		"%s",
		snapshot.Status.c_str()
	);

	ImGui::EndChild();
}

} // namespace

int WINAPI wWinMain(
	HINSTANCE,
	HINSTANCE,
	PWSTR,
	int
) {
	loader::Renderer renderer;
	loader::Window window;

	if (!window.Create()) {
		return 1;
	}

	window.AttachRenderer(&renderer);

	if (!renderer.Initialize(window.Handle())) {
		return 1;
	}

	ApplyStyle();
	window.Show();

	Screen screen = Screen::ProductSelect;
	const loader::ProductDefinition* selectedProduct = nullptr;

	std::unique_ptr<loader::AuthController> auth;
	LicenseBuffer license{};
	std::string launchStatus;
	bool remember = true;
	bool running = true;

	while (running && window.PumpMessages()) {
		if (auth) {
			auth->Tick();
		}

		if (!renderer.IsReady()) {
			break;
		}

		renderer.BeginFrame();

		ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
		ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);

		ImGui::Begin(
			"##loader",
			nullptr,
			ImGuiWindowFlags_NoDecoration |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoSavedSettings
		);

		bool requestClose = false;

		DrawWindowChrome(requestClose);

		if (screen == Screen::ProductSelect) {
			const loader::ProductDefinition* requestedProduct = nullptr;

			DrawProductSelection(
				requestedProduct,
				screen
			);

			if (requestedProduct) {
				selectedProduct = requestedProduct;
				remember = true;
				launchStatus.clear();

				LoadRememberedLicense(
					*selectedProduct,
					license
				);

				auth =
					std::make_unique<loader::AuthController>(
						*selectedProduct
					);

				auth->Initialize();
			}
		} else if (
			screen == Screen::Authentication &&
			selectedProduct &&
			auth
		) {
			bool goBack = false;

			DrawAuthentication(
				*selectedProduct,
				*auth,
				license,
				remember,
				goBack,
				requestClose,
				launchStatus
			);

			if (goBack) {
				auth.reset();
				selectedProduct = nullptr;
				license.fill('\0');
				launchStatus.clear();
				screen = Screen::ProductSelect;
			}
		}

		ImGui::End();

		if (!renderer.EndFrame()) {
			running = false;
		}

		if (requestClose) {
			running = false;
		}
	}

	auth.reset();
	window.AttachRenderer(nullptr);
	renderer.Shutdown();
	window.Destroy();

	return 0;
}
