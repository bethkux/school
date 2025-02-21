
#include <iostream>
#include <algorithm>
#include <memory>
#include <vector>
#include <string>
#include <typeinfo>
#include <sstream>
#include <map>
#include <list>
#include <set>
#include <array>

#pragma region Overloaded operators

template <typename T>
using Vec2 = std::array<T, 2>;
using Vec2i = Vec2<int>;

template<typename T, size_t N>
std::array<T, N>& operator+=(std::array<T, N>& first, const std::array<T, N>& second) {
    for (size_t i = 0; i < N; ++i)
        first.at(i) += second.at(i);
    return first;
}

template<typename T, size_t N>
std::array<T, N> operator+(const std::array<T, N>& first, const std::array<T, N>& second) {
    std::array<T, N> sum = first;
    sum += second;
    return sum;
}

template<typename T, size_t N>
std::array<T, N> operator*=(std::array<T, N>& first, const int second) {
    for (size_t i = 0; i < N; ++i)
        first.at(i) *= second;
    return first;
}

template<typename T, size_t N>
std::array<T, N> operator*(const std::array<T, N>& first, const int second) {
    std::array<T, N> product = first;
    product *= second;
    return product;
}

template<typename A, typename B>
std::pair<B, A> flip_pair(const std::pair<A, B>& p) {
    return std::pair<B, A>(p.second, p.first);
}

template<typename A, typename B>
std::multimap<B, A> flip_map(const std::map<A, B>& src) {
    std::multimap<B, A> dst;
    std::transform(src.begin(), src.end(), std::inserter(dst, dst.begin()), flip_pair<A, B>);
    return dst;
}

#pragma endregion


class Unit;

#pragma region Effects

class Effect {
public:
    Effect(std::string_view type, int timer) : _type(type), _timer(timer) {}

    virtual ~Effect() noexcept = default;
    virtual std::unique_ptr<Effect> clone() const = 0;

    //Effect(const Effect& unit) = default;
    //Effect& operator=(const Effect& unit) = default;
    //Effect(Effect&& other) = default;
    //Effect& operator=(Effect&& other) = default;

    virtual void apply(std::unique_ptr<Unit>& p_unit, int time_diff) = 0;

    std::string_view type() const {
        return _type;
    }

    int timer() const {
        return _timer;
    }

    bool is_done() const {
        return _timer <= 0;
    }


protected:
    void decrement_timer(int factor = 1) {
        _timer -= factor;
    }


private:
    std::string _type;
    int _timer;
};


class DeadInside : public Effect {
public:
    DeadInside(int dmg = 1) : Effect("dead_inside", INT_MAX), _dmg(dmg) {}

    virtual std::unique_ptr<Effect> clone() const override {
        return std::make_unique<DeadInside>(*this);
    }

    void apply(std::unique_ptr<Unit>& p_unit, int time_diff);


private:
    int _dmg;
};

#pragma endregion

#pragma region Units

class Unit {
public:
#pragma region Constructor + Destructor

    Unit(
        std::string_view id, std::string_view type, Vec2i coor, int hp, int max_move, int max_step,
        int attack_dmg, int attack_distance, int attack_range,
        std::map<std::string, Vec2i> move_directions = { {"up", {-1,0}}, {"down", {1,0}}, {"left", {0,-1}}, {"right", {0,1}} },
        std::map<std::string, Vec2i> attack_directions = { {"up", {-1,0}}, {"down", {1,0}}, {"left", {0,-1}}, {"right", {0,1}} },
        std::vector<std::unique_ptr<Effect>> effects = {},
        std::map<std::string, int> max_active = { {"dead_inside", 1} }
    ) :
        _id(id), _type(type), _coor(coor), _hp(hp), _max_hp(hp), _max_move(max_move), _max_step(max_step),
        _attack_dmg(attack_dmg), _attack_distance(attack_distance), _attack_range(attack_range),
        _move_directions(std::move(move_directions)),
        _attack_directions(std::move(attack_directions)),
        _active_effects(std::move(effects)),
        _max_active(max_active)
    {}

