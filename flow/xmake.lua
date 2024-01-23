
target("flow_ecs")
    set_default(false)
    set_kind("static")
    add_files("*.cpp")
    add_includedirs(".", {public = true})
    add_deps("city_hash")
target_end()
