
#include "Query.hpp"
#include <ECS.hpp>
#include <iostream>

using namespace Flow;

struct PositionComponent {
    double x { 0.0F };
    double y { 0.0F };
};

struct VelocityComponent {
    double x { 1.0 };
    double y { 1.0 };
};

// using AllFilter = All<PositionComponent, VelocityComponent>;
// using AnyFilter = Any<>;
// using NoneFilter = None<>;
// using WriteFilter = Write<PositionComponent>;
// using UpdateVelocityQuery = Query<AllFilter, AnyFilter, NoneFilter, WriteFilter>;
using UpdateVelocityQuery = Query<All<PositionComponent, VelocityComponent>, Write<PositionComponent>>;

struct TestSystem : public SystemBase<UpdateVelocityQuery> {
    void Update(UpdateVelocityQuery& q)
    {
        const float dt = 1.0;
        ECS::ForEach(q, [=](EntityID entity, PositionComponent& pos, const VelocityComponent& v) {
            pos.x += v.x * dt;
            pos.y += v.y * dt;
            std::cout << "entity " << entity.value << " pos = [" << pos.x << ", " << pos.y << "] "
                      << " velocity = [" << v.x << ", " << v.y << "]\n";
        });
    }
};

int main()
{
    World world {};

    EntityID e1 = world.CreateEntity();
    EntityID e2 = world.CreateEntity();
    EntityID e3 = world.CreateEntity();
    Archetype& archetype1 = ECS::CreateArchetypeBuilder(world).AddComponent<PositionComponent>().BuildArchetype();
    archetype1.SetArchetype(e1).SetComponentData(PositionComponent { 100, 100 });
    archetype1.SetArchetype(e2).SetComponentData(PositionComponent { 200, 100 });
    archetype1.SetArchetype(e3).SetComponentData(PositionComponent { 300, 100 });

    Archetype& archetype2 = ECS::CreateArchetypeBuilder(world).AddComponent<VelocityComponent, PositionComponent>().BuildArchetype();
    archetype2.SetArchetype(e1).SetComponentData(PositionComponent { 400, 100 });
    archetype2.SetArchetype(e2).SetComponentData(PositionComponent { 500, 100 }, VelocityComponent { 5, 5 });
    archetype2.SetArchetype(e3).SetComponentData(PositionComponent { 600, 100 });

    world.RegisterSystem<TestSystem>();

    ECS::GetEntityLocation(world, e1).SetComponentData(VelocityComponent { 233, 233 });

    world.UpdateWorld();
    world.UpdateWorld();
    world.UpdateWorld();

    return 0;
}