    virtual ~Unit() noexcept = default;
    virtual std::unique_ptr<Unit> clone() const = 0;
    //Unit(const Unit& unit) = default;
    //Unit& operator=(const Unit& unit) = default;
    //Unit(Unit&& other) = default;
    //Unit& operator=(Unit&& other) = default;


#pragma endregion

#pragma region Observing

    std::string_view id() const {
        return _id;
    };

    std::string_view type() const {
        return _type;
    };

    Vec2i coor() const {
        return _coor;
    };

    int hp() const {
        return _hp;
    };

    int max_move() const {
        return _max_move;
    }

    int max_step() const {
        return _max_step;
    }

    int attack_dmg() const {
        return _attack_dmg;
    }

    int attack_distance() const {
        return _attack_distance;
    }

    int attack_range() const {
        return _attack_range;
    }

    std::map<std::string, Vec2i> const& move_directions() const {
        return _move_directions;
    }

    std::map<std::string, Vec2i> const& attack_directions() const {
        return _attack_directions;
    }

    std::vector<std::unique_ptr<Effect>> const& effects() const {
        return _active_effects;
    }

#pragma endregion

    void set_coor(Vec2i new_coor) {
        _coor = new_coor;
    };

    void set_hp(int new_hp) {
        _hp = new_hp;
    }

    void take_damage(int dmg) {
        set_hp(_hp - dmg);
    }

    void take_hp(int heal) {
        set_hp(std::min(_hp + heal, _max_hp));
    }

    bool is_effect_full(std::string type) {
        return active_effect_count(type) >= max_effect_count(type);
    }

    void add_effect(std::unique_ptr<Effect> effect) {
        _active_effects.push_back(std::move(effect));
    }

    void remove_overdue_effects() {
        std::erase_if(_active_effects, [&](std::unique_ptr<Effect>& effect) { return effect->is_done(); });
    }


protected:
    int active_effect_count(std::string type) {
        int sum = 0;
        for (auto& p_effect : _active_effects) {
            if (p_effect->type() == type)
                ++sum;
        }

        if (max_effect_count(type) < sum)
            throw std::runtime_error("Overflow of active effects of type " + type);

        return sum;
    }

    int max_effect_count(std::string type) {
        return _max_active.at(type);
    }

#pragma region Fields

    std::string _id;         // Name of the unit
    std::string _type;       // Type of unit
    Vec2i _coor;             // Location of the unit
    int _hp;                 // Health points
    int _max_hp;             // Max health points
    int _max_move;           // Size of the unit's move
    int _max_step;           // How high / low can the unit move
    int _attack_dmg;         // Damage delt to the enemy
    int _attack_distance;    // Distance at which the unit can attack an enemy
    int _attack_range;       // How many floors higher/lower can the unit attack an enemy 
    std::map<std::string, Vec2i> _move_directions;          // Directions to move
    std::map<std::string, Vec2i> _attack_directions;        // Directions to attack
    std::vector<std::unique_ptr<Effect>> _active_effects;   // Currently active effects
    std::map<std::string, int> _max_active;                 // Map of all effects and the amount of effects one unit can have (1 = non-stackable)

#pragma endregion
};

#pragma region Unit subclasses
class Footman : public Unit {
public:
    Footman(std::string_view id, Vec2i coor) : Unit(id, "footman", coor, 20, 1, 1, 1, 1, 0) {}

    virtual std::unique_ptr<Unit> clone() const override {
        std::unique_ptr<Footman> p_u = std::make_unique<Footman>(_id, _coor);
        for (const auto& p_e : _active_effects) {
            p_u->add_effect(p_e->clone());
        }
        p_u->_hp = _hp;
        p_u->_max_hp = _max_hp;
        p_u->_max_move = _max_move;
        p_u->_max_step = _max_step;
        p_u->_attack_dmg = _attack_dmg;
        p_u->_attack_distance = _attack_distance;
        p_u->_attack_range = _attack_range;
        p_u->_move_directions = _move_directions;
        p_u->_attack_directions = _attack_directions;
        p_u->_max_active = _max_active;
        return std::move(p_u);
    }
};

