// 1、修改了all_false(改对了)
// 1、调整了因子
// 1、增加了等价二墙点
// 2、将all_false 修改成一个梯度值
//-2:false_factor 阻止了我进门
//3: 增加了门因子
//4:修改boom权值
//5：区分进出门
//6:调整增长豆权重
//6：调整门的权重
#define _CRT_SECURE_NO_WARNINGS 1
#include <iostream>
#include <vector>
#include <cstring>
#include <cmath>
#include <cstdlib>
#include <ctime>

using namespace std;

// 枚举类型定义
enum Direction
{
    WEST,  // 西方向
    NORTH, // 北方向
    EAST,  // 东方向
    SOUTH  // 南方向
};

enum ItemType
{
    FOOD_SCORE, // 分数食物
    FOOD_GROW,  // 生长食物
    ITEM_BOOM,  // 炸弹道具
    ITEM_WALL   // 墙道具
}; // 增加一个道具DOOR

// 结构体定义
struct Coordinate // 定义坐标
{
    int x, y;
    Coordinate(int _x, int _y) : x(_x), y(_y) {} // 坐标构造函数
    bool operator==(const Coordinate &other) const
    { // 重载==运算符用于坐标比较
        return x == other.x && y == other.y;
    }
};

struct HeadInfo
{
    int x, y, player_id; // 头部信息：x坐标、y坐标、玩家ID
};

struct PlayerInfo
{
    int id, length, score, shield_cooldown, shield_duration; // 玩家信息：ID、长度、分数、护盾冷却时间、护盾持续时间
    HeadInfo head;                                           // 头部信息
    Direction facing_direction;                              // 当前面朝方向
    bool is_shield_active;                                   // 是否有护盾激活
    vector<Coordinate> body_positions;                       // 身体位置序列
};

struct Item
{
    int x, y, value; // 物品信息：x坐标、y坐标、值
    ItemType type;   // 物品类型
};

// 全局变量声明
const int MAX_ROWS = 30;                                                                                         // 最大行数
const int MAX_COLS = 40;                                                                                         // 最大列数
const int MY_PLAYER_ID = 2023202289;                                                                             // 自己的玩家ID
const int INFINITY_DIST = 100000000;                                                                             // 无限远距离
const int value_grow = 1.5; /********************************************************************************** */ // 修改增长豆的value
const double value_boom = 0;                                                                                     // xiugai
const double distance_factor = 1.7;//距离因子
const double false_factor = -1.5;//死穴因子
const double door_factor = 7;//门因子

int game_map[MAX_ROWS][MAX_COLS];           // 游戏地图
int remaining_time, num_items, num_players; // 剩余时间、物品数量、玩家数量
PlayerInfo *players, my_player;             // 玩家数组、自己的玩家信息
Item *items;                                // 物品数组

pair<int, int> move_directions[4] = {{0, -1}, {-1, 0}, {0, 1}, {1, 0}}; // 移动方向数组：西、北、东、南

// 函数声明
void read_game_info();                                                    // 读取游戏信息
bool is_valid_move(int x, int y, int direction_index);                    // 判断移动是否有效（超出地图边界，碰到墙，调头）
bool is_valid_move_false(int x, int y, int direction_1, int direction_2); // 用于all_false,判断预测移动是否有效
double Heuristic(int x, int y);                                           // 计算这个方向的效用值
bool will_collide_with_other_player(int x, int y);                        // 判断是否与其他玩家相撞
int decide_best_direction();                                              // 决定最佳移动方向
bool is_boom(int x, int y, int direction_index);
bool is_other_body(int x, int y, int direction_index);
bool is_other_head(int x, int y);
bool two_wall(int x, int y, int i);
bool equal_to_two_wall(int x, int y);
bool is_door(int x,int y);

