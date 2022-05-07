
package("yaml_cpp_struct")
set_homepage("https://github.com/fantasy-peak/yaml_cpp_struct.git")
set_description("")
set_license("")

add_urls("https://github.com/fantasy-peak/yaml_cpp_struct.git")

add_deps("cmake")
add_deps("magic_enum", {system = false})
add_deps("visit_struct", {system = false})
add_deps("yaml-cpp", {system = false})

on_install("linux", function (package)
    os.cp("include", package:installdir())
end)
package_end()