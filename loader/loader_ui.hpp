#pragma once

#include "auth_controller.hpp"
#include "license_store.hpp"
#include "product_registry.hpp"

#include <array>
#include <memory>
#include <string>

namespace loader {

void ApplyLoaderStyle();

class UiController final {
public:
	UiController() = default;
	~UiController();

	UiController(const UiController&) = delete;
	UiController& operator=(const UiController&) = delete;
	UiController(UiController&&) = delete;
	UiController& operator=(UiController&&) = delete;

	void Tick();
	void Draw(bool& requestClose, bool& requestMinimize);

private:
	enum class Page {
		Home,
		Products,
		Settings,
		About
	};

	using LicenseBuffer =
		std::array<char, license_store::kMaxLicenseLength + 1>;

	void EnsureSelection();
	void SelectProduct(const ProductDefinition& product);
	void LoadRememberedLicense(const ProductDefinition& product);

	void DrawSidebar();
	void DrawHeader(bool& requestClose, bool& requestMinimize);
	void DrawHome(bool& requestClose);
	void DrawProducts();
	void DrawSettings();
	void DrawAbout();

	Page m_Page = Page::Home;
	const ProductDefinition* m_SelectedProduct = nullptr;
	std::unique_ptr<AuthController> m_Auth;
	LicenseBuffer m_License{};
	std::string m_LaunchStatus;
	bool m_Remember = true;
};

} // namespace loader
