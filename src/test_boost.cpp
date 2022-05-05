
#include <spdlog/spdlog.h>

#include <boost/math/constants/constants.hpp>
#include <boost/multiprecision/cpp_dec_float.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

int main(int argc, char** argv) {
	auto print = [](auto& p) {
		std::stringstream ss;
		ss << p;
		return ss.str();
	};
	boost::uuids::random_generator generator;
	boost::uuids::uuid uuid = generator();
	spdlog::info("uuid: {}", print(uuid));
	boost::multiprecision::cpp_dec_float_50 decimal = 0.45;
	spdlog::info("decimal: {}", print(decimal));
	return 0;
}
