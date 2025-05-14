#include <string>
#include <vector>
#include <iostream>
#include <random>    // 添加随机数库
#include <algorithm> // 添加algorithm库，包含shuffle函数

using namespace std;
const int map_len = 10;

// 全局随机数生成器
random_device g_rd;
mt19937 g_gen(g_rd());
uniform_int_distribution<> g_dice_dist(1, 3); // 骰子分布 (1-3)

// 修改技能函数定义，不再接受步数参数，而是自己生成步数
// 默认技能：正常移动1-3步
int move_origin()
{
    return g_dice_dist(g_gen); // 直接返回1-3的随机数
}

// 技能：步数+1
int move_plus_one()
{
    return g_dice_dist(g_gen) + 1; // 随机数+1
}

// 技能：步数翻倍
int move_double()
{
    return g_dice_dist(g_gen) * 2; // 随机数*2
}

struct char_node
{
public:
    string name;
    char_node *next;
    char_node *prev;
    int pos;
    bool is_head;
    // 修改技能函数指针类型，不再接受参数
    int (*skill)();

    // 修改默认技能函数引用全局函数
    static int default_skill()
    {
        return move_origin();
    }
    char_node(string name)
        : name(name), next(nullptr), prev(nullptr), pos(0), is_head(false), skill(default_skill) {}
    char_node(string name, int (*skill_func)() = default_skill)
        : name(name), next(nullptr), prev(nullptr), pos(0), is_head(false), skill(skill_func) {}

    char_node(bool is_head)
        : name(""), next(nullptr), prev(nullptr), pos(0), is_head(is_head), skill(default_skill) {}

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
    void move(int step)
    {
        if (prev)
            prev->next = nullptr;
        char_node *temp = this;
        while (temp)
        {
            temp->pos += step;
            temp = temp->next;
        }
    }
};

struct map
{
public:
    vector<char_node *> _map;
    map(int len) : _map(len, nullptr)
    {
        for (int i = 0; i < len; i++)
        {
            _map[i] = new char_node(true);
        }
    }
    bool move(char_node *node)
    {
        int pos = node->get_pos();
        // 直接调用技能函数获取步数
        int actual_step = node->skill();
        int new_pos = pos + actual_step;

        // 添加调试信息 - 记录角色信息
        string character_name = node->name;

        if (new_pos >= map_len)
        {
            cout << "角色 " << character_name << " 尝试从位置 " << pos << " 移动 " << actual_step
                 << " 步到位置 " << new_pos << "，超出地图范围，移动失败" << endl;
            return false;
        }
        char_node *new_pos_node = _map[new_pos];
        node->move(actual_step); // 使用计算后的步数
        node->prev = new_pos_node->get_end();
        new_pos_node->get_end()->next = node;

        // 输出移动信息
        cout << "角色 " << character_name << " 从位置 " << pos << " 移动到位置 " << new_pos
             << "（实际步数: " << actual_step << "）" << endl;

        return true;
    }

    // 判断是否是终点
    bool is_finish_point(int pos)
    {
        return pos >= map_len;
    }
};

int main()
{
    map m(map_len);
    vector<char_node *> char_list(3);

    // 使用全局技能函数初始化角色
    char_list[0] = new char_node("角色A", move_plus_one); // 每次移动多走1步
    char_list[1] = new char_node("角色B", move_origin);   // 使用默认技能
    char_list[2] = new char_node("角色C", move_double);   // 步数翻倍

    cout << "游戏开始！地图长度: " << map_len << endl;
    cout << "角色A技能: 移动步数+1" << endl;
    cout << "角色B技能: 正常移动" << endl;
    cout << "角色C技能: 移动步数翻倍" << endl;
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