class Knight : public Unit {
public:
    Knight(std::string_view id, Vec2i coor) : Unit(id, "knight", coor, 50, 5, 1, 5, 1, 1) {}

    virtual std::unique_ptr<Unit> clone() const override {
        std::unique_ptr<Knight> p_u = std::make_unique<Knight>(_id, _coor);
        for (const auto& p_e : _active_effects) {
            p_u->add_effect(p_e->clone());
        }
        p_u->_hp = _hp;
        p_u->_max_hp = _max_hp;
        p_u->_max_move = _max_move;
        p_u->_max_step = _max_step;
        p_u->_attack_dmg = _attack_dmg;
        p_u->_attack_distance = _attack_distance;
        p_u->_attack_range = _attack_range;
        p_u->_move_directions = _move_directions;
        p_u->_attack_directions = _attack_directions;
        p_u->_max_active = _max_active;
        return std::move(p_u);
    }
};

class Rifleman : public Unit {
public:
    Rifleman(std::string_view id, Vec2i coor) : Unit(id, "rifleman", coor, 10, 2, 2, 3, INT_MAX, INT_MAX) {}

    virtual std::unique_ptr<Unit> clone() const override {
        std::unique_ptr<Rifleman> p_u = std::make_unique<Rifleman>(_id, _coor);
        for (const auto& p_e : _active_effects) {
            p_u->add_effect(p_e->clone());
        }
        p_u->_hp = _hp;
        p_u->_max_hp = _max_hp;
        p_u->_max_move = _max_move;
        p_u->_max_step = _max_step;
        p_u->_attack_dmg = _attack_dmg;
        p_u->_attack_distance = _attack_distance;
        p_u->_attack_range = _attack_range;
        p_u->_move_directions = _move_directions;
        p_u->_attack_directions = _attack_directions;
        p_u->_max_active = _max_active;
        return std::move(p_u);
    }
};

#pragma endregion

#pragma endregion

#pragma region Effects apply definitions

void DeadInside::apply(std::unique_ptr<Unit>& p_unit, int time_diff) {
    p_unit->take_damage(_dmg * time_diff);
    decrement_timer(time_diff);
}

#pragma endregion


#pragma region Tile + Battlefield

/* Tile of field - holds a height value and a vector of all units present in the tile. */
class Tile {
public:
    Tile(int height) : _height(height) {}

    ~Tile() noexcept = default;
    Tile(const Tile& tile) {
        for (const auto& p_u : tile._units) {
            add_unit((*p_u).clone());
        }
        _height = tile._height;
    }
    Tile& operator=(const Tile& tile) {
        return *this = Tile(tile);
    }
    Tile(Tile&& other) = default;
    Tile& operator=(Tile&& other) = default;

    int height() const {
        return _height;
    }

    bool is_tile_empty() const {
        return _units.empty();
    }

    void add_unit(std::unique_ptr<Unit> unit) {
        _units.push_back(std::move(unit));
    }

#pragma region Find Units

    std::vector<std::unique_ptr<Unit>>& find_all_units() {
        return _units;
    }

    std::unique_ptr<Unit>& find_first_unit() {
        if (is_tile_empty())
            throw std::runtime_error("There is no unit!");
        return _units[0];
    }

    std::unique_ptr<Unit>& find_one_unit(std::string_view id) {
        for (size_t i = 0; i < _units.size(); ++i) {
            if (_units[i]->id() == id) {
                return _units[i];
                break;
            }
        }
        throw std::runtime_error("No such unit!");
    }

#pragma endregion

#pragma region Remove Units

