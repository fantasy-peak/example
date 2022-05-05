#pragma once

#include <cstdlib>
#include <iostream>
#include <thread>
#include <utility>

#include <folly/experimental/coro/Task.h>

#include <boost/asio.hpp>

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
class AcceptorAwaiter {
public:
	AcceptorAwaiter(boost::asio::ip::tcp::acceptor& acceptor, boost::asio::ip::tcp::socket& socket)
		: m_acceptor(acceptor)
		, m_socket(socket) {
	}

	bool await_ready() const noexcept { return false; }
	void await_suspend(std::coroutine_handle<> handle) {
		m_acceptor.async_accept(m_socket, [this, handle](auto ec) mutable {
			m_ec = ec;
			handle.resume();
		});
	}
	auto await_resume() noexcept { return m_ec; }

private:
	boost::asio::ip::tcp::acceptor& m_acceptor;
	boost::asio::ip::tcp::socket& m_socket;
	boost::system::error_code m_ec{};
};

inline folly::coro::Task<boost::system::error_code> async_accept(
	boost::asio::ip::tcp::acceptor& acceptor, boost::asio::ip::tcp::socket& socket) noexcept {
	co_return co_await AcceptorAwaiter{acceptor, socket};
}

template <typename Socket, typename AsioBuffer>
struct ReadSomeAwaiter {
public:
	ReadSomeAwaiter(Socket& socket, AsioBuffer&& buffer)
		: m_socket(socket)
		, m_buffer(buffer) {
	}

	bool await_ready() { return false; }
	auto await_resume() { return std::make_pair(m_ec, size_); }
	void await_suspend(std::coroutine_handle<> handle) {
		m_socket.async_read_some(std::move(m_buffer), [this, handle](auto ec, auto size) mutable {
			m_ec = ec;
			size_ = size;
			handle.resume();
		});
	}

private:
	Socket& m_socket;
	AsioBuffer m_buffer;

	boost::system::error_code m_ec{};
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
		: m_socket(socket)
		, m_buffer(buffer) {
	}

	bool await_ready() { return false; }
	auto await_resume() { return std::make_pair(m_ec, size_); }
	void await_suspend(std::coroutine_handle<> handle) {
		boost::asio::async_read(m_socket, m_buffer, [this, handle](auto ec, auto size) mutable {
			m_ec = ec;
			size_ = size;
			handle.resume();
		});
	}

private:
	Socket& m_socket;
	AsioBuffer& m_buffer;

	boost::system::error_code m_ec{};
	size_t size_{0};
};

template <typename Socket, typename AsioBuffer>
inline folly::coro::Task<std::pair<boost::system::error_code, size_t>> async_read(Socket& socket, AsioBuffer& buffer) noexcept {
	co_return co_await ReadAwaiter{socket, buffer};
}

template <typename Socket, typename AsioBuffer>
struct ReadUntilAwaiter {
public:
	ReadUntilAwaiter(Socket& socket, AsioBuffer& buffer, boost::asio::string_view delim)
		: m_socket(socket)
		, m_buffer(buffer)
		, delim_(delim) {
	}

	bool await_ready() { return false; }
	auto await_resume() { return std::make_pair(m_ec, size_); }
	void await_suspend(std::coroutine_handle<> handle) {
		boost::asio::async_read_until(m_socket, m_buffer, delim_, [this, handle](auto ec, auto size) mutable {
			m_ec = ec;
			size_ = size;
			handle.resume();
		});
	}

private:
	Socket& m_socket;
	AsioBuffer& m_buffer;
	boost::asio::string_view delim_;

	boost::system::error_code m_ec{};
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
		: m_socket(socket)
		, m_buffer(std::move(buffer)) {
	}

	bool await_ready() { return false; }
	auto await_resume() { return std::make_pair(m_ec, size_); }
	void await_suspend(std::coroutine_handle<> handle) {
		boost::asio::async_write(m_socket, std::move(m_buffer), [this, handle](auto ec, auto size) mutable {
			m_ec = ec;
			size_ = size;
			handle.resume();
		});
	}

private:
	Socket& m_socket;
	AsioBuffer m_buffer;

	boost::system::error_code m_ec{};
	size_t size_{0};
};

template <typename Socket, typename AsioBuffer>
inline folly::coro::Task<std::pair<boost::system::error_code, size_t>> async_write(Socket& socket, AsioBuffer&& buffer) noexcept {
	co_return co_await WriteAwaiter{socket, std::move(buffer)};
}

class ConnectAwaiter {
public:
	ConnectAwaiter(boost::asio::io_context& io_context, boost::asio::ip::tcp::socket& socket,
		const std::string& host, const std::string& port)
		: io_context_(io_context)
		, m_socket(socket)
		, host_(host)
		, port_(port) {
	}

	bool await_ready() const noexcept { return false; }
	void await_suspend(std::coroutine_handle<> handle) {
		boost::asio::ip::tcp::resolver resolver(io_context_);
		auto endpoints = resolver.resolve(host_, port_);
		boost::asio::async_connect(m_socket, endpoints, [this, handle](boost::system::error_code ec, boost::asio::ip::tcp::endpoint) mutable {
			m_ec = ec;
			handle.resume();
		});
	}
	auto await_resume() noexcept { return m_ec; }

private:
	boost::asio::io_context& io_context_;
	boost::asio::ip::tcp::socket& m_socket;
	std::string host_;
	std::string port_;
	boost::system::error_code m_ec{};
};

inline folly::coro::Task<boost::system::error_code> async_connect(boost::asio::io_context& io_context, boost::asio::ip::tcp::socket& socket,
	const std::string& host, const std::string& port) noexcept {
	co_return co_await ConnectAwaiter{io_context, socket, host, port};
}

class TimerAwaiter {
public:
	TimerAwaiter(boost::asio::steady_timer& m_steady_timer)
		: m_steady_timer(m_steady_timer) {
	}

	bool await_ready() const noexcept { return false; }
	void await_suspend(std::coroutine_handle<> handle) {
		m_steady_timer.async_wait([this, handle](const boost::system::error_code& ec) {
			m_ec = ec;
			handle.resume();
		});
	}
	auto await_resume() noexcept { return m_ec; }

private:
	boost::asio::steady_timer& m_steady_timer;
	boost::system::error_code m_ec{};
};

inline folly::coro::Task<boost::system::error_code> timeout(boost::asio::steady_timer& m_steady_timer) noexcept {
	co_return co_await TimerAwaiter{m_steady_timer};
}
