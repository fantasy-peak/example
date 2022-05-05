#include <nlohmann/json.hpp>

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

int main(int argc, char** argv) {
	try {
		Aws::SDKOptions options;
		options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Info;
		options.loggingOptions.logger_create_fn = [] {
			return Aws::MakeShared<Aws::Utils::Logging::ConsoleLogSystem>("console_logger", Aws::Utils::Logging::LogLevel::Info);
		};
		Aws::InitAPI(options);
		auto dynamo_client_ptr = createDynamoDBClient("us-east-1", "local", "local");
		AWS_LOGSTREAM_INFO("EXAMPLE", "start aws::lambda_runtime::run_handler");
		AWS_LOGSTREAM_FLUSH();
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
		Aws::ShutdownAPI(options);
	} catch (...) {
		AWS_LOGSTREAM_ERROR("nqf", "error");
	}
	return 0;
}
