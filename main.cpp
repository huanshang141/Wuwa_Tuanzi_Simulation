#include <string>
#include <vector>
#include <iostream>
#include <random>    // 添加随机数库
#include <algorithm> // 添加algorithm库，包含shuffle函数
#include <chrono>    // 用于获取时间戳作为种子
#include <numeric>   // 用于accumulate
#include <iomanip>   // 用于setprecision
// #include "skill_functions.h" // 引入技能函数头文件

using namespace std;
const int MAP_LENGTH = 27;

// 全局随机数生成器
random_device g_rd;
// 结合当前时间戳和random_device生成的随机数，确保每次运行都有不同的种子
unsigned seed = g_rd() ^ static_cast<unsigned>(chrono::high_resolution_clock::now().time_since_epoch().count());
mt19937 g_gen(seed);
uniform_int_distribution<> g_dice_dist(1, 3);  // 骰子分布 (1-3)
uniform_int_distribution<> g_range_dist(2, 3); // 守岸人骰子分布 (2-3)
uniform_int_distribution<> g_prob_dist(0, 99); // 概率分布 (0-99)

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

// 守岸人技能：骰子只会掷出2或3
SkillResult move_minimum_two(int pos, const map *game_map);

struct char_node
{
public:
    string name;
    char_node *next;
    char_node *prev;
    int pos;
    bool is_head;
    skill_func_t skill;

    char_node(string name)
        : name(name), next(nullptr), prev(nullptr), pos(0), is_head(false), skill(default_skill)
    {
    }
    char_node(string name, skill_func_t skill_func)
        : name(name), next(nullptr), prev(nullptr), pos(0), is_head(false)
    {
        skill = skill_func;
    }

    char_node(bool is_head) // 头节点
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
        // cout << "test" << endl;
        if (!next)
            return this;
        else
            return next->get_end();
    }

    // 将节点移动到所在链表的最后
    void move_to_end_of_list()
    {
        // 如果没有后继节点或前驱节点，说明已经是单个节点或末尾节点
        if (!next || !prev)
        {
            return;
        }

        // 断开当前位置的连接
        prev->next = next;
        next->prev = prev;

        // 找到链表末尾节点
        char_node *end_node = this;
        while (end_node->next)
        {
            end_node = end_node->next;
        }

        // 将自己连接到链表末尾
        end_node->next = this;
        this->prev = end_node;
        this->next = nullptr;
    }

    int move(int step, bool skill_activated)
    {
        if (name == "椿" && skill_activated)
        {
            if (prev)
            {
                // 不带别人走
                prev->next = this->next;
                if (this->next)
                    this->next->prev = prev;
            }
            this->next = nullptr;
            this->pos += step;
            return 1;
        }
        // 带别人走
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
    // 判断是否踩在别人头上
    bool ccb()
    {
        if (this->prev)
        {
            if (!(this->prev->is_head))
            {
                return true;
            }
        }
        return false;
    }
    void set_pos(int new_pos, map *game_map);
};
struct map
{
public:
    vector<char_node *> _map;
    vector<int> cell_count;         // 添加格子角色计数数组
    vector<char_node *> characters; // 存储所有角色

    map(int len) : _map(len, nullptr), cell_count(len, 0) // 初始化格子角色计数数组
    {
        for (int i = 0; i < len; i++)
        {
            _map[i] = new char_node(true);
        }
    }

    // 注册角色到地图
    void register_character(char_node *character)
    {
        characters.push_back(character);
    }

    // 更新排名（不显示）
    void update_rankings_silent()
    {
        // 按位置排序角色（位置大的排前面）
        sort(characters.begin(), characters.end(), [](char_node *a, char_node *b)
             { return a->get_pos() > b->get_pos(); });
    }

    // 显示排名
    void display_rankings()
    {
        // 确保排名是最新的
        update_rankings_silent();

        // cout << "\n当前排名:" << endl;
        // for (size_t i = 0; i < characters.size(); i++)
        // {
        //     cout << "第" << (i + 1) << "名: " << characters[i]->name
        //          << " (位置: " << characters[i]->get_pos() << ")" << endl;
        // }
    }

    // 原有的更新并显示排名方法保留，以保持兼容性
    void update_rankings()
    {
        update_rankings_silent();
    }

