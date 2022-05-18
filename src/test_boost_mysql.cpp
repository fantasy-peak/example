#include <algorithm>
#include <iostream>
#include <numeric>
#include <thread>
#include <utility>

#include <folly/experimental/coro/BlockingWait.h>
#include <folly/experimental/coro/Task.h>
#include <spdlog/spdlog.h>
#include <boost/mysql.hpp>

#include "io_context_pool.h"

class ConnectAwaiter {
public:
	ConnectAwaiter(boost::mysql::tcp_connection& conn, boost::asio::ip::tcp::resolver::results_type& ep, boost::mysql::connection_params& conn_params)
		: m_conn(conn)
		, m_ep(ep)
		, m_conn_params(conn_params) {
	}

	bool await_ready() const noexcept { return false; }
	void await_suspend(std::coroutine_handle<> handle) {
		m_conn.async_connect(*m_ep.begin(), m_conn_params, m_additional_info, [this, handle](boost::system::error_code ec) {
			m_ec = std::move(ec);
			handle.resume();
		});
	}
	auto await_resume() noexcept { return m_ec; }

private:
	boost::mysql::tcp_connection& m_conn;
	boost::asio::ip::tcp::resolver::results_type& m_ep;
	boost::mysql::connection_params& m_conn_params;
	boost::system::error_code m_ec{};
	boost::mysql::error_info m_additional_info;
};

inline folly::coro::Task<boost::system::error_code> async_connect(boost::mysql::tcp_connection& conn,
	boost::asio::ip::tcp::resolver::results_type& ep, boost::mysql::connection_params& conn_params) noexcept {
	co_return co_await ConnectAwaiter{conn, ep, conn_params};
}

class ResolveAwaiter {
public:
	ResolveAwaiter(boost::asio::io_context& io_context, const std::string& host, const std::string& port)
		: io_context_(io_context)
		, m_resolver(io_context.get_executor())
		, m_host(host)
		, m_port(port) {
	}

	bool await_ready() const noexcept { return false; }
	void await_suspend(std::coroutine_handle<> handle) {
		m_resolver.async_resolve(m_host.c_str(), m_port.c_str(),
			[this, handle](boost::system::error_code ec, boost::asio::ip::tcp::resolver::results_type results) {
				m_ec = std::move(ec);
				m_results_type = std::move(results);
				handle.resume();
			});
	}
	auto await_resume() noexcept {
		return std::make_pair(std::move(m_ec), std::move(m_results_type));
	}

private:
	boost::asio::io_context& io_context_;
	boost::asio::ip::tcp::resolver m_resolver;
	std::string m_host;
	std::string m_port;
	boost::system::error_code m_ec{};
	boost::asio::ip::tcp::resolver::results_type m_results_type;
};

inline folly::coro::Task<std::pair<boost::system::error_code, boost::asio::ip::tcp::resolver::results_type>> async_resolve(boost::asio::io_context& io_context,
	const std::string& host, const std::string& port) noexcept {
	co_return co_await ResolveAwaiter{io_context, host, port};
}

class QueryAwaiter {
public:
	QueryAwaiter(boost::mysql::tcp_connection& tcp_connection, const std::string& sql)
		: m_tcp_connection(tcp_connection)
		, m_sql(sql) {
	}

	bool await_ready() const noexcept { return false; }
	void await_suspend(std::coroutine_handle<> handle) {
		m_tcp_connection.async_query(m_sql.c_str(), m_additional_info, [this, handle](boost::system::error_code ec, auto&& result) {
			m_ec = std::move(ec);
			m_resultset = std::move(result);
			handle.resume();
		});
	}
	auto await_resume() noexcept {
		return std::make_tuple(std::move(m_ec), std::move(m_additional_info), std::move(m_resultset));
	}

private:
	boost::mysql::tcp_connection& m_tcp_connection;
	std::string m_sql;
	boost::system::error_code m_ec{};
	boost::mysql::error_info m_additional_info;
	boost::mysql::resultset<boost::asio::ip::tcp::socket> m_resultset;
};

