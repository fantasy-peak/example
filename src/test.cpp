#include "greeter_async_client.h"
#include "greeter_async_server.h"

#include "xmake_example.h"

#include <drogon/drogon.h>
#include <drogon/utils/coroutine.h>
#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <trantor/net/EventLoopThreadPool.h>

#include <spdlog/spdlog.h>

#include <sw/redis++/async_redis++.h>

#include <nlohmann/json.hpp>

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
#include <folly/experimental/coro/SharedMutex.h>
#include <folly/experimental/coro/Sleep.h>
#include <folly/experimental/coro/Task.h>
#include <folly/experimental/coro/Timeout.h>
#include <folly/futures/ThreadWheelTimekeeper.h>

#include <aws/core/Aws.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/core/client/ClientConfiguration.h>
#include <aws/core/platform/Environment.h>
#include <aws/core/utils/Outcome.h>
#include <aws/core/utils/logging/ConsoleLogSystem.h>
#include <aws/core/utils/logging/LogLevel.h>
#include <aws/core/utils/logging/LogMacros.h>
#include <aws/core/utils/memory/stl/SimpleStringStream.h>
#include <aws/dynamodb/DynamoDBClient.h>
#include <aws/dynamodb/model/ListTablesRequest.h>
#include <aws/dynamodb/model/ListTablesResult.h>
#include <aws/dynamodb/model/QueryRequest.h>
#include <aws/lambda-runtime/runtime.h>

#include <boost/math/constants/constants.hpp>
#include <boost/multiprecision/cpp_dec_float.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <sqlpp11/mysql/mysql.h>
#include <sqlpp11/sqlpp11.h>
#include <TestSqlppTable.h>

using namespace std::chrono_literals;

void init(int argc, char** argv) {
	folly::init(&argc, &argv);
	folly::SingletonVault::singleton()->registrationComplete();
}

auto get_id() {
	std::stringstream ss;
	ss << std::this_thread::get_id();
	return ss.str();
}

std::string getEndpoint() {
	char* localstack_hostname_ptr = getenv("LOCALSTACK_HOSTNAME");
	if (localstack_hostname_ptr == nullptr)
		return std::string{"localhost:4566"};
	else {
		char buffer[128]{};
		sprintf(buffer, "%s:4566", localstack_hostname_ptr);
		return std::string{buffer};
	}
}

auto createDynamoDBClient(const std::string& region, const std::string& access_key_id, const std::string& secret_key) {
	Aws::Client::ClientConfiguration config;
	config.region = region;
	config.scheme = Aws::Http::Scheme::HTTP;
	config.endpointOverride = getEndpoint();
	Aws::Auth::AWSCredentials aws_credentials{access_key_id, secret_key};
	return std::make_unique<Aws::DynamoDB::DynamoDBClient>(aws_credentials, config);
}

std::function<std::shared_ptr<Aws::Utils::Logging::LogSystemInterface>()> GetConsoleLoggerFactory() {
	return [] {
		return Aws::MakeShared<Aws::Utils::Logging::ConsoleLogSystem>(
			"console_logger", Aws::Utils::Logging::LogLevel::Trace);
	};
}

