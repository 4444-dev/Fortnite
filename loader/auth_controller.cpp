#include "auth_controller.hpp"

#include "license_store.hpp"

#include <exception>
#include <utility>

namespace loader {

AuthController::AuthController(const ProductDefinition& product)
	: m_Product(product),
	  m_App(
		std::string(product.KeyAuthName),
		std::string(product.KeyAuthOwnerId),
		std::string(product.KeyAuthVersion),
		std::string(product.KeyAuthUrl),
		std::string(product.KeyAuthPath)
	  ) {
}

AuthController::~AuthController() {
	JoinWorker();
}

void AuthController::Initialize() {
	if (!m_Product.Configured) {
		std::scoped_lock lock(m_Mutex);
		m_Snapshot.State = AuthState::Error;
		m_Snapshot.Status = "Product authentication configuration is incomplete.";
		m_Snapshot.Busy = false;
		m_Snapshot.Authenticated = false;
		return;
	}

	Run(
		AuthState::Connecting,
		"Connecting to " + std::string(m_Product.DisplayName) + "...",
		[this] {
			m_App.init();

			std::scoped_lock lock(m_Mutex);
			if (!m_App.response.success) {
				m_Snapshot.State = AuthState::Error;
				m_Snapshot.Status = m_App.response.message.empty()
					? "Authentication initialization failed."
					: m_App.response.message;
				m_Snapshot.Authenticated = false;
				return;
			}

			m_Snapshot.State = AuthState::Ready;
			m_Snapshot.Status =
				"Ready. Enter your " +
				std::string(m_Product.DisplayName) +
				" license key.";
			m_Snapshot.Authenticated = false;
		}
	);
}

void AuthController::Authenticate(std::string license, bool remember) {
	if (license.empty() || license.size() > license_store::kMaxLicenseLength) {
		std::scoped_lock lock(m_Mutex);
		m_Snapshot.State = AuthState::Error;
		m_Snapshot.Status = license.empty()
			? "Enter a license key."
			: "License key exceeds the supported length.";
		m_Snapshot.Busy = false;
		m_Snapshot.Authenticated = false;
		return;
	}

	Run(
		AuthState::Authenticating,
		"Validating " + std::string(m_Product.DisplayName) + " license...",
		[this, license = std::move(license), remember] {
			m_App.license(license);

			std::scoped_lock lock(m_Mutex);
			if (!m_App.response.success) {
				m_Snapshot.State = AuthState::Error;
				m_Snapshot.Status = m_App.response.message.empty()
					? "License validation failed."
					: m_App.response.message;
				m_Snapshot.Authenticated = false;
				return;
			}

			m_Snapshot.State = AuthState::Authenticated;
			m_Snapshot.Status =
				std::string(m_Product.DisplayName) +
				" authentication successful.";
			m_Snapshot.Authenticated = true;
			m_Snapshot.Username = m_App.user_data.username;

			if (!m_App.user_data.subscriptions.empty()) {
				const auto& subscription =
					m_App.user_data.subscriptions.front();
				m_Snapshot.Subscription = subscription.name;
				m_Snapshot.Expiry = subscription.expiry;
			} else {
				m_Snapshot.Subscription.clear();
				m_Snapshot.Expiry.clear();
			}

			if (remember) {
				if (!license_store::Save(m_Product.Slug, license)) {
					m_Snapshot.Status +=
						" License accepted, but Windows could not remember it.";
				}
			} else {
				license_store::Clear(m_Product.Slug);
			}

			m_NextSessionCheck =
				std::chrono::steady_clock::now() +
				std::chrono::seconds(60);
		}
	);
}

void AuthController::Tick() {
	{
		const auto now = std::chrono::steady_clock::now();
		std::scoped_lock lock(m_Mutex);

		if (
			!m_Snapshot.Authenticated ||
			m_Snapshot.Busy ||
			now < m_NextSessionCheck
		) {
			return;
		}
	}

	Run(
		AuthState::Authenticated,
		std::string(m_Product.DisplayName) + " session active.",
		[this] {
			m_App.check();

			std::scoped_lock lock(m_Mutex);
			if (!m_App.response.success) {
				m_Snapshot.State = AuthState::SessionInvalid;
				m_Snapshot.Status = m_App.response.message.empty()
					? "Session validation failed."
					: m_App.response.message;
				m_Snapshot.Authenticated = false;
				return;
			}

			m_Snapshot.State = AuthState::Authenticated;
			m_Snapshot.Status =
				std::string(m_Product.DisplayName) +
				" session active.";
			m_Snapshot.Authenticated = true;
			m_NextSessionCheck =
				std::chrono::steady_clock::now() +
				std::chrono::seconds(60);
		}
	);
}

AuthSnapshot AuthController::Snapshot() const {
	std::scoped_lock lock(m_Mutex);
	return m_Snapshot;
}

void AuthController::Run(
	AuthState state,
	std::string status,
	std::function<void()> task
) {
	if (m_Busy.exchange(true)) {
		return;
	}

	JoinWorker();

	{
		std::scoped_lock lock(m_Mutex);
		m_Snapshot.State = state;
		m_Snapshot.Status = std::move(status);
		m_Snapshot.Busy = true;
	}

	try {
		m_Worker = std::thread(
			[this, task = std::move(task)]() mutable {
				try {
					task();
				} catch (const std::exception& exception) {
					std::scoped_lock lock(m_Mutex);
					m_Snapshot.State = AuthState::Error;
					m_Snapshot.Status = exception.what();
					m_Snapshot.Authenticated = false;
				} catch (...) {
					std::scoped_lock lock(m_Mutex);
					m_Snapshot.State = AuthState::Error;
					m_Snapshot.Status = "Unexpected authentication error.";
					m_Snapshot.Authenticated = false;
				}

				{
					std::scoped_lock lock(m_Mutex);
					m_Snapshot.Busy = false;
				}

				m_Busy.store(false);
			}
		);
	} catch (const std::exception& exception) {
		std::scoped_lock lock(m_Mutex);
		m_Snapshot.State = AuthState::Error;
		m_Snapshot.Status =
			"Unable to start authentication worker: " +
			std::string(exception.what());
		m_Snapshot.Busy = false;
		m_Snapshot.Authenticated = false;
		m_Busy.store(false);
	}
}

void AuthController::JoinWorker() {
	if (m_Worker.joinable()) {
		m_Worker.join();
	}
}


} // namespace loader
