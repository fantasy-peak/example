package("folly")
    set_homepage("https://github.com/facebook/folly")
    set_description("An open-source C++ library developed and used at Facebook.")
    set_license("Apache-2.0")

    add_urls("https://github.com/facebook/folly/releases/download/v$(version).00/folly-v$(version).00.zip",
             "https://github.com/facebook/folly.git")
    add_versions("2021.06.28", "477765d43045d82ac6a2750142aed9534cd5efc1fbf2be622bb682a8c02a45a4")
    add_versions("2021.08.02", "03159657ef7a08c1207d90e63f02d4d6b1241dcae49f11a72441c0c269b269fa")
    add_versions("2021.11.01", "2620ad559f1e041f50328c91838cde666d422ed09f23b31bcdbf61e709da5c17")
    add_versions("2022.02.14", "6a50d4cc954f0f77efc85af231ee6b7f676a0d743c12b7080fb03fd3da3ffbf2")
    add_versions("2022.04.11", "a61cc06c622ba7c8fbae7edc1d134ffb8c7996e1e7fd2d272624172254a5eff1")

    -- add_patches("<=2022.04.11", path.join(os.scriptdir(), "patches", "2021.06.28", "reorder.patch"), "9a6bf283881580474040cfc7a8e89d461d68b89bae5583d89fff0a3198739980")
    -- add_patches("<=2022.04.11", path.join(os.scriptdir(), "patches", "2021.06.28", "regex.patch"), "6a77ade9f48dd9966d3f7154e66ca8a5c030ae2b6d335cbe3315784aefd8f495")

    if is_plat("windows") then
        add_configs("shared", {description = "Build shared library.", default = false, type = "boolean", readonly = true})
    end

    add_deps("cmake")
    add_deps("conan::libiberty/9.1.0", {alias = "libiberty"})
    add_deps("openssl 1.1.1n", {system=false})
    add_deps("boost 1.78.0", {system=false, configs = {context = true, filesystem = true, program_options = true, regex = true, system = true, thread = true}})
    add_deps("libevent 2.1.12", {system=false, configs = {openssl = true}})
    add_deps("gflags v2.2.2", {system=false})
    add_deps("glog v0.5.0", {system=false})
    add_deps("fmt 8.1.1", {system=false})
    add_deps("zlib v1.2.12", {system=false})
    add_deps("double-conversion v3.1.5" ,{system=false})
    add_deps("bzip2 1.0.8", {system=false})
    add_deps("lz4 v1.9.3", {system=false})
    add_deps("zstd v1.5.2", {optional = true}, {system=false})

    if is_plat("linux") then
        add_syslinks("pthread")
    end

    on_install("windows|x64", "linux", "macosx", function (package)
        local configs = {"-DBUILD_TESTS=OFF",
                         "-DCMAKE_DISABLE_FIND_PACKAGE_LibDwarf=ON",
                         "-DCMAKE_DISABLE_FIND_PACKAGE_Libiberty=OFF",
                         "-DCMAKE_DISABLE_FIND_PACKAGE_LibAIO=ON",
                         "-DCMAKE_DISABLE_FIND_PACKAGE_LibURCU=ON",
                         "-DLIBAIO_FOUND=OFF",
                         "-DLIBURCU_FOUND=OFF",
                         "-DBOOST_LINK_STATIC=ON"}
        table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
        table.insert(configs, "-DBUILD_SHARED_LIBS=" .. (package:config("shared") and "ON" or "OFF"))
        if package:is_plat("windows") then
            table.insert(configs, "-DBoost_USE_STATIC_RUNTIME=" .. (package:config("vs_runtime"):startswith("MT") and "ON" or "OFF"))
        end
        import("package.tools.cmake").install(package, configs)
    end)

    on_test(function (package)
        assert(package:check_cxxsnippets({test = [[
            #include <cassert>
            void test() {
                folly::CpuId id;
                assert(folly::kIsArchAmd64 == id.mmx());
            }
        ]]}, {configs = {languages = "c++17"}, includes = "folly/CpuId.h"}))
    end)
package_end()