inline folly::coro::Task<std::tuple<boost::system::error_code, boost::mysql::error_info, boost::mysql::resultset<boost::asio::ip::tcp::socket>>> async_query(
	boost::mysql::tcp_connection& tcp_connection, const std::string& sql) noexcept {
	co_return co_await QueryAwaiter{tcp_connection, sql};
}

class ReadAllAwaiter {
public:
	ReadAllAwaiter(boost::mysql::resultset<boost::asio::ip::tcp::socket>& resultset)
		: m_resultset(resultset) {
	}

	bool await_ready() const noexcept { return false; }
	void await_suspend(std::coroutine_handle<> handle) {
		m_resultset.async_read_all(m_additional_info, [this, handle](boost::system::error_code ec, std::vector<boost::mysql::row>&& rows) {
			m_rows = std::move(rows);
			m_ec = std::move(ec);
			handle.resume();
		});
	}
	auto await_resume() noexcept {
		return std::make_tuple(std::move(m_ec), std::move(m_additional_info), std::move(m_rows));
	}

private:
	boost::mysql::resultset<boost::asio::ip::tcp::socket>& m_resultset;
	boost::system::error_code m_ec{};
	boost::mysql::error_info m_additional_info{};
	std::vector<boost::mysql::row> m_rows;
};

inline folly::coro::Task<std::tuple<boost::system::error_code, boost::mysql::error_info, std::vector<boost::mysql::row>>> read_all(
	boost::mysql::resultset<boost::asio::ip::tcp::socket>& resultset) noexcept {
	co_return co_await ReadAllAwaiter{resultset};
}

class PrepareStatementAwaiter {
public:
	PrepareStatementAwaiter(boost::mysql::tcp_connection& tcp_connection, const std::string& sql)
		: m_tcp_connection(tcp_connection)
		, m_sql(sql) {
	}

	bool await_ready() const noexcept { return false; }
	void await_suspend(std::coroutine_handle<> handle) {
		m_tcp_connection.async_prepare_statement(m_sql.c_str(), m_additional_info, [this, handle](boost::system::error_code ec, auto&& result) {
			m_ec = std::move(ec);
			m_prepared_statement = std::move(result);
			handle.resume();
		});
	}
	auto await_resume() noexcept {
		return std::make_tuple(std::move(m_ec), std::move(m_additional_info), std::move(m_prepared_statement));
	}

private:
	boost::mysql::tcp_connection& m_tcp_connection;
	std::string m_sql;
	boost::system::error_code m_ec{};
	boost::mysql::error_info m_additional_info{};
	boost::mysql::prepared_statement<boost::asio::ip::tcp::socket> m_prepared_statement;
};

inline folly::coro::Task<std::tuple<boost::system::error_code, boost::mysql::error_info, boost::mysql::prepared_statement<boost::asio::ip::tcp::socket>>> async_prepare_statement(
	boost::mysql::tcp_connection& tcp_connection, const std::string& sql) noexcept {
	co_return co_await PrepareStatementAwaiter{tcp_connection, sql};
}

class AsyncExecuteAwaiter {
public:
	AsyncExecuteAwaiter(boost::mysql::prepared_statement<boost::asio::ip::tcp::socket>& stmt, std::vector<boost::mysql::value> params)
		: m_stmt(stmt)
		, m_params(std::move(params)) {
	}

	bool await_ready() const noexcept { return false; }
	void await_suspend(std::coroutine_handle<> handle) {
		m_stmt.async_execute(m_params, m_additional_info, [this, handle](boost::system::error_code ec, auto&& result) {
			m_ec = std::move(ec);
			m_resultset = std::move(result);
			handle.resume();
		});
	}
	auto await_resume() noexcept {
		return std::make_pair(std::move(m_ec), std::move(m_resultset));
	}

private:
	boost::mysql::prepared_statement<boost::asio::ip::tcp::socket>& m_stmt;
	std::vector<boost::mysql::value> m_params;

	boost::system::error_code m_ec{};
	boost::mysql::error_info m_additional_info{};
	boost::mysql::resultset<boost::asio::ip::tcp::socket> m_resultset;
};

inline folly::coro::Task<std::pair<boost::system::error_code, boost::mysql::resultset<boost::asio::ip::tcp::socket>>> async_execute(
	boost::mysql::prepared_statement<boost::asio::ip::tcp::socket>& stmt, std::vector<boost::mysql::value> params) noexcept {
	co_return co_await AsyncExecuteAwaiter{stmt, std::move(params)};
}