// 函数定义
void read_game_info()
{
    int temp_x, temp_y, temp_value;
    scanf("%d", &remaining_time); // 读取剩余时间
    scanf("%d", &num_items);      // 读取物品数量
    items = new Item[num_items];  // 分配物品数组内存
    for (int i = 0; i < 30; i++)
        for (int j = 0; j < 40; j++)
            game_map[i][j] = -11;
    // 读取物品信息
    for (int i = 0; i < num_items; i++)
    {
        scanf("%d %d %d", &items[i].x, &items[i].y, &items[i].value); // 读取物品坐标和值
        switch (items[i].value)
        {
        case -1:
            items[i].type = FOOD_GROW;   // 生长食物
            items[i].value = value_grow; // 在此修改增长豆的value
            break;
        case -2:
            items[i].type = ITEM_BOOM;   // 炸弹道具
            items[i].value = value_boom; // xiugai
            break;
        case -4:
            items[i].type = ITEM_WALL; // 墙道具
            break;
        default:
            items[i].type = FOOD_SCORE; // 分数食物
            break;
        }
        game_map[items[i].x][items[i].y] = items[i].type; // 在地图上标记物品类型
    }

    scanf("%d", &num_players);             // 读取玩家数量
    players = new PlayerInfo[num_players]; // 分配玩家数组内存

    // 读取玩家信息
    for (int i = 0; i < num_players; i++)
    {
        scanf("%d %d %d %d %d %d", &players[i].id, &players[i].length, &players[i].score, reinterpret_cast<int *>(&players[i].facing_direction), &players[i].shield_cooldown, &players[i].shield_duration); // 读取玩家ID、长度、分数、面朝方向、护盾冷却时间、护盾持续时间
        players[i].is_shield_active = players[i].shield_duration > 0;                                                                                                                                       // 判断护盾是否激活

        if (players[i].id != MY_PLAYER_ID)
        {                                                           // 如果不是自己的玩家
            scanf("%d %d", &players[i].head.x, &players[i].head.y); // 读取头部坐标
            game_map[players[i].head.x][players[i].head.y] = -6;    // 在地图上标记其他玩家的头部位置//+++++++++++++++++++++++头部位置改为-6
            for (int j = 1; j < players[i].length; j++)
            {
                scanf("%d %d", &temp_x, &temp_y);                                // 读取身体位置
                game_map[temp_x][temp_y] = -3;                                   // 在地图上标记其他玩家的身体位置
                players[i].body_positions.push_back(Coordinate(temp_x, temp_y)); // 记录身体位置到玩家信息中
            }
        }
        else
        {                                                         // 如果是自己的玩家
            my_player = players[i];                               // 记录自己的玩家信息
            scanf("%d %d", &my_player.head.x, &my_player.head.y); // 读取头部坐标
            for (int j = 1; j < players[i].length; j++)
            {
                scanf("%d %d", &temp_x, &temp_y);                               // 读取身体位置
                my_player.body_positions.push_back(Coordinate(temp_x, temp_y)); // 记录身体位置到自己的玩家信息中
            }
        }
    }
}

// 判断移动是否有效的函数
bool is_valid_move(int x, int y, int direction_index) // 如果走这个方向会撞墙，必不往这边走。但是陷阱和蛇身是可以走的
{
    if (x < 0 || x >= MAX_ROWS || y < 0 || y >= MAX_COLS)
        return false;                // 超出地图边界
    if (game_map[x][y] == ITEM_WALL) // 碰到墙  //其他蛇的身体和陷阱需要单独考虑
        return false;
    if ((direction_index == WEST && my_player.facing_direction == EAST) ||
        (direction_index == EAST && my_player.facing_direction == WEST) ||
        (direction_index == NORTH && my_player.facing_direction == SOUTH) ||
        (direction_index == SOUTH && my_player.facing_direction == NORTH))
        return false; // 与当前面朝方向相反
    return true;      // 其他情况为有效移动
}

bool is_valid_move_false(int x, int y, int direction_1, int direction_2) // 一级预测方向，二级预测方向
{
    if (x < 0 || x >= MAX_ROWS || y < 0 || y >= MAX_COLS)
        return false;                // 超出地图边界
    if (game_map[x][y] == ITEM_WALL) // 碰到墙  //其他蛇的身体和陷阱需要单独考虑
        return false;
    if ((direction_2 == WEST && direction_1 == EAST) ||
        (direction_2 == EAST && direction_1 == WEST) ||
        (direction_2 == NORTH && direction_1 == SOUTH) ||
        (direction_2 == SOUTH && direction_1 == NORTH))
        return false; // 与当前面朝方向相反
    return true;      // 其他情况为有效移动
}

