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
	~UiController() = default;

	UiController(const UiController&) = delete;
	UiController& operator=(const UiController&) = delete;
	UiController(UiController&&) = delete;
	UiController& operator=(UiController&&) = delete;

	void Tick();
	void Draw(bool& requestClose);

private:
	enum class Screen {
		ProductSelect,
		Authentication
	};

	using LicenseBuffer =
		std::array<char, license_store::kMaxLicenseLength + 1>;

	void LoadRememberedLicense(const ProductDefinition& product);
	void DrawProductSelection();
	void DrawAuthentication(bool& requestClose);

	Screen m_Screen = Screen::ProductSelect;
	const ProductDefinition* m_SelectedProduct = nullptr;
	std::unique_ptr<AuthController> m_Auth;
	LicenseBuffer m_License{};
	std::string m_LaunchStatus;
	bool m_Remember = true;
};

} // namespace loader
