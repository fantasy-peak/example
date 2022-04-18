package("aws-lambda-cpp")

set_homepage("https://github.com/awslabs/aws-lambda-cpp")
set_description("C++ implementation of the AWS Lambda runtime")

add_urls("https://github.com/awslabs/aws-lambda-cpp.git")

on_install("linux", function (package)
    local configs = {}
    table.insert(configs, "-DCMAKE_BUILD_TYPE=Release")
    import("package.tools.cmake").install(package, configs)
end)

package_end()