class AsyncCloseAwaiter {
public:
	AsyncCloseAwaiter(boost::mysql::tcp_connection& tcp_connection)
		: m_tcp_connection(tcp_connection) {
	}

	bool await_ready() const noexcept { return false; }
	void await_suspend(std::coroutine_handle<> handle) {
		m_tcp_connection.async_close(m_additional_info, [this, handle](boost::system::error_code ec) {
			m_ec = std::move(ec);
			handle.resume();
		});
	}
	auto await_resume() noexcept {
		return std::make_tuple(std::move(m_ec), std::move(m_additional_info));
	}

private:
	boost::mysql::tcp_connection& m_tcp_connection;
	boost::system::error_code m_ec{};
	boost::mysql::error_info m_additional_info{};
};

inline folly::coro::Task<std::tuple<boost::system::error_code, boost::mysql::error_info>> async_close(
	boost::mysql::tcp_connection& tcp_connection) noexcept {
	co_return co_await AsyncCloseAwaiter{tcp_connection};
}

int main() {
	IoContextPool ctx_pool{8};
	ctx_pool.start();
	folly::coro::blockingWait([&]() -> folly::coro::Task<void> {
		auto& ctx = ctx_pool.getIoContext();
		boost::mysql::tcp_connection conn{ctx};
		boost::mysql::connection_params params("mybb", "changeme", "mybb");
		std::string create_table_sql{"create table mysql_db_test(pk1 varchar(36) NOT NULL PRIMARY KEY, pk2 varchar(36), column1 varchar(32), column2 varchar(32), column3 varchar(32), column4 varchar(32))"};
		std::string insert_sql{"insert into mysql_db_test VALUES (?, 'pk2', 'test-column1', 'test-column2', 'test-column3', 'test-column4');"};
		std::string select_sql{"select * from mysql_db_test where pk1 = ?;"};
		{
			auto [ec, ep] = co_await async_resolve(ctx, "127.0.0.1", "3309");
			if (ec) {
				spdlog::error("async_resolve: {}", ec.message());
				co_return;
			}
			auto ec_ = co_await async_connect(conn, ep, params);
			if (ec_) {
				spdlog::error("async_connect: {}", ec_.message());
				co_return;
			}
		}
		{
			co_await async_query(conn, "DROP TABLE IF EXISTS mysql_db_test");
			auto [ec, ei, _] = co_await async_query(conn, create_table_sql);
			if (ec)
				spdlog::error("{}-{}", ec.message(), ei.message());
		}
		{
			auto [pre_insert_ec, pre_insert_ei, stm] = co_await async_prepare_statement(conn, insert_sql);
			if (pre_insert_ec) {
				spdlog::error("async_prepare_statement : {}-{}", pre_insert_ec.message(), pre_insert_ei.message());
				co_return;
			}
			std::vector<boost::mysql::value> value{boost::mysql::value{"10086"}};
			auto [insert_ec, _] = co_await async_execute(stm, std::move(value));
			if (insert_ec) {
				spdlog::error("async_execute : {}", insert_ec.message());
				co_return;
			}
		}
		{
			auto [ec, ei, stm] = co_await async_prepare_statement(conn, select_sql);
			if (ec) {
				spdlog::error("async_prepare_statement : {}-{}", ec.message(), ei.message());
				co_return;
			}
			std::vector<boost::mysql::value> value{boost::mysql::value{"10086"}};
			auto [select_ec, result] = co_await async_execute(stm, std::move(value));
			if (select_ec) {
				spdlog::error("async_execute : {}", ec.message());
				co_return;
			}
			auto [ec_, ei_, rows] = co_await read_all(result);
			if (ec_) {
				spdlog::error("read_all : {}-{}", ec_.message(), ei.message());
				co_return;
			}
			if (!result.complete()) {
				spdlog::error("read result incomplete");
				exit(1);
			}
			if (!rows.empty()) {
				std::stringstream ss;
				ss << rows[0];
				spdlog::info("{}", ss.str());
			}
		}
	}());
	ctx_pool.stop();
	return 0;
}
