
#include <io_context_pool.h>
#include <asio_util.hpp>

#include <spdlog/spdlog.h>

#include <folly/executors/CPUThreadPoolExecutor.h>
#include <folly/experimental/coro/BlockingWait.h>

class TcpServer final {
public:
	TcpServer(IoContextPool& pool)
		: m_pool(pool) {
	}

	folly::coro::Task<void> session(boost::asio::ip::tcp::socket sock) {
		for (;;) {
			const size_t max_length = 1024;
			char data[max_length]{};
			auto [error, length] = co_await async_read_some(sock, boost::asio::buffer(data, max_length));
			if (error) {
				spdlog::error("[session] {}", error.message());
				throw std::runtime_error(error.message());
			}
			co_await async_write(sock, boost::asio::buffer(data, length));
		}

		boost::system::error_code ec;
		sock.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
		sock.close(ec);
		co_return;
	}

	folly::coro::Task<void> start() {
		boost::asio::ip::tcp::acceptor acceptor(m_pool.getIoContext(), boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 8848));
		for (;;) {
			boost::asio::ip::tcp::socket socket(m_pool.getIoContext());
			auto error = co_await async_accept(acceptor, socket);
			spdlog::info("message: {}", error.message());
			if (error) {
				std::cout << "Accept failed, error: " << error.message() << '\n';
				continue;
			}
			std::cout << "New client comming.\n";
			session(std::move(socket)).scheduleOn(&m_thread_pool).start();
		}
		std::cout << "start" << std::endl;
		co_return;
	}

private:
	IoContextPool& m_pool;
	folly::CPUThreadPoolExecutor m_thread_pool{4, std::make_shared<folly::NamedThreadFactory>("TestThreadPool")};
};

int main(int argc, char** argv) {
	try {
		IoContextPool pool(10);
		std::thread thd([&] { pool.start(); });
		TcpServer server(pool);
		folly::coro::blockingWait(server.start());
		thd.join();
	} catch (std::exception& e) {
		std::cerr << "Exception: " << e.what() << "\n";
	}
	std::this_thread::sleep_for(std::chrono::seconds(1));
	return 0;
}
