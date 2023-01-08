#include <drogon/HttpAppFramework.h>
#include <drogon/HttpController.h>
#include <drogon/PubSubService.h>
#include <drogon/WebSocketClient.h>
#include <drogon/WebSocketController.h>
#include <drogon/drogon.h>
#include <drogon/utils/coroutine.h>

#include <spdlog/spdlog.h>
#include <trantor/net/EventLoopThreadPool.h>

#include <folly/executors/CPUThreadPoolExecutor.h>
#include <folly/experimental/coro/AsyncScope.h>
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

class SayHello : public drogon::HttpController<SayHello> {
public:
	METHOD_LIST_BEGIN
	// Drogon automatically appends the namespace and name of the controller to
	// the handlers of the controller. In this example, although we are adding
	// a handler to /. But because it is a part of the SayHello controller. It
	// ends up in path /SayHello/ (IMPORTANT! It is /SayHello/ not /SayHello
	// as they are different paths).
	METHOD_ADD(SayHello::genericHello, "/", drogon::Get);
	// Same for /hello. It ends up at /SayHello/hello
	METHOD_ADD(SayHello::personalizedHello, "/hello", drogon::Get);
	METHOD_LIST_END

private:
	drogon::Task<drogon::HttpResponsePtr> genericHello(drogon::HttpRequestPtr) {
		auto resp = drogon::HttpResponse::newHttpResponse();
		resp->setBody("Hello, this is a generic hello message from the SayHello controller");
		co_return resp;
	}

	drogon::Task<drogon::HttpResponsePtr> personalizedHello(drogon::HttpRequestPtr) {
		auto resp = drogon::HttpResponse::newHttpResponse();
		resp->setBody("Hi there, this is another hello from the SayHello Controller");
		co_return resp;
	}
};

class WebSocketChat : public drogon::WebSocketController<WebSocketChat> {
public:
	WS_PATH_LIST_BEGIN
	WS_PATH_ADD("/chat", drogon::Get);
	WS_PATH_LIST_END

	virtual void handleNewMessage(const drogon::WebSocketConnectionPtr& wsConnPtr, std::string&& message, const drogon::WebSocketMessageType& type) override {
		if (type == drogon::WebSocketMessageType::Ping)
			spdlog::info("WebSocketChat recv a ping");
		else if (type == drogon::WebSocketMessageType::Text)
			spdlog::info("WebSocketChat recv message: {}", message);
	}

	virtual void handleConnectionClosed(const drogon::WebSocketConnectionPtr&) override {
		spdlog::info("WebSocketChat websocket closed!");
	}

	virtual void handleNewConnection(const drogon::HttpRequestPtr&, const drogon::WebSocketConnectionPtr& conn) override {
		conn->send("haha!!!");
	}
};

// valgrind --log-file=a.txt --tool=memcheck --leak-check=full -s ./test_drogon
int main(int argc, char** argv) {
	std::thread([] { drogon::app().addListener("127.0.0.1", 8848).setThreadNum(4).run(); }).detach();
	std::this_thread::sleep_for(std::chrono::seconds(2));
	spdlog::info("------------------start test drogon---------------------------");
	// curl http://127.0.0.1:8848/SayHello/
	trantor::EventLoopThreadPool event_loop_thread_pool{std::thread::hardware_concurrency()};
	event_loop_thread_pool.start();
	std::vector<drogon::HttpClientPtr> http_clients;
	for (uint32_t i = 0; i < std::thread::hardware_concurrency(); i++)
		http_clients.emplace_back(drogon::HttpClient::newHttpClient("http://127.0.0.1:8848", event_loop_thread_pool.getNextLoop()));
	auto get_http_client = [&]() -> auto& {
		static std::mutex tx;
		static uint32_t count = 0;
		std::lock_guard<std::mutex> lk(tx);
		if (count == std::thread::hardware_concurrency())
			count = 0;
		return http_clients[count++];
	};
	auto ws_ptr = drogon::WebSocketClient::newWebSocketClient("ws://127.0.0.1:8848", event_loop_thread_pool.getNextLoop());
	auto req = drogon::HttpRequest::newHttpRequest();
	req->setPath("/chat");
	ws_ptr->setMessageHandler([](const std::string& message, const drogon::WebSocketClientPtr& cli, const drogon::WebSocketMessageType& type) {
		std::string message_type = "Unknown";
		if (type == drogon::WebSocketMessageType::Text)
			message_type = "text";
		else if (type == drogon::WebSocketMessageType::Pong)
			message_type = "pong";
		else if (type == drogon::WebSocketMessageType::Ping)
			message_type = "ping";
		else if (type == drogon::WebSocketMessageType::Binary)
			message_type = "binary";
		else if (type == drogon::WebSocketMessageType::Close)
			message_type = "Close";
		spdlog::info("WebSocketClient recv: [{}] [{}]", message_type, message);
		cli->getConnection()->shutdown();
	});
	ws_ptr->setConnectionClosedHandler([](const drogon::WebSocketClientPtr&) {
		spdlog::info("WebSocket connection closed!");
	});
	ws_ptr->connectToServer(req, [](drogon::ReqResult r, const drogon::HttpResponsePtr&, const drogon::WebSocketClientPtr& ws_ptr) {
		if (r != drogon::ReqResult::Ok) {
			spdlog::info("Failed to establish WebSocket connection!");
			ws_ptr->stop();
			return;
		}
		spdlog::info("WebSocket connected!");
		ws_ptr->getConnection()->setPingMessage("123", std::chrono::seconds(1));
		ws_ptr->getConnection()->send("hello!");
	});
	std::this_thread::sleep_for(std::chrono::seconds(2));
	auto task = [&]() -> drogon::Task<void> {
		spdlog::info("send http request...");
		auto http_request_ptr = drogon::HttpRequest::newHttpRequest();
		http_request_ptr->setPath("/SayHello/");
		auto query_resp = co_await get_http_client()->sendRequestCoro(std::move(http_request_ptr));
		spdlog::info("{}", query_resp->body());
		co_return;
	};
	auto create_lazy_task = [&]() -> folly::coro::Task<void> {
		try {
			co_await task();
		} catch (const std::runtime_error& e) {
			std::printf("%s\n", e.what());
		}
	};
	folly::coro::blockingWait(create_lazy_task());
	drogon::app().quit();
	spdlog::info("-----------------------");
	{
		trantor::EventLoopThreadPool thread_pool{1};
		thread_pool.start();
		auto loop_ptr = thread_pool.getNextLoop();
		loop_ptr->runInLoop([] {
			std::cout << std::this_thread::get_id() << std::endl;
		});
		std::this_thread::sleep_for(std::chrono::seconds(1));
		loop_ptr->queueInLoop([]() {
			std::cout << "queueInLoop:" << std::this_thread::get_id() << std::endl;
			drogon::async_run([]() -> drogon::Task<> {
				std::cout << "queueInLoop async_run:" << std::this_thread::get_id() << std::endl;
				co_return;
			});
		});
		std::this_thread::sleep_for(std::chrono::seconds(5));
		loop_ptr->quit();
		std::cout << "end" << std::endl;
	}
	return 0;
}
