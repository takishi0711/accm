#pragma once

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

struct RandomWalk {

private :
    int number_of_RW_execution; // RW の実行回数
    double alpha = 0.2; // RW の終了確率

public :

    // RW の実行回数を入手
    int get_number_of_RW_execution();

    // RW の終了確率を入手
    double get_alpha();
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


inline int RandomWalk::get_number_of_RW_execution() {
    return number_of_RW_execution;
}

inline double RandomWalk::get_alpha() {
    return alpha;
}

