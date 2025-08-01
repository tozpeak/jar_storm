# About

The project is started as a playground to learn C99.
Also to showcase my skills at learning and gained skills in C/C++.
C99 was chosen as an old standard, which is still in use. To better understand the core principles of modern C/C++ development I've decided to get a proper foundation. It's often easier to understand modern tools after working with older generation.

The main goal was to make a structurally complex C project. Visual appeal was not a priority though. Please, check the features and the code, and not just the looks.

# Disclaimer (!)

This project **DOES NOT** have a free license, thus it is **READ ONLY**. Feel free to learn from my experience, but do not copy it and definetely do not present it to someone else as your own.

# TODO

## Generic

- [x] Systems on fixed timers

- [ ] Core abilities:
    - [x] attacks
    - [ ] additional attacks and movement

- [x] Refactor attacks and spawners to make more generic code

## Deploy

- [x] other platforms than Linux
    - [x] windows build
        - [x] prototype
        - [x] **bugfix**
    - ~~web build~~
- [x] where
    - [x] github releases
    - ~~itch page~~
- [ ] better readme

## UI

- [ ] player hp
- [ ] cooldowns (abilities)
- [ ] items list
- [ ] damage numeric particles

## Item System

- [x] items (collect on collision)
- [x] basic item effects:
    - move speed,
    - attack speed,
- [x] damage multiplier (requires better spawner structure)
- [x] prototype custom item effect
- [x] interactible objects
- [x] chests
- [x] coins on kill
- [ ] (optimization) rewrite mushroom to single attack

### New Items and Effects

- [ ] (creative, low priority) add more items and effects to this list

## Enemy Behaviour

- [x] make structure for flow field calculation,
- [x] make components for more interesting enemy behaviour:
    - [x] attack abilities,
    - [x] targeted movement,
- [ ] FSM or behaviour trees

## Levels & Progression

- [x] where from:
    - [x] (prototype is ready) premade files
    - ~~generated on the go~~
- [x] limited view:
    - [x] (prototype is ready) level bigger than viewport with moving camera
    - ~~fog of war~~
    - ~~segmented level, like rooms in Binding of Isaac~~
- [x] teleporters to the next stage
- [ ] **victory state**
- [ ] boss enemies
- [ ] pits + jump/fly systems
    - [x] prototype
    - [x] cleanup code
    - [ ] **bugfix**

## Main Menu

- [x] how to play (input info)
- [ ] **select character**
- [ ] ? select input method
- [ ] ? highscores

## Physics

- [ ] collision point
- [x] collision depth
- [x] collision normal
- [ ] rigidbody system
    - [x] prototype (good enough?)

## ECS

- [ ] make **access for flag components** to use entity_store.flag_array (too free some space in bitmask for actual data-containing components)
- [ ] make more complex queries, for example with option to exclude components

---

# Backlog

## Generic

- [x] Split the main.c
- [x] Bullets have health
- [x] DealDamageOnCollision component:
    - target: self, other, both
    - ~~damage layer mask~~

- [x] Move player to ECS
- [x] Move input to ECS

- [x] Melee attacks
- [x] Implement raycast (for alternative bullets)

## Physics

- [x] Collision system to store collision data:
    - other collision entity

## ECS

- [x] make a way to have several query buffers for systems that benefit from cross-comparing entity sets (this may speed up enumerating only valid pairs)

# Known Issues

## Physics might be slow when too much entities

So far the solution is quite naive. Take all the entities with collision component and compare them all-to-all, checking layer mask first, then evaluating the collision.

I'm not sure, if thise early in the project is the time to change it. It can be changed later during the development, since making a perfect physics is not the primary goal of the project, while making the whole game is.

Possible solutions:

- Find a physics library, which can be integrated with current ECS approach. Ideally it would be a small enough library to read through, as an opportunity to learn from others people code.

- Make two buffers to query layers independently, then enumerate each layer entity against entities of other layers, depending on masks. This could be a major improvement, since bullet layers don't collide with self and each other, and bullets could be the most populated layers. Complexity of enumeration and comparisons would decrease from (B+C)^2 to B*C, where B = bullets and C = characters.

- Split the space into collision groups, evaluating position's hash while/after moving the objects. Since the space is enclosed and defined in size, it can be easily split into 64 chunks, using long int as a bitmask for each collision component.

## Drawing order

For now the entities are drawing in random order.
To get isometric view look nice, we would need an ability to draw the sprites in correct order.
Since it is a realtime game, sorting the entities or a drawing queue would be quite performance heavy.
Sadly, Raylib 5.5 does not allow us to use z-buffer in 2d mode, which could solve our problem by not overdrawing closer objects.
Best solution so far would be to switch to 2.5d mode, drawing sprites as 3d objects facing the camera. If the performance would be not good, the custom shader could be used to eliminate unnecessary transformations on gpu.

## AI Pathfinding

Pathfinding is implemented with caching distance map. With this structure Pathfinder only needs to check 8 neigboured tiles with own tile to decide where it should go.
While this structure is made in BFS filling in mind, currently it just fills empty tiles with circle distances, ignoring that obstacles can form complex path.
However, this solution may still work, since AI doesn't need to be too smart, it just should be intimidating for the player. Range attacks still force the player to move constantly, so it could be fine. Playtest is needed to be sure.
