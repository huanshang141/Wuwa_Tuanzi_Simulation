#include <string>
#include <vector>
#include <iostream>
#include <random>    // 添加随机数库
#include <algorithm> // 添加algorithm库，包含shuffle函数
// #include "skill_functions.h" // 引入技能函数头文件

using namespace std;
const int map_len = 20;

// 全局随机数生成器
random_device g_rd;
mt19937 g_gen(g_rd());
uniform_int_distribution<> g_dice_dist(1, 3); // 骰子分布 (1-3)

// 前向声明
struct map;

struct SkillResult
{
    int steps;            // 移动步数
    bool skill_activated; // 是否触发技能

    SkillResult(int s, bool activated = false) : steps(s), skill_activated(activated) {}
};

// 技能函数类型定义，返回SkillResult
typedef SkillResult (*skill_func_t)(int pos, const map *game_map);

// 前向声明默认技能函数
SkillResult default_skill(int pos, const map *game_map);

struct char_node
{
public:
    string name;
    char_node *next;
    char_node *prev;
    int pos;
    bool is_head;
    skill_func_t skill; // 现在已经定义了类型

    char_node(string name)
        : name(name), next(nullptr), prev(nullptr), pos(0), is_head(false), skill(default_skill)
    {
    }
    char_node(string name, skill_func_t skill_func)
        : name(name), next(nullptr), prev(nullptr), pos(0), is_head(false)
    {
        skill = skill_func;
    }

    char_node(bool is_head)
        : name(""), next(nullptr), prev(nullptr), pos(0), is_head(is_head), skill(default_skill)
    {
    }
    int get_pos() const
    {
        return pos;
    }
    char_node *get_head()
    {
        if (is_head)
            return this;
        else
            return prev->get_head();
    }
    char_node *get_end()
    {
        if (!next)
            return this;
        else
            return next->get_end();
    }
    int move(int step, bool skill_activated)
    {
        if (name == "椿" && skill_activated)
        {
            if (prev)
            {
                prev->next = this->next;
                this->next->prev = prev;
            }
            this->next = nullptr;
            this->pos += step;
            return 1;
        }
        if (prev)
            prev->next = nullptr;
        char_node *temp = this;
        int num = 0;
        int pos = this->pos;
        int new_pos = pos + step;
        while (temp)
        {
            ++num;
            temp->pos = new_pos;
            temp = temp->next;
        }
        return num;
    }
};
struct map
{
public:
    vector<char_node *> _map;
    vector<int> cell_count; // 添加格子角色计数数组

    map(int len) : _map(len, nullptr), cell_count(len, 0) // 初始化格子角色计数数组
    {
        for (int i = 0; i < len; i++)
        {
            _map[i] = new char_node(true);
        }
    }

    bool move(char_node *node)
    {
        int pos = node->get_pos();
        // 传入当前位置和地图指针到技能函数，获取技能结果
        SkillResult result = node->skill(pos, this);
        int actual_step = result.steps;
        int new_pos = pos + actual_step;

        // 添加调试信息 - 记录角色信息
        string character_name = node->name;

        // 输出技能触发信息
        if (result.skill_activated)
        {
            cout << "【" << character_name << " 技能触发】" << endl;
        }

        if (new_pos >= map_len)
        {
            cout << "角色 " << character_name << " 尝试从位置 " << pos << " 移动 " << actual_step
                 << " 步到位置 " << new_pos << "，超出地图范围，移动失败" << endl;
            return false;
        }
        char_node *new_pos_node = _map[new_pos];
        int num = node->move(actual_step, result.skill_activated); // 使用计算后的步数
        if (pos > 0)
            cell_count[pos] -= num; // 减少起点格子角色数量
        cell_count[new_pos] += num; // 增加目标格子角色数量
        node->prev = new_pos_node->get_end();
        new_pos_node->get_end()->next = node;

        // 输出移动信息
        cout << "角色 " << character_name << " 从位置 " << pos << " 移动到位置 " << new_pos
             << "（实际步数: " << actual_step << "）" << endl;

        return true;
    }

    // 获取指定位置的角色数量
    int get_cell_count(int pos) const
    {
        if (pos <= 0 || pos >= cell_count.size())
        {
            return 0; // 起点或无效位置返回0
        }
        return cell_count[pos];
    }

    // 判断是否是终点
    bool is_finish_point(int pos)
    {
        return pos >= map_len;
    }
};

// 技能结果结构体，包含移动步数和是否触发技能

// 默认技能函数
SkillResult default_skill(int pos, const map *game_map);

// 柯莱塔技能：28%的概率使得步数翻倍
SkillResult move_double_28(int pos, const map *game_map);

// 椿技能：50%概率额外移动等同于当前格子角色数量的步数
SkillResult move_with_crowd_bonus(int pos, const map *game_map);