// bool all_false(int x, int y, int d)
// {
//     int p = 0;
//     for (int j = 0; j < 4; j++)
//     {
//         int next_x = x + move_directions[j].first;
//         int next_y = y + move_directions[j].second;
//         if (!is_valid_move_false(next_x, next_y, d, j) || is_other_body(next_x, next_y, j))
//             p++;
//     }
//     if (p == 4)
//         return true;
//     return false;
// }
int all_false(int x, int y, int d)//一级x，一级y，一级方向
{
    int p = 0;//严重程度   最低为1，最高为4
    for (int j = 0; j < 4; j++)
    {
        int next_x = x + move_directions[j].first;
        int next_y = y + move_directions[j].second;
        if (!is_valid_move_false(next_x, next_y, d, j) || is_other_body(next_x, next_y, j))
            p++;
    }
    return p;
}

bool two_wall(int x, int y, int i)//拐角  一级x，一级y，一级方向
{
    int p = 0, q = 0;
    for (int j = 0; j < 4; j++)
    {
        if (j == ((i + 2) % 4))
            continue;
        int next_x = x + move_directions[j].first;
        int next_y = y + move_directions[j].second;
        if (game_map[next_x][next_y] == ITEM_WALL)
            p += j * pow(-1, q++);
    }
    if (q == 2 && (p == -1 || p == -3))
        return true;
    return false;
}
bool is_door(int x, int y, int i)
{
    int p = 0, q = 0;
    for (int j = 0; j < 4; j++)
    {
        if (j == ((i + 2) % 4))//二级方向和一级方向冲突
            continue;
        int next_x = x + move_directions[j].first;
        int next_y = y + move_directions[j].second;
        if (game_map[next_x][next_y] == ITEM_WALL)
            p += j * pow(-1, q++);
    }
    if (q == 2 && (p == -2))
        return true;
    return false;
}

bool equal_to_two_wall(int x, int y)//等价于拐角
{
    if ((x == 9 && y == 16) || (x == 11 && y == 14) || (x == 19 && y == 14) || (x == 21 && y == 16) || (x == 21 && y == 24) || (x == 19 && y == 26) || (x == 11 && y == 26) || (x == 9 && y == 24))
        return true;
    return false;
}

bool is_boom(int x, int y, int direction_index) // 判断是不是炸弹道具
{
    if (game_map[x][y] == ITEM_BOOM) //-----------------------------可能有问题！！！！！！！！！！！！！！！！！
        return true;
    else
        return false;
}

bool is_other_body(int x, int y, int direction_index) // 判断是不是别人的身体
{
    if (game_map[x][y] == -3 || game_map[x][y] == -6)
        return true;
    else
        return false;
}

bool is_other_head(int x, int y)//是不是别人的头
{
    if (game_map[x][y] == -6)
        return true;
    else
        return false;
}

// 计算到最近食物的距离的函数
double Heuristic(int x, int y)
{
    double max_value_distance_ratio = -INFINITY_DIST; // 初始设为无限远距离
    double max_score;
    double sum_ratio = 0;
    for (int i = 0; i < num_items; i++)
    {
        if (items[i].type == FOOD_SCORE || items[i].type == FOOD_GROW || items[i].type == ITEM_BOOM)
        { // 如果是分数食物或生长食物

            int distance = abs(x - items[i].x) + abs(y - items[i].y);                                                 // 计算距离
            double value_distance_ratio = static_cast<double>(items[i].value) / (pow(distance, distance_factor) + 1); //************ */扩充此项  //----------------平方不一定合理
            sum_ratio = sum_ratio + value_distance_ratio;
            // if (value_distance_ratio > max_value_distance_ratio)
            // {
            //     max_value_distance_ratio = value_distance_ratio; // 更新最大启发式值
            // }
        }
    }
    // max_score = max_value_distance_ratio;
    max_score = sum_ratio;
    return max_score; // 返回最大启发式值
}

// 判断是否与其他玩家相撞的函数
bool will_collide_with_other_player(int x, int y)
{
    for (int i = 0; i < num_players; i++)
    {
        if (players[i].id != MY_PLAYER_ID)
        { // 排除自己的玩家
            for (int d = 0; d < 4; d++)
            {
                int next_x = players[i].head.x + move_directions[d].first;
                int next_y = players[i].head.y + move_directions[d].second;
                if (next_x == x && next_y == y)
                {
                    return true; // 与其他玩家的头部位置相撞
                }
            }
        }
    }
    return false; // 不与其他玩家相撞
}