    std::vector<std::unique_ptr<Unit>> remove_all_units() {
        auto units = std::move(_units);
        _units.clear();
        return units;
    }

    std::unique_ptr<Unit> remove_one_unit(std::string_view id) {
        std::unique_ptr<Unit> p_unit;
        for (size_t i = 0; i < _units.size(); ++i) {
            if (_units[i]->id() == id) {
                p_unit = std::move(_units[i]);
                _units.erase(_units.begin() + i);
                return p_unit;
            }
        }
        throw std::runtime_error("No such unit!");
    }

#pragma endregion  


private:
    int _height;
    std::vector<std::unique_ptr<Unit>> _units;
};


class BattleField {
public:
    BattleField(int M, int N, int time = 0) : _M(M), _N(N), _time(time) {
        _field.resize(_M);
        for (int i = 0; i < _M; ++i) {
            for (int j = 0; j < _N; ++j) {
                int height; std::cin >> height;
                _field[i].push_back(height);
            }
        }
    }

    ~BattleField() noexcept = default;
    BattleField(const BattleField& bf) {
        _M = bf._M;
        _N = bf._N;
        _time = bf._time;
        _coordinates = bf._coordinates;
        _field = bf._field;
    }

    BattleField& operator=(const BattleField& bf) {
        return *this = BattleField(bf);
    };
    BattleField(BattleField&& other) = default;
    BattleField& operator=(BattleField&& other) = default;


    int time() const {
        return _time;
    }

    void set_time(int t) {
        _time = t;
    }

    /* Adds the given unit with the given <id> to the scene. */
    void spawn_unit(const std::string& type, const std::string& id, const int x, const int y) {
        Vec2i coor = { x, y };

        if (!is_inside_field(coor) || !_field[x][y].is_tile_empty() || _coordinates.contains(id))   // The tile is not available
            return;

        auto p_unit = create_unit(type, id, coor);

        _coordinates.emplace(id, p_unit->coor());
        _field[x][y].add_unit(std::move(p_unit));
    }

    /* Removes the object with the given <id> from the battle field. */
    std::unique_ptr<Unit> remove_unit(const std::string& id) {
        auto p_unit = find_tile(_coordinates.at(id)).remove_one_unit(id);
        _coordinates.erase(id);
        return p_unit;
    }

    /* Unit's attack function. */
    void attack_unit(const std::string& id, const std::string& dir) {
        if (!_coordinates.contains(id))     // No such attacking unit
            return;

        auto& p_attacker = find_tile(_coordinates.at(id)).find_one_unit(id);

        for (int i = 1; i <= p_attacker->attack_distance(); ++i) {
            if (!is_inside_field(_coordinates.at(id) + p_attacker->attack_directions().at(dir) * i) ||           // Target is outside the field
                initialize_attack(id, p_attacker->attack_directions().at(dir) * i))                              // Attack was successful
                return;
        }
    }

    /* Moves given unit to coors (x,y). */
    void move_unit(const std::string& id, const int x, const int y) {
        Vec2i target = { x,y };

        bool result = is_move_successful(id, target);

        // Even if is not able to move, the target coor will be saved and teoretically unit will be standing there (in case we'll need to move the unit later)
        find_tile(_coordinates.at(id)).find_one_unit(id)->set_coor(target);

        if (result) {       // If is able to move, it will actually move
            auto p_unit = remove_unit(id);

            _coordinates.emplace(id, p_unit->coor());
            _field[x][y].add_unit(std::move(p_unit));
        }
        else {
            auto& unit = find_tile(_coordinates.at(id)).find_one_unit(id);
            //unit->set_nonstacking_effect("dead_inside", true);    // This will never change back to false - even if it moves somewhere else

            if (!unit->is_effect_full("dead_inside")) {
                auto dead_inside = std::make_unique<DeadInside>();
                unit->add_effect(std::move(dead_inside));
            }
        }
    }