    bool move(char_node *node)
    {
        int pos = node->get_pos();
        // 传入当前位置和地图指针到技能函数，获取技能结果
        SkillResult result = node->skill(pos, this);

        // 添加调试信息 - 记录角色信息
        string character_name = node->name;

        if (character_name == "卡卡罗" && is_last_place(character_name) && node->pos != 0)
        {
            // cout << "卡卡罗处于最后位置，触发追赶技能！" << endl;
            result.steps += 3;
            result.skill_activated = true;
        }
        int actual_step = result.steps;
        int new_pos = pos + actual_step;
        // 输出技能触发信息
        // if (result.skill_activated)
        // {
        //     cout << "【" << character_name << " 技能触发】" << endl;
        // }

        if (new_pos >= MAP_LENGTH)
        {
            // cout << "角色 " << character_name << " 尝试从位置 " << pos << " 移动 " << actual_step
            //      << " 步到位置 " << new_pos << "，超出地图范围，移动失败" << endl;
            return false;
        }
        char_node *new_pos_node = _map[new_pos];
        int num = node->move(actual_step, result.skill_activated); // 使用计算后的步数
        if (pos > 0)
            cell_count[pos] -= num; // 减少起点格子角色数量
        cell_count[new_pos] += num; // 增加目标格子角色数量
        node->prev = new_pos_node->get_end();
        new_pos_node->get_end()->next = node;
        // 每个人踩过都触发
        // 遍历链表中在该节点前面的所有节点，如果有节点的name=="今汐"，则将"今汐"节点move_to_end_of_list
        char_node *current = node->prev;
        while (current && !current->is_head)
        {
            if (current->name == "今汐")
            {
                // 有40%的概率触发
                int roll = g_prob_dist(g_gen);
                if (roll < 40)
                { // 40%的概率
                    // cout << "【今汐技能触发】攀上新高" << endl;
                    current->move_to_end_of_list();
                }
                break; // 找到一个就够了，不需要继续查找
            }
            current = current->prev;
        }

        // 输出移动信息
        // cout << "角色 " << character_name << " 从位置 " << pos << " 移动到位置 " << new_pos
        //      << "（实际步数: " << actual_step << "）" << endl;

        // 仅更新排名，不显示
        update_rankings_silent();

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
        return pos >= MAP_LENGTH;
    }

    // 判断角色是否是最后一名（包括并列最后一名）
    bool is_last_place(const string &character_name) const
    {
        // 先找到该角色
        auto it = find_if(characters.begin(), characters.end(),
                          [&character_name](const char_node *node)
                          {
                              return node->name == character_name;
                          });

        if (it == characters.end())
        {
            return false; // 没找到该角色
        }

        int char_pos = (*it)->get_pos();

        // 检查是否有位置更靠后的角色
        for (auto character : characters)
        {
            if (character->name != character_name && character->get_pos() < char_pos)
            {
                return false; // 存在位置更小的角色，不是最后一名
            }
        }

        return true; // 没有其他角色位置更小，是最后一名或并列最后一名
    }

    // 通过角色名获取排名
    int get_rank_by_name(const string &character_name) const
    {
        // 创建角色列表副本进行排序
        vector<char_node *> sorted_chars = characters;

        // 按位置排序角色（位置大的排前面）
        sort(sorted_chars.begin(), sorted_chars.end(), [](char_node *a, char_node *b)
             { return a->get_pos() > b->get_pos(); });

        // 查找角色并返回其排名
        for (size_t i = 0; i < sorted_chars.size(); i++)
        {
            if (sorted_chars[i]->name == character_name)
            {
                return i + 1; // 排名从1开始
            }
        }

        // 未找到该角色
        return -1;
    }
};