// 决定最佳移动方向的函数
int decide_best_direction()
{
    int best_direction_index = -1;     // 最佳方向索引初始化为-1
    double max_score = -INFINITY_DIST; // 到食物的最大启发式值初始化为负无穷

    int p;
    std::srand(std::time(0));
    p = std::rand() % 4;

    for (int j = 0; j < 4; j++) // 错开方向
    {                           // 遍历四个方向
        int i;
        i = (p + j) % 4;
        double score = 0;

        int next_x = my_player.head.x + move_directions[i].first;
        int next_y = my_player.head.y + move_directions[i].second;

        if (!is_valid_move(next_x, next_y, i)) //+++++++++++++++++++++++++++++++调整is_valid_move
            continue;                          // 若移动无效则跳过此方向
        score = Heuristic(next_x, next_y);     // 计算到该方向的最大启发式值
        /*接下来是防贪心函数*/
        // if ((my_player.shield_cooldown > 0) && (my_player.shield_cooldown <= 26) && all_false(next_x, next_y, i))
        //     continue;
        if(all_false(next_x,next_y,i)==4)
        {
            if (((my_player.shield_cooldown > 0) && (my_player.shield_cooldown <= 29)) || my_player.score <= 20) // 如果我没办法开护盾
                continue;
            else
            {
                score = -3000;
            }    
        }
        score = score + (all_false(next_x,next_y,i)-1)*(false_factor);
        // if(is_door(next_x,next_y,i)&&((next_x==9&&next_y==20)||(next_x==15&&next_y==14)||(next_x==21&&next_y==20)||(next_x==15&&next_y==26)))
        // {
        //     score=score+door_factor;
        // }
        if(is_door(next_x,next_y,i))//出门，进门
        {
            if(next_x==9&&next_y==20)
            {
                if(i==3)//进门
                score = score + door_factor;
                else if(i==1)//出门
                score = score - door_factor;
            }
            if(next_x==15&&next_y==14)
            {
                if(i==2)//进门
                score = score + door_factor;
                else if(i==0)//出门
                score = score - door_factor;
            }
            if(next_x==21&&next_y==20)
            {
                if(i==1)//进门
                score = score + door_factor;
                else if(i==3)//出门
                score = score - door_factor;
            }
            if(next_x==15&&next_y==26)
            {
                if(i==0)//进门
                score = score + door_factor;
                else if(i==2)//出门
                score = score - door_factor;
            }

        }
        if (will_collide_with_other_player(next_x, next_y)) // 如果会与对方的头相撞
        {
            score = -5000;
        }
        if (is_boom(next_x, next_y, i)) // 炸弹
        {
            score = score-3;
        }
        if (is_other_body(next_x, next_y, i)) // 别人的身体
        {
            score = -10000;
        }
        if (game_map[next_x][next_y] != FOOD_SCORE && game_map[next_x][next_y] != FOOD_GROW && (two_wall(next_x, next_y, i) || equal_to_two_wall(next_x, next_y)))
        {
            score = -500;
        }
        if (score > max_score)
        {
            max_score = score;        // 更新最大启发式值
            best_direction_index = i; // 更新最佳方向索引
        }
    }

    if (max_score == -5000) // 如果下一个回合要碰头或者前面就是头，且碰头是最佳选择。。。开护盾
    {
        if (my_player.is_shield_active)
        {
            return best_direction_index;
        }
        else
            return 4;
    }

    if (max_score == -10000) // 如果最佳选择是撞别人的身体
    {
        if (my_player.is_shield_active)
        {
            return best_direction_index;
        }
        else
            return 4;
    }
    return best_direction_index == -1 ? 4 : best_direction_index // best_direction_index == -1 ? 4 : best_direction_index
        ;                       // 返回最佳方向索引，若无有效方向则返回4
}

// 主函数
int main()
{
    memset(game_map, 0, sizeof(game_map)); // 初始化游戏地图为0
    read_game_info();                      // 读取游戏信息

    int selected_direction = decide_best_direction(); // 决定最佳移动方向
    cout << selected_direction;                       // 输出最佳移动方向

    return 0;
}

/*需要改进的地方：(1)提高蛇往中间走的权重
                (2) 提高自己的保命能力*/