// 基础技能函数
SkillResult move_origin(int pos, const map *game_map);
SkillResult move_plus_one(int pos, const map *game_map);
SkillResult move_double(int pos, const map *game_map);

// 技能函数实现
SkillResult default_skill(int pos, const map *game_map)
{
    return SkillResult(g_dice_dist(g_gen), false); // 不触发技能，返回随机数
}

SkillResult move_double_28(int pos, const map *game_map)
{
    // 创建0-99的均匀分布
    static std::uniform_int_distribution<> prob_dist(0, 99);

    // 生成骰子点数
    int dice_roll = g_dice_dist(g_gen);

    // 生成概率判断值(0-99)
    int probability = prob_dist(g_gen);

    // 28%的概率翻倍
    if (probability < 28)
    {
        std::cout << "【触发技能】骰子点数翻倍！" << std::endl;
        return SkillResult(dice_roll * 2, true); // 触发技能，返回翻倍点数
    }
    else
    {
        return SkillResult(dice_roll, false); // 未触发技能，返回原始点数
    }
}

SkillResult move_with_crowd_bonus(int pos, const map *game_map)
{
    int dice_roll = g_dice_dist(g_gen);
    int crowd_bonus = 0;

    // 创建概率分布(0-99)
    static std::uniform_int_distribution<> prob_dist(0, 99);

    // 检查是否触发技能(50%概率)
    bool can_trigger = (prob_dist(g_gen) < 50);

    // 获取当前格子的角色数量（不包括自己）
    if (pos > 0)
    {
        crowd_bonus = game_map->get_cell_count(pos) - 1;
        if (crowd_bonus < 0)
            crowd_bonus = 0;
    }

    // 只有在有其他角色且通过50%概率检查时才触发技能
    if (crowd_bonus > 0 && can_trigger)
    {
        std::cout << "【触发技能】当前格有" << crowd_bonus << "个其他角色，额外移动" << crowd_bonus << "步！" << std::endl;
        return SkillResult(dice_roll + crowd_bonus, true); // 触发技能，增加额外步数
    }

    return SkillResult(dice_roll, false); // 未触发技能，返回原始点数
}

SkillResult move_origin(int pos, const map *game_map)
{
    return SkillResult(g_dice_dist(g_gen), false); // 不触发技能，返回随机数
}

SkillResult move_plus_one(int pos, const map *game_map)
{
    int dice = g_dice_dist(g_gen);
    return SkillResult(dice + 1, true); // 触发技能，步数+1
}

SkillResult move_double(int pos, const map *game_map)
{
    int dice = g_dice_dist(g_gen);
    return SkillResult(dice * 2, true); // 触发技能，步数翻倍
}
int main()
{
    map m(map_len);
    vector<char_node *> char_list(2); // 修改为2个角色

    // 使用全局技能函数初始化角色
    char_list[0] = new char_node("椿", move_with_crowd_bonus);
    char_list[1] = new char_node("柯莱塔", move_double_28); // 使用新技能
    // 移除第三个角色

    cout << "游戏开始！地图长度: " << map_len << endl;
    cout << "椿技能: 50%概率额外移动当前格其他角色数量的步数" << endl;
    cout << "柯莱塔技能: 28%的概率使得步数翻倍" << endl;
    cout << "-----------------------------------------" << endl;

    bool game_over = false;
    int round = 1;

    // 游戏主循环
    while (!game_over)
    {
        cout << "\n第 " << round << " 轮开始" << endl;

        // 创建角色索引数组，用于随机排序
        vector<int> action_order(char_list.size());
        for (size_t i = 0; i < action_order.size(); i++)
        {
            action_order[i] = i;
        }

        // 随机打乱行动顺序 - 使用全局随机数生成器
        shuffle(action_order.begin(), action_order.end(), g_gen);

        cout << "本轮行动顺序: ";
        for (size_t i = 0; i < action_order.size(); i++)
        {
            cout << char_list[action_order[i]]->name;
            if (i < action_order.size() - 1)
                cout << " > ";
        }
        cout << endl;

        // 按随机顺序让角色行动
        for (size_t i = 0; i < action_order.size(); i++)
        {
            int idx = action_order[i];

            // 角色行动
            cout << "\n"
                 << char_list[idx]->name << " 行动:" << endl;
            bool move_result = m.move(char_list[idx]);

            // 检查是否有角色到达终点
            if (!move_result)
            {
                cout << "\n恭喜！" << char_list[idx]->name << " 到达终点，游戏结束！" << endl;
                game_over = true;
                break;
            }
        }

        // 显示当前所有角色位置
        cout << "\n当前位置状态:" << endl;
        for (size_t i = 0; i < char_list.size(); i++)
        {
            cout << char_list[i]->name << ": 位置 " << char_list[i]->get_pos() << endl;
        }

        round++;
        cout << "-----------------------------------------" << endl;
    }

    return 0;
}