void char_node::set_pos(int new_pos, map *game_map)
{

    this->pos = new_pos;
    this->prev = game_map->_map[new_pos]->get_end(); // 更新前驱节点
    game_map->_map[new_pos]->get_end()->next = this; // 更新地图上的位置

    game_map->cell_count[new_pos]++; // 增加格子角色数量
}

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
// 柯莱塔技能：28%的概率使得步数翻倍
SkillResult move_double_28(int pos, const map *game_map)
{
    // 生成骰子点数
    int dice_roll = g_dice_dist(g_gen);

    // 生成概率判断值(0-99)
    int probability = g_prob_dist(g_gen);

    // 28%的概率翻倍
    if (probability < 28)
    {
        // std::cout << "【触发技能】骰子点数翻倍！" << std::endl;
        return SkillResult(dice_roll * 2, true); // 触发技能，返回翻倍点数
    }
    else
    {
        return SkillResult(dice_roll, false); // 未触发技能，返回原始点数
    }
}
// 椿技能：50%概率额外移动等同于当前格子其他角色数量的步数
SkillResult move_with_crowd_bonus(int pos, const map *game_map)
{
    int dice_roll = g_dice_dist(g_gen);
    int crowd_bonus = 0;

    // 检查是否触发技能(50%概率)
    bool can_trigger = (g_prob_dist(g_gen) < 50);

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
        // std::cout << "【触发技能】当前格有" << crowd_bonus << "个其他角色，额外移动" << crowd_bonus << "步！" << std::endl;
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

// 守岸人技能：骰子只会掷出2或3
SkillResult move_minimum_two(int pos, const map *game_map)
{
    int dice_roll = g_range_dist(g_gen);

    // std::cout << "【触发技能】骰子最小值为2！" << std::endl;
    return SkillResult(dice_roll, true); // 始终触发技能
}

// 添加结构体用于存储统计信息
struct GameStats
{
    vector<int> wins;             // 各角色胜场
    vector<int> skill_triggers;   // 各角色技能触发次数
    vector<vector<int>> rankings; // 各角色每局的排名

    GameStats(int player_count) : wins(player_count, 0), skill_triggers(player_count, 0)
    {
        // 初始化排名数组，每个角色有一个排名记录数组
        rankings.resize(player_count);
    }

    // 打印统计信息
    void print_stats(const vector<string> &char_list) const
    {
        cout << "\n===== 游戏统计 =====" << endl;
        for (size_t i = 0; i < char_list.size(); i++)
        {
            cout << char_list[i]
                 << " - 胜场: " << wins[i]
                 << " (" << (wins[i] * 100.0 / accumulate(wins.begin(), wins.end(), 0)) << "%)" << endl;

            // 添加平均排名信息
            if (!rankings[i].empty())
            {
                double avg_rank = accumulate(rankings[i].begin(), rankings[i].end(), 0.0) / rankings[i].size();
                cout << "    平均排名: " << fixed << setprecision(2) << avg_rank << endl;
            }
        }
        cout << "===================" << endl;
    }

    // 计算指定角色的排名分布
    vector<int> get_rank_distribution(int player_index, int max_rank) const
    {
        vector<int> distribution(max_rank, 0); // 初始化为0

        for (int rank : rankings[player_index])
        {
            if (rank > 0 && rank <= max_rank)
            {
                distribution[rank - 1]++; // 排名从1开始，数组索引从0开始
            }
        }

        return distribution;
    }

    // 绘制ASCII柱状图显示排名分布
    void draw_rank_chart(const vector<string> &char_list) const
    {
        const int max_rank = 6; // 最多6名角色

        for (size_t i = 0; i < char_list.size(); i++)
        {
            cout << "\n=== " << char_list[i] << " 排名分布 ===" << endl;

            vector<int> distribution = get_rank_distribution(i, max_rank);
            int max_count = *max_element(distribution.begin(), distribution.end());

            // 确定比例尺
            const int max_bar_width = 40;
            double scale = max_count > 0 ? max_bar_width / (double)max_count : 0;

            // 绘制柱状图
            for (int rank = 0; rank < max_rank; rank++)
            {
                int count = distribution[rank];
                int bar_width = static_cast<int>(count * scale);

                cout << "第" << (rank + 1) << "名: ";
                cout << string(bar_width, '#') << " " << count;

                // 显示百分比
                if (rankings[i].size() > 0)
                {
                    double percentage = (count * 100.0) / rankings[i].size();
                    cout << " (" << fixed << setprecision(1) << percentage << "%)";
                }
                cout << endl;
            }
        }
    }
};

// 修改技能函数，增加技能触发计数
void track_skill_trigger(int player_index, GameStats &stats)
{
    stats.skill_triggers[player_index]++;
}

int main()
{
    // 输出当前使用的随机数种子（方便调试）
    cout << "当前随机数种子: " << seed << endl;

    // 模拟次数
    const int SIMULATION_COUNT = 10000; // 增加模拟次数以获得更准确的统计结果

    // 统计信息
    GameStats stats(6); // 6个角色

    cout << "开始模拟！总模拟次数: " << SIMULATION_COUNT << endl;

    for (int sim = 0; sim < SIMULATION_COUNT; sim++)
    {
        // 创建地图和角色
        map m(MAP_LENGTH);
        vector<char_node *> char_list(6);

        // 初始化角色
        char_list[0] = new char_node("椿", move_with_crowd_bonus);
        char_list[1] = new char_node("柯莱塔", move_double_28);
        char_list[2] = new char_node("守岸人", move_minimum_two);
        char_list[3] = new char_node("卡卡罗", default_skill);
        char_list[4] = new char_node("长离", default_skill);
        char_list[5] = new char_node("今汐", default_skill);

        char_list[2]->set_pos(0, &m); // 守岸人初始位置为0
        char_list[0]->set_pos(1, &m); // 椿初始位置为1
        char_list[5]->set_pos(1, &m); // 今汐初始位置为1
        char_list[4]->set_pos(2, &m); // 长离初始位置为2
        char_list[1]->set_pos(2, &m); // 柯莱塔初始位置为2
        char_list[3]->set_pos(3, &m); // 卡卡罗初始位置为3

        // 注册角色到地图
        for (auto character : char_list)
        {
            m.register_character(character);
        }

        // 仅在第一次模拟时显示游戏介绍
        if (sim == 0)
        {
            cout << "游戏开始！地图长度: " << MAP_LENGTH << endl;
            cout << "椿技能: 50%概率额外移动当前格其他角色数量的步数" << endl;
            cout << "柯莱塔技能: 28%的概率使得步数翻倍" << endl;
            cout << "守岸人技能: 骰子只会掷出2或3" << endl;
            cout << "卡卡罗技能: 落后追赶，正常掷骰子(1-3)" << endl;
            cout << "长离技能: 踩在别人头上时，65%概率与最后行动角色交换顺序，正常掷骰子(1-3)" << endl;
            cout << "今汐技能: 攀上新高！，正常掷骰子(1-3)" << endl;
            cout << "-----------------------------------------" << endl;
        }

        bool game_over = false;
        int round = 1;

        // 是否在控制台显示详细过程
        bool verbose = (sim == 0); // 只在第一次模拟时输出详细信息

        // 游戏主循环
        while (!game_over)
        {
            if (verbose)
                cout << "\n第 " << round << " 轮开始" << endl;

            // 创建角色索引数组，用于随机排序
            vector<int> action_order(char_list.size());
            for (size_t i = 0; i < action_order.size(); i++)
            {
                action_order[i] = i;
            }

            // 随机打乱行动顺序
            shuffle(action_order.begin(), action_order.end(), g_gen);

            // 长离技能
            auto it = find_if(action_order.begin(), action_order.end(),
                              [&char_list](int idx)
                              { return char_list[idx]->name == "长离"; });
            if (it != action_order.end())
            {
                int changli_index = distance(action_order.begin(), it);

                if (char_list[*it]->ccb())
                {
                    int roll = g_prob_dist(g_gen);

                    if (roll < 65)
                    {
                        int last_index = action_order.size() - 1;
                        if (changli_index != last_index)
                        {
                            if (verbose)
                                cout << "【长离技能触发】与最后行动角色交换顺序！" << endl;
                            swap(action_order[changli_index], action_order[last_index]);
                            stats.skill_triggers[4]++; // 长离的技能触发计数
                        }
                    }
                }
            }

            if (verbose)
            {
                cout << "本轮行动顺序: ";
                for (size_t i = 0; i < action_order.size(); i++)
                {
                    cout << char_list[action_order[i]]->name;
                    if (i < action_order.size() - 1)
                        cout << " > ";
                }
                cout << endl;
            }

            // 按顺序行动
            for (size_t i = 0; i < action_order.size(); i++)
            {
                int idx = action_order[i];

                if (verbose)
                    cout << "\n"
                         << char_list[idx]->name << " 行动:" << endl;

                // 保存行动前的技能触发计数，后面用于检测是否触发
                int prev_trigger_count = stats.skill_triggers[idx];

                bool move_result = m.move(char_list[idx]);

                // 检查是否有技能触发
                if (verbose && prev_trigger_count < stats.skill_triggers[idx])
                {
                    cout << "【" << char_list[idx]->name << " 技能已触发】" << endl;
                }

                // 检查是否到达终点
                if (!move_result)
                {
                    if (verbose)
                        cout << "\n恭喜！" << char_list[idx]->name << " 到达终点，游戏结束！" << endl;
                    stats.wins[idx]++; // 记录胜场

                    // 记录所有角色的最终排名
                    for (size_t j = 0; j < char_list.size(); j++)
                    {
                        int rank = m.get_rank_by_name(char_list[j]->name);
                        stats.rankings[j].push_back(rank);
                        if (verbose)
                            cout << char_list[j]->name << " 本局排名: " << rank << endl;
                    }

                    game_over = true;
                    break;
                }
            }

            if (verbose)
            {
                // 显示位置状态
                cout << "\n当前位置状态:" << endl;
                for (size_t i = 0; i < char_list.size(); i++)
                {
                    cout << char_list[i]->name << ": 位置 " << char_list[i]->get_pos() << endl;
                }

                // 显示排名
                m.display_rankings();

                cout << "-----------------------------------------" << endl;
            }

            round++;
        }

        // 清理本次模拟的资源
        for (auto character : char_list)
        {
            delete character;
        }

        // 定期显示进度
        if ((sim + 1) % 100 == 0 || sim == SIMULATION_COUNT - 1)
        {
            cout << "已完成模拟: " << (sim + 1) << "/" << SIMULATION_COUNT << endl;
        }
    }

    // 显示最终统计结果
    cout << "\n===== 模拟结束 =====" << endl;
    cout << "总模拟次数: " << SIMULATION_COUNT << endl;

    vector<string> character_names = {"椿", "柯莱塔", "守岸人", "卡卡罗", "长离", "今汐"};
    stats.print_stats(character_names);

    // 绘制排名分布图表
    stats.draw_rank_chart(character_names);

    return 0;
}