#pragma once
 
// 44 bytes per record (40 + 4 bytes padding)
struct Record {
    char game_date[11]; //  11 bytes    e.g: 21/12/2022
    int team_id_home; //  4 bytes       e.g: 1610612738
    int pts_home; //  4 bytes           e.g: 112
    float fg_pct_home; // 4 bytes       e.g: 0.386
    float ft_pct_home; // 4 bytes       e.g: 0.84
    float fg3_pct_home; // 4 bytes      e.g: 0.317
    int ast_home; // 4 bytes            e.g: 26
    int reb_home; // 4 bytes            e.g: 62
    bool home_team_wins; // 1 byte      e.g: 0 or 1
};					