    /* Prints out the current state of the battlefield. */
    void state() {
        std::multimap<Vec2i, std::string> flipped_coordinates = flip_map(_coordinates);
        for (const auto& [coor, id] : flipped_coordinates) {
            std::unique_ptr<Unit>& p_unit = find_tile(coor).find_one_unit(id);
            std::cout << id << " " << p_unit->type() << " (" << coor[0] + 1 << ", " << coor[1] + 1 << ") " << p_unit->hp() << std::endl;
        }
        std::cout << "---" << std::endl;
    }

    /* Apllies all effects, (removes effects) and sets new time. */
    void update_tick(int new_time) {
        for (auto itr = _coordinates.cbegin(), next_it = itr; itr != _coordinates.cend(); itr = next_it) {
            ++next_it;
            std::unique_ptr<Unit>& p_unit = find_tile(itr->second).find_one_unit(itr->first);

            for (const auto& effect : p_unit->effects()) {
                effect->apply(p_unit, new_time - _time);
            }

            if (p_unit->hp() <= 0) {
                remove_unit(itr->first);
            }
            else {
                p_unit->remove_overdue_effects();
            }
        }

        set_time(new_time);
    }


private:
    int _M, _N;     // Size of battlefield
    int _time;      // Current time on the battlefield
    std::vector<std::vector<Tile>> _field;          // Field consisting of tiles
    std::map<std::string, Vec2i> _coordinates;      // A map of all unit ids and their coordinates on the field

    /* Creates a unit with the specific type, id and coordinates. */
    static std::unique_ptr<Unit> create_unit(std::string_view type, std::string_view id, Vec2i coor) {
        if (type == "footman") {
            return std::make_unique<Footman>(id, coor);
        }
        else if (type == "knight") {
            return std::make_unique<Knight>(id, coor);
        }
        else if (type == "rifleman") {
            return std::make_unique<Rifleman>(id, coor);
        }
        else {
            throw std::runtime_error("Unsupported unit!");
        }
    }

    /* Finds a tile at the given coordinates. */
    Tile& find_tile(Vec2i coor) {
        return _field[coor.at(0)][coor.at(1)];
    }

    /* Determines if tile contains the given id. */
    bool tile_contains_id(const std::string& id, const Vec2i target) {
        for (auto& u : find_tile(target).find_all_units()) {
            if (u->id() == id)
                return true;
        }
        return false;
    }

    /* Checks if the coordinates are valid (inside the field). */
    bool is_inside_field(const Vec2i coor) {
        if (coor.at(0) < 0 || coor.at(0) >= _M || coor.at(1) < 0 || coor.at(1) >= _N)
            return false;
        return true;
    }

    /* Returns the height difference of two tiles. Can be less than 0 if tile2 is higher than tile1. */
    int calculate_height_diff(const Tile& tile1, const Tile& tile2) {
        return tile1.height() - tile2.height();
    }

    /* Begin attack in the given direction. */
    bool initialize_attack(const std::string& id, const Vec2i dir) {
        if (find_tile(_coordinates.at(id) + dir).is_tile_empty())      // There is no target unit there
            return false;

        auto& p_victims = find_tile(_coordinates.at(id) + dir).find_all_units();
        auto& p_attacker = find_tile(_coordinates.at(id)).find_one_unit(id);

        int h_diff = calculate_height_diff(find_tile(_coordinates.at(id)), find_tile(_coordinates.at(id) + dir));
        if (abs(h_diff) > p_attacker->attack_range())
            return false;

        int damage = p_attacker->attack_dmg() + h_diff;

        if (damage < 0) // In case the height diff benefits the victim too hard
            damage = 0;

        for (auto& v : p_victims) {
            v->take_damage(damage);

            if (v->hp() <= 0) {
                remove_unit(std::string(v->id()));
            }
        }

        return true;
    }

