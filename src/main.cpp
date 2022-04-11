#include "xmake_example.h"

#include <drogon/drogon.h>
#include <drogon/utils/coroutine.h>
#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <spdlog/spdlog.h>
#include <sw/redis++/async_redis++.h>
#include <trantor/net/EventLoopThreadPool.h>
#include <nlohmann/json.hpp>

int main() {
	spdlog::info("------------------start test redis-plus-plus---------------------------");
	sw::redis::ConnectionOptions opts;
	opts.host = "127.0.0.1";
	opts.port = 6380;
	auto redis = sw::redis::AsyncRedis(opts);
	auto fut = redis.get("key").then([](sw::redis::Future<sw::redis::Optional<std::string>> fut) {
		auto val = fut.get();
		if (val)
			std::cout << *val << std::endl;
	});
	fut.wait();
	spdlog::info("------------------start test drogon---------------------------");
	trantor::EventLoopThreadPool event_loop_thread_pool{std::thread::hardware_concurrency()};
	event_loop_thread_pool.start();
	std::vector<drogon::HttpClientPtr> http_clients;
	for (uint32_t i = 0; i < std::thread::hardware_concurrency(); i++)
		http_clients.emplace_back(drogon::HttpClient::newHttpClient("http://127.0.0.1:9200", event_loop_thread_pool.getNextLoop()));
	auto get_http_client = [&]() -> auto& {
		static std::mutex tx;
		static uint32_t count = 0;
		std::lock_guard<std::mutex> lk(tx);
		if (count == std::thread::hardware_concurrency())
			count = 0;
		return http_clients[count++];
	};
	auto task = [&]() -> drogon::Task<void> {
		auto http_request_ptr = drogon::HttpRequest::newHttpRequest();
		auto query_resp = co_await get_http_client()->sendRequestCoro(std::move(http_request_ptr));
		spdlog::info("end query!!!");
		co_return;
	};
	auto create_lazy_task = [&]() -> drogon::Task<void> {
		try {
			co_await task();
		} catch (const std::runtime_error& e) {
			fmt::print("{}\n", e.what());
		} catch (nlohmann::json::exception& e) {
			fmt::print("{}\n", e.what());
		}
	};
	drogon::sync_wait(create_lazy_task());
	return 0;
}