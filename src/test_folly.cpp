#include <spdlog/spdlog.h>

#include <folly/Portability.h>
#include <folly/init/Init.h>

#include <folly/Singleton.h>
#include <folly/executors/CPUThreadPoolExecutor.h>
#include <folly/executors/GlobalExecutor.h>
#include <folly/executors/ManualExecutor.h>
#include <folly/experimental/ThreadWheelTimekeeperHighRes.h>
#include <folly/experimental/coro/AsyncScope.h>
#include <folly/experimental/coro/Baton.h>
#include <folly/experimental/coro/BlockingWait.h>
#include <folly/experimental/coro/Collect.h>
#include <folly/experimental/coro/CurrentExecutor.h>
#include <folly/experimental/coro/FutureUtil.h>
#include <folly/experimental/coro/Generator.h>
#include <folly/experimental/coro/Mutex.h>
#include <folly/experimental/coro/Promise.h>
#include <folly/experimental/coro/SharedMutex.h>
#include <folly/experimental/coro/Sleep.h>
#include <folly/experimental/coro/Task.h>
#include <folly/experimental/coro/Timeout.h>
#include <folly/futures/ThreadWheelTimekeeper.h>

auto get_id() {
	std::stringstream ss;
	ss << std::this_thread::get_id();
	return ss.str();
}

