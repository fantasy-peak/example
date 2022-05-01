
#pragma once

#include <cstdlib>
#include <iostream>
#include <thread>
#include <utility>

#include <folly/experimental/coro/Task.h>

#include <boost/asio.hpp>

template <typename AsioBuffer>
std::pair<boost::system::error_code, size_t> read_some(boost::asio::ip::tcp::socket& sock, AsioBuffer&& buffer) {
	boost::system::error_code error;
	size_t length = sock.read_some(std::forward<AsioBuffer>(buffer), error);
	return std::make_pair(error, length);
}

template <typename AsioBuffer>
std::pair<boost::system::error_code, size_t> write(boost::asio::ip::tcp::socket& sock, AsioBuffer&& buffer) {
	boost::system::error_code error;
	auto length = boost::asio::write(sock, std::forward<AsioBuffer>(buffer), error);
	return std::make_pair(error, length);
}

std::pair<boost::system::error_code, boost::asio::ip::tcp::socket> accept(boost::asio::ip::tcp::acceptor& a) {
	boost::system::error_code error;
	auto socket = a.accept(error);
	return std::make_pair(error, std::move(socket));
}

std::pair<boost::system::error_code, boost::asio::ip::tcp::socket> connect(
	boost::asio::io_context& io_context, std::string host, std::string port) {
	boost::asio::ip::tcp::socket s(io_context);
	boost::asio::ip::tcp::resolver resolver(io_context);
	boost::system::error_code error;
	boost::asio::connect(s, resolver.resolve(host, port), error);
	return std::make_pair(error, std::move(s));
}

class AcceptorAwaiter {
public:
	AcceptorAwaiter(boost::asio::ip::tcp::acceptor& acceptor,
		boost::asio::ip::tcp::socket& socket)
		: acceptor_(acceptor)
		, socket_(socket) {}
	bool await_ready() const noexcept { return false; }
	void await_suspend(std::coroutine_handle<> handle) {
		acceptor_.async_accept(socket_, [this, handle](auto ec) mutable {
			ec_ = ec;
			handle.resume();
		});
	}
	auto await_resume() noexcept { return ec_; }

private:
	boost::asio::ip::tcp::acceptor& acceptor_;
	boost::asio::ip::tcp::socket& socket_;
	boost::system::error_code ec_{};
};

inline folly::coro::Task<boost::system::error_code> async_accept(
	boost::asio::ip::tcp::acceptor& acceptor, boost::asio::ip::tcp::socket& socket) noexcept {
	co_return co_await AcceptorAwaiter{acceptor, socket};
}

template <typename Socket, typename AsioBuffer>
struct ReadSomeAwaiter {
public:
	ReadSomeAwaiter(Socket& socket, AsioBuffer&& buffer)
		: socket_(socket)
		, buffer_(buffer) {
	}

	bool await_ready() { return false; }
	auto await_resume() { return std::make_pair(ec_, size_); }
	void await_suspend(std::coroutine_handle<> handle) {
		socket_.async_read_some(std::move(buffer_),
			[this, handle](auto ec, auto size) mutable {
				ec_ = ec;
				size_ = size;
				handle.resume();
			});
	}

private:
	Socket& socket_;
	AsioBuffer buffer_;

	boost::system::error_code ec_{};
	size_t size_{0};
};

template <typename Socket, typename AsioBuffer>
inline folly::coro::Task<std::pair<boost::system::error_code, size_t>> async_read_some(Socket& socket, AsioBuffer&& buffer) noexcept {
	co_return co_await ReadSomeAwaiter{socket, std::move(buffer)};
}

template <typename Socket, typename AsioBuffer>
struct ReadAwaiter {
public:
	ReadAwaiter(Socket& socket, AsioBuffer& buffer)
		: socket_(socket)
		, buffer_(buffer) {
	}

	bool await_ready() { return false; }
	auto await_resume() { return std::make_pair(ec_, size_); }
	void await_suspend(std::coroutine_handle<> handle) {
		boost::asio::async_read(socket_, buffer_, [this, handle](auto ec, auto size) mutable {
			ec_ = ec;
			size_ = size;
			handle.resume();
		});
	}

private:
	Socket& socket_;
	AsioBuffer& buffer_;

	boost::system::error_code ec_{};
	size_t size_{0};
};

