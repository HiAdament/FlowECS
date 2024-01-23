
target("play_ground")
    set_default(true)
    set_kind("binary")
    add_files("*.cpp")
    add_packages("local_entt")
    add_deps("city_hash")
    add_deps("flow_ecs")

    