// valgrind --log-file=a.txt --tool=memcheck --leak-check=full -s ./test_folly
int main(int argc, char** argv) {
	spdlog::info("------------------start test folly---------------------------");
	folly::CPUThreadPoolExecutor thread_pool{4, std::make_shared<folly::NamedThreadFactory>("TestThreadPool")};
	auto thread_wheel_timekeeper_ptr = std::make_shared<folly::ThreadWheelTimekeeper>();

	std::vector<folly::Future<folly::Unit>> futures;
	for (size_t i = 0; i < 4; ++i) {
		futures.emplace_back(folly::via(&thread_pool, [] {
			spdlog::info("{}", get_id());
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
		}));
	}
	folly::collectAll(std::move(futures)).get();

	auto make_task = [&]() -> folly::coro::Task<void> {
		spdlog::info("start folly::coro::sleep");
		co_await folly::coro::sleep(std::chrono::seconds(1), thread_wheel_timekeeper_ptr.get());
		spdlog::info("end folly::coro::sleep");
		co_return;
	};
	auto v = make_task().scheduleOn(&thread_pool).start();
	std::move(v).get();

	spdlog::info("start test SharedMutex");
	folly::coro::SharedMutex mutex;
	auto make_writer_task = [&]() -> folly::coro::Task<void> {
		auto lock = co_await mutex.co_scoped_lock();
	};
	auto make_reader_task = [&]() -> folly::coro::Task<void> {
		auto lock = co_await mutex.co_scoped_lock_shared();
	};
	auto w1 = make_writer_task().scheduleOn(&thread_pool).start();
	auto r1 = make_reader_task().scheduleOn(&thread_pool).start();
	std::move(w1).get();
	std::move(r1).get();

	folly::coro::blockingWait([&]() -> folly::coro::Task<void> {
		auto make_task = [&]() -> folly::coro::Task<int32_t> {
			throw std::runtime_error("async exection");
			co_return 100;
		};
		auto f = make_task().scheduleOn(&thread_pool).start();
		try {
			[[maybe_unused]] auto ret = co_await std::move(f);
			// auto ret = co_await folly::coro::toTask(std::move(f));
		} catch (const std::runtime_error& e) {
			spdlog::error("{}", e.what());
		}
		co_return;
	}());

	folly::coro::blockingWait([&]() -> folly::coro::Task<void> {
		spdlog::info("start test AsyncScope");
		auto executor = co_await folly::coro::co_current_executor;
		auto make_task = [&]() -> folly::coro::Task<> {
			co_return;
		};
		folly::coro::AsyncScope scope;
		for (int i = 0; i < 2; ++i)
			scope.add(make_task().scheduleOn(executor));
		co_await scope.joinAsync();
		co_return;
	}());

	folly::coro::blockingWait([&]() -> folly::coro::Task<void> {
		auto [a, b, c] = co_await folly::coro::collectAllTry(
			[&]() -> folly::coro::Task<int> {
				co_await folly::coro::sleep(std::chrono::seconds(1), thread_wheel_timekeeper_ptr.get());
				spdlog::info("task1 thread id: {}", get_id());
				co_return 42;
			}(),
			[&]() -> folly::coro::Task<float> {
				co_await folly::coro::sleep(std::chrono::seconds(2), thread_wheel_timekeeper_ptr.get());
				spdlog::info("task2 thread id: {}", get_id());
				co_return 3.14f;
			}(),
			[]() -> folly::coro::Task<void> {
				spdlog::info("task3 thread id: {}", get_id());
				co_await folly::coro::co_reschedule_on_current_executor;
				spdlog::info("task3.1 thread id: {}", get_id());
				throw std::runtime_error("error");
			}());
		spdlog::info("a: {}, b: {}, c: {}", a.hasValue(), b.hasValue(), c.hasException());
	}());

	struct Awaiter {
		Awaiter() {}
		bool await_ready() { return false; }
		void await_suspend(std::coroutine_handle<> handle) {
			auto func = [this, handle]() mutable {
				std::this_thread::sleep_for(std::chrono::seconds(5));
				spdlog::info("-----------resume-------------------");
				handle.resume();
			};
			std::thread(std::move(func)).detach();
		}
		void await_resume() {}
	};

	auto test_timeout = [&]() -> folly::coro::Task<> {
		spdlog::info("test_timeout thread id: {}", get_id());
		auto make_task = [&]() -> folly::coro::Task<int> {
			spdlog::info("start timeout Awaiter");
			co_await Awaiter{};
			spdlog::info("timeout return 42");
			co_return 42;
		};
		spdlog::info("start timeout");
		auto result = co_await folly::coro::co_awaitTry(folly::coro::timeout(make_task(), std::chrono::seconds(1), thread_wheel_timekeeper_ptr.get()));
		spdlog::info("has FutureTimeout Exception: {}", result.hasException<folly::FutureTimeout>());
		spdlog::info("timeout thread id: {}", get_id());
	};
	auto t1 = test_timeout().scheduleOn(&thread_pool).start();
	std::move(t1).get();

	folly::coro::blockingWait([]() -> folly::coro::Task<void> {
		auto [promise, future] = folly::makePromiseContract<int>();
		promise.setException(std::runtime_error(""));
		auto res = co_await folly::coro::co_awaitTry(std::move(future));
		spdlog::info("hasException: {}", res.hasException<std::runtime_error>());
	}());

	std::shared_ptr<int> int_ptr(new int(996), [](auto* p) {
		spdlog::info("Deconstruction int_ptr!!!");
		delete p;
	});

	// clang-format off
	folly::coro::co_invoke([int_ptr = std::move(int_ptr)]() -> folly::coro::Task<void> {
		spdlog::info("int_ptr: {}", *int_ptr);
		co_return;
	}).scheduleOn(&thread_pool).start();
	// clang-format on

	folly::coro::blockingWait([]() -> folly::coro::Task<void> {
		auto [promise, future] = folly::coro::makePromiseContract<int>();
		auto waiter = [](auto future) -> folly::coro::Task<int> {
			co_return co_await std::move(future);
		}(std::move(future));

		auto fulfiller = [](auto promise) -> folly::coro::Task<> {
			promise.setValue(42);
			//promise.setException(std::logic_error("test error"));
			co_return;
		}(std::move(promise));

		auto [res, _] = co_await folly::coro::collectAll(
			folly::coro::co_awaitTry(std::move(waiter)), std::move(fulfiller));

		if (res.hasException<std::logic_error>()) {
			spdlog::info("hasException: {}", res.exception().what());
		}
		if (res.hasValue()) {
			spdlog::info("value: {}", res.value());
		}
	}());
	std::this_thread::sleep_for(std::chrono::seconds(1));

	return 0;
}