int main(int argc, char** argv) {
	try {
		// ~/.xmake/packages/s/sqlpp11/0.61/954c070d3e81433da1f5ee354493f1fa/bin/sqlpp11-ddl2cpp ./test_sqlpp_table.sql TestSqlppTable TestProject
		// export LIBRARY_PATH="/usr/lib64/mysql:$LIBRARY_PATH"
		// export CPLUS_INCLUDE_PATH="/usr/include/mysql/:$CPLUS_INCLUDE_PATH"
		sqlpp::mysql::connection_config config;
		config.host = "127.0.0.1";
		config.user = "mybb";
		config.password = "changeme";
		config.database = "mybb";
		config.port = 3307;
		config.auto_reconnect = true;
		config.debug = false;
		auto config_ptr = std::make_shared<sqlpp::mysql::connection_config>(std::move(config));
		sqlpp::mysql::connection db(config_ptr);
		db.execute(R"(DROP TABLE IF EXISTS TestSqlppTable)");
		db.execute(R"(CREATE TABLE TestSqlppTable (name VARCHAR(100),age INTEGER))");
		// insert
		TestProject::TestSqlppTable stu{};
		db(sqlpp::insert_into(stu).set(stu.age = 12, stu.name = "one"));
		// 插入多行
		auto multi_insert = sqlpp::insert_into(stu).columns(stu.age, stu.name);
		for (int i = 0; i < 10; i++) {
			std::ostringstream ostr;
			ostr << "linyiong" << i;
			multi_insert.values.add(stu.age = i, stu.name = ostr.str());
		}
		db(multi_insert);
		// delete
		db(sqlpp::remove_from(stu).where(stu.age == 12));
		// select
		for (const auto& row : db(sqlpp::select(stu.name, stu.age).from(stu).where(stu.age > 1))) {
			std::cout << row.name << ":" << row.age << std::endl;
		}
		// update
		db(sqlpp::update(stu).set(stu.name = "linyilong3").where(stu.age>3));
	} catch (const sqlpp::exception& e) {
		spdlog::error("{}", e.what());
	}
	std::thread([] {
		ServerImpl server;
		server.Run();
	}).detach();
	std::thread([] {
		std::string target_str = "localhost:50051";
		GreeterClient greeter(
			grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));
		std::string user("world");
		std::string reply = greeter.SayHello(user);
		std::cout << "Greeter received: " << reply << std::endl;
	}).detach();
	boost::uuids::random_generator generator;
	boost::uuids::uuid uuid1 = generator();
	std::cout << uuid1 << std::endl;
	boost::multiprecision::cpp_dec_float_50 decimal = 0.45;
	std::cout << decimal << std::endl;
	try {
		Aws::SDKOptions options;
		options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Trace;
		options.loggingOptions.logger_create_fn = GetConsoleLoggerFactory();
		Aws::InitAPI(options);
		auto dynamo_client_ptr = createDynamoDBClient("us-east-1", "local", "local");
		aws::lambda_runtime::run_handler([&](aws::lambda_runtime::invocation_request const&) {
			Aws::DynamoDB::Model::ListTablesRequest list_tables_request;
			list_tables_request.SetLimit(50);
			nlohmann::json j;
			const auto& list_tables_outcome = dynamo_client_ptr->ListTables(list_tables_request);
			if (!list_tables_outcome.IsSuccess()) {
				j["statusCode"] = 500;
				return aws::lambda_runtime::invocation_response::failure(j.dump(), list_tables_outcome.GetError().GetMessage());
			}
			const auto& all_tables = list_tables_outcome.GetResult().GetTableNames();
			j["statusCode"] = 200;
			j["body"]["list_tables"] = all_tables;
			j["body"]["text"] = "cpp's list api";
			return aws::lambda_runtime::invocation_response::success(j.dump(), "application/json");
		});
	} catch (...) {
		spdlog::error("aws error");
	}
	init(argc, argv);
	spdlog::info("main thread id: {}", get_id());
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
	auto create_lazy_task = [&]() -> folly::coro::Task<void> {
		try {
			co_await task();
		} catch (const std::runtime_error& e) {
			fmt::print("{}\n", e.what());
		} catch (nlohmann::json::exception& e) {
			fmt::print("{}\n", e.what());
		}
	};
	folly::coro::blockingWait(create_lazy_task());
	spdlog::info("------------------start test folly---------------------------");
	folly::CPUThreadPoolExecutor thread_pool{4, std::make_shared<folly::NamedThreadFactory>("TestThreadPool")};
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
		auto ptr = folly::Singleton<folly::ThreadWheelTimekeeper>::try_get();
		co_await folly::coro::sleep(1s, ptr.get());
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

	folly::coro::blockingWait([]() -> folly::coro::Task<void> {
		auto [a, b, c] = co_await folly::coro::collectAllTry(
			[]() -> folly::coro::Task<int> {
				co_await folly::coro::sleep(1s);
				spdlog::info("task1 thread id: {}", get_id());
				co_return 42;
			}(),
			[]() -> folly::coro::Task<float> {
				co_await folly::coro::sleep(2s);
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

	auto thread_wheel_timekeeper_ptr = std::make_shared<folly::ThreadWheelTimekeeper>();
	auto test_timeout = [&]() -> folly::coro::Task<> {
		spdlog::info("test_timeout thread id: {}", get_id());
		auto make_task = []() -> folly::coro::Task<int> {
			co_await folly::coro::sleep(0s);
			spdlog::info("timeout return 42");
			co_return 42;
		};
		spdlog::info("start timeout");
		auto result = co_await folly::coro::co_awaitTry(folly::coro::timeout(make_task(), 1s, thread_wheel_timekeeper_ptr.get()));
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

	return 0;
}
