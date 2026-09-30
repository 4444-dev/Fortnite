#pragma once

#include "product_registry.hpp"

#include <auth.hpp>

#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <thread>

namespace loader {

enum class AuthState {
	Connecting,
	Ready,
	Authenticating,
	Authenticated,
	Error,
	SessionInvalid
};

struct AuthSnapshot {
	AuthState State = AuthState::Connecting;
	std::string Status = "Connecting to authentication service...";
	std::string Username;
	std::string Subscription;
	std::string Expiry;
	bool Busy = true;
	bool Authenticated = false;
};

class AuthController final {
public:
	explicit AuthController(const ProductDefinition& product);
	~AuthController();

	AuthController(const AuthController&) = delete;
	AuthController& operator=(const AuthController&) = delete;
	AuthController(AuthController&&) = delete;
	AuthController& operator=(AuthController&&) = delete;

	void Initialize();
	void Authenticate(std::string license, bool remember);
	void Tick();

	[[nodiscard]] AuthSnapshot Snapshot() const;

private:
	template <typename Fn>
	void Run(AuthState state, std::string status, Fn&& fn);

	void JoinWorker();

	const ProductDefinition& m_Product;
	KeyAuth::api m_App;

	mutable std::mutex m_Mutex;
	AuthSnapshot m_Snapshot{};
	std::thread m_Worker;
	std::atomic<bool> m_Busy{false};
	std::chrono::steady_clock::time_point m_NextSessionCheck{};
};

} // namespace loader