template <typename Socket, typename AsioBuffer>
inline folly::coro::Task<std::pair<boost::system::error_code, size_t>> async_read(
	Socket& socket, AsioBuffer& buffer) noexcept {
	co_return co_await ReadAwaiter{socket, buffer};
}

template <typename Socket, typename AsioBuffer>
struct ReadUntilAwaiter {
public:
	ReadUntilAwaiter(Socket& socket, AsioBuffer& buffer,
		boost::asio::string_view delim)
		: socket_(socket)
		, buffer_(buffer)
		, delim_(delim) {
	}

	bool await_ready() { return false; }
	auto await_resume() { return std::make_pair(ec_, size_); }
	void await_suspend(std::coroutine_handle<> handle) {
		boost::asio::async_read_until(socket_, buffer_, delim_,
			[this, handle](auto ec, auto size) mutable {
				ec_ = ec;
				size_ = size;
				handle.resume();
			});
	}

private:
	Socket& socket_;
	AsioBuffer& buffer_;
	boost::asio::string_view delim_;

	boost::system::error_code ec_{};
	size_t size_{0};
};

template <typename Socket, typename AsioBuffer>
inline folly::coro::Task<std::pair<boost::system::error_code, size_t>> async_read_until(Socket& socket, AsioBuffer& buffer,
	boost::asio::string_view delim) noexcept {
	co_return co_await ReadUntilAwaiter{socket, buffer, delim};
}

template <typename Socket, typename AsioBuffer>
struct WriteAwaiter {
public:
	WriteAwaiter(Socket& socket, AsioBuffer&& buffer)
		: socket_(socket)
		, buffer_(std::move(buffer)) {}
	bool await_ready() { return false; }
	auto await_resume() { return std::make_pair(ec_, size_); }
	void await_suspend(std::coroutine_handle<> handle) {
		boost::asio::async_write(socket_, std::move(buffer_),
			[this, handle](auto ec, auto size) mutable {
				ec_ = ec;
				size_ = size;
				handle.resume();
			});
	}

private:
	Socket& socket_;
	AsioBuffer buffer_;

	boost::system::error_code ec_{};
	size_t size_{0};
};

template <typename Socket, typename AsioBuffer>
inline folly::coro::Task<std::pair<boost::system::error_code, size_t>> async_write(
	Socket& socket, AsioBuffer&& buffer) noexcept {
	co_return co_await WriteAwaiter{socket, std::move(buffer)};
}

class ConnectAwaiter {
public:
	ConnectAwaiter(boost::asio::io_context& io_context, boost::asio::ip::tcp::socket& socket,
		const std::string& host, const std::string& port)
		: io_context_(io_context)
		, socket_(socket)
		, host_(host)
		, port_(port) {
	}

	bool await_ready() const noexcept { return false; }
	void await_suspend(std::coroutine_handle<> handle) {
		boost::asio::ip::tcp::resolver resolver(io_context_);
		auto endpoints = resolver.resolve(host_, port_);
		boost::asio::async_connect(socket_, endpoints,
			[this, handle](boost::system::error_code ec,
				boost::asio::ip::tcp::endpoint) mutable {
				ec_ = ec;
				handle.resume();
			});
	}
	auto await_resume() noexcept { return ec_; }

private:
	boost::asio::io_context& io_context_;
	boost::asio::ip::tcp::socket& socket_;
	std::string host_;
	std::string port_;
	boost::system::error_code ec_{};
};

inline folly::coro::Task<boost::system::error_code> async_connect(
	boost::asio::io_context& io_context, boost::asio::ip::tcp::socket& socket,
	const std::string& host, const std::string& port) noexcept {
	co_return co_await ConnectAwaiter{io_context, socket, host, port};
}

class TimerAwaiter {
public:
	TimerAwaiter(boost::asio::steady_timer& steady_timer_)
		: steady_timer_(steady_timer_) {
	}

	bool await_ready() const noexcept { return false; }
	void await_suspend(std::coroutine_handle<> handle) {
		steady_timer_.async_wait([this, handle](const boost::system::error_code& ec) {
			ec_ = ec;
			handle.resume();
		});
	}
	auto await_resume() noexcept { return ec_; }

private:
	boost::asio::steady_timer& steady_timer_;
	boost::system::error_code ec_{};
};

inline folly::coro::Task<boost::system::error_code> timeout(boost::asio::steady_timer& steady_timer_) noexcept {
	co_return co_await TimerAwaiter{steady_timer_};
}
