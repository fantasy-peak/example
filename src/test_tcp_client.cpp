
#include <asio_util.hpp>
#include <boost/asio.hpp>

#include <spdlog/spdlog.h>

#include <folly/executors/CPUThreadPoolExecutor.h>
#include <folly/experimental/coro/BlockingWait.h>

folly::coro::Task<void> start(boost::asio::io_context& io_context, std::string host, std::string port) {
	boost::asio::ip::tcp::socket socket(io_context);
	auto ec = co_await async_connect(io_context, socket, host, port);
	if (ec) {
		spdlog::error("Connect error: {}", ec.message());
		co_return;
	}
	spdlog::info("Connect to {}:{} successfully", host, port);
	constexpr int max_length = 1024;
	char write_buf[max_length] = {"hello"};
	char read_buf[max_length]{};
	constexpr int count = 100;
	for (int i = 0; i < count; ++i) {
		co_await async_write(socket, boost::asio::buffer(write_buf, max_length));
		auto [error, reply_length] = co_await async_read_some(socket, boost::asio::buffer(read_buf, max_length));
		if (error == boost::asio::error::eof) {
			spdlog::error("server close");
			break;
		}
		else if (error) {
			spdlog::error("error: {}", error.message());
			break;
		}
	}
	spdlog::info("Finished send and recieve");
	boost::system::error_code ignore_ec;
	socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignore_ec);
	io_context.stop();
}

int main(int argc, char** argv) {
	try {
		boost::asio::io_context io_context;
		std::thread thd([&] {
			boost::asio::io_context::work work(io_context);
			io_context.run();
		});
		folly::coro::blockingWait(start(io_context, "127.0.0.1", "8848"));
		thd.join();
	} catch (std::exception& e) {
		spdlog::error("Exception: {}", e.what());
	}
	return 0;
}