    /* Tries to move a unit to the specific coordinates and if successful returns true, otherwise false. */
    bool is_move_successful(const std::string& id, const Vec2i target) {
        std::unique_ptr<Unit>& p_unit = find_tile(_coordinates.at(id)).find_one_unit(id);

        if (!is_inside_field(target) ||                      // If isn't valid
            (!find_tile(target).is_tile_empty() && !tile_contains_id(id, target)) ||         // If is occupied
            !BFS(_coordinates.at(id), target, p_unit->max_move(), p_unit->max_step(), p_unit->move_directions()))       // If can't be accessed
            return false;

        return true;
    }

    /* Returns true if there is a free path from coor to target using max_move, max_step and given directions. */
    bool BFS(Vec2i coor, const Vec2i target, const int max_move, const int max_step, const std::map<std::string, Vec2i> dirs) {
        std::pair<Vec2i, int> pair;
        std::vector <std::vector<bool>> visited;
        visited.resize(_M, std::vector<bool>(_N, false));

        std::list<std::pair<Vec2i, int>> queue;

        // Mark the current node as visited and enqueue it
        visited[coor.at(0)][coor.at(1)] = true;
        queue.push_back(std::make_pair(coor, 0));

        while (!queue.empty()) {

            pair = queue.front();
            queue.pop_front();

            if (pair.first == target)
                return true;

            for (const auto& [word, vector] : dirs) {
                coor = pair.first + vector;
                if (is_inside_field(coor) &&
                    !visited[coor.at(0)][coor.at(1)] &&            // Tile not visited
                    pair.second + 1 <= max_move &&                 // The distance is no longer than max_steps
                    abs(calculate_height_diff(find_tile(pair.first), find_tile(coor))) <= max_step &&      // Height difference no bigger than max_height
                    find_tile(coor).is_tile_empty()) {             // The tile must be empty

                    visited[coor.at(0)][coor.at(1)] = true;
                    queue.push_back(std::make_pair(coor, pair.second + 1));
                }
            }
        }

        return false;
    }
};

#pragma endregion


bool check_command() {
    std::cin >> std::ws;
    char c = std::cin.peek();

    if (c != ';')
        return true;
    return false;
}

bool check_lowercase(const std::string& id) {
    for (size_t i = 0; i < id.size(); ++i)
    {
        if (!isalpha(id[i]) || !islower(id[i]))
            return false;
    }
    return true;
}

int main()
{
    //char c = std::cin.peek();
    //while (c == ' ') {
    //    std::cin.get(c);
    //    c = std::cin.peek();
    //}
    //if (c == '\n')
    //    std::cout << true << std::endl;



    int M{}, N{};
    std::cin >> M;
    std::cin >> N;
    BattleField battle_field{ M,N };

    bool endline = false;

    //std::string line;
    //while (getline(std::cin, line)) {
    //    line >> time;

    //}

    for (std::string cmd; std::cin >> cmd; ) {
        if (cmd == "spawn") {
            std::string type;
            std::cin >> type;

            std::string id;
            std::cin >> id;

            int x{}, y{};
            std::cin >> x >> y;
            --x; --y;

            if (check_lowercase(id))
                battle_field.spawn_unit(type, id, x, y);
        }
        else if (cmd == "move") {
            std::string id;
            std::cin >> id;

            int x{}, y{};
            std::cin >> x >> y;
            --x; --y;

            battle_field.move_unit(id, x, y);
        }
        else if (cmd == "attack") {
            std::string id;
            std::cin >> id;

            std::string dir;
            std::cin >> dir;

            battle_field.attack_unit(id, dir);
        }
        else if (cmd == "state") {
            battle_field.state();
        }
        else if (cmd == "new_command") {

        }
        else if (cmd == ";") {}
        else {
            int temp = std::stoi(cmd);
            if (battle_field.time() >= temp)
                return 1;

            battle_field.update_tick(std::stoi(cmd));
            //throw std::runtime_error("Unsupported command!");
        }
    }
}