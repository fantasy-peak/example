
#include <io_context_pool.h>
#include <asio_util.hpp>

#include <spdlog/spdlog.h>

#include <folly/executors/CPUThreadPoolExecutor.h>
#include <folly/experimental/coro/BlockingWait.h>

class Executor : public folly::Executor {
public:
	Executor(boost::asio::io_context& io_context)
		: m_io_context(io_context) {
	}

	virtual void add(folly::Func func) override {
		boost::asio::post(m_io_context, std::move(func));
	}

	boost::asio::io_context& m_io_context;
};

class TcpServer final {
public:
	TcpServer(IoContextPool& pool)
		: m_pool(pool) {
	}

	folly::coro::Task<void> session(boost::asio::ip::tcp::socket sock, boost::asio::steady_timer steady_timer) {
		constexpr int32_t max_length = 1024;
		for (;;) {
			spdlog::info("start test timeout");
			steady_timer.expires_after(std::chrono::seconds(2));
			auto ec = co_await timeout(steady_timer);
			// https://www.boost.org/doc/libs/master/boost/asio/error.hpp
			// ec == boost::asio::error::operation_aborted ?
			spdlog::info("end test timeout: {}", ec.message());
			char data[max_length]{};
			auto [error, length] = co_await async_read_some(sock, boost::asio::buffer(data, max_length));
			if (error) {
				spdlog::error("[session] {}", error.message());
				break;
			}
			co_await async_write(sock, boost::asio::buffer(data, length));
		}
		boost::system::error_code ec;
		sock.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
		sock.close(ec);
		co_return;
	}

	folly::coro::Task<void> start() {
		boost::asio::ip::tcp::acceptor acceptor(m_pool.getIoContext());
		boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 8848));
		acceptor.open(endpoint.protocol());
		acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
		acceptor.bind(endpoint);
		acceptor.listen();
		for (;;) {
			auto& context = m_pool.getIoContext();
			if (!m_executor_map.contains(&context))
				m_executor_map.emplace(&context, Executor{context});
			boost::asio::ip::tcp::socket socket(context);
			auto error = co_await async_accept(acceptor, socket);
			if (error) {
				spdlog::error("Accept failed, error: {}", error.message());
				continue;
			}
			boost::asio::steady_timer steady_timer_{context};
			session(std::move(socket), std::move(steady_timer_)).scheduleOn(&m_executor_map.at(&context)).start();
		}
		co_return;
	}

private:
	IoContextPool& m_pool;
	std::unordered_map<boost::asio::io_context*, Executor> m_executor_map;
};

int main(int argc, char** argv) {
	try {
		IoContextPool pool(10);
		std::thread thd([&] { pool.start(); });
		TcpServer server(pool);
		folly::coro::blockingWait(server.start());
		pool.stop();
		thd.join();
	} catch (std::exception& e) {
		spdlog::error("Exception: {}", e.what());
	}
	return 0;
}
