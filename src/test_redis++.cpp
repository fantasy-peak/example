#include <spdlog/spdlog.h>

#include <folly/experimental/coro/BlockingWait.h>

#include <coroutine>
#include <iostream>

#define BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION
#define BOOST_THREAD_PROVIDES_EXECUTORS
#define BOOST_THREAD_USES_MOVE

#include <sw/redis++/async_redis++.h>
#include <boost/thread/executors/basic_thread_pool.hpp>

namespace sw {

namespace redis {

template <typename T>
struct FutureAwaiter {
	FutureAwaiter(boost::future<T>&& future)
		: _future(std::move(future)) {
	}

	FutureAwaiter(boost::future<T>&& future, boost::executors::basic_thread_pool* pool_ptr)
		: _future(std::move(future))
		, _pool_ptr(pool_ptr) {
	}

	bool await_ready() { return false; }
	void await_suspend(std::coroutine_handle<> handle) {
		auto func = [this, handle](sw::redis::Future<T> fut) mutable {
			try {
				_result = fut.get();
			} catch (...) {
				try {
					boost::rethrow_exception(fut.get_exception_ptr());
				} catch (const std::exception_ptr& ep) {
					_exception = ep;
				}
			}
			handle.resume();
		};
		if (_pool_ptr != nullptr)
			_future.then(*_pool_ptr, std::move(func));
		else
			_future.then(std::move(func));
	}

	T await_resume() {
		if (_exception)
			std::rethrow_exception(_exception);
		return std::move(_result);
	}

	boost::future<T> _future;
	boost::executors::basic_thread_pool* _pool_ptr{nullptr};
	std::decay_t<decltype(std::declval<boost::future<T>>().get())> _result;
	std::exception_ptr _exception{nullptr};
};

template <>
struct FutureAwaiter<void> {
	FutureAwaiter(boost::future<void>&& future)
		: _future(std::move(future)) {
	}

	FutureAwaiter(boost::future<void>&& future, boost::executors::basic_thread_pool* pool_ptr)
		: _future(std::move(future))
		, _pool_ptr(pool_ptr) {
	}

	bool await_ready() { return false; }
	void await_suspend(std::coroutine_handle<> handle) {
		auto func = [this, handle](sw::redis::Future<void> fut) mutable {
			try {
				fut.wait();
			} catch (...) {
				try {
					boost::rethrow_exception(fut.get_exception_ptr());
				} catch (const std::exception_ptr& ep) {
					_exception = ep;
				}
			}
			handle.resume();
		};
		if (_pool_ptr != nullptr)
			_future.then(*_pool_ptr, std::move(func));
		else
			_future.then(std::move(func));
	}

	void await_resume() {
		if (_exception)
			std::rethrow_exception(_exception);
		return;
	}

	boost::future<void> _future;
	boost::executors::basic_thread_pool* _pool_ptr{nullptr};
	std::exception_ptr _exception{nullptr};
};

} // namespace redis

} // namespace sw

int main(int argc, char** argv) {
	spdlog::info("------------------start test redis-plus-plus---------------------------");
	sw::redis::ConnectionOptions opts;
	opts.host = "127.0.0.1";
	opts.port = 6380;
	boost::executors::basic_thread_pool pool(3);
	auto async_redis_cli = sw::redis::AsyncRedis(opts);
	folly::coro::blockingWait([&]() mutable -> folly::coro::Task<void> {
		std::unordered_map<std::string, std::string> m = {{"a", "b"}, {"c", "d"}};
		boost::future<void> hmset_res = async_redis_cli.hmset("hash", m.begin(), m.end());
		co_await sw::redis::FutureAwaiter{std::move(hmset_res), &pool};

		boost::future<bool> set_ret = async_redis_cli.set("name", "xiaoli");
		auto flag = co_await sw::redis::FutureAwaiter{std::move(set_ret), &pool};
		if (flag)
			std::cout << "set key success!!!" << std::endl;

		boost::future<std::optional<std::string>> get_data = async_redis_cli.get("name");
		auto data = co_await sw::redis::FutureAwaiter{std::move(get_data), &pool};
		if (data)
			std::cout << "name:" << data.value() << std::endl;

		boost::future<std::vector<std::string>> hgetall_res = async_redis_cli.hgetall<std::vector<std::string>>("hash");
		auto hgetall_data = co_await sw::redis::FutureAwaiter{std::move(hgetall_res)};
		for (auto& d : hgetall_data)
			std::cout << d << std::endl;
	}());
	return 0;
}
