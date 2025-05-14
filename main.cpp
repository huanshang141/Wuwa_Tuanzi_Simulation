#include <string>
#include <vector>
#include <iostream>
using namespace std;
const int map_len = 10;

struct char_node
{
public:
    string name;
    char_node *next;
    char_node *prev;
    int pos;
    bool is_head;
    char_node(string name) : name(name), next(nullptr), prev(nullptr), pos(0), is_head(false) {}
    char_node(bool is_head) : name(""), next(nullptr), prev(nullptr), pos(0), is_head(is_head) {}
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
        if (next == nullptr)
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
    bool move(char_node *node, int step)
    {
        int pos = node->get_pos();
        int new_pos = pos + step;
        if (new_pos > map_len)
        {
            return false;
        }
        char_node *new_pos_node = _map[new_pos];
        node->move(step);
        new_pos_node->get_end()->next = node;
    }
};

int main()
{

    map m(map_len);
    vector<char_node *> char_list(3);
    for (int i = 0; i < 3; i++)
    {
        char_list[i] = new char_node("char" + to_string(i));
    }
    m.move(char_list[0], 0);
    m.move(char_list[1], 0);
    m.move(char_list[0], 1);
    cout << char_list[1]->get_pos() << endl;
}